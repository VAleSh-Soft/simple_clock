#pragma once
#include <Arduino.h>
#include <DS3231.h>  // https://github.com/NorthernWidget/DS3231
#include <FastLED.h> // https://github.com/FastLED/FastLED

// =========== настройки компонентов часов ===========

// === используемые экраны (использовать только один))

#define TM1637_DISPLAY // использовать семисегментный экран на драйвере TM1637
// #define MAX72XX_7SEGMENT_DISPLAY // использовать семисегментный экран на драйвере MAX7219 или MAX7221, четыре цифры
// #define MAX72XX_MATRIX_DISPLAY // использовать матричный экран на драйвере MAX7219 или MAX7221 и четырех матрицах 8х8 светодиодов
// #define WS2812_MATRIX_DISPLAY // использовать матричный экран 8х32 на базе адресных светодиодов

// ==== календарь ====================================

// #define USE_CALENDAR // использовать или нет вывод даты по клику кнопкой Down

// ==== будильник ====================================

// #define USE_ALARM        // использовать или нет будильник

#ifdef USE_ALARM
// #define USE_ONECLICK_TO_SET_ALARM // использовать одинарный клик кнопкой Set для входа в настройки будильника, иначе вход по двойному клику
#endif

// ==== датчики ======================================

// #define USE_LIGHT_SENSOR // использовать или нет датчик света на пине А3 для регулировки яркости экрана
// #define USE_SET_BRIGHTNESS_MODE // использовать режим настройки минимального и максимального уровней яркости
// #define USE_TEMP_DATA // использовать или нет вывод на экран температуры по клику кнопкой Up

#ifdef USE_TEMP_DATA
// #define USE_DS18B20 // использовать для вывода температуры датчик DS18b20
// #define USE_NTC     // использовать для вывода температуры NTC термистор
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
#define DISPLAY_CS_PIN 10  // пин для подключения экрана - CS
#elif defined(WS2812_MATRIX_DISPLAY)
// пины для подключения адресных светодиодов настраиваются в файле setting_for_WS2812.h
#endif

#ifdef USE_ALARM
#define BUZZER_PIN 5    // пин для подключения пищалки
#define ALARM_LED_PIN 7 // пин для подключения светодиода - индикатора будильника
#endif

#ifdef USE_LIGHT_SENSOR
#define LIGHT_SENSOR_PIN A3 // пин для подключения датчика света
#endif

#if defined(USE_DS18B20)
#define DS18B20_PIN 8 // пин для подключения датчика DS18b20
#elif defined(USE_NTC)
#define NTC_PIN A0 // пин для подключения NTC термистора
#endif

// ==== прочее =======================================

#ifdef USE_ALARM
#define ALARM_EEPROM_INDEX 100 // индекс в EEPROM для сохранения настроек будильника
#endif
#ifdef USE_LIGHT_SENSOR
#define MIN_BRIGHTNESS_VALUE 98 // индекс в EEPROM для сохранения  минимального значения яркости экрана
#endif
#define MAX_BRIGHTNESS_VALUE 99 // индекс в EEPROM для сохранения  максимального значение яркости экрана

// ==== работа с экраном =============================
enum DisplayMode : uint8_t
{
  DISPLAY_MODE_SHOW_TIME, // основной режим - вывод времени на индикатор
  DISPLAY_MODE_SET_HOUR,  // режим настройки часов
  DISPLAY_MODE_SET_MINUTE // режим настройки минут
#ifdef USE_CALENDAR
  ,
  DISPLAY_MODE_SET_DAY,   // режим настройки дня месяца
  DISPLAY_MODE_SET_MONTH, // режим настройки месяца
  DISPLAY_MODE_SET_YEAR   // режим настройки года
#endif
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
#ifdef USE_CALENDAR
  ,
  DISPLAY_MODE_SHOW_CALENDAR // режим вывода даты
#endif
#ifdef USE_SET_BRIGHTNESS_MODE
#ifdef USE_LIGHT_SENSOR
  ,
  DISPLAY_MODE_SET_BRIGHTNESS_MIN // режим настройки минимального уровня яркости экрана
#endif
  ,
  DISPLAY_MODE_SET_BRIGHTNESS_MAX // режим настройки максимального уровня яркости экрана
#endif
};

// ==== опрос кнопок =================================
void checkButton();
void checkSetButton();
void checkUpDownButton();

// ==== задачи =======================================
void rtcNow();
void blink();
void returnToDefMode();
void showTimeSetting();
void setDisp();
#ifdef USE_CALENDAR
void showCalendar();
#endif
#ifdef USE_ALARM
void checkAlarm();
void runAlarmBuzzer();
#endif
#ifdef USE_TEMP_DATA
void showTemp();
#ifdef USE_DS18B20
void checkDS18b20();
#endif
#endif
#ifdef USE_LIGHT_SENSOR
void setBrightness();
#endif
#ifdef USE_SET_BRIGHTNESS_MODE
void showBrightnessSetting();
#endif

// ==== вывод данных =================================
/**
 * @brief вывод данных на экран
 *
 */
void setDisplay();

/**
 * @brief вывод на экран данных в режиме настройки времени или будильника
 *
 * @param hour  часы
 * @param minute  минуты
 */
void showTimeData(uint8_t hour, uint8_t minute);

#ifdef USE_ALARM
/**
 * @brief вывод на экран данных по состоянию будильника
 *
 * @param _state состояние (включено/выключено)
 */
void showAlarmState(uint8_t _state);
#endif

// ==== разное =======================================
/**
 * @brief изменение данных по клику кнопки с контролем выхода за предельное значение
 *
 * @param dt данные для изменения
 * @param max максимально возможное значение
 * @param toUp увеличение (true) или уменьшение данных (false)
 * @param min минимально мозможное значение
 * @param toLoop если true, то изменение данных закольцовано, иначе только от минимума к максимуму и наоборот
 */
void checkData(uint8_t &dt, uint8_t max, bool toUp, uint8_t min = 0, bool toLoop = true);
