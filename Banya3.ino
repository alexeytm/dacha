/*
 * Смеклистый огород
 * Модуль умная банька
 * Версия 3 от 2016 года, начало проекта 2015 год
 */

#include <math.h>
#include <Wire.h>
#include <HTU21D.h>
#include <DHT.h>
#include <MAX6675.h>
#include <SPI.h>
#include <Dacha.h>
#include "FlashLed.h"

// Режим отладки (вывод на последовательный порт)
//#define DEBUG

// Определения для работы таймера для парной
// Вермя горения светодиода таймера
#define MIN_BLINK_MS  200
// Период по 10 секундам
#define PERIOD10     10000
#define PERIOD10on   PERIOD10-MIN_BLINK_MS
#define PERIOD10off  PERIOD10
// Период по минутному интервалу
#define PERIOD60     60000
#define PERIOD60on   PERIOD60-MIN_BLINK_MS*3
#define PERIOD60off  PERIOD60-MIN_BLINK_MS*2
// Времена для кнопки
#define HOLD_TIME     2000    // время "удерживания" до Помогите!!! (2 секунды)
#define PERIOD_TIME   1000    // период ожидания между нажатиями не более 1 секунды

#define ON    HIGH
#define OFF   LOW

// Период через который идет измерение
#define MEASURE_PERIOD 30000
// Период мигания светодиодв
#define BLINK_LED 1500
// Порт, на котором сидит вентилятор
#define PIN_MOTOR 10
//Порт, на котором висит светодиод таймера

// Определение структуры пакета передачи данных в головной модуль
typedef union {
  packet_t packet;
  byte bytes[sizeof(packet_t)];
} SendByte;

SendByte outByte;

unsigned long previousMillis; // последний момент измерения
volatile int KeyStateTimer;   // Состояние таймера на запуск
volatile boolean nowMeassure; // Досрочный запрос на измерение параметров
volatile boolean warning;     // Помогите!!!!
boolean LedIsOn;

// Датчик в парной
//HTU21D sensor;

// Определяем DHT датчики для 16mhz Arduino
DHT dht1(8, DHT22);  // 8 температура и влажность за изоляцией
DHT dht2(7, DHT21);  // 7 температура и влажность на улице
DHT dht3(9, DHT22);  // 9 температура и влажность в предбаннике
// Подключаем термопару (последний параметр 1- в цельсиях)
MAX6675 thermocouple(4,5,6,1);

// Определяем где какие светотдиоды, как они минают
// Вода в баке (12 - синий, 11 - красный
Flasher LedHotWater(11, BLINK_LED, BLINK_LED);
Flasher LedColdWater(12, BLINK_LED, BLINK_LED);
// Воздух в парной
Flasher LedHotAir(14, BLINK_LED, BLINK_LED);
Flasher LedColdAir(16, BLINK_LED, BLINK_LED);
Flasher LedInfo(13, BLINK_LED, BLINK_LED);

/*
 * Управление светодиодом таймера, что есть вещь особая!
 */
void LedOff(){
   digitalWrite(2, OFF);
   LedIsOn= false;
   pinMode(2, INPUT);
//   attachInterrupt(0, buttonClick, CHANGE);
#ifdef DEBUG
    Serial.println("Led Off");
#endif
}
void LedOn(){
//   detachInterrupt(0);
  LedIsOn= true;
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
#ifdef DEBUG
    Serial.println("Led On");
#endif
}

