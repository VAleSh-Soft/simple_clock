#include <Wire.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>
#include <DS3231.h>        // https://github.com/NorthernWidget/DS3231
#include <shButton.h>      // https://github.com/VAleSh-Soft/shButton
#include <shTaskManager.h> // https://github.com/VAleSh-Soft/shTaskManager
#include "header_file.h"
#if defined(TM1637_DISPLAY)
#include "display_TM1637.h"
#elif defined(MAX72XX_7SEGMENT_DISPLAY) || defined(MAX72XX_MATRIX_DISPLAY)
#include "display_MAX72xx.h"
#elif defined(WS2812_MATRIX_DISPLAY)
#include <FastLED.h> // https://github.com/FastLED/FastLED
#include "display_WS2812.h"
#endif
#ifdef USE_ALARM
#include "alarm.h"
#endif
#ifdef USE_TEMP_DATA
#if defined(USE_DS18B20)
#include "ds1820.h"
#elif defined(USE_NTC)
#include "ntc.h"
#endif
#endif

// ==== настройки ====================================
#ifdef USE_ALARM
#define ALARM_DURATION 60        // продолжительность сигнала будильника, секунд
#define ALARM_SNOOZE_DELAY 120   // задержка повтора сигнала будильника, секунд
#define ALARM_REPETITION_COUNT 3 // количество повторов сигнала будильника
#endif
#define LIGHT_THRESHOLD 300 // порог переключения для датчика света
#define AUTO_EXIT_TIMEOUT 6 // время автоматического возврата в режим показа текущего времени из любых других режимов при отсутствии активности пользователя, секунд
// ===================================================

#if defined(TM1637_DISPLAY)
DisplayTM1637 disp(DISPLAY_CLK_PIN, DISPLAY_DAT_PIN);
#elif defined(MAX72XX_7SEGMENT_DISPLAY)
DisplayMAX72xx7segment<DISPLAY_CS_PIN> disp;
#elif defined(MAX72XX_MATRIX_DISPLAY)
DisplayMAX72xxMatrix<DISPLAY_CS_PIN> disp;
#elif defined(WS2812_MATRIX_DISPLAY)
CRGB leds[256];
DisplayWS2812Matrix disp(leds, CRGB::Red, BY_COLUMNS);
#endif

DS3231 clock; // SDA - A4, SCL - A5
RTClib RTC;
#ifdef USE_ALARM
Alarm alarm(ALARM_LED_PIN, ALARM_EEPROM_INDEX);
#endif
#ifdef USE_TEMP_DATA
#if defined(USE_DS18B20)
DS1820 temp_sensor(DS18B20_PIN); // вход датчика DS18b20
#elif defined(USE_NTC)
NTCSensor temp_sensor(NTC_PIN, 10000, 9850, 3950);
#endif
#endif

shTaskManager tasks; // создаем список задач, количество задач укажем в setup()

shHandle rtc_guard;              // опрос микросхемы RTC по таймеру, чтобы не дергать ее откуда попало
shHandle blink_timer;            // блинк
shHandle return_to_default_mode; // таймер автовозврата в режим показа времени из любого режима настройки
shHandle set_time_mode;          // режим настройки времени
shHandle display_guard;          // вывод данных на экран
#ifdef USE_ALARM
shHandle alarm_guard;  // отслеживание будильника
shHandle alarm_buzzer; // пищалка будильника
#endif
#ifdef USE_CALENDAR
shHandle show_calendar_mode; // режим вывода даты
#endif
#ifdef USE_TEMP_DATA
shHandle show_temp_mode; // режим показа температуры
#if defined(USE_DS18B20)
shHandle ds18b20_guard; // опрос датчика DS18b20
#endif
#endif
#ifdef USE_LIGHT_SENSOR
shHandle light_sensor_guard; // отслеживание показаний датчика света
#endif
#ifdef USE_SET_BRIGHTNESS_MODE
shHandle set_brightness_mode; // режим настройки яркости экрана
#endif

DisplayMode displayMode = DISPLAY_MODE_SHOW_TIME;
bool blink_flag = false; // флаг блинка, используется всем, что должно мигать
DateTime curTime;

// ==== класс кнопок с предварительной настройкой ====
enum ButtonFlag : uint8_t
{
  BTN_FLAG_NONE, // флаг кнопки - ничего не делать
  BTN_FLAG_NEXT, // флаг кнопки - изменить значение
  BTN_FLAG_EXIT  // флаг кнопки - возврат в режим показа текущего времени
};

class clcButton : public shButton
{
private:
  ButtonFlag _flag = BTN_FLAG_NONE;

public:
  clcButton(uint8_t button_pin) : shButton(button_pin)
  {
    shButton::setTimeoutOfLongClick(1000);
    shButton::setLongClickMode(LCM_ONLYONCE);
    shButton::setVirtualClickOn(true);
  }

