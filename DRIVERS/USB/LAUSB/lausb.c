#include	"USB_DRIVER.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35)
#define usb_free_coherent(a, b, c, d) usb_buffer_free(a, b, c, d)
#define usb_alloc_coherent(a, b, c, d) usb_buffer_alloc(a, b, c, d)
#endif

static const char* driverVersion = "1.0.13300.1632";

/* table of devices that work with this driver */
static const struct usb_device_id lausb_table[] = {
        { USB_DEVICE(RSH_VID, LA2USB_PID)      },
	{ USB_DEVICE(RSH_VID, LA20USB_PID)     },
	{ USB_DEVICE(RSH_VID, LA50USB_PID)     },
	{ USB_DEVICE(RSH_VID, SIRIUS_PID)      },
	{ USB_DEVICE(RSH_VID, LAN10_12USB_PID) },
	{ USB_DEVICE(RSH_VID, LAI24USB_PID)    },
        { USB_DEVICE(RSH_VID, GSPF053USB_PID)  },
        { }                                     /* Terminating entry */
};
MODULE_DEVICE_TABLE(usb, lausb_table);

/* Structure to hold all of our device specific stuff */
struct lausb {
	struct usb_device       *udev;                  /* the usb device for this device */
	struct usb_interface    *interface;             /* the interface for this device */
        struct semaphore        limit_sem;              /* limiting the number of writes in progress */
	struct usb_anchor       submitted;              /* in case we need to retract our submissions */
	struct urb              *bulk_in_urb;           /* the urb to read data with */
	unsigned char           *bulk_in_buffer;        /* the buffer to receive data */
	size_t                  bulk_in_buffer_currSize;           /* the size of the receive buffer */
	size_t			bulk_in_filled;
	
	__u8                    bulk_in_endpointAddr;   /* the address of the bulk in endpoint; READ operations */
        __u8                    bulk_out_endpointAddr;   /* the address of the bulk out endpoint; WRITE operations */
        __u8                    bulk_in_endpointAddr_STAT;   /* the address of the bulk in endpoint; READ STATUS  */
        
        const char*		deviceName; 		/* short device name */
        
	int                     errors;                 /* the last request tanked */
	int                     open_count;             /* count the number of openers */        
	bool                    ongoing_read;           /* a read is going on */
        spinlock_t              err_lock;               /* lock for errors */
        struct kref             kref;
        struct mutex            io_mutex;               /* synchronize I/O with disconnect */
        struct completion       bulk_in_completion;     /* to wait for an ongoing read */
};
#define to_lausb_dev(d) container_of(d, struct lausb, kref)

static struct usb_driver lausb_driver;
static void lausb_draw_down(struct lausb *dev);

static void lausb_delete(struct kref *kref)
{
        struct lausb *dev = to_lausb_dev(kref);
        
	usb_free_coherent(dev->udev, dev->bulk_in_buffer_currSize, dev->bulk_in_buffer, dev->bulk_in_urb->transfer_dma);
	usb_free_urb(dev->bulk_in_urb);        
	usb_put_dev(dev->udev);
        kfree(dev);
}

static int lausb_open(struct inode *inode, struct file *file)
{
        struct lausb *dev;
        struct usb_interface *interface;
        int subminor;
        int retval = 0;

        subminor = iminor(inode);

        interface = usb_find_interface(&lausb_driver, subminor);
        if (!interface) {
	  
	  dev_err(&dev->interface->dev,
			"[%s] %s - error, can't find device for minor %d", dev->deviceName,
			__func__, subminor);

                retval = -ENODEV;
                goto exit;
        }

        dev = usb_get_intfdata(interface);
        if (!dev) {
                retval = -ENODEV;
                goto exit;
        }

        /* increment our usage count for the device */
        kref_get(&dev->kref);

        /* lock the device to allow correctly handling errors
         * in resumption */
        mutex_lock(&dev->io_mutex);

        if (!dev->open_count++) {
                retval = usb_autopm_get_interface(interface);
                        if (retval) {
                                dev->open_count--;
                                mutex_unlock(&dev->io_mutex);
                                kref_put(&dev->kref, lausb_delete);
                                goto exit;
                        }
        } /* else { //uncomment this block if you want exclusive open
                retval = -EBUSY;
                dev->open_count--;
                mutex_unlock(&dev->io_mutex);
                kref_put(&dev->kref, lausb_delete);
                goto exit;
        } */
        /* prevent the device from being autosuspended */

        /* save our object in the file's private structure */
        file->private_data = dev;
        mutex_unlock(&dev->io_mutex);

exit:
        return retval;
}