/* ***********************************************************************************
        Светодиод - таймер
        Каждые 10 секунд - мигаем 1 раз
        каждую минуту мигаем 2 раза
        По окончании мигаем 3 раза
        state = 1 - 3 минуты
        state = 2 - 5 минут
        state = 3 - 10 минут
        state = 4 - 15 минут
*********************************************************************************** */
void LedTimer(unsigned long currentMillis, int state=0){
  static unsigned long previous10Millis,previous60Millis,previousMillis;   // начали в момент времени
  static unsigned long period10Millis,period60Millis,periodMillis;       // Прошедшее время абсолютное
  static unsigned long periodOn,periodOff,period;     //
  static bool ledState;             // Предидущее состояние светодиода
  bool ledOnSec;                    // В каком состоянии должен теперь находится светодиод
  static bool timerRun;             // таймер запущен
  static int countState;            // Сколько раз помигать диодом перед началом работы таймера

  // Устанавливаем период таймера и запускаем его в работу
  if ( state ==  -1 ) { timerRun = false; return;}
  if ( state !=0 ){
    previous10Millis = currentMillis;
    previous60Millis = currentMillis;
    previousMillis   = currentMillis;
    ledState    = false;    // текущее состояние светодиода 
    ledOnSec    = false;    // светодиод по всему таймеру
    timerRun    = true;     //
    switch ( state ){
      case 1:
        period    = 180000;  // 3 минуты
        break;
      case 2:
        period    = 300000; // 5 минуть
        break;
      case 3:
        period    = 600000;  // 10 минуть
        break;
      case 4:
      default:
        period    = 900000;  // 15 минуть
        break;
    }
    periodOn  = period - MIN_BLINK_MS*5;
    periodOff = period - MIN_BLINK_MS*4;
    period    += MIN_BLINK_MS*5;
    countState = state;
    LedOn();
    #ifdef DEBUG
      Serial.print("Initial parameter");
      Serial.println(state);
    #endif
    return;
  }

  // Если начинаем работу, с начала помигаем на сколько установлен таймер
  if (countState > 0)
  {
    if (ledState && (currentMillis - previous10Millis) > MIN_BLINK_MS)
    {
        previous10Millis = currentMillis;
        ledState = false;
        LedOff();
        countState--;
    #ifdef DEBUG
      Serial.print("countState :");
      Serial.print(countState);
    #endif

    }
    if (!ledState && (currentMillis - previous10Millis) > MIN_BLINK_MS)
    {
        previous10Millis = currentMillis;
        ledState = true;
        LedOn();
    }
    return;
  }
  
  // Проверяем состояние таймера, если он запущен, идем дальше.
  if ( !timerRun ) return;
  // Расчет фактически прошедшего времени
  period10Millis = currentMillis-previous10Millis;
  period60Millis = currentMillis-previous60Millis;
  periodMillis   = currentMillis-previousMillis;
  
  // Мигаем каждые 10 секунд ( 10000 мл сек )
  if ( period10Millis <= PERIOD10 ) 
    ledOnSec = ( (period10Millis >= PERIOD10on ) && (period10Millis <= PERIOD10off) );
  else previous10Millis = currentMillis;

  // Мигаем каждые 60 секунд, после 10 секндного со смещением в пол перирода ( 60000 мл сек )
   if ( period60Millis <= PERIOD60 )
    ledOnSec |= ( (period60Millis >= PERIOD60on ) && (period60Millis <= PERIOD60off) );
  else previous60Millis = currentMillis;

  // Мигаем по окончании периода счета со смещением в пол перирода (state)
  // Как дошли до конца - выключаем таймер
   if ( periodMillis <= period ) {
    ledOnSec |= ( (periodMillis >= periodOn ) && (periodMillis <= periodOff) );
  } else timerRun = false;
  // Состояние не поменялось - выходим
  if ( ledState == ledOnSec ) return;
  // Новое состояние светодиода
  ledState = ledOnSec;
  /* 
   * Теперь меняем состояние порта 
   * ВНИАМНИЕ!! Нельзя напрямую менять состояние порта! Там кнопка с землей!
   * Всем хана придет!
   */
  if ( ledState ) LedOn(); else LedOff();
  return;
}
/* ==================================================================================
 * 
 *                            Подготовка программы к запуску
 * 
 * ===================================================================================
 */
void setup()
{
  // Порт на вывод, включаем свентодиод
  #ifdef DEBUG
    Serial.begin(9600);
    Serial.println("Setup");
  #endif

  /*if (sensor.begin() != true)
  {
    Serial.println(F("HTU21D sensor is not present"));
    delay(5000);
  }*/

  LedHotWater.On();
  LedColdWater.On();
  LedHotAir.On();
  LedColdAir.On();
  
  #ifdef DEBUG
    Serial.println("Init");
  #endif
  
  nowMeassure = true;
  warning = false;
  previousMillis = 0;
  KeyStateTimer = 0;

  LedHotWater.Off();
  LedColdWater.Off();  
  LedHotAir.Off();
  LedColdAir.Off();
  LedInfo.On();
  /* 
   * До достижения температуры в 60 градусов, горит светодиод таймера, потому как нефиг 
   * сидеть в парной!
   */
  pinMode(PIN_MOTOR, OUTPUT);
  digitalWrite(PIN_MOTOR,LOW);   // Включаем вентилятор
}
/* ============================================================================
 * 
 *                                Основноый цикл программы
 * 
 *  ===========================================================================
 */
