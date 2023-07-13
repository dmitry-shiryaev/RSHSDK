/*!
 * \copyright 2015 JSC "Rudnev-Shilyaev"
 * 
 * \file LA-n2USB_StartStop.cpp
 * \date 28.12.2015
 * 
 * \~english
 * \brief
 * Data acquisition in "Start-Stop" mode for LA-n2USB device
 * 
 * \~russian
 * \brief
 * Сбор данных в режиме "Старт-Стоп" для устройства ЛА-н2USB
 * 
 */

//Файл RshApi.cpp включается директивой #include, чтобы содержащиеся в нем определения методов и функций
//были скомпилированы, иначе возникнут проблемы при линковке. 
//
//В крупных проектах может быть удобнее сделать файл-обертку rshsdkwrapper.cpp, внутри которого прописать
//#include <RshApi.cpp>
//и сделать его частью проекта.
//
#include <RshApi.cpp>


#include <iostream>
#include <sstream>
#include <iomanip>

using std::cout;
using std::cin;
using std::endl;

//RshApi не использует namespace 
//все названия классов, констант и функций начинаются с префикса Rsh


#if defined(RSH_MSWINDOWS)
 #include   <conio.h>
 //Полный путь к каталогу, в который будет произведена запись данных. 
 //Если используется пустая строка, файлы будут записываться в текущий каталог. 
 //Если будет указан реальный путь к какому-нибудь каталогу, в конце должен быть обратный слэш ("\\") - например "c:\\"
 const char* FilePath = ""; 
#elif defined(RSH_LINUX)
 //Полный путь к каталогу, в который будет произведена запись данных. 
 //Если используется пустая строка, файлы будут записываться в текущий каталог. 
 //Если будет указан реальный путь к какому-нибудь каталогу, в конце должен быть  слэш ("/") - например "/home/username/Desktop/"
 const char* FilePath ="";
#endif
// Язык вывода ошибок. (В случае проблем с кодировкой выбирайте английский язык)
const RSH_LANGUAGE ErrorLanguage = RSH_LANGUAGE_RUSSIAN; 
//const RSH_LANGUAGE ErrorLanguage = RSH_LANGUAGE_ENGLISH; 


//======================ПАРАМЕТРЫ СБОРА===========================
// Служебное имя устройства, с которым будет работать программа.
const char* BoardName = "LAN2USB" ;
//Номер устройства. Если в системе установлено несколько одинаковых устройств, можно выбрать, к какому из них подключаться. Нумерация начинается с 1.
const U32 DeviceID = 1;
// Частота дискретизации (на канал).
const double SampleFreq = 400.0e+6;
// Количество задействованных каналов
const U32 ChannelNumber = 1; 


// Размер собираемого блока данных в отсчётах (на канал).
const U32 BufferSize = 2097152;


const std::string screenDelimiter(80, '=');
const char* StringSuccess = "SUCCESS!";

//Функция выводит сообщение об ошибке и завершает работу программы
U32 SayGoodBye(RshDllClient& Client, U32 st, const char* desc=0)
{
	cout<<endl<<screenDelimiter<<endl;
	
	if(desc)
		cout << "Error on " << desc << " call." << endl;

	RshError::PrintError(st, ErrorLanguage, true);

	//вызов Free() выгружает все загруженные классом RshDllClient библиотеки.
	Client.Free();

	cout << endl << "Press 'Enter' key to end up the program.";

	cin.ignore();
	return st;
}


