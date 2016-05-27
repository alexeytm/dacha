/*
 * 
 * Смекалистая дачка
 * модуль управление насосов
 * версия 1
 */
#define DEBUG

#include <DallasTemperature.h>  // Обслуживание датчиков темперературы
#include <OneWire.h>            // Работа с шиной данных (для Dallas)

// Расстояние до датчика от поверхности воды
#define MINLEVEL    88    // "пустой" бак
#define MAXLEVEL    1     // полный бак

#define ZUMMER_PIN  9

#define PUMP1_PIN   5
#define PUMP2_PIN   6

#define TEMPERA_PIN 4

#define DATA_PIN    15
#define LATCH_PIN   17
#define CLOCK_PIN   16

#define RELAY_PIN  14

#define ECHO_PIN    7
#define TRIG_PIN    8

#define HOLL_PIN    A6
#define BTN_PIN     A7

#define OFF         false
#define ON          true

#define MEASURE_PERIOD 30000

volatile bool state; // Состояние false - стоим, true - работаем.
volatile bool isFull,isPump1,isPump2,isRelay,isChange;
byte tempWater;
int level;
byte byteArray[8]={0b00000001,0b00000011,0b00000111,0b00001111,0b00011111,0b00111111,0b01111111,0b11111111};

OneWire oneWire(TEMPERA_PIN);
DallasTemperature tempera(&oneWire);
void displayLevel( int level, bool clapan=false, bool ozon=false,bool pump1=false, bool pump2=false);
/*
 * Прерывания
 */
void pushButton( void ){
  static unsigned long millis_prev; // Для устранения дребезка контактов

  // Устранение "дребезга" контактов
  if(millis()-100 > millis_prev) {
    state = !state;
    isChange = true;
  } else millis_prev = millis();
}
void full ( void ){
  static unsigned long millis_prev; // Для устранения дребезка контактов

  // Устранение "дребезга" контактов
  if(millis()-100 > millis_prev) {
    isFull = true;
    isChange = true;
  } else millis_prev = millis();  
}
/*
 * Измерение дистанции
 */
int duration( void );     // измерение расстояния

/*
 * Инициализация системы
 */
void setup() {
  #ifdef DEBUG
    Serial.begin(9600);
    Serial.println("Initialize");
    delay(10);
  #endif
  isFull  = false;
  isPump1 = false;
  isPump2 = false;
  isRelay = false;
  isChange= false;
  // Настройка на прерывания
  attachInterrupt(0, pushButton, LOW);
  attachInterrupt(1, full, RISING);
  #ifdef DEBUG
    Serial.println("Interrupt");
  #endif
  // Настраиваем выводы на измерение расстояния до зеркала воды
  pinMode(TRIG_PIN ,OUTPUT);
  pinMode(ECHO_PIN ,INPUT );
  #ifdef DEBUG
    Serial.println("Sonar");
  #endif
  // Настраиваем выводы управление светодиодным модулем
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN,  OUTPUT);
  #ifdef DEBUG
    Serial.println("Display");
  #endif
  // Выводы на насосоы
  actionPump(0, OFF );
  actionPump(1, OFF );
  // Управление релюшками
  pinMode(RELAY_PIN, OUTPUT);
  relayOff();
  /*
  tone(ZUMMER_PIN,245);
  delay(10);
  noTone(ZUMMER_PIN);
  */
}

void loop() {
  // put your main code here, to run repeatedly:
  #ifdef  DEBUG
    Serial.println(" cnt"); 
  #endif
  display();
  //TestOut();
}

void measure(unsigned long currentMillis ) {
  unsigned long previousMillis;

  if (currentMillis - previousMillis >= MEASURE_PERIOD) {
     // Измеряем температуру в баке
     tempWater = tempera.getTempCByIndex(0);
     // Измеряем уровень воды в баке
     int localLevel = duration();
     // Изменились ли показания?
     isChange = (isChange || (level != localLevel));
     level = localLevel;
     isFull = (level <= MAXLEVEL);
  } else previousMillis = currentMillis;
}