void loop()
{
   unsigned long currentMillis = millis(); // текущее время в миллисекундах
   // Нажата и удерживается кнока, АВАРИЯ! Внимание!!!!!
  if ( warning ) {
    // Пока еще нажата кнопка, ждем пока ее отпустят!
    while (!digitalRead(2)) ;
    warning = false;
    LedInfo.Change( BLINK_LED/2, BLINK_LED/2 );
  }
  // Даем управление на таймер
  LedTimer(currentMillis);
  // работаем по кнопке
  buttonClick(currentMillis);
  doMeasurement(currentMillis);
  // Определяем поведение светодиодов
  // Светодиодная иллюминация
  LedHotWater.Update(currentMillis);
  LedColdWater.Update(currentMillis);
  LedHotAir.Update(currentMillis);
  LedColdAir.Update(currentMillis);
  LedInfo.Update(currentMillis);
}

// Процедура измерения параметров
void doMeasurement(unsigned long currentMillis)
{
//Чтение датчиков и подготовка структыры передачи данных серии DHT
   /*float t1;    
    byte  h1;    // влажность в парной
    float t2;    // температура за изоляцией парной
    byte  h2;    // влажность за изоляцией парной
    float t3;    // температура на улице
    byte  h3;    // влажность на улице
    float t4;    // температура в предбаннике
    byte  h4;    // влажность в предбаннике
    float t5;    // температура воды в баке
    byte key;    // Нажата кнопка (количество 1,2,3; 10 - Вызов
   */
  
   if((nowMeassure) || (currentMillis - previousMillis >= MEASURE_PERIOD))
   {
      #ifdef DEBUG
        Serial.println("Measure");
      #endif
      nowMeassure = false;
      // температура и влажность в парной
      // Пока только термометр сопротивления!!!
      outByte.packet.t1 = Thermister(analogRead(A5));
      
/*    outByte.packet.h1 = Thermister(analogRead(A4));
      si7021_env data = sensor.getHumidityAndTemperature();
      outByte.packet.t1 = sensor.getCelsiusHundredths();
      outByte.packet.h1 = sensor.getHumidityPercent();   
      outByte.packet.t1 = data.celsiusHundredths-data.celsiusHundredths/100*100;
      outByte.packet.h1 = data.humidityBasisPoints-data.humidityBasisPoints/100*100;
      */
/*      outByte.packet.t1 = sensor.readTemperature();
      outByte.packet.h1 = sensor.readHumidity();*/
      #ifdef DEBUG
/*        outByte.packet.t1 = 25;
        outByte.packet.h1 = 67;
        Serial.println("si7021");*/
        Serial.print("T = ");
        Serial.print(outByte.packet.t1);
        Serial.println(" C");
/*        Serial.print(",H = ");
        Serial.print(outByte.packet.h1);
        Serial.println(" %");*/
      #endif            
      // температура и влажность за изоляцией парной 
      outByte.packet.t2 = dht1.readTemperature();
      outByte.packet.h2 = byte(dht1.readHumidity());  
      #ifdef DEBUG
        Serial.println("DHT1");
        Serial.print("T = ");
        Serial.print(outByte.packet.t2);
        Serial.print(" C");
        Serial.print(",H = ");
        Serial.print(outByte.packet.h2);
        Serial.println(" %");
      #endif            
      // температура и влажность на улице
      outByte.packet.t3 = dht2.readTemperature();
      outByte.packet.h3 = byte(dht2.readHumidity());
      #ifdef DEBUG
        Serial.println("DHT2");
        Serial.print("T = ");
        Serial.print(outByte.packet.t3);
        Serial.print(" C");
        Serial.print(",H = ");
        Serial.print(outByte.packet.h3);
        Serial.println(" %");
      #endif            
      // температура и влажность в предбаннике
      outByte.packet.t4 = dht3.readTemperature();
      outByte.packet.h4 = byte(dht3.readHumidity());
      #ifdef DEBUG
        Serial.println("DHT3");
        Serial.print("T = ");
        Serial.print(outByte.packet.t4);
        Serial.print(" C");
        Serial.print(",H = ");
        Serial.print(outByte.packet.h4);
        Serial.println(" %");
      #endif               
      // Показания термопары! устанавливаем параметры мигания
      outByte.packet.t5 = thermocouple.read_temp();
      #ifdef DEBUG
//        outByte.packet.t5 = 85;
        Serial.println("Termopare");
        Serial.print("T = ");
        Serial.print(outByte.packet.t5);
        Serial.println(" C");        
      #endif            

     // Устанавливаем параметры мигания по температуре в парной
      if (outByte.packet.t1<=30)
      {
        LedColdAir.On();
        LedHotAir.Off();
#ifndef DEBUG
        LedOn();
#endif
      } else if ((outByte.packet.t1>30)&&(outByte.packet.t1<=60))
      {
        LedColdAir.Change(BLINK_LED, BLINK_LED);
        LedHotAir.Off();
#ifndef DEBUG        
        LedOff();
#endif        
      } else if ((outByte.packet.t1>60)&&(outByte.packet.t1<=80))
      {
        LedColdAir.Off();
        LedHotAir.Change(BLINK_LED, BLINK_LED);
#ifndef DEBUG        
        LedOff();
#endif        
      } else {
        LedColdAir.Off();
        LedHotAir.On();
#ifndef DEBUG        
        LedOff();
#endif        
      }
      
     // Устанавливаем параметры мигания по воде
      if (outByte.packet.t5 <= 20) {
        LedColdWater.On();
        LedHotWater.Off();
      }
      else if ((outByte.packet.t5 > 20)&&(outByte.packet.t5 <= 40))
      {
        LedColdWater.Change(BLINK_LED, BLINK_LED);
      }
      else if ((outByte.packet.t5 > 40)&&(outByte.packet.t5 <= 50))
      {
        LedColdWater.On();
        LedHotWater.On();
      }
      else if ((outByte.packet.t5 > 50)&&(outByte.packet.t5 <= 80))
      {
        LedColdWater.Off();
        LedHotWater.Change(BLINK_LED, BLINK_LED);
      }
      else if ((outByte.packet.t5 > 80)&&(outByte.packet.t5 <= 100))
      {
        LedHotWater.On();
        LedColdWater.Off();
      }
        
      // Передача данных 
      #ifndef DEBUG
        if (Serial)
           Serial.write(outByte.bytes,sizeof(outByte.bytes));
      #endif
      
      previousMillis = currentMillis;
   }
}
// Измерение температуры через сопротивление
double Thermister(int RawADC) {
  double Temp;
  Temp = log(((10240000/RawADC) - 10000));
  Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * Temp * Temp ))* Temp );
  Temp = Temp - 273.15;   // Kelvin to Celcius
  return Temp;
}

