#pragma once
#include <Arduino.h>
#include <avr/pgmspace.h>
#include <shMAX72xxMini.h> // https://github.com/VAleSh-Soft/shMAX72xxMini

// ==== класс для 7-сегментного индикатора MAX72xx ===

template <uint8_t cs_pin>
class DisplayMAX72xx7segment : public shMAX72xx7Segment<cs_pin, 1, 4>
{
private:
  byte data[4];

public:
  DisplayMAX72xx7segment() : shMAX72xx7Segment<cs_pin, 1, 4>() { clear(); }

  // очистка буфера экрана, сам экран при этом не очищается
  void clear()
  {
    for (byte i = 0; i < 4; i++)
    {
      data[i] = 0x00;
    }
  }

  // очистка экрана
  void sleep()
  {
    clear();
    shMAX72xx7Segment<cs_pin, 1, 4>::clearAllDevices(true);
  }

  // установка разряда _index буфера экрана
  void setDispData(byte _index, byte _data)
  {
    if (_index < 4)
    {
      data[_index] = _data;
    }
  }

  // получение значения разряда _index буфера экрана
  byte getDispData(byte _index)
  {
    return ((_index < 4) ? data[_index] : 0);
  }

  void setSegments(byte *data)
  {
    shMAX72xx7Segment<cs_pin, 1, 4>::clearAllDevices();
    for (byte i = 0; i < 4; i++)
    {
      shMAX72xx7Segment<cs_pin, 1, 4>::setChar(i, data[i]);
    }

    shMAX72xx7Segment<cs_pin, 1, 4>::update();
  }

  // отрисовка на экране содержимого его буфера
  void show()
  {
    bool flag = false;
    static byte _data[4] = {0x00, 0x00, 0x00, 0x00};
    for (byte i = 0; i < 4; i++)
    {
      flag = _data[i] != data[i];
      if (flag)
      {
        break;
      }
    }
    // отрисовка экрана происходит только если изменился хотя бы один разряд
    if (flag)
    {
      for (byte i = 0; i < 4; i++)
      {
        _data[i] = data[i];
      }
      setSegments(data);
    }
  }

  // вывод на экран  времени; если задать какое-то из значений hour или minute отрицательным, эта часть экрана будет очищена - можно организовать мигание, например, в процессе настройки времени; show_colon - отображать или нет двоеточие между часами и минутами
  void showTime(int8_t hour, int8_t minute, bool show_colon)
  {
    clear();
    if (hour >= 0)
    {
      data[0] = shMAX72xx7Segment<cs_pin, 1, 4>::encodeDigit(hour / 10);
      data[1] = shMAX72xx7Segment<cs_pin, 1, 4>::encodeDigit(hour % 10);
    }
    if (minute >= 0)
    {
      data[2] = shMAX72xx7Segment<cs_pin, 1, 4>::encodeDigit(minute / 10);
      data[3] = shMAX72xx7Segment<cs_pin, 1, 4>::encodeDigit(minute % 10);
    }
    if (show_colon)
    {
      data[1] |= 0x80; // для показа двоеточия установить старший бит во второй цифре
    }
  }

  // вывод на экран температуры в диапазоне от -99 до +99 градусов; вне диапазона выводится строка минусов
  void showTemp(int temp)
  {
    clear();
    data[3] = 0x63;
    // если температура отрицательная, сформировать минус впереди
    if (temp < 0)
    {
      temp = -temp;
      data[1] = minusSegments;
    }
    // если температура выходит за диапазон, сформировать строку минусов
    if (temp > 99)
    {
      for (byte i = 0; i < 4; i++)
      {
        data[i] = minusSegments;
      }
    }
    else
    {
      if (temp > 9)
      {
        if (data[1] == minusSegments)
        { // если температура ниже -9, переместить минус на крайнюю левую позицию
          data[0] = minusSegments;
        }
        data[1] = shMAX72xx7Segment<cs_pin, 1, 4>::encodeDigit(temp / 10);
      }
      data[2] = shMAX72xx7Segment<cs_pin, 1, 4>::encodeDigit(temp % 10);
    }
  }

  // установка яркости экрана; реально яркость будет изменена только после вызова метода show()
  void setBrightness(byte brightness)
  {
    brightness = map(brightness, 1, 7, 0, 15);
    shMAX72xxMini<cs_pin, 1>::setBrightness(0, brightness);
  }
};

// ==== класс для матрицы 8х8х4 MAX72xx ==============

// цифры 6x8
const uint8_t PROGMEM font_digit[] = {
    0x7E, 0x85, 0x89, 0x91, 0xA1, 0x7E, // 0
    0x00, 0x00, 0x41, 0xFF, 0x01, 0x00, // 1
    0x43, 0x85, 0x89, 0x89, 0x89, 0x73, // 2
    0x42, 0x81, 0x91, 0x91, 0x91, 0x6E, // 3
    0x0C, 0x14, 0x24, 0x44, 0x84, 0xFF, // 4
    0xF2, 0xA1, 0xA1, 0xA1, 0xA1, 0x9E, // 5
    0x7E, 0x91, 0x91, 0x91, 0x91, 0x4E, // 6
    0xC0, 0x80, 0x87, 0x88, 0x90, 0xE0, // 7
    0x6E, 0x91, 0x91, 0x91, 0x91, 0x6E, // 8
    0x72, 0x89, 0x89, 0x89, 0x89, 0x7E, // 9
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // space
    0x00, 0x08, 0x08, 0x08, 0x08, 0x08, // -
    0x00, 0x08, 0x08, 0x3E, 0x08, 0x08, // +
    0xA0, 0x40, 0x1E, 0x21, 0x21, 0x12, // grad
    0x7F, 0x88, 0x88, 0x88, 0x88, 0x7F, // A
    0xFF, 0x01, 0x01, 0x01, 0x01, 0x03, // L
    0x11, 0x0A, 0x04, 0x0A, 0x11, 0x00, // x
    0x04, 0x02, 0x01, 0x06, 0x18, 0x20  // √
};

