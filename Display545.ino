#define DEBUG

#define PUMP1_PIN   5
#define PUMP2_PIN   6

#define DATA_PIN    A1
#define LATCH_PIN   A2
#define CLOCK_PIN   A3

#define ECHO_PIN    8
#define TRIG_PIN    7
#define TEMP_PIN    A0

void setup() {
  #ifdef DEBUG
    Serial.begin(9600);
    Serial.println("Initialize");
    delay(10);
  #endif
  
  // Выключаем ШИМ
/*  pinMode(PUMP1_PIN, INPUT);
  pinMode(PUMP2_PIN, INPUT);*/
  
  // Настраиваем выводы управление светодиодным модулем
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN,  OUTPUT);
  
  pinMode(TEMP_PIN, INPUT);
  
  pinMode(ECHO_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);

  // "обнуление" индикатора
  displayClear();
  #ifdef DEBUG
    Serial.println("Out pin`s");
  #endif
}

void loop() {
  //Serial.print("Distance=");
//  int t = duration( );
  display();
}

// Вывод данных на информационную панель
void display (void){
  #ifdef  DEBUG
    Serial.println("Display"); 
  #endif
  for ( int i=0; i<14; i++)
  {
    digitalWrite(LATCH_PIN, LOW);
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, 0b00000001);
    digitalWrite(LATCH_PIN, HIGH);
    Serial.print(i); 
    delay(5000);
  }
  displayClear();
  
  #ifdef DEBUG
    Serial.println("Stop");
  #endif
 }

void displayClear( void ) {
  #ifdef DEBUG
    Serial.println("Clear pins");
  #endif
    // "обнуление" индикатора
  for (int i=0; i<14;i++)
  {
    digitalWrite(LATCH_PIN, LOW);
    shiftOut(DATA_PIN, CLOCK_PIN, LSBFIRST, 0);
    digitalWrite(LATCH_PIN, HIGH);
  }

}

/*
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
  #ifdef DEBUG
    Serial.print("Distance=");
    Serial.print(cm); 
    Serial.println(" cm"); 
  #endif  
  return cm;
}

// Измерение температуры
double Thermister(int RawADC) {
  double Temp;
  Temp = log(((10240000/RawADC) - 10000));
  Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * Temp * Temp ))* Temp );
  Temp = Temp - 273.15;   // Kelvin to Celcius
  //Temp = (Temp * 9.0)/ 5.0 + 32.0;    // 1 способ Convert Celcius to Fahrenheit 
  //Temp = (Temp * 1.8) + 32.0;   // 2 способ Convert Celcius to Fahrenheit
  return Temp;
}
*/
