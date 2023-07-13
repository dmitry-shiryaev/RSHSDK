/*!
 * \copyright JSC "Rudnev-Shilyaev"
 * 
 * \file Python_Continuous.cpp
 * \date 17.03.2014
 * \version 1.0 [SDK 2.00]
 * 
 * \~english
 * \brief
 * Working with Python device in autostarting mode
 * 
 * \~russian
 * \brief
 * Работа с устройством Питон в режиме автозапуска
 * 
 */

#include <conio.h>

//Заголовочные файлы Rsh SDK. RshApi.cpp тоже включен с помощью #include для простоты
#include "RshApi.h"
#include "RshApi.cpp"

//Константы для управления калибратором
//Данные константы не включены в основные файлы SDK, т.к. используются только
//в устройстве Питон
#define PYTHON_CALIBRATOR_TIME_PULSE 1
#define PYTHON_CALIBRATOR_TIME_PERIOD 2
#define PYTHON_CALIBRATOR_AMPLITUDE_POSITIVE 3
#define PYTHON_CALIBRATOR_AMPLITUDE_NEGATIVE 4

//Если дефайн активен, будет осуществляться запись данных в файл
#define SAVE_DATA

// Язык вывода ошибок. (В случае проблем с кодировкой, выбирайте английский язык)
#define __ERROR_LANGUAGE__ RSH_LANGUAGE_RUSSIAN 
//#define __ERROR_LANGUAGE__ RSH_LANGUAGE_ENGLISH

//======================ПАРАМЕТРЫ СБОРА===========================
// Служебное имя устройства, с которым будет работать программа.
#define BOARD_NAME "Python"
//Номер устройства. Если в системе установлено несколько одинаковых устройств, можно выбрать, к какому из них подключаться. Нумерация начинается с 1.
#define DEVICE_ID 1
// Частота дискретизации (на канал).
#define SAMPLE_FREQ 1.0e+9
// Количество задействованных каналов
#define CHANNEL_NUMBER 2 




