#pragma once
#include <Arduino.h>
#include <DS3231.h>

// =========== настройки компонентов часов ===========

// === используемые экраны (использовать только один))

#define TM1637_DISPLAY // использовать семисегментный экран на драйвере TM1637
// #define MAX72XX_7SEGMENT_DISPLAY // использовать семисегментный экран на драйвере MAX7219 или MAX7221, четыре цифры
// #define MAX72XX_MATRIX_DISPLAY // использовать матричный экран на драйвере MAX7219 или MAX7221 и четырех матрицах 8х8 светодиодов

// ==== будильник ====================================

// #define USE_ALARM        // использовать или нет будильник
#ifdef USE_ALARM
// #define USE_ONECLICK_TO_SET_ALARM // использовать одинарный клик кнопкой Set для входа в настройки будильника, иначе вход по двойному клику
#endif

// ==== датчики ======================================

// #define USE_LIGHT_SENSOR // использовать или нет датчик света на пине А3 для регулировки яркости экрана
// #define USE_TEMP_DATA // использовать или нет вывод температуры по клику кнопкой Up
#ifdef USE_TEMP_DATA
// #define USE_DS18B20 // использовать для вывода температуры датчик DS18b20
#endif

// ===================================================

// ==== пины =========================================

#define BTN_SET_PIN 4  // пин для подключения кнопки Set
#define BTN_DOWN_PIN 6 // пин для подключения кнопки Down
#define BTN_UP_PIN 9   // пин для подключения кнопки Up

#define DS3231_SDA_PIN A4 // пин для подключения вывода SDA модуля DS3231 (не менять!!!)
#define DS3231_SCL_PIN A5 // пин для подключения вывода SCL модуля DS3231 (не менять!!!)

#if defined(TM1637_DISPLAY)
#define DISPLAY_CLK_PIN 11 // пин для подключения экрана - CLK
#define DISPLAY_DAT_PIN 10 // пин для подключения экрана - DAT
#elif defined(MAX72XX_7SEGMENT_DISPLAY) || defined(MAX72XX_MATRIX_DISPLAY)
#define DISPLAY_CLK_PIN 13 // пин для подключения экрана - CLK (не менять!!!)
#define DISPLAY_DIN_PIN 11 // пин для подключения экрана - DAT (не менять!!!)
#define DISPLAY_CS_PIN 10  // пин для подключения экрана - DAT
#endif

#ifdef USE_ALARM
#define BUZZER_PIN 5    // пин для подключения пищалки
#define ALARM_LED_PIN 7 // пин для подключения светодиода - индикатора будильника
#endif

#ifdef USE_LIGHT_SENSOR
#define LIGHT_SENSOR_PIN A3 // пин для подключения датчика света
#endif

#ifdef USE_DS18B20
#define DS18B20_PIN 8 // пин для подключения датчика DS18b20
#endif

// ==== прочее =======================================

#ifdef USE_ALARM
#define ALARM_EEPROM_INDEX 100 // индекс в EEPROM для сохранения настроек будильника
#endif

// ==== режимы экрана ================================
enum DisplayMode : uint8_t
{
  DISPLAY_MODE_SHOW_TIME, // основной режим - вывод времени на индикатор
  DISPLAY_MODE_SET_HOUR,  // режим настройки часов
  DISPLAY_MODE_SET_MINUTE // режим настройки минут
#ifdef USE_ALARM
  ,
  DISPLAY_MODE_ALARM_ON_OFF,    // режим настройки будильника - вкл/выкл
  DISPLAY_MODE_SET_ALARM_HOUR,  // режим настройки будильника - часы
  DISPLAY_MODE_SET_ALARM_MINUTE // режим настройки будильника - минуты
#endif
#ifdef USE_TEMP_DATA
  ,
  DISPLAY_MODE_SHOW_TEMP // режим вывода температуры
#endif
};

// ==== опрос кнопок =================================
void checkButton();
void checkSetButton();
void checkUpDownButton();

// ==== задачи =======================================
void rtcNow();
void blink();
void restartBlink();
void returnToDefMode();
void showTimeSetting();
void setDisp();
#ifdef USE_ALARM
void checkAlarm();
void runAlarmBuzzer();
#endif
#ifdef USE_TEMP_DATA
void showTemp();
#endif
#ifdef USE_DS18B20
void checkDS18b20();
#endif
#ifdef USE_LIGHT_SENSOR
void setBrightness();
#endif

// ==== вывод данных =================================
// вывод данных на экран
void setDisplay();

// вывод на экран времени
void showTime(DateTime dt);

// вывод на экран данных в режиме настройки времени или будильника
void showTimeData(byte hour, byte minute);

#ifdef USE_ALARM
// вывод на экран данных по состоянию будильника
void showAlarmState(byte _state);
#endif

// ==== часы =========================================
// сохранение времени после настройки
void saveTime(byte hour, byte minute);

// ==== разное =======================================
// изменение данных по клику кнопки с контролем выхода за предельное значение
void checkData(byte &dt, byte max, bool toUp);