// Исполнительный механизм
void action( void ) {

  if ( !isChange ) return;
  // Остановть насосы если они запущены и
  if ( isFull && isPump1 && isPump2 ){
      actionPump( 0, OFF );
      actionPump( 1, OFF );
      return;
  }
}
/*
 ********************  Система управление насосами ********************
 *  ON - плавный запуск двигателя
 *  OFF - остановка двигателя
 */
// Управление насосами
void actionPump( int pump, bool stat ){
  int pump_pin;
  if ( pump == 0 ){
    pump_pin = PUMP1_PIN;
    isPump1 = stat;
  } else {
    pump_pin = PUMP2_PIN;
    isPump2 = stat;
  }
  isChange = true;
  // Запуск идет постепенно
  if ( stat ){
    for ( int i=0; i++; i<255){
      analogWrite( pump_pin, i );
      _delay_ms(500); 
    }
  } else {
    analogWrite( pump_pin, 0 ); 
  }
  #ifdef DEBUG
    Serial.println("Pump ");
    Serial.print(pump);
    Serial.print(stat ? " ON":" OFF");
  #endif

}
// Включить релюшку (0/1)
void relayOn( void ) {
  isRelay=true;
  digitalWrite(RELAY_PIN, LOW);
  #ifdef DEBUG
    Serial.println("Relay ON");
  #endif
}
//Выключить релюшку (0/1)
void relayOff( void ) {
  isRelay=false;
  digitalWrite(RELAY_PIN, HIGH);
  #ifdef DEBUG
    Serial.println("Relay OFF");
  #endif
}

// Измерение расстояния с помощью сонара
int duration( void )
{
  int duration, cm; 
  digitalWrite(TRIG_PIN, LOW); 
  delayMicroseconds(2); 
  digitalWrite(TRIG_PIN, HIGH); 
  delayMicroseconds(10); 
  digitalWrite(TRIG_PIN, LOW); 
  duration = pulseIn(ECHO_PIN, HIGH);
  cm = duration / 58;
  #ifdef  DEBUG
    Serial.print(cm); 
    Serial.println(" cm"); 
  #endif  
  return cm;
}

// Расчет какие показания вывести на панель
int CurrentLevel(){
  return round(level / (MAXLEVEL - MINLEVEL / 10));
}

// Расчет количества воды в литрах
int CurrentVolume(){
  return 450 - (level * 4,926);
}


// Читаем кнопки
int getPressedButton()
{
  int buttonValue = analogRead(BTN_PIN); // считываем значения с аналогового входа(A0) 
  if (buttonValue < 100) {
    return 1;
  }
  else if (buttonValue < 200) {
    return 2;
  }
  else if (buttonValue < 400){
    return 3;
  }
  else if (buttonValue < 600){
    return 4;
  }
  else if (buttonValue < 800){
    return 5;
  }
  return 0;
}

/*
 * Выодим информацию на дисплей
 */
void displayLevel( int level, bool clapan, bool ozon,bool pump1, bool pump2) {
  byte bytsIsDisplay;
  bytsIsDisplay = 0;
  bytsIsDisplay |= clapan ?  0b00010000 : 0;
  bytsIsDisplay |= ozon   ?  0b00100000 : 0;
  bytsIsDisplay |= pump1  ?  0b01000000 : 0;
  bytsIsDisplay |= pump2  ?  0b10000000 : 0;
  
  digitalWrite(LATCH_PIN,LOW);
  if ( level < 7 ) {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, bytsIsDisplay);
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, byteArray[level]);
  } else {
    bytsIsDisplay |= byteArray[level - 7];
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, bytsIsDisplay);
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, 255);
  }
  digitalWrite(LATCH_PIN,HIGH);
}
