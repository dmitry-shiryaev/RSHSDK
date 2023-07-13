#include "../../../HEADERS/RshDefChk.h"

#if defined(RSH_LINUX)

#define MAGICK_NUMBER 0x800 //!< unique number to differentiate our set of ioctl calls from other ioctl calls

#pragma pack(1)

//! structure to transfer data with our ioctl call between user-space and kernel-space
typedef struct __IOCTL_BUFFER {
    int inSize; // size in bytes
    int outSize; // size in bytes
    unsigned char inBuffer[4096];
    unsigned char outBuffer[4096];
} IOCTL_BUFFER, *PIOCTL_BUFFER;

typedef struct  __SendPacketGETID
{
    unsigned short  vid;
    unsigned short  pid;
    unsigned short  rev;
} SendPacketGETID, *PSendPacketGETID;

#pragma pack()

#define IOCTL_BULK_GET_STATUS  _IOWR(MAGICK_NUMBER, 1, IOCTL_BUFFER)
#define IOCTL_GET_CONFIG       _IOR(MAGICK_NUMBER, 2, SendPacketGETID)
#define IOCTL_RESET_PIPE_WRITE _IO(MAGICK_NUMBER,  3)
#define IOCTL_RESET_PIPE_READ  _IO(MAGICK_NUMBER, 4)

#endif