  ButtonFlag getBtnFlag()
  {
    return (_flag);
  }

  void setBtnFlag(ButtonFlag flag)
  {
    _flag = flag;
  }

  uint8_t getButtonState()
  {
    uint8_t _state = shButton::getButtonState();
    switch (_state)
    {
    case BTN_DOWN:
    case BTN_DBLCLICK:
    case BTN_LONGCLICK:
      // в любом режиме, кроме стандартного, каждый клик любой кнопки перезапускает таймер автовыхода в стандартный режим
      if (tasks.getTaskState(return_to_default_mode))
      {
        tasks.restartTask(return_to_default_mode);
      }
#ifdef USE_ALARM
      // если сработал будильник, отключить его
      if (alarm.getAlarmState() == ALARM_YES)
      {
        alarm.setAlarmState(ALARM_ON);
        shButton::resetButtonState();
      }
#endif
      break;
    }
    return (_state);
  }
};
// ===================================================

clcButton btnSet(BTN_SET_PIN);   // кнопка Set - смена режима работы часов
clcButton btnUp(BTN_UP_PIN);     // кнопка Up - изменение часов/минут в режиме настройки
clcButton btnDown(BTN_DOWN_PIN); // кнопка Down - изменение часов/минут в режиме настройки

// ===================================================
void checkButton()
{
  checkSetButton();
  checkUpDownButton();
}

void checkSetButton()
{
  switch (btnSet.getButtonState())
  {
  case BTN_ONECLICK:
    switch (displayMode)
    {
    case DISPLAY_MODE_SET_HOUR:
    case DISPLAY_MODE_SET_MINUTE:
#ifdef USE_CALENDAR
    case DISPLAY_MODE_SET_DAY:
    case DISPLAY_MODE_SET_MONTH:
    case DISPLAY_MODE_SET_YEAR:
#endif
#ifdef USE_ALARM
    case DISPLAY_MODE_SET_ALARM_HOUR:
    case DISPLAY_MODE_SET_ALARM_MINUTE:
    case DISPLAY_MODE_ALARM_ON_OFF:
#endif
#ifdef USE_SET_BRIGHTNESS_MODE
#ifdef USE_LIGHT_SENSOR
    case DISPLAY_MODE_SET_BRIGHTNESS_MIN:
#endif
    case DISPLAY_MODE_SET_BRIGHTNESS_MAX:
#endif
      btnSet.setBtnFlag(BTN_FLAG_NEXT);
      break;
    case DISPLAY_MODE_SHOW_TIME:
#ifdef USE_ALARM
#ifdef USE_ONECLICK_TO_SET_ALARM
      displayMode = DISPLAY_MODE_ALARM_ON_OFF;
#endif
#endif
      break;
    default:
      break;
    }
    break;
  case BTN_DBLCLICK:
    switch (displayMode)
    {
    case DISPLAY_MODE_SHOW_TIME:
#ifdef USE_ALARM
#ifndef USE_ONECLICK_TO_SET_ALARM
      displayMode = DISPLAY_MODE_ALARM_ON_OFF;
#endif
#endif
      break;
    default:
      break;
    }
    break;
  case BTN_LONGCLICK:
    switch (displayMode)
    {
    case DISPLAY_MODE_SHOW_TIME:
      displayMode = DISPLAY_MODE_SET_HOUR;
      break;
#ifdef USE_CALENDAR
    case DISPLAY_MODE_SHOW_CALENDAR:
      tasks.stopTask(show_calendar_mode);
      displayMode = DISPLAY_MODE_SET_DAY;
      break;
#endif
    case DISPLAY_MODE_SET_HOUR:
    case DISPLAY_MODE_SET_MINUTE:
#ifdef USE_CALENDAR
    case DISPLAY_MODE_SET_DAY:
    case DISPLAY_MODE_SET_MONTH:
    case DISPLAY_MODE_SET_YEAR:
#endif
#ifdef USE_ALARM
    case DISPLAY_MODE_SET_ALARM_HOUR:
    case DISPLAY_MODE_SET_ALARM_MINUTE:
    case DISPLAY_MODE_ALARM_ON_OFF:
#endif
      btnSet.setBtnFlag(BTN_FLAG_EXIT);
      break;
    default:
      break;
    }
    break;
  }
}

void checkUDbtn(clcButton &btn)
{
  switch (btn.getLastState())
  {
  case BTN_DOWN:
  case BTN_DBLCLICK:
    btn.setBtnFlag(BTN_FLAG_NEXT);
    break;
  case BTN_LONGCLICK:
#ifdef USE_ALARM
    if (displayMode != DISPLAY_MODE_ALARM_ON_OFF)
#endif
    {
      btn.setBtnFlag(BTN_FLAG_NEXT);
    }
    break;
  }
}

