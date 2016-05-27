#include <math.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <Dacha.h>

#define DEBUG 

// Пакет для передачи данных
packet_t packet;

// Подключаем FR24
RF24 radio(9,10);
// 0 - прием, 1- передача
const uint64_t pipes[2]={ ADDRESS_CLIENT2, ADDRESS_SERVER1 };
// Переменный для обработки в прерываниях
volatile boolean flag_tx, flag_fail, flag_rx;

// Обработка прерываний
void check_radio(void);

void setup() {
  
  // настраиваем прерывание на 2 ноге
  attachInterrupt(0, check_radio, FALLING); 
  //Инициализация цифровых датчиков температуры/владности

#ifdef DEBUG
  Serial.begin(9600); 
  Serial.println("Initialize!\n\r");
#endif
  // Инициализация сети
  radio.begin();
  // передаем по каналу
  radio.setChannel(100);
  // Каналы для передачи и получения
  radio.openWritingPipe( pipes[1] );
  radio.openReadingPipe( 1,pipes[0] );
  // Скорость передачи (RF24_1MBPS, RF24_2MBPS, RF24_2501KBPS) одинаково на передатчике и приемнике!
  radio.setDataRate(RF24_1MBPS);
  // Уровень мощности передатчика (RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGHT,RF24_PA_MAX)
  radio.setPALevel(RF24_PA_MAX);
  // Проверка по контрольной сумме передачи/приема
  radio.setCRCLength(RF24_CRC_16);
  // С задержкой 15 мс 15 попыток
  radio.setRetries(15,15);
  radio.startListening();

#ifdef DEBUG
  Serial.println("NRF initialize!\n\r");
#endif

}

// обработчик прерывания
void check_radio(){ 
  bool tx, fail, rx;
    radio.whatHappened(tx,fail,rx); // читаем регистр
    flag_tx = tx;                   // 1 если успешно отправили данные
    flag_fail = fail;               // 1 если данные не отправленны   
    flag_rx = rx;                   // 1 если пришли данные для чтения 
}

void loop() {
  
  if (flag_rx){ // если пришли данные для чтения
     flag_rx = 0;
  }
  
  if (flag_fail){ // если данные не отправленны
    flag_fail = 0;
  }
   
  if (flag_tx){ // если данные отправленны 
    flag_tx = 0;
  } 
  
  // Ожидание перед запросом
  delay(10000);

  //Чтение датчиков и подготовка структыры передачи данных серии DHT

#ifdef DEBUG
  Serial.print("Humidity: "); 
  Serial.print("Temperature: "); 
  Serial.println("==========================\n\r");
#endif
  // передача данных на сервер
  radio.stopListening();
  bool ok = radio.write(&packet, sizeof(packet) );
  radio.startListening();
}


