#pragma once
#include "matrix_data.h"
#include <Arduino.h>
#include <avr/pgmspace.h>
#include <FastLED.h> // https://github.com/FastLED/FastLED

// ==== класс для матрицы 8х32 адресных светодиодов ==

/**
 * @brief тип матрицы по расположению светодиодов
 */
enum MatrixType : uint8_t
{
  /**
   * @brief светодиоды расположены по столбцам:
   *    _   _
   * | | | | |
   * |_| |_| |
   *
   */
  BY_COLUMNS,
  /**
   * @brief светодиоды расположены построчно:
   *  ________
   *  ________|
   * |________
   */
  BY_LINE
};

class DisplayWS2812Matrix
{
private:
  byte data[6];
  CRGB *leds = NULL;
  byte seg_offset[4] = {0, 8, 16, 24};
  MatrixType matrix_type = BY_COLUMNS;
  byte row_count = 8;
  byte col_count = 32;
  CRGB color = CRGB::Red;
  bool show_sec_col = false;

  byte getLedIndexOfStrip(byte row, byte col)
  {
    switch (matrix_type)
    {
    case BY_COLUMNS:
      return (col * row_count + (((col >> 0) & 0x01) ? row_count - row - 1 : row));
    case BY_LINE:
      return (row * col_count + (((row >> 0) & 0x01) ? col_count - col - 1 : col));
    }
  }

  void setColumn(byte col, byte data)
  {
    for (byte i = 0; i < 8; i++)
    {
      leds[getLedIndexOfStrip(i, col)] =
          (((data) >> (7 - i)) & 0x01) ? color : CRGB::Black;
    }
  }

  void setSegments()
  {
    for (uint16_t i = 0; i < 256; i++)
    {
      leds[i] = CRGB::Black;
    }

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
        setColumn(seg_offset[i] + j + offset,
                  pgm_read_byte(&font_digit[data[i] * 6 + j]));
      }
    }
    switch (data[4])
    {
    case 1: // отрисовка двоеточия
      setColumn(seg_offset[1] + 7, 0b00100100);
      break;
    case 2: // дорисовка значка градуса
      setColumn(seg_offset[2] + 7, 0b01000000);
      break;
    case 4: // отрисовка точки в дате
      setColumn(seg_offset[1] + 7, 0b00000001);
      break;
    }
    if (show_sec_col)
    {
      setColumn(31, data[5]);
    }
  }

public:
  /**
   * @brief конструктор
   *
   * @param _leds массив светодиодов
   * @param _color цвет
   * @param _type тип матрицы, собрана по столбцам или построчно
   */
  DisplayWS2812Matrix(CRGB *_leds, CRGB _color, MatrixType _type)
  {
    leds = _leds;
    color = _color;
    matrix_type = _type;
    sleep();
  }

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
    setSegments();
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
  bool show()
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
      setSegments();
    }

    return (flag);
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
};