void checkUpDownButton()
{
  btnUp.getButtonState();
  btnDown.getButtonState();

  switch (displayMode)
  {
  case DISPLAY_MODE_SHOW_TIME:
#ifdef USE_TEMP_DATA
    if (btnUp.getLastState() == BTN_ONECLICK)
    {
      displayMode = DISPLAY_MODE_SHOW_TEMP;
    }
#endif
#ifdef USE_CALENDAR
    if (btnDown.getLastState() == BTN_ONECLICK)
    {
      displayMode = DISPLAY_MODE_SHOW_CALENDAR;
    }
#endif
#ifdef USE_SET_BRIGHTNESS_MODE
    if (btnUp.isSecondButtonPressed(btnDown, BTN_LONGCLICK) ||
        btnDown.isSecondButtonPressed(btnUp, BTN_LONGCLICK))
    {
#ifdef USE_LIGHT_SENSOR
      displayMode = DISPLAY_MODE_SET_BRIGHTNESS_MIN;
#else
      displayMode = DISPLAY_MODE_SET_BRIGHTNESS_MAX;
#endif
    }
#endif
    break;
  case DISPLAY_MODE_SET_HOUR:
  case DISPLAY_MODE_SET_MINUTE:
#ifdef USE_CALENDAR
  case DISPLAY_MODE_SET_DAY:
  case DISPLAY_MODE_SET_MONTH:
  case DISPLAY_MODE_SET_YEAR:
#endif
#ifdef USE_ALARM
  case DISPLAY_MODE_SET_ALARM_HOUR:
  case DISPLAY_MODE_SET_ALARM_MINUTE:
  case DISPLAY_MODE_ALARM_ON_OFF:
#endif
#ifdef USE_SET_BRIGHTNESS_MODE
#ifdef USE_LIGHT_SENSOR
  case DISPLAY_MODE_SET_BRIGHTNESS_MIN:
#endif
  case DISPLAY_MODE_SET_BRIGHTNESS_MAX:
#endif
    if (!btnDown.isButtonClosed())
    {
      checkUDbtn(btnUp);
    }
    if (!btnUp.isButtonClosed())
    {
      checkUDbtn(btnDown);
    }
    break;
#ifdef USE_TEMP_DATA
  case DISPLAY_MODE_SHOW_TEMP:
    if (btnUp.getLastState() == BTN_ONECLICK)
    {
      returnToDefMode();
    }
    break;
#endif
#ifdef USE_CALENDAR
  case DISPLAY_MODE_SHOW_CALENDAR:
    if (btnDown.getLastState() == BTN_ONECLICK)
    {
      returnToDefMode();
    }
    break;
#endif
  default:
    break;
  }
}

// ===================================================
void rtcNow()
{
  curTime = RTC.now();
  if (displayMode == DISPLAY_MODE_SHOW_TIME)
  {
#if defined(MAX72XX_MATRIX_DISPLAY) || defined(WS2812_MATRIX_DISPLAY)
    disp.showTime(curTime.hour(), curTime.minute(), curTime.second(), blink_flag);
#else
    disp.showTime(curTime.hour(), curTime.minute(), blink_flag);
#endif
  }
}

void blink()
{
  static uint8_t cur_sec = curTime.second();
  static uint32_t tmr = 0;
  if (cur_sec != curTime.second())
  {
    cur_sec = curTime.second();
    blink_flag = true;
    tmr = millis();
  }
  else if (blink_flag && millis() - tmr >= 500)
  {
    blink_flag = false;
  }
}

void returnToDefMode()
{
  switch (displayMode)
  {
  case DISPLAY_MODE_SET_HOUR:
  case DISPLAY_MODE_SET_MINUTE:
#ifdef USE_CALENDAR
  case DISPLAY_MODE_SET_DAY:
  case DISPLAY_MODE_SET_MONTH:
  case DISPLAY_MODE_SET_YEAR:
#endif
#ifdef USE_ALARM
  case DISPLAY_MODE_SET_ALARM_HOUR:
  case DISPLAY_MODE_SET_ALARM_MINUTE:
  case DISPLAY_MODE_ALARM_ON_OFF:
#endif
#ifdef USE_SET_BRIGHTNESS_MODE
#ifdef USE_LIGHT_SENSOR
  case DISPLAY_MODE_SET_BRIGHTNESS_MIN:
#endif
  case DISPLAY_MODE_SET_BRIGHTNESS_MAX:
#endif
    btnSet.setBtnFlag(BTN_FLAG_EXIT);
    break;
#ifdef USE_TEMP_DATA
  case DISPLAY_MODE_SHOW_TEMP:
    displayMode = DISPLAY_MODE_SHOW_TIME;
    tasks.stopTask(show_temp_mode);
    break;
#endif
#ifdef USE_CALENDAR
  case DISPLAY_MODE_SHOW_CALENDAR:
    displayMode = DISPLAY_MODE_SHOW_TIME;
    tasks.stopTask(show_calendar_mode);
    break;
#endif
  default:
    break;
  }
  tasks.stopTask(return_to_default_mode);
}

