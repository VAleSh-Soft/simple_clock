#pragma once
#include <Arduino.h>
#include <OneWire.h> // https://github.com/PaulStoffregen/OneWire

#define ERROR_TEMP -127

class DS1820 : public OneWire
{
private:
  int16_t temperature = ERROR_TEMP;
  byte *addr = NULL;
  bool sensor_on = false;
  byte type_c = 10;

public:
  DS1820(byte data_pin) : OneWire(data_pin)
  {
    addr = new byte[8];
    OneWire::reset();
    sensor_on = OneWire::search(addr);
    // если датчик найден
    if (sensor_on)
    {
      sensor_on = OneWire::crc8(addr, 7) == addr[7];
      // если адрес верифицирован
      if (sensor_on)
      {
        // опрееляем тип чипа
        switch (addr[0])
        {
        case 0x10: // Chip = DS18S20 или old DS1820
          type_c = 0;
          break;
        case 0x28: // Chip = DS18B20"
        case 0x22: // Chip = DS1822
          type_c = 1;
          break;
        }
        sensor_on = type_c < 2;
      }
      if (sensor_on)
      {
        OneWire::reset();
        OneWire::select(addr);
        OneWire::write(0x44, 1);
      }
    }
  }

  // считывание показаний датчика; вызывать не чаще одного раза в секунду
  void readTemp()
  {
    if (sensor_on)
    {
      byte data[9];
      OneWire::reset();
      OneWire::select(addr);
      OneWire::write(0xBE);
      for (byte i = 0; i < 9; i++)
      {
        data[i] = OneWire::read();
      }
      if (OneWire::crc8(data, 8) != data[8])
      {
        temperature = ERROR_TEMP;
      }
      else
      {
        int16_t raw = (data[1] << 8) | data[0]; // считываем два байта температуры
        if (!type_c)
        // для датчиков DS1820 и DS18s20
        {
          raw = raw << 3; // разрешение по умолчанию 9 бит
          if (data[7] == 0x10)
          {
            // «количество оставшихся» дает полное 12-битное разрешение
            raw = (raw & 0xFFF0) + 12 - data[6];
          }
        }
        else
        // для датчиков DS18b20 и DS1822
        {
          byte cfg = (data[4] & 0x60);
          // при более низком разрешении младшие биты не определены, поэтому обнуляем их
          if (cfg == 0x00)
            raw = raw & ~7; // разрешение 9 бит, 93.75 ms
          else if (cfg == 0x20)
            raw = raw & ~3; // разрешение 10 бит, 187.5 ms
          else if (cfg == 0x40)
            raw = raw & ~1; // разрешение 11 бит, 375 ms
          //// разрешение по умолчанию 12 бит, время преобразования 750 ms
        }
        temperature = raw / 16;
        if (raw % 16 >= 8)
        {
          temperature++;
        }
      }

      // даем команду на конвертацию для следующего запроса
      OneWire::reset();
      OneWire::select(addr);
      OneWire::write(0x44, 1);
    }
  }

  int16_t getTemp() { return (temperature); }
};
