#pragma once
#include "matrix_data.h"
#include <Arduino.h>
#include <avr/pgmspace.h>
#include <shMAX72xxMini.h> // https://github.com/VAleSh-Soft/shMAX72xxMini

// ==== класс для 7-сегментного индикатора MAX72xx ===

#define NUM_DIGITS 8

template <uint8_t cs_pin>
class DisplayMAX72xx7segment : public shMAX72xx7Segment<cs_pin, 1, NUM_DIGITS>
{
private:
  byte data[4];

  void setSegments(byte *data)
  {
    shMAX72xx7Segment<cs_pin, 1, NUM_DIGITS>::clearAllDevices();
    for (byte i = 0; i < 4; i++)
    {
      shMAX72xx7Segment<cs_pin, 1, NUM_DIGITS>::setChar(7 - i, data[i]);
    }

    shMAX72xx7Segment<cs_pin, 1, NUM_DIGITS>::update();
  }

public:
  DisplayMAX72xx7segment() : shMAX72xx7Segment<cs_pin, 1, NUM_DIGITS>() { clear(); }

  /**
   * @brief очистка буфера экрана, сам экран при этом не очищается
   *
   */
  void clear()
  {
    for (byte i = 0; i < 4; i++)
    {
      data[i] = 0x00;
    }
  }

  /**
   * @brief очистка экрана
   *
   */
  void sleep()
  {
    clear();
    shMAX72xx7Segment<cs_pin, 1, NUM_DIGITS>::clearAllDevices(true);
  }

  /**
   * @brief установка разряда _index буфера экрана
   *
   * @param _index индекс разряда (0..3)
   * @param _data данные для установки
   */
  void setDispData(byte _index, byte _data)
  {
    if (_index < 4)
    {
      data[_index] = _data;
    }
  }

  /**
   * @brief получение значения разряда _index буфера экрана
   *
   * @param _index индекс разряда (0..3)
   * @return byte
   */
  byte getDispData(byte _index)
  {
    return ((_index < 4) ? data[_index] : 0);
  }

  /**
   * @brief отрисовка на экране содержимого его буфера
   *
   */
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

  /**
   * @brief вывод на экран  времени; если задать какое-то из значений hour или minute отрицательным, эта часть экрана будет очищена - можно организовать мигание, например, в процессе настройки времени
   *
   * @param hour часы
   * @param minute минуты
   * @param show_colon отображать или нет двоеточие между часами и минутами
   */
  void showTime(int8_t hour, int8_t minute, bool show_colon)
  {
    clear();
    if (hour >= 0)
    {
      data[0] = shMAX72xx7Segment<cs_pin, 1, NUM_DIGITS>::encodeDigit(hour / 10);
      data[1] = shMAX72xx7Segment<cs_pin, 1, NUM_DIGITS>::encodeDigit(hour % 10);
    }
    if (minute >= 0)
    {
      data[2] = shMAX72xx7Segment<cs_pin, 1, NUM_DIGITS>::encodeDigit(minute / 10);
      data[3] = shMAX72xx7Segment<cs_pin, 1, NUM_DIGITS>::encodeDigit(minute % 10);
    }
    if (show_colon)
    {
      data[1] |= 0x80; // для показа двоеточия установить старший бит во второй цифре
    }
  }

  /**
   * @brief вывод на экран температуры в диапазоне от -99 до +99 градусов; вне диапазона выводится строка минусов
   *
   * @param temp данные для вывода
   */
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
        data[1] = shMAX72xx7Segment<cs_pin, 1, NUM_DIGITS>::encodeDigit(temp / 10);
      }
      data[2] = shMAX72xx7Segment<cs_pin, 1, NUM_DIGITS>::encodeDigit(temp % 10);
    }
  }

  /**
   * @brief установка яркости экрана
   *
   * @param brightness значение яркости (1..7)
   */
  void setBrightness(byte brightness)
  {
    brightness = (brightness <= 15) ? brightness : 15;
    shMAX72xxMini<cs_pin, 1>::setBrightness(0, brightness);
  }
};

