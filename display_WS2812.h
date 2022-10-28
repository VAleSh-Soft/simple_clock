#pragma once
#include "matrix_data.h"
#include <Arduino.h>
#include <avr/pgmspace.h>
#include <FastLED.h> // https://github.com/FastLED/FastLED
#include <DS3231.h>  // https://github.com/NorthernWidget/DS3231

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
  CRGB *leds = NULL;
  MatrixType matrix_type = BY_COLUMNS;
  byte row_count = 8;
  byte col_count = 32;
  CRGB color = CRGB::Red;

  byte getLedIndexOfStrip(byte row, byte col)
  {
    byte result = 0;
    switch (matrix_type)
    {
    case BY_COLUMNS:
      result = col * row_count + (((col >> 0) & 0x01) ? row_count - row - 1 : row);
      break;
    case BY_LINE:
      result = row * col_count + (((row >> 0) & 0x01) ? col_count - col - 1 : col);
      break;
    }
    return (result);
  }

  void setNumString(byte offset, byte num,
                    byte width = 6, byte space = 1,
                    byte *_data = NULL, byte _data_count = 0)
  {
    setChar(offset, num / 10, width, _data, _data_count);
    setChar(offset + width + space, num % 10, width, _data, _data_count);
  }

  void setDayOfWeakString(byte offset, DateTime date, byte *_data = NULL, byte _data_count = 0)
  {
    uint8_t dow = getDayOfWeek(date.day(), date.month(), date.year());
    for (byte j = 0; j < 3; j++)
    {
      setChar(offset + j * 7,
              pgm_read_byte(&day_of_week[dow * 3 + j]), 5, _data, _data_count);
    }
  }

  void setTempString(byte offset, int16_t temp, byte *_data = NULL, byte _data_count = 0)
  {
    // если температура выходит за диапазон, сформировать строку минусов
    if (temp > 99 || temp < -99)
    {
      for (byte i = 0; i < 4; i++)
      {
        setChar(offset + 2 + i * 7, 0x2D, 5, _data, _data_count);
      }
    }
    else
    {
      bool plus = temp > 0;
      byte plus_pos = offset + 6;
      if (temp < 0)
      {
        temp = -temp;
      }
      setChar(offset + 13, temp % 10, 6, _data, _data_count);
      if (temp > 9)
      {
        // если температура двухзначная, переместить знак на позицию левее
        plus_pos = offset;
        setChar(offset + 6, temp / 10, 6, _data, _data_count);
      }
      // сформировать впереди плюс или минус
      if (temp != 0)
      {
        (plus) ? setChar(plus_pos, 0x2B, 5, _data, _data_count)
               : setChar(plus_pos, 0x2D, 5, _data, _data_count);
      }
      // сформировать в конце знак градуса Цельсия
      setChar(offset + 20, 0xB0, 5, _data, _data_count);
      setChar(offset + 25, 0x43, 5, _data, _data_count);
    }
  }

#ifdef USE_TICKER_FOR_DATE
  byte getOffset(byte index)
  {
    static const byte PROGMEM offset[] = {1, 17, 48, 72, 90, 108, 124, 157, 173};

    return ((index < 9) ? pgm_read_byte(&offset[index]) : 0);
  }

  void getDateString(byte *_data, byte _data_count, DateTime date)
  {
    for (byte i = 0; i < _data_count; i++)
    {
      _data[i] = 0x00;
    }

    // формирование строки времени
    setNumString(getOffset(0), date.hour(), 6, 1, _data, _data_count);
    _data[getOffset(0) + 14] = 0x24; // двоеточие
    setNumString(getOffset(1), date.minute(), 6, 1, _data, _data_count);

    // формирование строки дня недели
    setDayOfWeakString(getOffset(2), date, _data, _data_count);

    // формирование строки даты
    setNumString(getOffset(3), date.day(), 6, 2, _data, _data_count);
    _data[getOffset(3) + 15] = 0x01; // точка
    setNumString(getOffset(4), date.month(), 6, 2, _data, _data_count);
    _data[getOffset(4) + 15] = 0x01; // точка
    setNumString(getOffset(5), 20, 6, 2, _data, _data_count);
    setNumString(getOffset(6), date.year() % 100, 6, 2, _data, _data_count);

    // формирование строки времени
    setNumString(getOffset(7), date.hour(), 6, 1, _data, _data_count);
    _data[getOffset(7) + 14] = 0x24; // двоеточие
    setNumString(getOffset(8), date.minute(), 6, 1, _data, _data_count);
  }