static int lausb_release(struct inode *inode, struct file *file)
{
        struct lausb *dev;

        dev = file->private_data;
        if (dev == NULL)
                return -ENODEV;

        /* allow the device to be autosuspended */
        mutex_lock(&dev->io_mutex);
        if (!--dev->open_count && dev->interface)
                usb_autopm_put_interface(dev->interface);
        mutex_unlock(&dev->io_mutex);

        /* decrement the count on our device */
        kref_put(&dev->kref, lausb_delete);
        return 0;
}

static int lausb_flush(struct file *file, fl_owner_t id)
{
        struct lausb *dev;
        int res;

        dev = file->private_data;
        if (dev == NULL)
                return -ENODEV;

        /* wait for io to stop */
        mutex_lock(&dev->io_mutex);
        lausb_draw_down(dev);

        /* read out errors, leave subsequent opens a clean slate */
        spin_lock_irq(&dev->err_lock);
        res = dev->errors ? (dev->errors == -EPIPE ? -EPIPE : -EIO) : 0;
        dev->errors = 0;
        spin_unlock_irq(&dev->err_lock);

        mutex_unlock(&dev->io_mutex);

        return res;
}

static void lausb_read_bulk_callback(struct urb *urb)
{
        struct lausb *dev;

        dev = urb->context;

        spin_lock(&dev->err_lock);
        /* sync/async unlink faults aren't errors */
        if (urb->status) {
                if (!(urb->status == -ENOENT ||
                    urb->status == -ECONNRESET ||
                    urb->status == -ESHUTDOWN))
		  
		  	  dev_err(&dev->interface->dev,
			"[%s] %s - nonzero read bulk status received: %d", dev->deviceName,
			__func__, urb->status);

                dev->errors = urb->status;
        } else {
		dev->bulk_in_filled = urb->actual_length;
	}
        dev->ongoing_read = 0;
        spin_unlock(&dev->err_lock);

        complete(&dev->bulk_in_completion);
}

static void lausb_alloc_coherent(struct lausb *dev, size_t size, gfp_t mem_flags, dma_addr_t *dma)
{
	size_t buffer_size = 0;
	if (dev->bulk_in_buffer) {
		if(size <= dev->bulk_in_buffer_currSize)
			return;
		else {
			usb_free_coherent(dev->udev, dev->bulk_in_buffer_currSize, dev->bulk_in_buffer, dev->bulk_in_urb->transfer_dma);
			buffer_size = DIV_ROUND_UP(size, PAGE_SIZE) * PAGE_SIZE;
			dev->bulk_in_buffer = usb_alloc_coherent(dev->udev, buffer_size, mem_flags, dma);
			dev->bulk_in_buffer_currSize = buffer_size;
		
			if( dev->bulk_in_buffer == NULL ) 
			{
  		  	  dev_err(&dev->interface->dev,
			    "[%s] %s - couldn't allocate dma buffer with predefined size of %d bytes", dev->deviceName,
			    __func__, (int)size);

			  return;
			}
		}
	}
	else {
		buffer_size = DIV_ROUND_UP(size, PAGE_SIZE) * PAGE_SIZE;
		dev->bulk_in_buffer = usb_alloc_coherent(dev->udev, buffer_size, mem_flags, dma);
		dev->bulk_in_buffer_currSize = buffer_size;
		
		if( dev->bulk_in_buffer == NULL ) 
		{
		  dev_err(&dev->interface->dev,
			    "[%s] %s - couldn't allocate dma buffer with predefined size of %d bytes", dev->deviceName,
			    __func__, (int)size);
		  return;
		}
	}
	
	dev_err(&dev->interface->dev,
			    "[%s] %s - page size is %d, desired size is %d, allocated size is %d", dev->deviceName,
			    __func__, (int)PAGE_SIZE,(int)size,(int)buffer_size);
}

