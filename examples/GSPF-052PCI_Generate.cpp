/*!
 * \copyright 2015 JSC "Rudnev-Shilyaev"
 * 
 * \file GSPF-052PCI_Generate.cpp
 * \date 28.12.2015
 * 
 * \~english
 * \brief
 * Analog signal generation example for GSPF-052PCI device
 * 
 * \~russian
 * \brief
 * Пример генерации аналогового сигнала для устройства ГСПФ-052PCI
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

#ifdef RSH_MSWINDOWS
 #include <conio.h>
#endif

#include <iostream>
#include <iomanip>

using std::cout;
using std::cin;
using std::endl;

//================================Настройки==================================
// Служебное имя устройства, с которым будет работать программа.
const char* BoardName = "GSPF052PCI" ;
//Номер устройства. Если в системе установлено несколько одинаковых устройств, можно выбрать, к какому из них подключаться. Нумерация начинается с 1.
const U32 DeviceID = 1;
// Частота дискретизации
const double SampleFreq = 10000000.0;

// Частота генерируемого синуса в Герцах
const double  SignalFreq = 1000.0;   
// Амплитуда генерируемого синуса в вольтах      
const double SignalAmplitude = 1.0;       
// Язык вывода ошибок. (В случае проблем с кодировкой выбирайте английский язык)
const RSH_LANGUAGE ErrorLanguage = RSH_LANGUAGE_RUSSIAN; 
//const RSH_LANGUAGE ErrorLanguage = RSH_LANGUAGE_ENGLISH; 

//1МОм согласованная нагрузка. L Данная константа используется при расчете выходного напряжения генератора.
const double OutR = 1000000.0f; 


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
	printf("\nCore Library Version: %s\n", libCoreVersion.data);

	//============================= ПРОВЕРКА СОВМЕСТИМОСТИ =============================	
	RSH_U32 caps = RSH_CAPS_SOFT_GENERATION_IS_AVAILABLE; 
	//Проверим, поддерживает ли плата генерацию аналогового сигнала.
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

    //========================= ДОПОЛНИТЕЛЬНЫЕ ОСОБЕННОСТИ УСТРОЙСТВА ====================================
	
	cout << screenDelimiter << endl;
	cout << "Additional device features: " << endl;
	
	//Разрядность ЦАПа
	RSH_U32 DACResolution = 0;
    st = device->Get(RSH_GET_DEVICE_DATA_BITS, &DACResolution);  
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st, "device->Get(RSH_GET_DEVICE_DATA_BITS)");
	
	cout << "DAC resolution: " << DACResolution << "bits" << endl;
	
	//Минимальная частота дискретизации
	RSH_DOUBLE DACMinFreq = 0.0;
	device->Get(RSH_GET_DEVICE_MIN_FREQUENCY, &DACMinFreq); 
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st, "device->Get(RSH_GET_DEVICE_MIN_FREQUENCY)");	
	
	cout << "Minimum frequency: " << DACMinFreq << "Hz" << endl;
		
	//Максимальная частота дискретизации	
	RSH_DOUBLE DACMaxFreq = 0.0;
	device->Get(RSH_GET_DEVICE_MAX_FREQUENCY, &DACMaxFreq); 
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st, "device->Get(RSH_GET_DEVICE_MAX_FREQUENCY)");	
	
	cout << "Maximum frequency: " << DACMaxFreq << "Hz" << endl;
	
	//Максимальная амплитуда в вольтах
	RSH_DOUBLE DACRealMaxAmp;
	device->Get(RSH_GET_DEVICE_OUTPUT_RANGE_VOLTS, &DACRealMaxAmp);   
	
	cout << "Maximum out amplitude: " << DACRealMaxAmp << "V" << endl;

	//Расчет значения максимальной амплитуды в зависимости от сопротивления нагрузки
	double DACMaxAmp = 0.0;
	DACMaxAmp = OutR * (DACRealMaxAmp / (OutR + 50.0));  
	
	cout << "Maximum out amplitude for current output resist: " << DACMaxAmp << "V" << endl;

	//========================= ИНИЦИАЛИЗАЦИЯ ====================================	
	//Структура инициализации
	RshInitGSPF gspfPar;

    //Запуск генерациии - программный
	gspfPar.startType  = RshInitGSPF::Program;
	
	//Ослабление не используется
	gspfPar.attenuator = RshInitGSPF::AttenuationOff; 
	
	//Включаем циклическое проигрование сигнала. 
	//Альтернатива - прямое управление флагами управляющего слова:
	//gspfPar.control |= RshInitGSPF::PlayLoop;
	gspfPar.SetPlayModeLoop(); 
	
	//Частота дискретизации
	gspfPar.frequency = SampleFreq;

	//Инициализация параметров
	st = device->Init(&gspfPar);  
    if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st, "device->Init(&gspfPar)");
	
	cout << endl << "Initialization parameters:" << endl << gspfPar << endl;

	//========================= ФОРМИРОВАНИЕ СИГНАЛА (СИНУС) ====================================
	//Размер буфера для сигнала в отсчетах
	const U32 bSize = 100000;
	
	//Инициализируем буфер для данных
	RSH_BUFFER_S16 buffer(bSize); 
	
	//проверим, не получилось ли значение для амплитуды равным 0
	if(RSH_CMPDOUBLE(DACMaxAmp, 0.0))
	{
		return SayGoodBye(Client, RSH_API_PARAMETER_INVALID, "RSH_CMPDOUBLE(DACMaxAmp, 0.0)");
	}
	
	//Коэффициент для пересчета из вольт в МЗР
	double amplitudeCoef = SignalAmplitude / DACMaxAmp;

	//Заполняем буфер данными
	for(U32 i = 0; i < buffer.PSize(); ++i) 
	{
		S16 value = static_cast<S16>( amplitudeCoef * 32768 * sin(i * (2 * 3.1415926 * SignalFreq) / gspfPar.frequency) ); 
		buffer.PushBack(value);
	}

	//Если нужно использовать только часть буфера, можно указать размер (не выделяя память заново)
	// buffer.SetSize(new_size); //new_size должен быть меньше или равен buffer.PSize()

    //Отправляем буфер в устройство
	st = device->GetData(&buffer);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st, "device->GetData(&buffer)");
    
	cout << screenDelimiter << endl;
	cout << "Press any key to start generation..." << endl;
#if defined(RSH_MSWINDOWS)
	_getch();
#else
	getc(stdin);
#endif

	//Запускаем проигрывание данных
	st = device->Start();
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st, "device->Start()");	
	
		cout << "Waiting for interrupt after buffer generation completion..." << endl;
	
	//Ожидание прерывания (происходит каждый раз при завершении проигрывания буфера)
	st = device->Get(RSH_GET_WAIT_BUFFER_READY_EVENT);
	if (st != RSH_API_SUCCESS)
		return SayGoodBye(Client, st, "device->Get(RSH_GET_WAIT_BUFFER_READY_EVENT)");
		
	cout << "Interrupt has taken place!" << endl;
		
	return SayGoodBye(Client, RSH_API_SUCCESS);
}