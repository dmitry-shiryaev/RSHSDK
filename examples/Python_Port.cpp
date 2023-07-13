/*!
 * \copyright 2014 JSC "Rudnev-Shilyaev"
 * 
 * \file Python_Port.cpp
 * \date 03.03.2014
 * 
 * \~english
 * \brief
 * Digital port IO for Python device
 * 
 * \~russian
 * \brief
 * Работа с цифровым портом для устройства Питон
 * 
 */

#include "RshApi.cpp"

#if defined(RSH_MSWINDOWS)
#include   <conio.h>
#elif defined(RSH_LINUX)

#endif
// Язык вывода ошибок. (В случае проблем с кодировкой, выбирайте английский язык)
#define __ERROR_LANGUAGE__ RSH_LANGUAGE_RUSSIAN 
//#define __ERROR_LANGUAGE__ RSH_LANGUAGE_ENGLISH

//======================ПАРАМЕТРЫ СБОРА===========================
// Служебное имя платы, с которой будет работать программа.
#define BOARD_NAME "Python"
//Номер устройства. Если в системе установлено несколько одинаковых устройств, можно выбрать, к какому из них подключаться. Нумерация начинается с 1.
#define DEVICE_ID 1

//Функция выводит сообщение об ошибке и завершает работу программы
U32 SayGoodBye(RshDllClient& Client, U32 st)
{
	printf("\n=============================================================\n");

	RshError::PrintError(st, __ERROR_LANGUAGE__, true);
	Client.Free();
	printf("\n\nPress Enter key to end up the program.");
	getchar();
	return st;
}

int main()
{
	//Статус выполнения операции. RSH_API_SUCCESS в случае успеха, либо код ошибки
	U32 st; 
	//Указатель на объект с интерфейсом IRshDevice, который используется для доступа к устройству.
	IRshDevice* device; 
	//Класс для загрузки библиотеки абстракции.
	RshDllClient Client;	
	//Инициализируем интерфейсный ключ.
	RshDllInterfaceKey DEV_KEY(BOARD_NAME,(void**)&device); 

	// Получаем экземляр класса устройства.
	st = Client.GetDeviceInterface(DEV_KEY); 
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);
	
	//===================== ИНФОРМАЦИЯ ОБ ИСПОЛЬЗУЕМЫХ БИБЛИОТЕКАХ =====================
	RSH_S8P libname, libVersion;

	st = device->Get(RSH_GET_LIBRARY_FILENAME, &libname);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);
	else
		printf("\nLibrary Name: %s", libname.data);
			
	st = device->Get(RSH_GET_LIBRARY_VERSION_STR,  &libVersion);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);
	else
		printf("\nLibrary Version: %s\n", libVersion.data);	
		
	//============================= ПРОВЕРКА СОВМЕСТИМОСТИ =============================
	// Проверим, есть ли возможность управлять цифровыми портами.
	RSH_U32 caps = RSH_CAPS_SOFT_DIGITAL_PORT_IS_AVAILABLE; 
	st = device->Get(RSH_GET_DEVICE_IS_CAPABLE, &caps); 
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);
		
	//========================= ПОДКЛЮЧЕНИЕ К УСТРОЙСТВУ ====================================
	// Создаем ключ, по которому будет выполняться подключение (в данном случае используется базовый адрес)
	RshDeviceKey key(DEVICE_ID);
	
	// Подключаемся к устройству	
	printf("\nConnecting to device (it may take some time)... ");
	st = device->Connect(&key);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);
	printf("SUCCESS!\n");
		
		
		
	//==================== ИНФОРМАЦИЯ ОБ УСТРОЙСТВЕ И ЕГО ПОРТАХ =======================
	RSH_U32 serialNumber;	
	st = device->Get(RSH_GET_DEVICE_NAME_VERBOSE, &libname);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);
	st = device->Get(RSH_GET_DEVICE_SERIAL_NUMBER, &serialNumber);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);
		
	printf("\nBoard info: %s [%d]\n\n",libname.data, serialNumber.data);
	
	// Узнаем параметры портов устройства и выведем информацию о них.
	RshBoardPortInfo bpi;
	st = device->Get(RSH_GET_DEVICE_PORT_INFO, &bpi);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);

	if(bpi.ports.Size()>0)
	{
		wprintf(L"\nDigital ports:\n%10s %10s %10s", L"Port Name", L"Bit Size", L"Address");
		for(int i = 0 ; i < bpi.ports.Size() ; i++)
			wprintf(L"\n%10s %10d %10d", bpi.ports[i].name, bpi.ports[i].bitSize, bpi.ports[i].address);
	}
	else
	{
		printf("There are no digital ports (bpi.ports is empty)!\n");
		return SayGoodBye(Client, RSH_API_SUCCESS);
	}

	if(bpi.confs.Size()>0)
	{
		wprintf(L"\nConfiguration ports:\n%10s %10s %10s", L"Port Name", L"Bit Size", L"Address");
		for(int i = 0 ; i < bpi.confs.Size() ; i++)
			wprintf(L"\n%10s %10d %10d", bpi.confs[i].name, bpi.confs[i].bitSize, bpi.confs[i].address);
	}
	
	wprintf(L"\n=============================================================\n");
		
		
	printf("\n\nPress Esc key to exit, any other key to write data to digital port of %s...\n\n",BOARD_NAME);
#if defined(RSH_MSWINDOWS)
		if(_getch() == 27) return SayGoodBye(Client, RSH_API_SUCCESS);
#else
		if (getc(stdin) == 27) return SayGoodBye(Client, RSH_API_SUCCESS);
#endif


	// Структура инициализации для работы с портами.
	RshInitPort p;
	
	//Тип операции - запись данных в цифровой порт. 
	p.operationType = RshInitPort::Write; 	
	// Адрес порта. Данный идентификатор используется для обращения к нужному порту, если их несколько.
	//Устройство "Питон" имеет два программно управляемых цифровых порта: 7битный порт, работающий на выдачу данных
	//и 2битный порт, работающий на чтение.
	//В данном случае мы используем порт для выдачи данных на 7 цифровых линий.
	p.portAddress = bpi.ports[0].address; 
	// Данные для порта. 
	p.portValue = 0x7f; 
	
	// Инициализация - запись значения в порт
	st = device->Init(&p);	
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);
	
	wprintf(L"\n----> 0x%x successfully written to %s.", p.portValue, bpi.ports[0].name);

	printf("\n\nPress Esc key to exit, any other key to read data from digital port of %s...\n\n", BOARD_NAME);
#if defined(RSH_MSWINDOWS)
		if(_getch() == 27) return SayGoodBye(Client, RSH_API_SUCCESS);
#else
		if (getc(stdin) == 27) return SayGoodBye(Client, RSH_API_SUCCESS);
#endif

	// Чтение данных из цифрового порта.
	p.operationType = RshInitPort::Read;
	// Адрес порта для чтения. 
	p.portAddress = bpi.ports[1].address;
	// Занулим значение поля структуры.
	p.portValue = 0x00;
	// Операция чтения из порта.
	st = device->Init(&p);	
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);
	
	wprintf(L"\n----> 0x%x got from %s.", p.portValue, bpi.ports[1].name);
		
	return SayGoodBye(Client, RSH_API_SUCCESS);
	
}