static int lausb_do_read_io(struct lausb *dev, size_t readsize)
{
        int rv;
//	int i;
	
        lausb_alloc_coherent(dev, readsize, GFP_KERNEL, &dev->bulk_in_urb->transfer_dma);


        /* prepare a read */
        usb_fill_bulk_urb(dev->bulk_in_urb,
                        dev->udev,
                        usb_rcvbulkpipe(dev->udev,
                                	dev->bulk_in_endpointAddr),
                        dev->bulk_in_buffer,
                        readsize,
                        lausb_read_bulk_callback,
                        dev);
        /* tell everybody to leave the URB alone */
        spin_lock_irq(&dev->err_lock);
        dev->ongoing_read = 1;
        spin_unlock_irq(&dev->err_lock);

        /* do it */
        rv = usb_submit_urb(dev->bulk_in_urb, GFP_KERNEL);

/*	for(i =0; i < 64; i++)
	  printk(" %d", ((char*)dev->bulk_in_buffer)[i]);
	
*/	
	if (rv < 0) {

		dev_err(&dev->interface->dev,
		    "[%s] %s - failed submitting read urb, error %d", dev->deviceName,
		    __func__, rv);
		
                dev->bulk_in_filled = 0;
                rv = (rv == -ENOMEM) ? rv : -EIO;
                spin_lock_irq(&dev->err_lock);
                dev->ongoing_read = 0;
                spin_unlock_irq(&dev->err_lock);
        }

        return rv;
}

static ssize_t lausb_read(struct file *file, char *buffer, size_t count,
                         loff_t *ppos)
{
        struct lausb *dev;
        int rv;
        bool ongoing_io;

        dev = file->private_data;
//printk("\nlausb:  read");
        /* if we cannot read at all, return EOF */
        if (!dev->bulk_in_urb || !count)
                return 0;
//printk("\nlausb: mutex_lock_interruptible");
        /* no concurrent readers */
        rv = mutex_lock_interruptible(&dev->io_mutex);
        if (rv < 0)
                return rv;

        if (!dev->interface) {          /* disconnect() was called */
                rv = -ENODEV;
                goto exit;
        }

        /* if IO is under way, we must not touch things */
retry:
        spin_lock_irq(&dev->err_lock);
        ongoing_io = dev->ongoing_read;
        spin_unlock_irq(&dev->err_lock);

        if (ongoing_io) {
                /* nonblocking IO shall not wait */
                if (file->f_flags & O_NONBLOCK) {
                        rv = -EAGAIN;
                        goto exit;
                }
                /*
                 * IO may take forever
                 * hence wait in an interruptible state
                 */
                rv = wait_for_completion_interruptible(&dev->bulk_in_completion);
                if (rv < 0)
                        goto exit;
        }

        /* errors must be reported */
        rv = dev->errors;
        if (rv < 0) {
                /* any error is reported once */
                dev->errors = 0;
                /* to preserve notifications about reset */
                rv = (rv == -EPIPE) ? rv : -EIO;

		dev->bulk_in_filled = 0;
                /* report it */
                goto exit;
        }
//printk("\nlausb: copy_to_user");
	if(dev->bulk_in_filled) {
		/* we had read data - copy it to user */
		if (copy_to_user(buffer,
                                 dev->bulk_in_buffer,
                                 dev->bulk_in_filled))
                	rv = -EFAULT;
                else
                        rv = dev->bulk_in_filled;
		dev->bulk_in_filled = 0;
	} else {/* read some data */
        	rv = lausb_do_read_io(dev, count);
        	if (rv < 0)
			goto exit;
		else if (!(file->f_flags & O_NONBLOCK))
			goto retry;
                rv = -EAGAIN;
        }
exit:
//printk("\nlausb: exit");
        mutex_unlock(&dev->io_mutex);
        return rv;
}