#endif

  void setChar(byte offset, byte chr,
               byte width = 6, byte *_arr = NULL, byte _arr_length = 0)
  {
    for (byte j = offset, i = 0; i < width; j++, i++)
    {
      byte chr_data = 0;
      switch (width)
      {
      case 5:
        chr_data = reverseByte(pgm_read_byte(&font_5_7[chr * width + i]));
        break;
      case 6:
        chr_data = pgm_read_byte(&font_digit[chr * width + i]);
        break;
      default:
        break;
      }

      if (_arr != NULL)
      {
        if (j < _arr_length)
        {
          _arr[j] = chr_data;
        }
      }
      else
      {
        if (j < 32)
        {
          setColumn(j, chr_data);
        }
      }
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
    clear(true);
  }

  /**
   * @brief запись столбца в буфер экрана
   *
   * @param col столбец
   * @param _data байт для записи
   */
  void setColumn(byte col, byte _data)
  {
    if (col < 32)
    {
      for (byte i = 0; i < 8; i++)
      {
        leds[getLedIndexOfStrip(i, col)] =
            (((_data) >> (7 - i)) & 0x01) ? color : CRGB::Black;
      }
    }
  }

  /**
   * @brief очистка буфера экрана
   *
   * @param upd при false очищается только буфер экрана, при true - очищается и сам экран
   */
  void clear(bool upd = false)
  {
    for (byte i = 0; i < 32; i++)
    {
      setColumn(i, 0x00);
    }
    if (upd)
    {
      FastLED.show();
    }
  }

  /**
   * @brief запись символа в буфера экрана
   *
   * @param offset индекс столбца, с которого начинается отрисовка символа (0..31)
   * @param chr символ для записи
   * @param width ширина символа, может иметь значение 5 или 6, определяет, какой набор символов будет использован: 5х7 (для текста) или 6х8 (для вывода цифр)
   */
  void setDispData(byte offset, byte chr, byte width = 6)
  {
    if (offset < 32)
    {
      setChar(offset, chr, width);
    }
  }

  /**
   * @brief вывести двоеточие в середине экрана
   *
   * @param toDot вместо двоеточия вывести точку
   */
  void setColon(bool toDot = false)
  {
    (toDot) ? setColumn(15, 0b00000001) : setColumn(15, 0b00100100);
  }

  /**
   * @brief отрисовка на экране содержимого его буфера
   *
   */
  void show()
  {
    FastLED.show();
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
      setNumString(1, hour, 6, 1);
    }
    if (minute >= 0)
    {
      setNumString(17, minute, 6, 1);
    }
    if (show_colon)
    {
      setColon(date);
    }

#ifdef SHOW_SECOND_COLUMN
    // формирование секундного столбца
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
    setColumn(31, col_sec);
#endif
  }

  /**
   * @brief вывод на экран температуры в диапазоне от -99 до +99 градусов; вне диапазона выводится строка минусов
   *
   * @param temp данные для вывода
   */
  void showTemp(int temp)
  {
    clear();
    setTempString(1, temp);
  }

  /**
   * @brief вывод на экран даты
   *
   * @param date текущая дата
   * @param upd сбросить параметры и запустить заново
   * @return true если вывод завершен
   */
  bool showDate(DateTime date, bool upd = false)
  {
    static byte n = 0;
    bool result = false;

    if (upd)
    {
#ifdef USE_TICKER_FOR_DATE
      n = 32;
#else
      n = 0;
#endif
      return (result);
    }
    clear();

// бегущая строка
#ifdef USE_TICKER_FOR_DATE
    byte date_str[190];
    getDateString(date_str, 190, date);

    for (byte i = 32, j = n; i > 0, j > 0; i--, j--)
    {
      setColumn(i, date_str[j - 1]);
    }
// последовательный вывод - день недели, число и месяц, год
#else
    switch (n)
    {
    case 0:
      setDayOfWeakString(7, date);
      break;
    case 1:
      setNumString(1, date.day());
      setColon(true); // точка
      setNumString(17, date.month());
      break;
    case 2:
      setNumString(1, 20, 6, 2);
      setNumString(17, date.year() % 100, 6, 2);
      break;
    }
#endif

    FastLED.show();

#ifdef USE_TICKER_FOR_DATE
    result = (n++ >= 188);
#else
    result = (n++ >= 3);
#endif

    return (result);
  }

  /**
   * @brief вывод на экран данных по настройке яркости экрана
   *
   * @param br величина яркости
   * @param toSensor используется или нет датчик освещенности
   * @param toMin если true, то настраивается минимальный уровень яркости, иначе - максимальный
   */
  void showBrightnessData(byte br, bool toSensor = false, bool toMin = false)
  {
    clear();

#ifdef USE_RU_LANGUAGE
    setChar(0, 0xDF, 5); // Я
    setChar(6, 0xF0, 5); // р
    byte x = 0xEA;       // к
    if (toSensor)
    {
      x = (toMin) ? 0 : 1;
      x += 0x30;
    }
    setChar(12, x, 5);
#else
    setChar(0, 0x42, 5); // B
    setChar(6, 0x72, 5); // r
    if (toSensor)
    {
      byte x = (toMin) ? 0 : 1;
      x += 0x30;
      setChar(12, x, 5);
    }
#endif
    setColumn(18, 0b00100100);
    setChar(20, br / 10 + 0x30, 5);
    setChar(26, br % 10 + 0x30, 5);
  }
};
