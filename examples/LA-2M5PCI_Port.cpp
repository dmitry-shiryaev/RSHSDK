/*!
 * \copyright 2015 JSC "Rudnev-Shilyaev"
 * 
 * \file LA-2M5PCI_Port.cpp
 * \date 28.12.2015
 * 
 * \~english
 * \brief
 * Digital port IO for LA-2M5PCI device
 * 
 * \~russian
 * \brief
 * Работа с цифровым портом для устройства ЛА-2М5PCI
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


#ifdef RSH_MSWINDOWS
 #include <conio.h>
#endif

// Язык вывода ошибок. (В случае проблем с кодировкой выбирайте английский язык)
const RSH_LANGUAGE ErrorLanguage = RSH_LANGUAGE_RUSSIAN; 
//const RSH_LANGUAGE ErrorLanguage = RSH_LANGUAGE_ENGLISH; 


// Служебное имя устройства, с которым будет работать программа.
const char* BoardName = "LA2M5PCI" ;
//Номер устройства. Если в системе установлено несколько одинаковых устройств, можно выбрать, к какому из них подключаться. Нумерация начинается с 1.
const U32 DeviceID = 1;


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
	// Проверим, есть ли возможность управлять цифровыми портами.
	RSH_U32 caps = RSH_CAPS_SOFT_DIGITAL_PORT_IS_AVAILABLE; 
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
	
		
		
		
		
	// Узнаем параметры цифровых портов устройства и выведем информацию о них.
	RshBoardPortInfo bpi;
	st = device->Get(RSH_GET_DEVICE_PORT_INFO, &bpi);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);

	cout << screenDelimiter << endl;
	cout << BoardName << " port configuration" << endl << endl;

	if(bpi.ports.Size()>0)
	{
		std::cout << "Digital ports:" << endl;
		std::cout << std::setw(10) << "Port Name" << std::setw(10) << "Bit Size" << std::setw(10) << "Address" << endl;

		for(U32 i = 0; i < bpi.ports.Size(); ++i)
			std::wcout << std::setw(10) << bpi.ports[i].name << std::setw(10) << bpi.ports[i].bitSize << std::setw(10) << bpi.ports[i].address << endl;
	}
	else
	{
		cout << "There are no digital ports (bpi.ports is empty)!" << endl;
		return SayGoodBye(Client, RSH_API_SUCCESS);
	}

	if(bpi.confs.Size()>0)
	{
		std::cout << "Configuration ports:" << endl;
		std::cout << std::setw(10) << "Port Name" << std::setw(10) << "Bit Size" << std::setw(10) << "Address" << endl;

		for(U32 i = 0; i < bpi.confs.Size(); ++i)
			std::wcout << std::setw(10) << bpi.confs[i].name << std::setw(10) << bpi.confs[i].bitSize << std::setw(10) << bpi.confs[i].address << endl;
	}
		
		
	cout << screenDelimiter << endl;
	cout << "Press Esc key to exit, any other key to write data to digital port..." << endl;
#if defined(RSH_MSWINDOWS)
		if(_getch() == 27) return SayGoodBye(Client, RSH_API_SUCCESS);
#else
		if (getc(stdin) == 27) return SayGoodBye(Client, RSH_API_SUCCESS);
#endif

	// Структура инициализации для работы с портами.
	RshInitPort p;
	
	//Тип операции - запись данных в цифровой порт.
	p.operationType = RshInitPort::Write; 	
	// Адрес порта.
	p.portAddress = bpi.ports[0].address; 
	// Данные для порта.
	p.portValue = 0xff; 
	
	// Инициализация - запись значения в порт
	st = device->Init(&p);	
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st, "device->Init(&p);");
	
	std::wcout << "0x" << std::hex << p.portValue << L" was successfully written to " << bpi.ports[0].name << endl;
	
	cout << screenDelimiter << endl;
	cout << "Press Esc key to exit, any other key to read data from digital port..." << endl;
	
#if defined(RSH_MSWINDOWS)
	if(_getch() == 27) return SayGoodBye(Client, RSH_API_SUCCESS);
#else
	if (getc(stdin) == 27) return SayGoodBye(Client, RSH_API_SUCCESS);
#endif

	// Чтение данных из цифрового порта.
	p.operationType = RshInitPort::Read;
	// Адрес порта.
	p.portAddress = bpi.ports[0].address;
	// Занулим значение поля структуры.
	p.portValue = 0x0;
	// Операция чтения из порта.
	st = device->Init(&p);	
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st, "device->Init(&p)");
	
	std::wcout << "Read operation from " << bpi.ports[0].name << " was successfull, value=0x" << std::hex << p.portValue << endl;

		
		
	return SayGoodBye(Client, RSH_API_SUCCESS);	
	
}