static void lausb_write_bulk_callback(struct urb *urb)
{
        struct lausb *dev;

        dev = urb->context;

        /* sync/async unlink faults aren't errors */
        if (urb->status) {
                if (!(urb->status == -ENOENT ||
                    urb->status == -ECONNRESET ||
                    urb->status == -ESHUTDOWN))
			dev_err(&dev->interface->dev,
				"[%s] %s - nonzero write bulk status received: %d", dev->deviceName,
				__func__, urb->status);

                spin_lock(&dev->err_lock);
                dev->errors = urb->status;
                spin_unlock(&dev->err_lock);
        }

        /* free up our allocated buffer */
        usb_free_coherent(urb->dev, urb->transfer_buffer_length,
                          urb->transfer_buffer, urb->transfer_dma);
        up(&dev->limit_sem);
}

static ssize_t lausb_write(struct file *file, const char *user_buffer,
                          size_t count, loff_t *ppos)
{
        struct lausb *dev;
        int retval = 0;
        struct urb *urb = NULL;
        char *buf = NULL;
	/* MAX_TRANSFER - 8 bytes packet, change it if you need to write more than 8 bytes command packet */
        size_t writesize = min(count, (size_t)MAX_TRANSFER); 

        dev = file->private_data;

        /* verify that we actually have some data to write */
        if (count == 0)
                goto exit;

        /*
         * limit the number of URBs in flight to stop a user from using up all
         * RAM
         */
        if (!(file->f_flags & O_NONBLOCK)) {
                if (down_interruptible(&dev->limit_sem)) {
                        retval = -ERESTARTSYS;
                        goto exit;
                }
        } else {
                if (down_trylock(&dev->limit_sem)) {
                        retval = -EAGAIN;
                        goto exit;
                }
        }

        spin_lock_irq(&dev->err_lock);
        retval = dev->errors;
        if (retval < 0) {
                /* any error is reported once */
                dev->errors = 0;
                /* to preserve notifications about reset */
                retval = (retval == -EPIPE) ? retval : -EIO;
        }
        spin_unlock_irq(&dev->err_lock);
        if (retval < 0)
                goto error;

        /* create a urb, and a buffer for it, and copy the data to the urb */
        urb = usb_alloc_urb(0, GFP_KERNEL);
        if (!urb) {
                retval = -ENOMEM;
                goto error;
        }

        buf = usb_alloc_coherent(dev->udev, writesize, GFP_KERNEL,
                                 &urb->transfer_dma);
        if (!buf) {
                retval = -ENOMEM;
                goto error;
        }

        if (copy_from_user(buf, user_buffer, writesize)) {
                retval = -EFAULT;
                goto error;
        }

        /* this lock makes sure we don't submit URBs to gone devices */
        mutex_lock(&dev->io_mutex);
        if (!dev->interface) {          /* disconnect() was called */
                mutex_unlock(&dev->io_mutex);
                retval = -ENODEV;
                goto error;
        }

        /* initialize the urb properly */
        usb_fill_bulk_urb(urb, dev->udev,
                          usb_sndbulkpipe(dev->udev, dev->bulk_out_endpointAddr),
                          buf, writesize, lausb_write_bulk_callback, dev);
        urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
        usb_anchor_urb(urb, &dev->submitted);

        /* send the data out the bulk port */
        retval = usb_submit_urb(urb, GFP_KERNEL);
        mutex_unlock(&dev->io_mutex);
	if (retval) {
		dev_err(&dev->interface->dev,
			"[%s] %s - failed submitting write urb, error %d", dev->deviceName,
			__func__, retval);
		goto error_unanchor;
	}

        /*
         * release our reference to this urb, the USB core will eventually free
         * it entirely
         */
        usb_free_urb(urb);


        return writesize;

error_unanchor:
        usb_unanchor_urb(urb);
error:
        if (urb) {
                usb_free_coherent(dev->udev, writesize, buf, urb->transfer_dma);
                usb_free_urb(urb);
        }
        up(&dev->limit_sem);

exit:
        return retval;
}

