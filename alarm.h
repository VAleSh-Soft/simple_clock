#pragma once
#include <Arduino.h>
#include <DS3231.h>
#include <EEPROM.h>

#define MAX_DATA 1439 // максимальное количество минут для установки будильника (23 ч, 59 мин)

enum IndexOffset : uint8_t // смещение от стартового индекса в EEPROM для хранения настроек
/* общий размер настроек - 3 байта */
{
  ALARM_STATE = 0, // состояние будильника, включен/нет, uint8_t
  ALARM_POINT = 1  // точка срабатывания будильника в минутах от полуночи, uint16_t
};

enum AlarmState : uint8_t // состояние будильника
{
  ALARM_OFF, // будильник выключен
  ALARM_ON,  // будильник включен
  ALARM_YES  //будильник сработал
};

class Alarm
{
private:
  byte led_pin;
  uint16_t eeprom_index;
  AlarmState state;

  uint8_t
  read_eeprom_8(IndexOffset _index)
  {
    return (EEPROM.read(eeprom_index + _index));
  }

  uint16_t read_eeprom_16(IndexOffset _index)
  {
    uint16_t _data;
    EEPROM.get(eeprom_index + _index, _data);
    return (_data);
  }

  void write_eeprom_8(IndexOffset _index, uint8_t _data) { EEPROM.update(eeprom_index + _index,
                                                                         _data); }

  void write_eeprom_16(IndexOffset _index, uint16_t _data) { EEPROM.put(eeprom_index + _index,
                                                                        _data); }

  void setLed()
  {
    static byte n = 0;
    byte led_state = LOW;
    switch (state)
    {
    case ALARM_ON: // при включенном будильнике светодиод горит
      led_state = HIGH;
      n = 0;
      break;
    case ALARM_YES: // при сработавшем будильнике светодиод мигает с периодом 0.2 секунды
      led_state = n != 0;
      if (++n > 1)
      {
        n = 0;
      }
      break;
    default:
      break;
    }
    digitalWrite(led_pin, led_state);
  }

public:
  Alarm(byte _led_pin, uint16_t _eeprom_index)
  {
    led_pin = _led_pin;
    pinMode(led_pin, OUTPUT);
    eeprom_index = _eeprom_index;
    if (read_eeprom_8(ALARM_STATE) > 1)
    {
      write_eeprom_8(ALARM_STATE, 0);
    }
    if (read_eeprom_16(ALARM_POINT) > MAX_DATA)
    {
      write_eeprom_16(ALARM_POINT, 0);
    }
    state = (AlarmState)read_eeprom_8(ALARM_STATE);
  }

  AlarmState getAlarmState() { return (state); }

  void setAlarmState(AlarmState _state) { state = _state; }

  bool getOnOffAlarm() { return (bool)read_eeprom_8(ALARM_STATE); }

  void setOnOffAlarm(bool _state)
  {
    write_eeprom_8(ALARM_STATE, (byte)_state);
    state = (AlarmState)_state;
  }

  uint16_t getAlarmPoint() { return (read_eeprom_16(ALARM_POINT)); }

  void setAlarmPoint(uint16_t _time) { write_eeprom_16(ALARM_POINT, _time); }

  void tick(DateTime _time)
  {
    setLed();
    switch (state)
    {
    case ALARM_ON:
      uint32_t tm;
      tm = _time.hour() * 3600ul + _time.minute() * 60ul + _time.second();
      if (tm == getAlarmPoint() * 60ul)
      {
        state = ALARM_YES;
      }
      break;
    default:
      break;
    }
  }
};