void stopSetting(shHandle task)
{
  tasks.stopTask(task);
  tasks.stopTask(return_to_default_mode);
}

void showTimeSetting()
{
  static bool time_checked = false;
  static uint8_t curHour = 0;
  static uint8_t curMinute = 0;
#ifdef USE_CALENDAR
  static const uint8_t PROGMEM days_of_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
#endif

  if (!tasks.getTaskState(set_time_mode))
  {
    tasks.startTask(set_time_mode);
    tasks.startTask(return_to_default_mode);
    switch (displayMode)
    {
#ifdef USE_ALARM
    case DISPLAY_MODE_SET_ALARM_HOUR:
    case DISPLAY_MODE_SET_ALARM_MINUTE:
      curHour = alarm.getAlarmPoint() / 60;
      curMinute = alarm.getAlarmPoint() % 60;
      break;
    case DISPLAY_MODE_ALARM_ON_OFF:
      curHour = (uint8_t)alarm.getOnOffAlarm();
      break;
#endif
    default:
      break;
    }
    time_checked = false;
  }

  if (!time_checked)
  {
    switch (displayMode)
    {
    case DISPLAY_MODE_SET_HOUR:
    case DISPLAY_MODE_SET_MINUTE:
      curHour = curTime.hour();
      curMinute = curTime.minute();
      break;
#ifdef USE_CALENDAR
    case DISPLAY_MODE_SET_DAY:
    case DISPLAY_MODE_SET_MONTH:
      curHour = curTime.day();
      curMinute = curTime.month();
      break;
    case DISPLAY_MODE_SET_YEAR:
      curHour = 20;
      curMinute = curTime.year() % 100;
      break;
#endif
    default:
      break;
    }
  }

  // опрос кнопок =====================
  if (btnSet.getBtnFlag() > BTN_FLAG_NONE)
  {
    if (time_checked)
    {
      switch (displayMode)
      {
      case DISPLAY_MODE_SET_HOUR:
      case DISPLAY_MODE_SET_MINUTE:
        clock.setHour(curHour);
        clock.setMinute(curMinute);
        clock.setSecond(0);
        break;
#ifdef USE_CALENDAR
      case DISPLAY_MODE_SET_DAY:
      case DISPLAY_MODE_SET_MONTH:
        clock.setDate(curHour);
        clock.setMonth(curMinute);
        rtcNow();
        break;
      case DISPLAY_MODE_SET_YEAR:
        clock.setYear(curMinute);
        break;
#endif
#ifdef USE_ALARM
      case DISPLAY_MODE_SET_ALARM_HOUR:
      case DISPLAY_MODE_SET_ALARM_MINUTE:
        alarm.setAlarmPoint(curHour * 60 + curMinute);
        break;
      case DISPLAY_MODE_ALARM_ON_OFF:
        alarm.setOnOffAlarm((bool)curHour);
        break;
#endif
      default:
        break;
      }
      time_checked = false;
    }
    if (btnSet.getBtnFlag() == BTN_FLAG_NEXT)
    {
      switch (displayMode)
      {
      case DISPLAY_MODE_SET_HOUR:
#ifdef USE_CALENDAR
      case DISPLAY_MODE_SET_MINUTE:
      case DISPLAY_MODE_SET_DAY:
      case DISPLAY_MODE_SET_MONTH:
#endif
#ifdef USE_ALARM
      case DISPLAY_MODE_SET_ALARM_HOUR:
#endif
        displayMode = DisplayMode(uint8_t(displayMode + 1));
        stopSetting(set_time_mode);
        break;
#ifdef USE_ALARM
      case DISPLAY_MODE_ALARM_ON_OFF:
        displayMode = (curHour) ? DISPLAY_MODE_SET_ALARM_HOUR : DISPLAY_MODE_SHOW_TIME;
        stopSetting(set_time_mode);
        break;
#endif
      default:
        displayMode = DISPLAY_MODE_SHOW_TIME;
        stopSetting(set_time_mode);
        break;
      }
    }
    else
    {
      displayMode = DISPLAY_MODE_SHOW_TIME;
      stopSetting(set_time_mode);
    }
    btnSet.setBtnFlag(BTN_FLAG_NONE);
  }

  if ((btnUp.getBtnFlag() == BTN_FLAG_NEXT) || (btnDown.getBtnFlag() == BTN_FLAG_NEXT))
  {
    bool dir = btnUp.getBtnFlag() == BTN_FLAG_NEXT;
    switch (displayMode)
    {
    case DISPLAY_MODE_SET_HOUR:
#ifdef USE_ALARM
    case DISPLAY_MODE_SET_ALARM_HOUR:
#endif
      checkData(curHour, 23, dir);
      break;
    case DISPLAY_MODE_SET_MINUTE:
#ifdef USE_ALARM
    case DISPLAY_MODE_SET_ALARM_MINUTE:
#endif
      checkData(curMinute, 59, dir);
      break;
#ifdef USE_ALARM
    case DISPLAY_MODE_ALARM_ON_OFF:
      checkData(curHour, 1, true);
      break;
#endif
#ifdef USE_CALENDAR
    case DISPLAY_MODE_SET_DAY:
      uint8_t i;
      i = pgm_read_byte(&days_of_month[curMinute - 1]);
      if (curMinute == 2 && (curTime.year() % 4 == 0))
      {
        i++;
      }
      checkData(curHour, i, dir, 1);
      break;
    case DISPLAY_MODE_SET_MONTH:
      checkData(curMinute, 12, dir, 1);
      break;
    case DISPLAY_MODE_SET_YEAR:
      checkData(curMinute, 99, dir);
      break;
#endif
    default:
      break;
    }
    time_checked = true;
    btnUp.setBtnFlag(BTN_FLAG_NONE);
    btnDown.setBtnFlag(BTN_FLAG_NONE);
  }

  // вывод данных на экран ============
  switch (displayMode)
  {
#ifdef USE_ALARM
  case DISPLAY_MODE_ALARM_ON_OFF:
    showAlarmState(curHour);
    break;
#endif
  default:
    showTimeData(curHour, curMinute);
    break;
  }
}

