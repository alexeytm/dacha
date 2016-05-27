#define DEBUG

// индикатор   0   1   2   3  4   5   6  7  8  9  10  11   // Контакты на индикаторе
//int pin[] = { 19, 18, -1, 17, 7, -1, 16, 4, 5, 8, 15, 6}; // Контакты на arduino
//            1   2   3  4   5    6  7  8  9  10  11  12
//            0   1   2  3   4    5  6  7  8  9   10  11
int pin[] = { 8, 17, -1, 16, 18, -1, 7, 6, 5, 15, 19, 4}; // Контакты на arduino
// Порядок сегментов, точку в этом проекте не используем!
int settingsSegments[] = {pin[10], pin[6], pin[3], pin[1], pin[0], pin[4] ,pin[9]}; 
int segments[] = {0b0111111, 0b0000110, 0b1011011, 0b1001111, 0b1100110, 0b1101101, 0b1111101, 0b0000111, 0b1111111, 0b1101111, 0b1000000}; //0, 1, 2, 3, 4, 5, 6, 7, 8, 9, '-'
// Обход по сегментам, элемент 1 (0), 3(2) повторяются трижды для каждого из знаков!
//             7654321    7654321    7654321    7654321    7654321    7654321    7654321
int obhod[]={0b0000001 ,0b0000010 ,0b0000100 ,0b0001000 ,0b0010000 ,0b0100000 ,0b1000000};

void led3Test()
{
   pinMode(pin[11], OUTPUT);
   pinMode(pin[8], INPUT);
   pinMode(pin[7], INPUT);
  for ( int i = 0; i < 7 ;i++)
  {
    	for(int j = 0; j < 7; j++) // Передаем число
	      if(obhod[i] & (1 << j))
          {
       	    digitalWrite(settingsSegments[j], HIGH);
            delay(100);
            digitalWrite(settingsSegments[j], LOW);
          }
  }
   pinMode(pin[11], INPUT);
   pinMode(pin[8], OUTPUT);
  for ( int i = 0; i < 7 ;i++)
  {
      for(int j = 0; j < 7; j++) // Передаем число
        if(obhod[i] & (1 << j))
          {
            digitalWrite(settingsSegments[j], HIGH);
            delay(100);
            digitalWrite(settingsSegments[j], LOW);
          }
  }
   pinMode(pin[8], INPUT);
   pinMode(pin[7], OUTPUT);
  for ( int i = 0; i < 7 ;i++)
  {
      for(int j = 0; j < 7; j++) // Передаем число
        if(obhod[i] & (1 << j))
          {
            digitalWrite(settingsSegments[j], HIGH);
            delay(100);
            digitalWrite(settingsSegments[j], LOW);
          }
  }

}

void led3DigitDisplay(int num, float time)
{
	unsigned long ltime = millis();

	for(int i = 0; i < 12; ++i) pinMode(pin[i], OUTPUT); // Определяем пины как выход
         

	int minus = 3;

	if(num > -100 && num < 0) // Разбираемся с отрицательными числами
	{
		minus--;
		if(num > -10) minus--;
		num = -num;
	}
	
	for(int i = 0, temp; millis() - ltime <= time * 100; i++)
	{
		if(i == 3) i = 0;

		temp = int(num / pow(10, i)) % 10; // Цифра которую передадим индикатору
		if(num >= 100 || num <= -100 || minus == i) // Если минус или переполнение, передаем '-'
			temp = 10;

                // включаем разряд
		if(i == 2 && (num >= 100 ||  minus == i)) pinMode(pin[11], OUTPUT); else pinMode(pin[11], INPUT); // Работаем с 3 разрядом
		if(i == 1 && (num >= 10 || minus == i)) pinMode(pin[8], OUTPUT); else pinMode(pin[8], INPUT); // Работаем с 2 разрядом
		if(i == 0) pinMode(pin[7], OUTPUT); else pinMode(pin[7], INPUT); // Работаем с 1 разрядом

		for(int j = 0; j < 7; j++) // Передаем число
			if(segments[temp] & (1 << j))
			  digitalWrite(settingsSegments[j], HIGH);
                delay(1);
                
                for(int j = 0; j < 7; j++) digitalWrite(settingsSegments[j], LOW); // Выключаем все светодиоды
	}
}

void setup()
{
#ifdef DEBUG
  Serial.begin(9600); 
  Serial.println("7 Segment`s!");
#endif
  for(int i = 0; i < 12; ++i) pinMode(pin[i], OUTPUT); // Определяем пины как выход
}


void loop()
{
        led3Test();
        for (int i = -99; i<99; i+=1)
        {
            led3DigitDisplay(i, 20);
            Serial.print(i);
            Serial.print(",");
            delay(50);
        }
}