static long lausb_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
	struct lausb *dev = file->private_data;
	PIOCTL_BUFFER ibuf = NULL;
	long status = -EFAULT;
	int bytes_read = 0;

	
	if( cmd == IOCTL_RESET_PIPE_WRITE )
	{
	    usb_reset_endpoint(dev->udev, dev->bulk_out_endpointAddr);
	    return 0;
	}
	
	if( cmd == IOCTL_RESET_PIPE_READ )
	{
	    usb_reset_endpoint(dev->udev, dev->bulk_in_endpointAddr);
	    return 0;
	} 
	
	
	ibuf = kmalloc(sizeof(IOCTL_BUFFER), GFP_KERNEL);
	if(ibuf == NULL) return status;
	if(copy_from_user(ibuf, (void*)arg, sizeof(IOCTL_BUFFER))) {kfree(ibuf); return status;}

	
	switch(cmd)
	{
		case IOCTL_BULK_GET_STATUS:
		{
			if( dev->bulk_in_endpointAddr_STAT == 0 )
			{
			    dev_info(&dev->interface,"[%s] %s this device doesn't have status endpoint!", dev->deviceName, __func__);
			    status = -EFAULT;
			    break;
			}
		  
			status = usb_bulk_msg(dev->udev,
					      usb_rcvbulkpipe(dev->udev, dev->bulk_in_endpointAddr_STAT),
					      ibuf->outBuffer,
					      ibuf->outSize,
					      &bytes_read,
					      0);
			
			if(!status)
			{
				status = copy_to_user((void*)arg, ibuf, sizeof(IOCTL_BUFFER));				
				if(status)
				{			
					dev_info(&dev->interface,"[%s] %s - failed copying data into user space, error %ld", dev->deviceName, __func__, status);	
					status = -EFAULT;
				}				
				else		
					status = 0;
			}
			else
				dev_info(&dev->interface,"[%s] %s - failed submitting read bulk, error %ld , %d - bytes_read", dev->deviceName, __func__, status, bytes_read);
		} break;
		case IOCTL_GET_CONFIG:
		{
			PSendPacketGETID prq = NULL;
			prq = kmalloc(sizeof(SendPacketGETID), GFP_KERNEL);
			if(prq == NULL) return status;

			prq->vid = dev->udev->descriptor.idVendor;
			prq->pid = dev->udev->descriptor.idProduct;
			prq->rev = dev->udev->descriptor.bcdDevice;

			status = copy_to_user((void*)arg, prq, sizeof(SendPacketGETID));
			if(status)
			{			
				dev_info(&dev->interface,"[%s] %s - failed copying data into user space, error %ld", dev->deviceName, __func__, status);	
				status = -EFAULT;
			}				
			else		
				status = 0;
			
			kfree(prq);
		} break;
		default:
		{
			dev_info(&dev->interface,"[%s] %s - unknown ioctl code", dev->deviceName, __func__);
		}break;
	}
	kfree(ibuf);
	return status;
} 
 
static const struct file_operations lausb_fops = {
        .owner 		= THIS_MODULE,
        .read 		= lausb_read,
        .write 		= lausb_write,
	.unlocked_ioctl = lausb_ioctl,
        .open 		= lausb_open,
        .release 	= lausb_release,
        .flush 		= lausb_flush,
        .llseek 	= noop_llseek,
};

/*
 * usb class driver info in order to get a minor number from the usb core,
 * and to have the device registered with the driver core
 */
static struct usb_class_driver lausb_class = {
        .name =         "lausb%d",
        .fops =         &lausb_fops,
        .minor_base =   LAUSB_MINOR_BASE,
};