//Функция выводит сообщение об ошибке и завершает работу программы
U32 SayGoodBye(RshDllClient& Client, U32 st)
{
	std::cout<<"\n============================================================="<<std::endl;

	//вывод описания ошибки Rsh + описание системной ошибки (если доступно)
	RshError::PrintError(st, __ERROR_LANGUAGE__, true);

	//Выгрузка библиотеки абстракции
	Client.Free();
	std::cout<<"\n\nPress any key to end up the program.";
	_getch();
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
	RSH_S8P libname, libVersion, libCoreName, libCoreVersion;
	
	// Получаем экземляр класса устройства.
	st = Client.GetDeviceInterface(DEV_KEY); 
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);
	
	//===================== ИНФОРМАЦИЯ ОБ ИСПОЛЬЗУЕМЫХ БИБЛИОТЕКАХ =====================
	st = device->Get(RSH_GET_LIBRARY_FILENAME, &libname);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);
	else
		std::cout<<"\nLibrary Name: "<<libname.data<<std::endl;
			
	st = device->Get(RSH_GET_LIBRARY_VERSION_STR,  &libVersion);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);
	else
		std::cout<<"\nLibrary Version: "<<libVersion.data<<std::endl;

	//========================= ПОДКЛЮЧЕНИЕ К УСТРОЙСТВУ ====================================
	std::cout<<"Connecting now..."<<std::endl;
	// Подключаемся к устройству.
	S8* connectString = (S8*)"TCP:192.168.0.1:1024;UDP:8000";
	RshDeviceKey key = RshDeviceKey(connectString);
	st = device->Connect(&key, RSH_CONNECT_MODE_IP_ADDRESS);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);
	std::cout<<"Connected successfully!"<<std::endl;

	// Для подключения к устройству в данном случае используется передача строки с параметрами подключения (IP адрес и настройки порта)
	// Есть возможность использовать подключение по базовому адресу:
	// RshDeviceKey key = RshDeviceKey(1); //базовый адрес 1, будет использована дефолтная строка подключения - "TCP:192.168.0.1:1024;UDP:8000"
	// st = device->Connect(&key); //данный способ используется по умолчанию, поэтому второй параметр метода можно не указывать
	//

	//========================= РАБОТА С GPS ====================================
	//проверка поддержки данной возможности
	RSH_U32 caps = RSH_CAPS_DEVICE_GPS_MODULE_INSTALLED; 
	if(device->Get(RSH_GET_DEVICE_IS_CAPABLE,&caps) == RSH_API_SUCCESS)
	{
		//Можно получить данные в виде ANSI строки
		RSH_S8P gps;
		st = device->Get(RSH_GET_DEVICE_GPS_DATA,  &gps);
		if (st != RSH_API_SUCCESS)
			return SayGoodBye(Client, st);
		else
			std::cout<<"\nGPS data: "<<gps.data<<std::endl;
	
		//Или в формате UTF-16
		RSH_U16P gpsutf;
		st = device->Get(RSH_GET_DEVICE_GPS_DATA_UTF16,  &gpsutf);
		if (st != RSH_API_SUCCESS)
			return SayGoodBye(Client, st);
		else
			std::wcout<<L"\nGPS data utf16: "<<(wchar_t*)gpsutf.data<<std::endl;
	}
	else
	{
		std::cout<<"GPS data is not available!"<<std::endl;
	}
	
	//================== НАПРЯЖЕНИЕ ИСТОЧНИКА ПИТАНИЯ ============================
	RSH_DOUBLE powerVoltage = 0.0;
	st = device->Get(RSH_GET_DEVICE_POWER_SOURCE_VOLTAGE, &powerVoltage);
	if (st != RSH_API_SUCCESS)
			return SayGoodBye(Client, st);
	std::cout<<"Power supply voltage: "<<powerVoltage.data<<std::endl;

	//========================= ИНИЦИАЛИЗАЦИЯ ====================================
	//Параметры работы с устройством
	std::cout<<"\n-->Python performance test <--\n\n";


	//получим список допустимых размеров буфера (в отсчетах на канал):
	RSH_BUFFER_U32 sizeList;
	st = device->Get(RSH_GET_DEVICE_SIZE_LIST,&sizeList);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);

	//размеры не кратны 2, т.к. внутри библиотеки абстракции производится 
	//дополнительная обработка - часть данных приходится удалять для коррекции
	//и точной установки точки срабатывания синхронизации

	//Размер блока (в отсчетах на канал, будет откорректирован в меньшую сторону)
	U32 blockSize=sizeList[sizeList.Size()-1];
	std::cout<<"\nEnter buffer size {"<<sizeList[0]<<", ";
	for(U32 i = 1;i<sizeList.Size()-1;++i)
	{
		std::cout<<sizeList[i]<<", ";
		if(i%4 == 0)
			std::cout<<std::endl;
	}
	std::cout<<sizeList[sizeList.Size()-1]<<"}[default="<<blockSize<<"]:"<<std::endl;
	std::string input;
    std::getline( std::cin, input );
    if ( !input.empty() )
	{
        std::istringstream stream( input );
        stream >> blockSize;
    }
	
	blockSize = CutV(blockSize, sizeList[0], sizeList[sizeList.Size()-1]);

	//интервал автозапуска в мс
	U32 timeinterval=100;
	std::cout<<"\nEnter time interval(ms) [default=100]:";
	std::getline( std::cin, input );
    if ( !input.empty() )
	{
        std::istringstream stream( input );
        stream >> timeinterval;
    }

	//таймаут ожидания данных в мс
	U32 timeout=5000;
	std::cout<<"\nEnter timeout (ms) [default=5000]:";
	std::getline( std::cin, input );
    if ( !input.empty() )
	{
        std::istringstream stream( input );
        stream >> timeout;
    }

	//размер окна усреднения (для статистики пакетов)
	U32 window=100;
	std::cout<<"\nEnter averaging window size [default=100]:";
	std::getline( std::cin, input );
    if ( !input.empty() )
	{
        std::istringstream stream( input );
        stream >> window;
    }
	window = CutV(window,1UL,10000UL);



	// Установка интервала запуска
	RSH_U32 timer = timeinterval;
	st = device->Get(RSH_GET_DEVICE_AUTO_START_INTERVAL_SET, &timer);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);



	//Управление калибратором
	//Данные параметры нужно задать ДО вызова метода Init.
	//(параметры будут применены при вызове метода Init)
	//Для доступа к настройкам используется гет-код RSH_GET_DEVICE_REGISTER_BOARD_SPACE2_SET
	RshRegister calibParam;

	//Задаем амплитуду импульса - положительная полярность.
	//Амплитуда задается в диапазоне от 0 до 255.
	calibParam.offset = PYTHON_CALIBRATOR_AMPLITUDE_POSITIVE;
	calibParam.value = 10;
	st = device->Get(RSH_GET_DEVICE_REGISTER_BOARD_SPACE2_SET, &calibParam);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);

	//Задаем амплитуду импульса - отрицательная полярность.
	//Амплитуда задается в диапазоне от 0 до 255.
	calibParam.offset = PYTHON_CALIBRATOR_AMPLITUDE_NEGATIVE;
	calibParam.value = 10;
	st = device->Get(RSH_GET_DEVICE_REGISTER_BOARD_SPACE2_SET, &calibParam);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);

	//Задаем длительность импульса в нс
	//диапазон допустимых значений завист от выбранной частоты дискретизации:
	//для частоты 1ГГц (и режим удвоения) диапазон от 4 до 340нс, шаг - 4нс
	//для остальных частот - от 8 до 680нс шаг 8нс
	//Если заданное значение выходит за диапазон, оно будет откорректировано
	//при вызове метода Init.
	calibParam.offset = PYTHON_CALIBRATOR_TIME_PULSE;
	calibParam.value = 100;
	st = device->Get(RSH_GET_DEVICE_REGISTER_BOARD_SPACE2_SET, &calibParam);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);

	//Задаем период следования импульсов
	//диапазон допустимых значений завист от выбранной частоты дискретизации:
	//для частоты 1ГГц (и режим удвоения) диапазон от 12 до 1020нс, шаг - 4нс
	//для остальных частот - от 24 до 2040нс шаг 8нс.
	//При этом, период следования должен быть как минимум в 3 раза больше, чем
	//заданная длительность импульса.
	//Если заданное значение выходит за диапазон, оно будет откорректировано
	//при вызове метода Init.
	calibParam.offset = PYTHON_CALIBRATOR_TIME_PERIOD;
	calibParam.value = 800;
	st = device->Get(RSH_GET_DEVICE_REGISTER_BOARD_SPACE2_SET, &calibParam);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);

	// Структура для инициализации параметров работы устройства.
	RshInitMemory  p; 
	
	// Программный запуск устройства (синхронизация не используется).
	// Чтобы включить внутреннюю синхронизацию, нужно установить флаг RshInitMemory::Internal
	p.startType = RshInitMemory::Program; 
	// Размер блока данных (в отсчетах на канал)
	p.bufferSize = blockSize;
	// Частота дискретизации.
	p.frequency = SAMPLE_FREQ; 
	// Включение режима автозапуска. В этом режиме после вызова
	// метода Start() процесс запуска устройства и передачи данных 
	// будет осуществляться циклически, с заданным интервалом (RSH_GET_DEVICE_AUTO_START_INTERVAL_SET)
	// Программа сбора данных должна ожидать получения события готовности очередной порции данных и немедленно
	// получать их с помощью метода GetData(). Если данные не будут вычитаны до начала следующего цикла сбора данных
	// (заданный интервал), они будут потеряны.
	p.control |= RshInitMemory::AutoStartOn;

	//размер предыстории (актуально только при использовании синхронизации)
	//Для устройства Python предыстория задается не как обычно (в частях буфера от 0/16 до 15/16), 
	//а плавно - от 0 до 255. При этом, единица предыстории соответствует 8 точкам в буфере с данными 
	//(16 точек в режиме удвоения частоты).
	p.preHistory = 0;
	
	//настройка измерительных каналов
	for(int i=0;i<CHANNEL_NUMBER;++i)
	{
		//Сделаем канал активным.
		p.channels[i].control = RshChannel::Used; 
		
		//Зададим коэффициент усиления
		p.channels[i].gain = 1;			
	}

	//Если нужна синхронизация, можно задать канал, по которому
	//она будет осуществляться, включив соответствующий флаг
	//p.channels[0].control |= RshChannel::Synchro;

	// Инициализация параметров работы устройства и драйвера. 
	st = device->Init(&p); 
	// Параметры могут быть скорректированны, это отразится в структуре "p".
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);
		
	//==================== ИНФОРМАЦИЯ О ПРЕДСТОЯЩЕМ СБОРЕ ДАННЫХ =======================
	RSH_U32 activeChanNumber, serialNumber;
	st = device->Get(RSH_GET_DEVICE_ACTIVE_CHANNELS_NUMBER, &activeChanNumber);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);
	st = device->Get(RSH_GET_DEVICE_NAME_VERBOSE, &libname);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);
	st = device->Get(RSH_GET_DEVICE_SERIAL_NUMBER, &serialNumber);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);

	std::cout<<"\nThe name of the connected device: "<<libname.data<<std::endl;
	std::cout<<"nSerial number of the connected device: "<<serialNumber.data<<std::endl;
	std::cout<<"nData to be collected: "<<p.bufferSize*activeChanNumber.data<<"samples"<<std::endl;
	std::cout<<"nADC frequency: "<<p.frequency<<"Hz"<<std::endl;
	std::cout<<"nThe number of active channels: "<<activeChanNumber.data<<std::endl;
	std::cout<<"\n=============================================================\n";
		
	// Номер цикла (для именования файлов)  
	U32 loopNum = 0; 
	// Максимальное время ожидания готовности данных (в миллисекундах). 
	RSH_U32 waitTime = timeout;

	std::cout<<"\n\nPress Esc key to exit, any other key to start "<<BOARD_NAME<<"..."<<std::endl;
	if(_getch() == 27)
		return 0;	

	// Буфер с данными в МЗР.
	RSH_BUFFER_S16 userBuffer;
	// Выделим память под данные.
	st = userBuffer.Allocate( p.bufferSize * activeChanNumber.data ); 
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);

	// можно получить буфер с данными в вольтах, используя тип буфера RSH_BUFFER_DOUBLE
	// (при этом значения будут пересчитаны внутри библиотеки абстракции)

	//Переменные для сбора статистики
	int lastPacket = 0;
	int curPacket = 0;
	int totalPackets = 0;
	int goodPackets = 0;
	double* percents = new double[window];
	DWORD* timerIntervals = new DWORD[window];
	DWORD curTimer, lastTimer;
	double averagePercent = 0.0;
	double averageSpeed = 0.0;
	int maxpercent=0;
	loopNum = 0;	
	RshRegister reg;
	
	//размер собираемого блока данных в байтах (размер отсчета - 2 байта)
	double dataSizeBytes = p.bufferSize * activeChanNumber.data * 2;


	std::cout<<"\n--> Press ESC to stop"<<std::endl;
	// Запускаем процесс сбора данных.
	st = device->Start(); 
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st);

	curTimer = lastTimer = GetTickCount();

	//Основной цикл сбора данных (бесконечный, выход по нажатию Esc)
	do 
	{
		//ожидание готовности
		st = device->Get(RSH_GET_WAIT_BUFFER_READY_EVENT, &waitTime );
		if( st != RSH_API_SUCCESS )
			return SayGoodBye(Client, st);	


		// Получаем буфер с данными.	
		st = device->GetData( &userBuffer );
		if (st != RSH_API_SUCCESS)
			return SayGoodBye(Client, st);			

		curTimer = GetTickCount();
		//информация о номере фрейма (в данном случае используется код RSH_GET_DEVICE_REGISTER_BOARD_SPACE1,
		//чтобы не делать новых кодов - только для устройства Python)
		st = device->Get(RSH_GET_DEVICE_REGISTER_BOARD_SPACE1,&reg);
		if (st != RSH_API_SUCCESS)
			return SayGoodBye(Client, st);

		//Расчет статистики
		if(loopNum == 0 )
		{
			//самый первый фрейм				
			lastPacket = curPacket = reg.value;
			goodPackets = 1;
			totalPackets = 1;
		}
		else
		{
			//номер фрейма (метаданные от устройства)
			curPacket = reg.value;
			goodPackets = 1;
			totalPackets = (curPacket-lastPacket+255)%255;
			if(totalPackets == 0)
				totalPackets = 1;
		}

		percents[loopNum%window] = (double)goodPackets*100.0/(double)totalPackets;
		timerIntervals[loopNum%window] = curTimer - lastTimer;
		lastPacket = curPacket;
		lastTimer = curTimer;
		if(loopNum+1<window)
			maxpercent = loopNum+1;
		else
			maxpercent = window;

		averagePercent = 0.0;
		averageSpeed = 0.0;
		for(int i=0;i<maxpercent;++i)
		{
			averagePercent+=percents[i];
			averageSpeed += (double)timerIntervals[i];
		}
		averageSpeed = 1000.0*(maxpercent*dataSizeBytes)/averageSpeed/1048576.0;
		averagePercent /= maxpercent;
		printf("\rCnt: %5d; frm: %5d; Stats: %5.1f%% (%5.1f%% lost); speed: %3.2fMb/s", ++loopNum, curPacket, averagePercent,100.0-averagePercent,averageSpeed);


#ifdef SAVE_DATA
		// Сформируем имя файла.
		std::ostringstream ln; 
		ln << loopNum;
		std::string fn_tmp =			
			std::string(BOARD_NAME) +
			std::string("_auto") +
			std::string(ln.str())  +
			std::string(".dat");

		std::wstring FileName = std::wstring( fn_tmp.begin(), fn_tmp.end());

		// Сохраняем буфер с данными в мзр-формате на жесткий диск.
		st = userBuffer.WriteDataToFile(FileName.c_str()); 
		if( st != RSH_API_SUCCESS )	
			return SayGoodBye(Client, st);
#endif

		//Проверка нажатия клавиш
		if(_kbhit())
		{
			if( _getch() == 27)
			{
				//остановка сбора данных
				device->Stop();
				//выход из цикла
				break;
			}
		}

	}while(1);
	
	//успешное завершение работы
	return SayGoodBye(Client, RSH_API_SUCCESS);	
}
