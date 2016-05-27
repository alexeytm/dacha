/*
 * Разумная дача
 * модуль управления обогревом от трубы парилки
 * верси 1
 */
#include <DallasTemperature.h>  // Обслуживание датчиков темперературы
#include <OneWire.h>            // Работа с шиной данных (для Dallas)
#include <Servo.h>

#define DBEUG
// Период через который снимаются показания с термометров
#define MEASURE_PERIOD 6000

#define TEMP_IN_PIN   8       //Термометр Dallas 18 в комнате
#define TEMP_OUT_PIN  7       //Термометр Dallas 18 на трубе
#define HOLL          9       // Датчик холла (закрыта задвижка от вентиляторов)
#define PWM1          6       // вентилятор 1
#define PWM2          5       // вентилятор 2
#define SERVO_IN      10      // Задвижка от вентиляторов
#define SERVO_OUT     4       // Жалюзи в комнату

Servo servoIn, servoOut;
/* 
 * Состояние автомата 
 * 0 - выключено
 * 1 - ручное управлени
 * 2 - автоматика
 */
struct {
  // находится в состоянии автоматика
  byte state=0;
  // Состояние вентиляторов при старте (выключено)
  byte motor1=0;
  byte motor2=0;
  // Состояние заслонки и жалюзи
  byte servo1=0;
  byte servo2=0;
  // Измерение
  byte tempIn=0;
  byte tempOut=0;
  //Параметры системы изменились
  bool change = false;
} data;

// Инициализируем подсистему датчиков температуры
OneWire oneWire1(TEMP_IN_PIN);
DallasTemperature tempIn(&oneWire1);
OneWire oneWire2(TEMP_OUT_PIN);
DallasTemperature tempOut(&oneWire2);
/*
 * Измерение температуры
 */
void measure(unsigned long currentMillis){
  static unsigned long previousMillis;
  byte temperaIn, temperaOut;
  // Пришло время измерения параметров системы
  if( currentMillis - previousMillis >= MEASURE_PERIOD ) 
   {
    temperaIn  = tempIn.getTempCByIndex(0);
    temperaOut = tempOut.getTempCByIndex(0);
    if ( (data.tempIn != temperaIn) || (data.tempOut != temperaOut))
    {
      data.tempIn  = temperaIn;
      data.tempOut = temperaOut;
      data.change = true;
    }
   } else previousMillis = currentMillis;
}
/*
 *  Обработка данных и реакция на них
 */
void automatic( void ){
  // Система в режиме автоматика и данные по температуре изменились
  if ( (data.state == 1) && data.change){
    // В доме холодно!
    if ( (data.tempOut < 0) && (data.tempIn > data.tempOut ) ) { 
      data.servo1 = 180;  // Задвижка открыта на максимум
      data.servo2 = 180;  // Жалюзи открыто на максимум
      data.motor1 = 255;  // Двигатель 1 на максимум
      data.motor2 = 255;  // Двигатель 2 на максимум
    }
    // Дошли до нормальной температуры, гасим движки
    if ( data.tempOut > 23 ) { data.motor1 = 0; data.motor2 = 0; }
    // Становиться жарковато, вырубаем движки, закрываем жалюзи и задвижку
    if ( data.tempOut > 25 ) { data.motor1 = 0; data.motor2 = 0; data.servo1 = 0; data.servo2 = 0;}
  }
}
/*
 * Обработка нажатий клавиш
 */

/*void buttonClick(){
  static unsigned long millis_prevStart;    // Для устранения дребезка контактов кнопка "Старт" автоматики
  static unsigned long millis_prevStop;     // Для устранения дребезка контактов кнопка "Стоп"
  static unsigned long millis_prevMotor;    // Для устранения дребезка контактов кнопка "Вентиляторы"
  static unsigned long millis_prevServoIn;  // Для устранения дребезка контактов кнопка "Задвижка"
  static unsigned long millis_prevServoOut; // Для устранения дребезка контактов кнопка "Жалюзи"
  
  // Устранение "дребезга" контактов
  if(millis()-100 > millis_prev){
    // Кнопка нажата
  }
  else millis_prev = millis();
  
}*/

/*
 *      Подготовка системы к работе
 */
void setup() {
  // Инициализируем подсистему измерения 
  tempIn.begin(); 
  tempOut.begin(); 
  // Определяем выводы на ШИМ
  pinMode(PWM1,OUTPUT);
  pinMode(PWM2,OUTPUT);
  // Управление задвижкой и жалюзи
  servoIn.attach(SERVO_IN);
  servoOut.attach(SERVO_OUT);
}

/*
 * Основной цикл обработки данных
 */
void loop() {
  unsigned long currentMillis = millis();
  measure( currentMillis );  
}