static int lausb_probe(struct usb_interface *interface,
                      const struct usb_device_id *id)
{
        struct lausb *dev;
        int retval = -ENOMEM;

        /* allocate memory for our device state and initialize it */
        dev = kzalloc(sizeof(*dev), GFP_KERNEL);
        if (!dev) {
                dev_err(&interface->dev,"Can't allocate memory for lausb device!");
                goto error;
        }
        kref_init(&dev->kref);
        sema_init(&dev->limit_sem, WRITES_IN_FLIGHT);
        mutex_init(&dev->io_mutex);
        spin_lock_init(&dev->err_lock);
        init_usb_anchor(&dev->submitted);
        init_completion(&dev->bulk_in_completion);

        dev->udev = usb_get_dev(interface_to_usbdev(interface));
        dev->interface = interface;

	/* prepare urb for reading to save us from overhead during time critical lausb_read
	 * TODO: not important, but do all the allocation for writing (i.e. urb and buffer)
	 * TODO: also not important, but in LDD it's advised to minimize USB probing time, so we can safely move 
         * all code from here to lausb_open, except for usb_register_dev and dev_info functions;
	 */
        dev->bulk_in_urb = usb_alloc_urb(0, GFP_KERNEL);
        if (!dev->bulk_in_urb) {
		dev_err(&interface->dev,
			"Could not allocate bulk_in_urb!\n");
		goto error;
	}
	dev->bulk_in_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	dev->bulk_in_buffer 	     = NULL;
	dev->bulk_in_buffer_currSize = 0;

	/* set up bulk in endpoint information */
	/* LA50USB (possibly, some other boards as well) has different endpoint than other boards supported by this driver 
	 * TODO: make endpoint initialization neater
         */	
	if( dev->udev->descriptor.idVendor != RSH_VID )
	{
	      dev_err(&interface->dev,"Device vid=%d,pid=%d. It looks like this device was not produced by JSC \"Rudshel\"!",
		dev->udev->descriptor.idVendor, dev->udev->descriptor.idProduct );
              goto error;
	}
	
	
	switch( dev->udev->descriptor.idProduct ) // let's save device name
	{
	  case LA2USB_PID: dev->deviceName	 = "LA2USB"; break;
	  case LA50USB_PID: dev->deviceName	 = "LA50USB"; break;
	  case LAI24USB_PID: dev->deviceName	 = "LAI24USB"; break;
	  case LA20USB_PID: dev->deviceName	 = "LA20USB"; break;
	  case GSPF053USB_PID: dev->deviceName	 = "GSPF053USB"; break;
	  case LAN10_12USB_PID: dev->deviceName	 = "LAN10_12USB"; break;
	  case SIRIUS_PID: dev->deviceName	 = "SIRIUSUSB"; break;
	  
	  default:
	    dev->deviceName = "LAUSB"; break;
	}

	  
	switch( dev->udev->descriptor.idProduct )
	{
	  case LA50USB_PID: 
	    dev->bulk_in_endpointAddr = LA50USB_DATA_EP;
	    dev->bulk_out_endpointAddr = LAUSB_CMD_EP;
	    dev->bulk_in_endpointAddr_STAT = 0; // no status endpoint, doesn't support ioctl
	    break;
	  case GSPF053USB_PID:
	    dev->bulk_in_endpointAddr = GSPF053USB_DATA_EP;
	    dev->bulk_out_endpointAddr = GSPF053USB_CMD_EP;
	    dev->bulk_in_endpointAddr_STAT = 0; // no status endpoint, doesn't support ioctl
	    break;
	  default:
	    dev->bulk_in_endpointAddr = LAUSB_DATA_EP;
	    dev->bulk_out_endpointAddr = LAUSB_CMD_EP;
	    dev->bulk_in_endpointAddr_STAT = LAUSB_STAT_EP;
	}
	  		
        /* save our data pointer in this interface device */
        usb_set_intfdata(interface, dev);
	
	/* Once again, all code above can be moved to lausb_open function, to minimize USB probing time */

        /* we can register the device now, as it is ready */
        retval = usb_register_dev(interface, &lausb_class);
        if (retval) {
                /* something prevented us from registering this driver */
                dev_info(&interface->dev,"Not able to get a minor for %s device", dev->deviceName);
                usb_set_intfdata(interface, NULL);
                goto error;
        }


        /* let the user know what node this device is now attached to */
        dev_info(&interface->dev,
                 "%s device is now attached to /dev/lausb%d",
                 dev->deviceName, interface->minor);
	
	dev_info(&interface->dev,
                 "/dev/lausb%d data EP = 0x%x, command EP = 0x%x, status EP = 0x%x",
                  interface->minor,
		  dev->bulk_in_endpointAddr,
		  dev->bulk_out_endpointAddr,
		  dev->bulk_in_endpointAddr_STAT );
	
        return 0;

error:
        if (dev)
                /* this frees allocated memory */
                kref_put(&dev->kref, lausb_delete);
        return retval;
}

