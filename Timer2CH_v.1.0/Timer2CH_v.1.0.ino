
/*
   Пример вывода на дисплей с регистром TM1637
   показывает все возможности библиотеки GyverTM1637
   AlexGyver Technologies http://alexgyver.ru/
   https://alexgyver.ru/tm1637_display/
   https://www.makerguides.com/tm1637-arduino-tutorial/
   beep(uint16_t dur, uint16_t frq);
   https://community.alexgyver.ru/threads/biblioteka-dlja-tm1637-s-desjatichnoj-tochkoj.2249/
   http://arduino.on.kg/obzor-displeya-na-baze-drayvera-TM1637
*/

#include <GyverTM1637.h>
#include <GyverButton.h>
#include <CyberLib.h>

#define CLK 12 //дисплей TM1637
#define DIO 13 //дисплей TM1637
#define CH1 7 // реле канал 1 (озонатор)
#define CH2 2 // реле канал 2 (вентилятор)
#define SW1 8 // кнопка 1
#define SW2 9 // кнопка 2
#define SND 4 // пищалка (динамик)
#define LED 5 // лента

GButton butt1(SW1);
GButton butt2(SW2);


GyverTM1637 disp(CLK, DIO); //инициализация дисплея


uint32_t tmr = 0; // для таймера
uint32_t flash = 0; // для мигалки

boolean flag = 0; // для мигалки
uint8_t modeFlash = 1; // режимы мигалок

boolean relay1 = 0; // переменная реле 1
boolean relay2 = 0; // переменная реле 2

boolean timerMainState = 0; // флаг запуска таймера основного
boolean timerCoolState = 0; // флаг запуска таймера охлаждения
uint16_t timerLoad = 60; // начальное значение таймера основного
uint16_t timerCool = 60; // начальное значение таймера охлаждения
uint8_t brightLedStrp = 0; // начальное значение яркости ленты
uint8_t fadeAmount = 5; //шаг яркости ленты

byte StartShow[4] = {_empty, _empty, _6, _0 }; //массив "60" ЗАСТАВКА!!!
byte CoolShow[4] = {_C, _O, _O, _L }; //массив COOL
byte EndShow[4] = {_empty, _E, _n, _d}; //массив End

void setup() {
  Serial.begin(9600);

  StartTimer1(timerIsr, 50000);           // установка таймера опроса кнопок на каждые 50000 микросекунд (== 50 мс)
  disp.clear();
  disp.brightness(5);  // яркость, 0 - 7 (минимум - максимум)

  disp.scrollByte(StartShow, 100); //ЗАСТАВКА "60" !!!


  //=====НАСТРОЙКА ПОРТОВ====
  pinMode(CH1, OUTPUT);
  pinMode(CH2, OUTPUT);
  pinMode(SND, OUTPUT);

  tone(SND, 500, 100);
  delay_ms(100);
  tone(SND, 600, 100);
  delay_ms(100);
  tone(SND, 700, 100);
  delay_ms(100);
  tone(SND, 800, 100);
  delay_ms(100);
  tone(SND, 900, 100);
}


void timerIsr() {   // прерывание таймера для кнопок
  butt1.tick();     // отработка кнопок находится здесь
  butt2.tick();
}


