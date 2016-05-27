/*
 * SegmentKey
 * v 2.0.0
 * 2015 - 2016 
 * Кичев А.А. для любимой дачи (c)
 * 
 * Проект "задумчивая парилка" ^))
 * 
 * Обслеживание датчика темперетуры (воздуха и почвы) и влажности в теплице.
 * Используются датчики температуры Dallas DS18b20 в SOIC исполнении и водонепрницаемый (для почвы)
 * Влажность почвы определяем по сопротивлению.
 * 
 */

//#define DEBUG     // Для отладки и получения информации о датчиках температуры и записи их в EEPROM
#ifdef DEBUG
  #define GETINFO
#endif
#include <avr/sleep.h>
#include <avr/power.h>

#include <DallasTemperature.h>  // Обслуживание датчиков темперературы
#include <OneWire.h>            // Работа с шиной данных (для Dallas)
#include <SPI.h>                //
#include <EEPROM.h>             // работа с EEPROM

#include <Dacha.h>          // Моя библиотека для автоматики дачи

/*******************************************************
 * Основные определения для программы
 * 
 *******************************************************/
#define ONE_WIRE_BUS 14  //указываем порт на котором у нас висят датчики температуры (A0)
#define BTNPIN 3         // Вывод кнопки
#define NRF_IRQ 2        // Ввод контроллера прерыванй
#define ANALOG_IN  7     // Вход к которому подключаем датчик влажности

/* 
 * Переменная счётчика (volatile означает указание компилятору не оптимизировать код её чтения, 
 */
volatile int count = 0;  // поскольку её значение изменяется внутри обработчика прерывания)

/*
 * Структура с данными об адресах датчиков температуры и имени для радио модуля
 */
struct MyDevices{
  DeviceAddress insideThermometer;    // адрес внутреннего датчика температуры
  DeviceAddress outsideThermometer;   // адрес вешненего датчика температуры
  byte number;                        // идентификатор модуля
  byte varnum[3];                     // пределы для сыро, норма и сухо
};


MyDevices devices;

// Полученные данные по температурам и влажности
packet_teplica pck;

/* 
 * Инициализация подсистемы датчиков температуры 
 */
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void wakeup()
{
  sleep_disable();
  detachInterrupt(1);
}
 
void EnterSleep()
{
  #ifdef DEBUG
    Serial.println("Sleep");
    delay(100);
  #endif
  
  noInterrupts();
  sleep_enable();
  power_all_disable();
  attachInterrupt(1, wakeup,CHANGE);
  interrupts();
  //sleep_enable();
  sleep_cpu();
}

/****************************************************
 *     Процедура начальной инициализации модуля
 ***************************************************/
  void setup()
{
    #ifdef DEBUG
      Serial.begin(115200);
      while(!Serial) {}            
      Serial.println("Teplica");
      Serial.println("sensors.begin(); ");
    #endif
    // Инициализация подсистемы датчиков температуры
    sensors.begin(); 

/* *************************************************************************************
 * Получаем данные от датчиков (при инициализации) и если надо записываем их в EEPROM
 * 1. Коментируем запись в EEPROM и снимаем коментарий с getSetSensor
 * 2. Анализируем данные от датчиков и заполняем структуру devices
 * 3. Коментируем getSetSensor и снимаем коментарии с EEPROM.put
 ************************************************************************************* */  

    // Получение и установка параметров для модуля
    #ifdef GETINFO
      getSetSensor(sensors);
    #endif
      #ifdef DEBUG
     /*   Old devices
      *    MyDevices devices={
          { 0x28, 0xFF, 0x9B, 0xC3, 0x53, 0x14, 0x01, 0x28 },
          { 0x28, 0xFF, 0xF0, 0xEB, 0x64, 0x14, 0x01, 0x81 },
          4,     //4 метра например
          {800,400,200}
        };*/
 /*        MyDevices devices={               
          { 0x28, 0xFF, 0xB9, 0xF2, 0x53, 0x14, 0x01, 0x94 },
          { 0x28, 0xFF, 0xF0, 0xEB, 0x64, 0x14, 0x01, 0x81 },
          4,     //4 метра например
          {800,400,200}
         };*/
        // Читаем из EEPROM!
      /*  EEPROM.put(0, devices);     
        exit;*/
      #endif

 // Чтение основых параметров из EEPROM
   EEPROM.get(0, devices);
  // Инициализируем данные для передачи по радиоканалу и индикации
  pck.t1 = 0;
  pck.t2 = 0;
  pck.h1 = 0; 
  count = 1;
  //**
   #ifdef DEBUG
      Serial.println("Get data In EEPROM:");
      Serial.println("");
      Serial.print("Number device:");
      Serial.println(devices.number,DEC);
      Serial.print("Inside device:");
      printAddress(devices.insideThermometer);
      Serial.println("----");
      Serial.print("Outside device:");
      printAddress(devices.outsideThermometer);
      Serial.println("=====================");
    #endif
  
    // Инициализация подсистемы индикации
    led3Initialize();
    // Инициализация подсистемы прерываний
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}

/* *****************************************************
 * 
 * Основной цикл обработки данных
 * 
 * *****************************************************
 */
void loop()
{
   power_all_enable();
   // Читаем показания датчиков
   sensors.requestTemperatures();           // посылаем комманду запроса температуры
   pck.t1 = sensors.getTempC(devices.insideThermometer);   // читаем данные внутреннего термометра
   pck.t2 = sensors.getTempC (devices.outsideThermometer); // читаем данные внешнего термометра
   pck.h1 = analogRead(ANALOG_IN);          // Измеряем сопротивление почты
           
        #ifdef DEBUG
          Serial.print("DS18b20: ");
          Serial.print(pck.t1);
          Serial.print(" C; Analog termometr: ");
          Serial.print(pck.t2);
          Serial.print(" C; Higro: ");
          Serial.print(pck.h1);
          Serial.println(" UE");
        #endif
                
   led3DigitDisplay(pck.t1, 100);      // Отображаем температуру в теплице
   led3DigitDisplay(pck.t2, 100);      // Отображаем температуру почвы
   led3DigitDisplay(pck.h1, 100);      // Отображаем влажность почвы
   // Поработали а теперь пора спать!
   EnterSleep(); 
}