#ifdef USE_TEMP_DATA
#ifdef USE_DS18B20
void checkDS18b20()
{
  temp_sensor.readData();
}
#endif
void showTemp()
{
  if (!tasks.getTaskState(show_temp_mode))
  {
    tasks.startTask(return_to_default_mode);
    tasks.startTask(show_temp_mode);
  }

#if defined(USE_DS18B20) || defined(USE_NTC)
  disp.showTemp(temp_sensor.getTemp());
#else
  disp.showTemp((int)round(clock.getTemperature()));
#endif
}
#endif

void setDisp()
{
  disp.show();
}

#ifdef USE_CALENDAR
void showCalendar()
{
  if (!tasks.getTaskState(show_calendar_mode))
  {
    tasks.startTask(show_calendar_mode);
    disp.showDate(curTime, true);
  }

  if (disp.showDate(curTime))
  {
    returnToDefMode();
  }
}
#endif

#ifdef USE_ALARM
void checkAlarm()
{
  alarm.tick(curTime);
  if (alarm.getAlarmState() == ALARM_YES && !tasks.getTaskState(alarm_buzzer))
  {
    runAlarmBuzzer();
  }
}

void runAlarmBuzzer()
{
  static uint8_t n = 0;
  static uint8_t k = 0;
  static uint8_t m = 0;
  // "мелодия" пищалки: первая строка - частота, вторая строка - длительность
  static const PROGMEM uint32_t pick[2][8] = {
      {2000, 0, 2000, 0, 2000, 0, 2000, 0},
      {70, 70, 70, 70, 70, 70, 70, 510}};

  if (!tasks.getTaskState(alarm_buzzer))
  {
    tasks.startTask(alarm_buzzer);
    n = 0;
    k = 0;
    m = 0;
  }
  else if (alarm.getAlarmState() == ALARM_ON)
  { // остановка пищалки, если будильник отключен
    tasks.stopTask(alarm_buzzer);
    return;
  }

  tone(BUZZER_PIN, pgm_read_dword(&pick[0][n]), pgm_read_dword(&pick[1][n]));
  tasks.setTaskInterval(alarm_buzzer, pgm_read_dword(&pick[1][n]), true);
  if (++n >= 8)
  {
    n = 0;
    if (++k >= ALARM_DURATION)
    { // приостановка пищалки через заданное число секунд
      k = 0;
      if (++m >= ALARM_REPETITION_COUNT)
      { // отключение пищалки после заданного количества срабатываний
        tasks.stopTask(alarm_buzzer);
        tasks.setTaskInterval(alarm_buzzer, 50, false);
        alarm.setAlarmState(ALARM_ON);
      }
      else
      {
        tasks.setTaskInterval(alarm_buzzer, ALARM_SNOOZE_DELAY * 1000ul, true);
      }
    }
  }
}
#endif

#ifdef USE_LIGHT_SENSOR
void setBrightness()
{
#ifdef USE_SET_BRIGHTNESS_MODE
  if (tasks.getTaskState(set_brightness_mode))
  {
    return; // в режиме настройки яркости ничего не регулировать
  }
#endif

  static uint16_t b;
  b = (b * 2 + analogRead(LIGHT_SENSOR_PIN)) / 3;
  uint8_t x = 1;
  if (b < LIGHT_THRESHOLD)
  {
    x = EEPROM.read(MIN_BRIGHTNESS_VALUE);
  }
  else if (b > LIGHT_THRESHOLD + 50)
  {
    x = EEPROM.read(MAX_BRIGHTNESS_VALUE);
  }
#if defined(WS2812_MATRIX_DISPLAY)
  FastLED.setBrightness(x * 10);
#else
  disp.setBrightness(x);
#endif
}
#endif