template <uint8_t cs_pin>
class DisplayMAX72xxMatrix : public shMAX72xxMini<cs_pin, 4>
{
private:
  byte data[5];

public:
  DisplayMAX72xxMatrix() : shMAX72xxMini<cs_pin, 4>() { clear(); }

  // очистка буфера экрана, сам экран при этом не очищается
  void clear()
  {
    for (byte i = 0; i < 4; i++)
    {
      data[i] = 0x0a; // пустой символ в массиве идет под индексом 10
    }
  }

  // очистка экрана
  void sleep()
  {
    clear();
    shMAX72xxMini<cs_pin, 4>::clearAllDevices(true);
  }

  // установка разряда _index буфера экрана
  void setDispData(byte _index, byte _data)
  {
    if (_index < 5)
    {
      data[_index] = _data;
    }
  }

  // получение значения разряда _index буфера экрана
  byte getDispData(byte _index)
  {
    return ((_index < 5) ? data[_index] : 0);
  }

  void setSegments(byte *data)
  {
    shMAX72xxMini<cs_pin, 4>::clearAllDevices();
    for (byte i = 0; i < 4; i++)
    {
      byte offset;
      switch (data[4])
      {
      case 2:
        offset = (i == 3) ? 0 : 2 - i; // для температуры отступы цифр - 2, 1, 0, 0
        break;
      case 3:
        offset = 1;
        break;
      default:
        offset = 1 - i % 2; // иначе - 1, 0, 1, 0
        break;
      }
      for (byte j = 0; j < 6; j++)
      {
        shMAX72xxMini<cs_pin, 4>::setColumn(i,
                                            j + offset,
                                            pgm_read_byte(&font_digit[data[i] * 6 + j]));
      }
    }
    switch (data[4])
    {
    case 1: // отрисовка двоеточия
      shMAX72xxMini<cs_pin, 4>::setColumn(1, 7, 0b00100100);
      break;
    case 2: // дорисовка значка градуса
      shMAX72xxMini<cs_pin, 4>::setColumn(2, 7, 0b01000000);
      break;
    case 4: // отрисовка точки в дате
      shMAX72xxMini<cs_pin, 4>::setColumn(1, 7, 0b00000001);
      break;
    }

    shMAX72xxMini<cs_pin, 4>::update();
  }

  // отрисовка на экране содержимого его буфера
  void show()
  {
    bool flag = false;
    static byte _data[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
    for (byte i = 0; i < 5; i++)
    {
      flag = _data[i] != data[i];
      if (flag)
      {
        break;
      }
    }
    // отрисовка экрана происходит только если изменился хотя бы один разряд
    if (flag)
    {
      for (byte i = 0; i < 5; i++)
      {
        _data[i] = data[i];
      }
      setSegments(data);
    }
  }

  // вывод на экран  времени или даты; если задать какое-то из значений hour или minute отрицательным, эта часть экрана будет очищена - можно организовать мигание, например, в процессе настройки времени; show_colon - отображать или нет двоеточие между часами и минутами (или точку между днем и месяцем)
  void showTime(int8_t hour, int8_t minute, bool show_colon, bool date = false)
  {
    clear();
    if (hour >= 0)
    {
      data[0] = hour / 10;
      data[1] = hour % 10;
    }
    if (minute >= 0)
    {
      data[2] = minute / 10;
      data[3] = minute % 10;
    }
    data[4] = (date) ?(byte)show_colon + 3: show_colon;
  }

  // вывод на экран температуры в диапазоне от -99 до +99 градусов; вне диапазона выводится строка минусов
  void showTemp(int temp)
  {
    clear();
    data[3] = 0x0D;
    data[4] = 2;
    // сформировать впереди плюс или минус
    data[1] = (temp < 0) ? 0x0B : 0x0C;
    if (temp < 0)
    {
      temp = -temp;
    }
    // если температура выходит за диапазон, сформировать строку минусов
    if (temp > 99)
    {
      for (byte i = 0; i < 4; i++)
      {
        data[i] = 0x0B;
      }
      data[4] = 3;
    }
    else
    {
      if (temp > 9)
      {
        // если температура двухзначная, переместить знак в первую позицию
        data[0] = data[1];
        data[1] = temp / 10;
      }
      data[2] = temp % 10;
    }
  }

  // установка яркости экрана
  void setBrightness(byte brightness)
  {
    brightness = map(brightness, 1, 7, 0, 15);
    for (byte i = 0; i < 4; i++)
    {
      shMAX72xxMini<cs_pin, 4>::setBrightness(i, brightness);
    }
  }
};