void loop() {


  //========ОТРАБОТКА КНОПКИ ВЫБОРА ВРЕМЕНИ ТАЙМЕРА======
  if (butt1.isClick()) { //выбор времени таймера
    if (timerCoolState == 1) { //если работает охлаждение - ничего не делать
      tone(SND, 300, 500);
    }
    else {
      tone(SND, 1000, 100);
      timerLoad = (timerLoad + 60);
      if (timerLoad == 240) timerLoad = 300;
      if (timerLoad == 360) timerLoad = 60;
      disp.displayInt(timerLoad); // выводим на дисплей значение таймера
    }
  }

  //========ОТРАБОТКА КНОПКИ СТАРТ/СТОП======
  if (butt2.isClick()) { //старт/стоп таймера
    if (timerCoolState == 1) { //если работает охлаждение - ничего не делать
      tone(SND, 300, 500); // ничего не делать
    }
    else {
      timerMainState = 1;
      tone(SND, 500, 500);
    }
  }

  if (butt2.isHold()) { //если кнопку старта зажать
    timerMainState = 0; // отключить основной таймер
    timerCoolState = 1; // запуск таймера охлаждения
    tone(SND, 700, 500); //звук таймера охлаждения
  }



  //========ТАЙМЕР ГЛАВНЫЙ======
  if (timerMainState == 1) {
    disp.displayInt(timerLoad); // выводим на дисплей значение таймера
    modeFlash = 2;
    relay1 = 1; // включить реле 1
    relay2 = 1; // включить реле 2

    if (millis() - tmr > 1000) {
      tmr = millis();
      timerLoad--; // убавляем значение таймера раз в секунду
    }

    if (timerLoad <= 0) { //если таймер закончился
      timerMainState = 0; // первый таймер выключить
      timerCoolState = 1; // таймер охлаждения включить
      //      relay1 = 0; //выключить реле основного таймера
      timerLoad = 10; //начальное значение таймера
      tone(SND, 700, 500); //звук окончания таймера
    }
  }



  //========ТАЙМЕР ВЕНТИЛЯТОРА======
  if (timerCoolState == 1) {
    disp.displayByte(CoolShow);
    modeFlash = 3; // режим быстрого мигания
    relay1 = 0; // выключить реле 1
    relay2 = 1; // включить реле 2


    if (millis() - tmr > 1000) {  // каждую секунду меняем значение флага на противоположное
      tmr = millis();
      timerCool--; // убавляем значение таймера
      if (timerCool <= 0) { //если таймер закончился
        modeFlash = 1; // режим плавного мигания
        timerCoolState = 0; // этот таймер отключаем
        relay2 = 0; //выключить реле вентилятора
        timerCool = 60;
        timerLoad = 60;
        fadeAmount = 5;
        disp.scrollByte(EndShow, 100); //АНИМАЦИЯ End
        //звук окончания второго таймера
        tone(SND, 900, 100);
        delay_ms(100);
        tone(SND, 800, 100);
        delay_ms(100);
        tone(SND, 700, 100);
        delay_ms(100);
        tone(SND, 600, 100);
        delay_ms(100);
        tone(SND, 500, 100);

      }
    }
  }




  //========ПОДСВЕТКА======

  switch (modeFlash) {
    case 1:
      brightLedStrp = brightLedStrp + fadeAmount;
      if (brightLedStrp <= 0 || brightLedStrp >= 130) {
        fadeAmount = -fadeAmount;
      }
      break;
    case 2:
      if (millis() - flash > 900) {       //мигалка медленная (главный таймер)
        flash = millis();
        flag = !flag;
        if (flag == 0) brightLedStrp = 0;
        if (flag == 1) brightLedStrp = 100;
      }
      break;
    case 3:
      if (millis() - flash > 250) {       //мигалка быстрая (охлаждение)
        flash = millis();
        flag = !flag;
        if (flag == 0) brightLedStrp = 0;
        if (flag == 1) brightLedStrp = 100;
      }
      break;
  }



  analogWrite (LED, brightLedStrp);


  //========ВКЛ/ВЫКЛ ВЫХОДОВ на РЕЛЕ======
  if (relay1 == 1) digitalWrite (CH1, HIGH);
  if (relay1 == 0) digitalWrite (CH1, LOW);
  if (relay2 == 1) digitalWrite (CH2, HIGH);
  if (relay2 == 0) digitalWrite (CH2, LOW);

  Serial.print("modeFlash=   ");
  Serial.println(modeFlash);
  Serial.print("timerMainState=   ");
  Serial.println(timerMainState);
  Serial.print("timerCoolState=   ");
  Serial.println(timerCoolState);
  Serial.print("fadeAmount=   ");
  Serial.println(fadeAmount);






} //loop закрыть