#ifdef USE_SET_BRIGHTNESS_MODE
void showBrightnessSetting()
{
  static uint8_t x = 0;

  if (!tasks.getTaskState(set_brightness_mode))
  {
    tasks.startTask(set_brightness_mode);
    tasks.startTask(return_to_default_mode);
    x = EEPROM.read(MAX_BRIGHTNESS_VALUE);
#ifdef USE_LIGHT_SENSOR
    if (displayMode == DISPLAY_MODE_SET_BRIGHTNESS_MIN)
    {
      x = EEPROM.read(MIN_BRIGHTNESS_VALUE);
    }
#endif
  }

  // ==== опрос кнопок ===============================
  if (btnSet.getBtnFlag() > BTN_FLAG_NONE)
  {
    switch (displayMode)
    {
    case DISPLAY_MODE_SET_BRIGHTNESS_MAX:
      EEPROM.update(MAX_BRIGHTNESS_VALUE, x);
      displayMode = DISPLAY_MODE_SHOW_TIME;
      stopSetting(set_brightness_mode);
      break;
#ifdef USE_LIGHT_SENSOR
    case DISPLAY_MODE_SET_BRIGHTNESS_MIN:
      EEPROM.update(MIN_BRIGHTNESS_VALUE, x);
      if (btnSet.getBtnFlag() == BTN_FLAG_NEXT)
      {
        displayMode = DISPLAY_MODE_SET_BRIGHTNESS_MAX;
      }
      else
        displayMode = DISPLAY_MODE_SHOW_TIME;
      stopSetting(set_brightness_mode);
      break;
#endif
    default:
      break;
    }
    btnSet.setBtnFlag(BTN_FLAG_NONE);
  }

  if ((btnUp.getBtnFlag() == BTN_FLAG_NEXT) || (btnDown.getBtnFlag() == BTN_FLAG_NEXT))
  {
    bool dir = btnUp.getBtnFlag() == BTN_FLAG_NEXT;
#if defined(MAX72XX_7SEGMENT_DISPLAY) || defined(MAX72XX_MATRIX_DISPLAY)
    checkData(x, 15, dir, 0, false);
#elif defined(WS2812_MATRIX_DISPLAY)
    checkData(x, 25, dir, 1, false);
#else
    checkData(x, 7, dir, 1, false);
#endif

    btnUp.setBtnFlag(BTN_FLAG_NONE);
    btnDown.setBtnFlag(BTN_FLAG_NONE);
  }

  // ==== вывод данных на экран ======================
#if defined(WS2812_MATRIX_DISPLAY)
  FastLED.setBrightness(x * 10);
#else
  disp.setBrightness(x);
#endif
  bool blink = !blink_flag && !btnUp.isButtonClosed() && !btnDown.isButtonClosed();
  bool snr = false;
#ifdef USE_LIGHT_SENSOR
  snr = true;
#endif
  disp.showBrightnessData(x, blink, snr,
                          displayMode != DISPLAY_MODE_SET_BRIGHTNESS_MAX);
}
#endif

// ===================================================
void showTimeData(uint8_t hour, uint8_t minute)
{
  // если наступило время блинка и кнопки Up/Down не нажаты, то стереть соответствующие разряды; при нажатых кнопках Up/Down во время изменения данных ничего не мигает
  if (!blink_flag && !btnUp.isButtonClosed() && !btnDown.isButtonClosed())
  {
    switch (displayMode)
    {
    case DISPLAY_MODE_SET_HOUR:
#ifdef USE_CALENDAR
    case DISPLAY_MODE_SET_DAY:
#endif
#ifdef USE_ALARM
    case DISPLAY_MODE_SET_ALARM_HOUR:
#endif
      hour = -1;
      break;
    case DISPLAY_MODE_SET_MINUTE:
#ifdef USE_CALENDAR
    case DISPLAY_MODE_SET_MONTH:
    case DISPLAY_MODE_SET_YEAR:
#endif
#ifdef USE_ALARM
    case DISPLAY_MODE_SET_ALARM_MINUTE:
#endif
      minute = -1;
      break;
    default:
      break;
    }
  }

  bool toDate = false;
  bool toColon = false;
#if defined(USE_CALENDAR)
  toDate = (displayMode >= DISPLAY_MODE_SET_DAY && displayMode <= DISPLAY_MODE_SET_YEAR);
  toColon = displayMode != DISPLAY_MODE_SET_YEAR;
#endif
#if defined(MAX72XX_MATRIX_DISPLAY) || defined(WS2812_MATRIX_DISPLAY)
  disp.showTime(hour, minute, 0, toColon, toDate);
#else
  disp.showTime(hour, minute, toColon);
#endif
}