/*
 * Обработка прерывания от нажатия кнопки
*/
void buttonClick(unsigned long currentMillis){
  static boolean lastKeyState;        // Предидущее состояние кнопки
  static unsigned long pressTime;            // переменная времени нажатия кнопки
  
  if ( LedIsOn ) return;              // Если горит светодиод, нельзя понять что с кнопкой, выходим
  boolean pressKey = !digitalRead(2); // Текущее стостояние кнопки
  
  // На кнопку нажали, запоминаем время нажатия
  if ( pressKey && !lastKeyState ) pressTime = currentMillis;
  // Устраняем дребез контактов
  if ( (pressKey&&lastKeyState)&&(currentMillis-pressTime) > HOLD_TIME ){
    LedTimer(currentMillis ,-1);
    warning = true;
    KeyStateTimer = 0;
    pressTime = currentMillis;
#ifdef DEBUG
        Serial.println("Press long");
#endif
  }
  //
  if ( (!pressKey && lastKeyState) && ((currentMillis-pressTime) > 100)){
     KeyStateTimer++;
     Serial.print("Press short");
     Serial.println(KeyStateTimer );
     pressTime = currentMillis;
  }
// Если истек период, и никто не жал на ПОМОГИТЕ!
  if (!warning&&(KeyStateTimer != 0) && (currentMillis-pressTime) > PERIOD_TIME ) {
    LedTimer(currentMillis ,KeyStateTimer+1);
    LedInfo.On();
    KeyStateTimer = 0;
  }

  lastKeyState = pressKey;
}

