/* Небольшая библиотека для работы с датчиками температуры DS1820, DS18s20, DS18b20 или DS1822;
   Работает только с одним датчиком, выдает температуру в градусах Цельсия в формате int16_t, имеет контроль наличия датчика на линии, в случае, если датчика нет, выдает -127 градусов.

   Для работы требует наличие библиотеки OneWire.h - https://github.com/PaulStoffregen/OneWire

   Методы библиотеки

   DS1820 temp_sensor(data_pin) - конструктор, data_pin - пин, к которому подключен датчик (наличие резистора 4.7кОм между пином данных и VCC обязательно)

   void readData() - опрос датчика; следует вызывать не чаще одного раза в секунду, а лучше реже, т.к. при слишком частом опросе микросхема датчика начинает вносить искажения в температуру за счет собственного разогрева; считанные данные помещаются в поле temp;

   int16_t getTemp() - получение ранее считанной из дачика температуры;
*/
#pragma once
#include <Arduino.h>
#include <OneWire.h> // https://github.com/PaulStoffregen/OneWire

#define ERROR_TEMP -127

class DS1820 : public OneWire
{
private:
  int16_t temp = ERROR_TEMP;
  uint8_t addr[8];
  uint8_t type_c = 10;

  bool checkData(uint8_t *data)
  {
    // проверка данных на валидность - сначала по контрольной сумме;
    // в случае отсутствия датчика data[] заполняется нулями, поэтому
    // проверка CRC ничего не даст - CRC суммы нулей равно нулю, а
    // именно нуль будет записан в ячейке CRC массива data[] в этом
    // случае; поэтому проверяем ячейку, в которой указано разрешение
    // конвертации, оно нулю равняться не должно
    bool result = OneWire::crc8(data, 8) == data[8];
    if (result && data[8] == 0)
    {
      result = (!type_c && data[7] != 0x00) && (type_c && data[4] != 0x00);
    }

    return (result);
  }

public:
  DS1820(uint8_t data_pin) : OneWire(data_pin)
  {
    OneWire::reset();
    // если датчик найден
    if (OneWire::search(addr))
    {
      // если адрес верифицирован
      if (OneWire::crc8(addr, 7) == addr[7])
      {
        // определяем тип чипа
        switch (addr[0])
        {
        case 0x10: // Chip = DS18S20 или старый DS1820
          type_c = 0;
          break;
        case 0x28: // Chip = DS18B20
        case 0x22: // Chip = DS1822
          type_c = 1;
          break;
        }
      }
      if (type_c < 2)
      {
        OneWire::reset();
        OneWire::select(addr);
        OneWire::write(0x44, 1);
      }
    }
  }

  /**
   * @brief считывание показаний датчика; вызывать не чаще одного раза в секунду
   *
   */
  void readData()
  {
    if (type_c < 2)
    {
      uint8_t data[9];
      // считывание данных из датчика
      OneWire::reset();
      OneWire::select(addr);
      OneWire::write(0xBE);
      for (uint8_t i = 0; i < 9; i++)
      {
        data[i] = OneWire::read();
      }

      if (!checkData(data))
      {
        temp = ERROR_TEMP;
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
          uint8_t cfg = (data[4] & 0x60);
          // при более низком разрешении младшие биты не определены, поэтому обнуляем их
          if (cfg == 0x00)
            raw = raw & ~7; // разрешение 9 бит, 93.75 ms
          else if (cfg == 0x20)
            raw = raw & ~3; // разрешение 10 бит, 187.5 ms
          else if (cfg == 0x40)
            raw = raw & ~1; // разрешение 11 бит, 375 ms
          //// разрешение по умолчанию 12 бит, время преобразования 750 ms
        }
        temp = raw / 16;
        // округляем остаток от деления
        if (raw % 16 >= 8)
        {
          temp++;
        }
      }

      // даем команду на конвертацию для следующего запроса
      OneWire::reset();
      OneWire::select(addr);
      OneWire::write(0x44, 1);
    }
  }

  /**
   * @brief получение ранее считанной температуры
   *
   * @return int16_t
   */
  int16_t getTemp() { return (temp); }
};