#ifdef USE_ALARM
void showAlarmState(uint8_t _state)
{
#if defined(TM1637_DISPLAY)
  disp.setDispData(0, 0b01110111); // "A"
  disp.setDispData(1, 0b10111000); // "L:"
  disp.setDispData(2, 0x00);
#elif defined(MAX72XX_7SEGMENT_DISPLAY)
  disp.setDispData(0, 0b01110111); // "A"
  disp.setDispData(1, 0b10001110); // "L:"
  disp.setDispData(2, 0x00);
#elif defined(MAX72XX_MATRIX_DISPLAY) || defined(WS2812_MATRIX_DISPLAY)
  disp.clear();
#ifdef USE_RU_LANGUAGE
  disp.setDispData(1, 0xC1, 5);  // "Б"
  disp.setDispData(7, 0xE4, 5);  // "д"
  disp.setDispData(13, 0xEA, 5); // "к"
#else
  disp.setDispData(1, 0x41, 5);  // "A"
  disp.setDispData(7, 0x6C, 5);  // "l"
  disp.setDispData(13, 0x6D, 5); // "r"
#endif
#ifdef MAX72XX_MATRIX_DISPLAY
  disp.setColumn(2, 5, 0b00100100);
#else
  disp.setColumn(21, 0b00100100);
#endif
#endif

  if (!blink_flag && !btnUp.isButtonClosed() && !btnDown.isButtonClosed())
  {
#if defined(TM1637_DISPLAY) || defined(MAX72XX_7SEGMENT_DISPLAY)
    disp.setDispData(3, 0x00);
#endif
  }
  else
  {
#if defined(TM1637_DISPLAY)
    disp.setDispData(3, (_state) ? 0b01011100 : 0b00001000);
#elif defined(MAX72XX_7SEGMENT_DISPLAY)
    disp.setDispData(3, (_state) ? 0b00011101 : 0b00001000);
#elif defined(MAX72XX_MATRIX_DISPLAY) || defined(WS2812_MATRIX_DISPLAY)
    disp.setDispData(25, (_state) ? 0x0C : 0x0B);
#endif
  }
}
#endif

// ===================================================
void checkData(uint8_t &dt, uint8_t max, bool toUp, uint8_t min, bool toLoop)
{
  (toUp) ? dt++ : dt--;
  if ((dt > max) || (min > 0 && dt < min))
  {
    if (toLoop)
    {
      dt = (toUp) ? min : max;
    }
    else
    {
      dt = (toUp) ? max : min;
    }
  }
}

// ===================================================
void setDisplay()
{
  switch (displayMode)
  {
  case DISPLAY_MODE_SET_HOUR:
  case DISPLAY_MODE_SET_MINUTE:
#ifdef USE_CALENDAR
  case DISPLAY_MODE_SET_DAY:
  case DISPLAY_MODE_SET_MONTH:
  case DISPLAY_MODE_SET_YEAR:
#endif
#ifdef USE_ALARM
  case DISPLAY_MODE_SET_ALARM_HOUR:
  case DISPLAY_MODE_SET_ALARM_MINUTE:
  case DISPLAY_MODE_ALARM_ON_OFF:
#endif
    if (!tasks.getTaskState(set_time_mode))
    {
      showTimeSetting();
    }
    break;
#ifdef USE_TEMP_DATA
  case DISPLAY_MODE_SHOW_TEMP:
    if (!tasks.getTaskState(show_temp_mode))
    {
      showTemp();
    }
    break;
#endif
#ifdef USE_CALENDAR
  case DISPLAY_MODE_SHOW_CALENDAR:
    if (!tasks.getTaskState(show_calendar_mode))
    {
      showCalendar();
    }
    break;
#endif
#ifdef USE_SET_BRIGHTNESS_MODE
#ifdef USE_LIGHT_SENSOR
  case DISPLAY_MODE_SET_BRIGHTNESS_MIN:
#endif
  case DISPLAY_MODE_SET_BRIGHTNESS_MAX:
    if (!tasks.getTaskState(set_brightness_mode))
    {
      showBrightnessSetting();
    }
    break;
#endif
  default:
    break;
  }
}