static void lausb_disconnect(struct usb_interface *interface)
{
        struct lausb *dev;
        int minor = interface->minor;

        dev = usb_get_intfdata(interface);
        usb_set_intfdata(interface, NULL);

        /* give back our minor */
        usb_deregister_dev(interface, &lausb_class);

        /* prevent more I/O from starting */
        mutex_lock(&dev->io_mutex);
        dev->interface = NULL;
        mutex_unlock(&dev->io_mutex);

        usb_kill_anchored_urbs(&dev->submitted);

        /* decrement our usage count */
        kref_put(&dev->kref, lausb_delete);

        dev_info(&interface->dev, "%s(/dev/lausb%d) is now disconnected", dev->deviceName, minor);
}

static void lausb_draw_down(struct lausb *dev)
{
        int time;

        time = usb_wait_anchor_empty_timeout(&dev->submitted, 1000);
        if (!time)
                usb_kill_anchored_urbs(&dev->submitted);
        usb_kill_urb(dev->bulk_in_urb);
}

static int lausb_suspend(struct usb_interface *intf, pm_message_t message)
{
        struct lausb *dev = usb_get_intfdata(intf);

        if (!dev)
                return 0;
        lausb_draw_down(dev);
        return 0;
}

static int lausb_resume(struct usb_interface *intf)
{
        return 0;
}

static int lausb_pre_reset(struct usb_interface *intf)
{
        struct lausb *dev = usb_get_intfdata(intf);

        mutex_lock(&dev->io_mutex);
        lausb_draw_down(dev);

        return 0;
}

static int lausb_post_reset(struct usb_interface *intf)
{
        struct lausb *dev = usb_get_intfdata(intf);

        /* we are sure no URBs are active - no locking needed */
        dev->errors = -EPIPE;
        mutex_unlock(&dev->io_mutex);

        return 0;
}

static struct usb_driver lausb_driver = {
        .name =         "lausb",
        .probe =        lausb_probe,
        .disconnect =   lausb_disconnect,
        .suspend =      lausb_suspend,
        .resume =       lausb_resume,
        .pre_reset =    lausb_pre_reset,
        .post_reset =   lausb_post_reset,
        .id_table =     lausb_table,
        .supports_autosuspend = 1,
};

static int __init lausb_init(void)
{
	int result;
 
         /* register this driver with the USB subsystem */
         result = usb_register(&lausb_driver);
         if (result)
                 printk("nLaUSB usb_register failed. Error number %d", result);
	
	 printk("\nLaUSB driver has been successfully registered. Version: [%s]", driverVersion);
 
         return result;
}

static void __exit lausb_exit(void)
{
	/* deregister this driver with the USB subsystem */
         usb_deregister(&lausb_driver);
}
 
module_init(lausb_init);
module_exit(lausb_exit);

MODULE_DESCRIPTION("JSC \"Rudnev-Shilyaev\" USB driver for ADC/DAC boards");
MODULE_AUTHOR("JSC \"Rudnev-Shilyaev\" <soft@rudshel.ru>");
MODULE_LICENSE("GPL v2");
MODULE_SUPPORTED_DEVICE("{{Rudnev-Shilyaev,LA-I24USB},{Rudnev-Shilyaev,LA-50USB},{Rudnev-Shilyaev,LA-20USB},{Rudnev-Shilyaev,LA-2USB},{Rudnev-Shilyaev,LAn10-12USB},{Rudnev-Shilyaev,GSPF053USB},{Rudnev-Shilyaev,Sirius}}");