int main()
{
	//Статус выполнения операции. RSH_API_SUCCESS в случае успеха, либо код ошибки
	U32 st = RSH_API_SUCCESS; 
	//Указатель на объект с интерфейсом IRshDevice, который используется для доступа к устройству.
	IRshDevice* device = 0; 
	//Класс для загрузки библиотеки абстракции.
	RshDllClient Client;	

	//===================== ПОЛУЧЕНИЕ СПИСКА ЗАРЕГИСТРИРОВАННЫХ БИБЛИОТЕК =====================
	//В данном примере результаты этого вызова не используются,
	//так как мы заранее знаем имя библиотеки (константа BoardName)
	std::vector<std::string> allDevices;
	st = Client.GetRegisteredList(allDevices);
	if(st == RSH_API_SUCCESS)
	{
		cout << "List of registered device libraries:" << endl;
		for(std::vector<std::string>::iterator it = allDevices.begin();
			it != allDevices.end(); ++it)
		{
			cout << *it << endl;
		}
	}
	cout << screenDelimiter << endl;

	//===================== ЗАГРУЗКА КЛАССА ДЛЯ РАБОТЫ С УСТРОЙСТВОМ ИЗ БИБЛИОТЕКИ =====================
	//Инициализируем интерфейсный ключ.
	RshDllInterfaceKey DevKey(BoardName, device); 

	// Получаем экземляр класса устройства.
	cout << "Loading device interface from dynamic library... ";
	st = Client.GetDeviceInterface(DevKey); 
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st, "Client.GetDeviceInterface(DevKey)");
	else 
		cout << StringSuccess << endl;
	
	//После успешного выполнения вызова Client.GetDeviceInterface()
	//указатель device ссылается на инстанцированный объект,
	//который можно использовать для управления устройством
	
		
	//===================== ИНФОРМАЦИЯ ОБ ИСПОЛЬЗУЕМЫХ БИБЛИОТЕКАХ =====================
	RSH_S8P libname = 0;
	st = device->Get(RSH_GET_LIBRARY_FILENAME, &libname);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st, "device->Get(RSH_GET_LIBRARY_FILENAME)");
	else
		cout << "Library Name: "<< libname << endl;
		
	RSH_S8P libVersion = 0;
	st = device->Get(RSH_GET_LIBRARY_VERSION_STR,  &libVersion);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st, "device->Get(RSH_GET_LIBRARY_VERSION_STR)");
	else
		cout << "Library Version: " << libVersion << endl;
	
	RSH_S8P libCoreName = 0;
	st = device->Get(RSH_GET_CORELIB_FILENAME, &libCoreName);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st, "device->Get(RSH_GET_CORELIB_FILENAME)");
	else
		cout << "Core Library Name: " << libCoreName << endl;

	RSH_S8P  libCoreVersion = 0;
	st = device->Get(RSH_GET_CORELIB_VERSION_STR, &libCoreVersion);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st, "device->Get(RSH_GET_CORELIB_VERSION_STR)");
	else	
		cout << "Core Library Version: " << libCoreVersion << endl;
		
		
	//============================= ПРОВЕРКА СОВМЕСТИМОСТИ =============================
	// Проверим, поддерживает ли устройство режим работы "Старт-Стоп".
	RSH_U32 caps = RSH_CAPS_SOFT_GATHERING_IS_AVAILABLE;
	st = device->Get(RSH_GET_DEVICE_IS_CAPABLE, &caps); 
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st, "device->Get(RSH_GET_DEVICE_IS_CAPABLE)");
		
		
		
	//========================= ПОДКЛЮЧЕНИЕ К УСТРОЙСТВУ ====================================
	// Создаем ключ, по которому будет выполняться подключение (в данном случае используется базовый адрес)
	RshDeviceKey connectKey(DeviceID);
	
	// Подключаемся к устройству	
	cout << "Connecting to device (it may take some time)... ";
	st = device->Connect(&connectKey);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st,"device->Connect()");
	else 
		cout << StringSuccess << endl;		

	// Полное (точное) имя устройства и заводской номер доступны только после подключения к устройству
	RSH_S8P fullDeviceName = 0;
	st = device->Get(RSH_GET_DEVICE_NAME_VERBOSE, &fullDeviceName);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st, "device->Get(RSH_GET_DEVICE_NAME_VERBOSE)");
	else
		cout << "The name of the connected device: " << fullDeviceName << endl;

	RSH_U32 serialNumber = 0;
	st = device->Get(RSH_GET_DEVICE_SERIAL_NUMBER, &serialNumber);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st, "device->Get(RSH_GET_DEVICE_SERIAL_NUMBER)");
	else		
		cout << "Serial number of the connected device:  " << serialNumber << endl;
	
		
		
	//========================= ИНИЦИАЛИЗАЦИЯ ====================================
	cout << screenDelimiter << endl;
	cout << "--> Start-Stop data acquisition mode <--"<<endl;
	cout << screenDelimiter << endl;
	
	// Структура для инициализации параметров работы устройства.
	RshInitMemory  p; 

	// Программный запуск платы.
	p.startType = RshInitMemory::Program; 
	// Размер блока данных (в отсчетах на канал)
	p.bufferSize = BufferSize;
	// Частота дискретизации.
	p.frequency = SampleFreq; 

	//настройка измерительных каналов
	p.channels.SetSize(ChannelNumber);
	for(int i=0;i<ChannelNumber;++i)
	{
		//Сделаем канал активным.
		p.channels[i].SetUsed(); 
		
		//Зададим коэффициент усиления
		p.channels[i].gain = 1;			
	}
	
	// Инициализация параметров работы устройства и драйвера. 
	cout << "Initializing device with Init structure... ";
	st = device->Init(&p); 
	// Параметры могут быть скорректированны, это отразится в структуре "p".
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st, "device->Init(&p)");
	else 
		cout <<StringSuccess << endl;
		
		
		
	//==================== ИНФОРМАЦИЯ О ПРЕДСТОЯЩЕМ СБОРЕ ДАННЫХ =======================
	RSH_U32 activeChanNumber = 0; 
	st = device->Get(RSH_GET_DEVICE_ACTIVE_CHANNELS_NUMBER, &activeChanNumber);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st, "device->Get(RSH_GET_DEVICE_ACTIVE_CHANNELS_NUMBER)");

	if(activeChanNumber == 0)
	{
		cout << "ActiveChanNumber is zero, something is wrong!" << endl;
		return SayGoodBye(Client, RSH_API_PARAMETER_CHANNELWASNOTSELECTED);
	}		

	cout << endl << "Initialization parameters:" << endl << p << endl;
	cout << "The number of active channels: " << activeChanNumber << endl;
	cout << "The estimated time of gathering completion: " << (p.bufferSize / p.frequency ) <<"s"<< endl;
		
		// Номер цикла (для именования файлов)  
	U32 loopNum = 0; 
	// Максимальное время ожидания готовности данных (в миллисекундах). 
	RSH_U32 waitTime = 10000;
	
	// Буфер для данных (в МЗР)
	RSH_BUFFER_S16 userBuffer( p.bufferSize * activeChanNumber);
	// В данном случае желаемый размер буфера передается в конструкторе.
	// В дальнейшем можно изменить размер существующего буфера, используя вызов метода Allocate():
	// st = userBuffer.Allocate( p.bufferSize * activeChanNumber); 

	// Буфер для данных (в вольтах)
	RSH_BUFFER_DOUBLE userBufferD( p.bufferSize * activeChanNumber ); 

	
	do // Алгоритм сбора данных в режиме "Старт-Стоп".
	{
		cout << screenDelimiter << endl;
		cout << "Press Esc key to exit, any other key to start data acquisition..." << endl;
#if defined(RSH_MSWINDOWS)
		if(_getch() == 27) break;
#else
		if (getc(stdin) == 27) break;
#endif

		// Запускаем процесс сбора данных.
		st = device->Start(); 
		if (st != RSH_API_SUCCESS)
			return SayGoodBye(Client, st, "device->Start()");

		cout << "Collecting buffer... ";

		// Ожидаем готовность буфера.
		st = device->Get(RSH_GET_WAIT_BUFFER_READY_EVENT, &waitTime );
		if(st != RSH_API_SUCCESS)
			return SayGoodBye(Client, st, "device->Get(RSH_GET_WAIT_BUFFER_READY_EVENT)");
		
		cout << StringSuccess << endl;
		cout << "Interrupt has taken place!" << endl;

		//Вызов Stop() - чаще всего не нужен (прерывание получено, сбор уже завершен), но на всякий случай можно сделать.
		//Вызов Stop() сбрасывает процесс ожидания готовности данных и переводит устройство в состояние "готово к запуску"
		device->Stop();
		
		// Получаем буфер с данными (целочисленные данные - отсчеты АЦП).	
		st = device->GetData( &userBuffer );
		if (st != RSH_API_SUCCESS)
			return SayGoodBye(Client, st, "device->GetData( &userBuffer )");

		// Получаем буфер с данными (те же самые данные, но преобразованные в вольты).
		st = device->GetData(&userBufferD); 
		if (st == RSH_API_SUCCESS)
		{   
			//данная опция может поддерживаться не всеми устройствами, поэтому ошибка в данном случае не фатальная
			//если вызов успешный, печать содержимого буфера (первые 10 элементов)
			userBufferD.Print(10);
		} 

		// Сформируем имя файла.
		std::ostringstream nameStream; 
		nameStream << FilePath << BoardName << "_StartStop_" << std::setw(4) << std::setfill('0') << ++loopNum << ".dat";
		std::string fileName = nameStream.str();
			
		// Сохраняем буфер с данными в мзр-формате на жесткий диск.
		st = userBuffer.WriteBufferToFile(fileName); 

		if( st == RSH_API_SUCCESS )
		{			
			cout <<"Data was successfully collected and saved to " << fileName << endl;
			cout <<"File size: " << userBuffer.ByteSize()/1024.0 << "kilobytes" << endl;
		}
		else
		{
			cout << "Couldn't write to file " << fileName << endl;
			RshError::PrintError(st, ErrorLanguage, true);
		}	
	}while(true);
		
		
	return SayGoodBye(Client, RSH_API_SUCCESS);	
	
}