// ===================================================
void setup()
{
  // ==== часы =======================================
  Wire.begin();
  clock.setClockMode(false); // 24-часовой режим
  rtcNow();

  // ==== кнопки Up/Down =============================
  btnUp.setLongClickMode(LCM_CLICKSERIES);
  btnUp.setIntervalOfSerial(100);
  btnDown.setLongClickMode(LCM_CLICKSERIES);
  btnDown.setIntervalOfSerial(100);

// ==== датчики ======================================
#if defined(USE_NTC)
  temp_sensor.setADCbitDepth(10); // установить разрядность АЦП вашего МК, для AVR обычно равна 10 бит
#endif
  // проверить корректность заданных уровней яркости
  uint8_t x = EEPROM.read(MAX_BRIGHTNESS_VALUE);
#if defined(MAX72XX_7SEGMENT_DISPLAY) || defined(MAX72XX_MATRIX_DISPLAY)
  x = (x > 15) ? 8 : x;
#elif defined(WS2812_MATRIX_DISPLAY)
  x = (x > 25) ? 15 : x;
#else
  x = ((x > 7) || (x == 0)) ? 7 : x;
#endif
  EEPROM.update(MAX_BRIGHTNESS_VALUE, x);
#ifdef USE_LIGHT_SENSOR
  x = EEPROM.read(MIN_BRIGHTNESS_VALUE);
#if defined(MAX72XX_7SEGMENT_DISPLAY) || defined(MAX72XX_MATRIX_DISPLAY)
  x = (x > 15) ? 0 : x;
#elif defined(WS2812_MATRIX_DISPLAY)
  x = ((x > 25) || (x == 0)) ? 1 : x;
#else
  x = ((x > 7) || (x == 0)) ? 1 : x;
#endif
  EEPROM.update(MIN_BRIGHTNESS_VALUE, x);
#endif

// ==== экраны =======================================
#if defined(WS2812_MATRIX_DISPLAY)
  setFastLEDData(leds, 256);

#elif defined(MAX72XX_MATRIX_DISPLAY) || defined(MAX72XX_7SEGMENT_DISPLAY)
  disp.shutdownAllDevices(false);
#ifdef MAX72XX_MATRIX_DISPLAY
  disp.setDirection(2); // задайте нужный вам поворот картинки (0-3)
  // disp.setFlip(true);   // раскомментируйте, если нужно включить отражение картинки по горизонтали
#endif
#endif

// выставить яркость в минимум, чтобы при включении не сверкало максимальной яркостью
#if defined(WS2812_MATRIX_DISPLAY)
  FastLED.setBrightness(0);
#elif defined(MAX72XX_MATRIX_DISPLAY) || defined(MAX72XX_7SEGMENT_DISPLAY)
  disp.setBrightness(0);
#else
  disp.setBrightness(1);
#endif

  // ==== задачи =====================================
  uint8_t task_count = 5; // базовое количество задач
#ifdef USE_LIGHT_SENSOR
  task_count++;
#endif
#ifdef USE_TEMP_DATA
  task_count++;
#if defined(USE_DS18B20)
  task_count++;
#endif
#endif
#ifdef USE_CALENDAR
  task_count++;
#endif
#ifdef USE_ALARM
  task_count += 2;
#endif
#ifdef USE_SET_BRIGHTNESS_MODE
  task_count++;
#endif
  tasks.init(task_count);

  rtc_guard = tasks.addTask(50ul, rtcNow);
  blink_timer = tasks.addTask(50ul, blink);
  return_to_default_mode = tasks.addTask(AUTO_EXIT_TIMEOUT * 1000ul, returnToDefMode, false);
  set_time_mode = tasks.addTask(100ul, showTimeSetting, false);
#ifdef USE_TEMP_DATA
  show_temp_mode = tasks.addTask(500ul, showTemp, false);
#if defined(USE_DS18B20)
  ds18b20_guard = tasks.addTask(3000ul, checkDS18b20);
#endif
#endif
#ifdef USE_CALENDAR
#if (defined(WS2812_MATRIX_DISPLAY) || defined(MAX72XX_MATRIX_DISPLAY)) && defined(USE_TICKER_FOR_DATE)
  uint32_t t = 1000ul / TICKER_SPEED;
#else
  uint32_t t = 1000ul;
#endif
  show_calendar_mode = tasks.addTask(t, showCalendar, false);
#endif
#ifdef USE_ALARM
  alarm_guard = tasks.addTask(200ul, checkAlarm);
  alarm_buzzer = tasks.addTask(50ul, runAlarmBuzzer, false);
#endif
  display_guard = tasks.addTask(50ul, setDisp);
#if defined(USE_LIGHT_SENSOR)
  light_sensor_guard = tasks.addTask(100ul, setBrightness);
#elif defined(WS2812_MATRIX_DISPLAY)
  FastLED.setBrightness(EEPROM.read(MAX_BRIGHTNESS_VALUE) * 10);
#else
  disp.setBrightness(EEPROM.read(MAX_BRIGHTNESS_VALUE));
#endif
#ifdef USE_SET_BRIGHTNESS_MODE
  set_brightness_mode = tasks.addTask(100ul, showBrightnessSetting, false);
#endif
}

void loop()
{
  checkButton();
  tasks.tick();
  setDisplay();
}