// ==== класс для матрицы 8х8х4 MAX72xx ==============

template <uint8_t cs_pin>
class DisplayMAX72xxMatrix : public shMAX72xxMini<cs_pin, 4>
{
private:
  byte data[6];
  bool show_sec_col = false;

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
    if (show_sec_col)
    {
      setColumn(3, 7, data[5]);
    }

    shMAX72xxMini<cs_pin, 4>::update();
  }

public:
  DisplayMAX72xxMatrix() : shMAX72xxMini<cs_pin, 4>() { clear(); }

  /**
   * @brief очистка буфера экрана, сам экран при этом не очищается
   *
   */
  void clear()
  {
    for (byte i = 0; i < 4; i++)
    {
      data[i] = 0x0a; // пустой символ в массиве идет под индексом 10
    }
    data[4] = 0x00;
    data[5] = 0x00;
  }

  /**
   * @brief включение режима показа секундного столбца
   *
   * @param flag флаг состояния опции
   */
  void setShowSecondColumn(bool flag)
  {
    show_sec_col = flag;
  }

  /**
   * @brief очистка экрана
   *
   */
  void sleep()
  {
    clear();
    shMAX72xxMini<cs_pin, 4>::clearAllDevices(true);
  }

  /**
   * @brief установка разряда _index буфера экрана
   *
   * @param _index индекс разряда (0..5)
   * @param _data данные для установки
   */
  void setDispData(byte _index, byte _data)
  {
    if (_index < 6)
    {
      data[_index] = _data;
    }
  }

  /**
   * @brief получение значения разряда _index буфера экрана
   *
   * @param _index индекс разряда (0..3)
   * @return byte
   */
  byte getDispData(byte _index)
  {
    return ((_index < 6) ? data[_index] : 0);
  }

  /**
   * @brief отрисовка на экране содержимого его буфера
   *
   */
  void show()
  {
    bool flag = false;
    static byte _data[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    for (byte i = 0; i < 6; i++)
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
      for (byte i = 0; i < 6; i++)
      {
        _data[i] = data[i];
      }
      setSegments(data);
    }
  }

  /**
   * @brief вывод на экран  времени; если задать какое-то из значений hour или minute отрицательным, эта часть экрана будет очищена - можно организовать мигание, например, в процессе настройки времени
   *
   * @param hour часы
   * @param minute минуты
   * @param second секунды
   * @param show_colon отображать или нет двоеточие между часами и минутами
   * @param date флаг, показывающий, что выводится дата, а не время
   */
  void showTime(int8_t hour, int8_t minute, uint8_t second, bool show_colon, bool date = false)
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
    data[4] = (date) ? (byte)show_colon + 3 : show_colon;

    // формирование секундного столбца
    if (show_sec_col)
    {
      byte col_sec = 0;
      byte x = second / 5;
      for (byte i = 0; i < x; i++)
      {
        if (i < 6)
        { // нарастание снизу вверх
          col_sec += 1;
          col_sec = col_sec << 1;
        }
        else
        { // убывание снизу вверх
          col_sec = col_sec << 1;
          col_sec &= ~(1 << 7);
        }
      }
      data[5] = col_sec;
    }
  }

  /**
   * @brief вывод на экран температуры в диапазоне от -99 до +99 градусов; вне диапазона выводится строка минусов
   *
   * @param temp данные для вывода
   */
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

  /**
   * @brief установка яркости экрана
   *
   * @param brightness значение яркости (0..15)
   */
  void setBrightness(byte brightness)
  {
    brightness = (brightness <= 15) ? brightness : 15;
    for (byte i = 0; i < 4; i++)
    {
      shMAX72xxMini<cs_pin, 4>::setBrightness(i, brightness);
    }
  }
};
