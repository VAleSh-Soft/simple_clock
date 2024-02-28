#pragma once
#include "matrix_data.h"
#include <Arduino.h>
#include <avr/pgmspace.h>
#include <DS3231.h>        // https://github.com/NorthernWidget/DS3231
#include <shMAX72xxMini.h> // https://github.com/VAleSh-Soft/shMAX72xxMini

// ==== класс для 7-сегментного индикатора MAX72xx ===

#define NUM_DIGITS 8

template <uint8_t cs_pin>
class DisplayMAX72xx7segment : public shMAX72xx7Segment<cs_pin, 1, NUM_DIGITS>
{
private:
  uint8_t data[4];

  void setSegments(uint8_t *data)
  {
    shMAX72xx7Segment<cs_pin, 1, NUM_DIGITS>::clearAllDevices();
    for (uint8_t i = 0; i < 4; i++)
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
    for (uint8_t i = 0; i < 4; i++)
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
  void setDispData(uint8_t _index, uint8_t _data)
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
   * @return uint8_t
   */
  uint8_t getDispData(uint8_t _index)
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
    static uint8_t _data[4] = {0x00, 0x00, 0x00, 0x00};
    for (uint8_t i = 0; i < 4; i++)
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
      for (uint8_t i = 0; i < 4; i++)
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
      for (uint8_t i = 0; i < 4; i++)
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
   * @brief вывод на экран даты
   *
   * @param date текущая дата
   * @param upd сбросить параметры и запустить заново
   * @return true если вывод завершен
   */
  bool showDate(DateTime date, bool upd = false)
  {
    static uint8_t n = 0;
    bool result = false;

    if (upd)
    {
      n = 0;
      return (result);
    }

    clear();

    switch (n)
    {
    case 0:
      showTime(date.day(), date.month(), true);
      break;
    case 1:
      showTime(20, date.year() % 100, false);
      break;
    }

    result = (n++ >= 2);

    return (result);
  }

  /**
   * @brief вывод на экран данных по настройке яркости экрана
   *
   * @param br величина яркости
   * @param blink используется для мигания изменяемого значения
   * @param toSensor используется или нет датчик освещенности
   * @param toMin если true, то настраивается минимальный уровень яркости, иначе - максимальный
   */
  void showBrightnessData(uint8_t br, bool blink, bool toSensor = false, bool toMin = false)
  {
    clear();
    data[0] = 0b00011111;
    if (toSensor)
    {
      data[1] = (toMin) ? encodeDigit(0) : encodeDigit(1);
    }
    else
    {
      data[1] = 0b00000101;
    }
    data[1] |= 0x80; // для показа двоеточия установить старший бит во второй цифре
    if (!blink)
    {
      data[2] = encodeDigit(br / 10);
      data[3] = encodeDigit(br % 10);
    }
  }

  /**
   * @brief установка яркости экрана
   *
   * @param brightness значение яркости (1..7)
   */
  void setBrightness(uint8_t brightness)
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
  void setNumString(uint8_t offset, uint8_t num,
                    uint8_t width = 6, uint8_t space = 1,
                    uint8_t *_data = NULL, uint8_t _data_count = 0)
  {
    setChar(offset, num / 10, width, _data, _data_count);
    setChar(offset + width + space, num % 10, width, _data, _data_count);
  }

  void setDayOfWeakString(uint8_t offset, DateTime date, uint8_t *_data = NULL, uint8_t _data_count = 0)
  {
    uint8_t dow = getDayOfWeek(date.day(), date.month(), date.year());
    for (uint8_t j = 0; j < 3; j++)
    {
      setChar(offset + j * 7,
              pgm_read_byte(&day_of_week[dow * 3 + j]), 5, _data, _data_count);
    }
  }

  void setTempString(uint8_t offset, int16_t temp, uint8_t *_data = NULL, uint8_t _data_count = 0)
  {
    // если температура выходит за диапазон, сформировать строку минусов
    if (temp > 99 || temp < -99)
    {
      for (uint8_t i = 0; i < 4; i++)
      {
        setChar(offset + 2 + i * 7, 0x2D, 5, _data, _data_count);
      }
    }
    else
    {
      bool plus = temp > 0;
      uint8_t plus_pos = offset + 6;
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
  uint8_t getOffset(uint8_t index)
  {
    static const uint8_t PROGMEM offset[] = {1, 48, 82, 167};

    return ((index < 9) ? pgm_read_byte(&offset[index]) : 0);
  }

  void getDateString(uint8_t *_data, uint8_t _data_count, DateTime date)
  {
    for (uint8_t i = 0; i < _data_count; i++)
    {
      _data[i] = 0x00;
    }

    uint8_t offset = getOffset(0);
    // формирование строки времени
    setNumString(offset, date.hour(), 6, 1, _data, _data_count);
    _data[offset + 14] = 0x24; // двоеточие
    setNumString(offset + 16, date.minute(), 6, 1, _data, _data_count);

    // формирование строки дня недели
    offset = getOffset(1);
    setDayOfWeakString(offset, date, _data, _data_count);

    // формирование строки даты
    offset = getOffset(2);
    setNumString(offset, date.day(), 6, 2, _data, _data_count);
    _data[offset + 15] = 0x01; // точка
    setNumString(offset + 18, date.month(), 6, 2, _data, _data_count);
    _data[offset + 33] = 0x01; // точка
    setNumString(offset + 36, 20, 6, 2, _data, _data_count);
    setNumString(offset + 52, date.year() % 100, 6, 2, _data, _data_count);

    // формирование строки времени
    offset = getOffset(3);
    setNumString(offset, date.hour(), 6, 1, _data, _data_count);
    _data[offset + 14] = 0x24; // двоеточие
    setNumString(offset + 16, date.minute(), 6, 1, _data, _data_count);
  }
#endif

  void setChar(uint8_t offset, uint8_t chr,
               uint8_t width = 6, uint8_t *_arr = NULL, uint8_t _arr_length = 0)
  {
    for (uint8_t j = offset, i = 0; i < width; j++, i++)
    {
      uint8_t chr_data = 0;
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
          shMAX72xxMini<cs_pin, 4>::setColumn(j / 8, j % 8, chr_data);
        }
      }
    }
  }

public:
  DisplayMAX72xxMatrix() : shMAX72xxMini<cs_pin, 4>() { clear(); }

  /**
   * @brief очистка экрана
   *
   * @param upd при false очищается только буфер экрана, при true - очищается и сам экран
   */
  void clear(bool upd = false)
  {
    shMAX72xxMini<cs_pin, 4>::clearAllDevices(upd);
  }

  /**
   * @brief запись символа в буфера экрана
   *
   * @param offset индекс столбца, с которого начинается отрисовка символа (0..31)
   * @param chr символ для записи
   * @param width ширина символа, может иметь значение 5 или 6, определяет, какой набор символов будет использован: 5х7 (для текста) или 6х8 (для вывода цифр)
   */
  void setDispData(uint8_t offset, uint8_t chr, uint8_t width = 6)
  {
    setChar(offset, chr, width);
  }

  /**
   * @brief вывести двоеточие в середине экрана
   *
   * @param toDot вместо двоеточия вывести точку
   */
  void setColon(bool toDot = false)
  {
    (toDot) ? shMAX72xxMini<cs_pin, 4>::setColumn(1, 7, 0b00000001)
            : shMAX72xxMini<cs_pin, 4>::setColumn(1, 7, 0b00100100);
  }

  /**
   * @brief отрисовка на экране содержимого его буфера
   *
   */
  void show()
  {
    shMAX72xxMini<cs_pin, 4>::update();
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
    uint8_t col_sec = 0;
    uint8_t x = second / 5;
    for (uint8_t i = 0; i < x; i++)
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
    static uint8_t n = 0;
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
    shMAX72xxMini<cs_pin, 4>::clearAllDevices();

// бегущая строка
#ifdef USE_TICKER_FOR_DATE
    uint8_t str_len = 200;
    uint8_t date_str[str_len];
    getDateString(date_str, str_len, date);

    for (uint8_t i = 32, j = n; i > 0 && j > 0; i--, j--)
    {
      shMAX72xxMini<cs_pin, 4>::setColumn((i - 1) / 8, (i - 1) % 8, date_str[j - 1]);
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

    shMAX72xxMini<cs_pin, 4>::update();

#ifdef USE_TICKER_FOR_DATE
    result = (n++ >= str_len - 2);
#else
    result = (n++ >= 3);
#endif

    return (result);
  }

  /**
   * @brief вывод на экран данных по настройке яркости экрана
   *
   * @param br величина яркости
   * @param blink используется для мигания изменяемого значения
   * @param toSensor используется или нет датчик освещенности
   * @param toMin если true, то настраивается минимальный уровень яркости, иначе - максимальный
   */
  void showBrightnessData(uint8_t br, bool blink, bool toSensor = false, bool toMin = false)
  {
    shMAX72xxMini<cs_pin, 4>::clearAllDevices();

#ifdef USE_RU_LANGUAGE
    setChar(0, 0xDF, 5); // Я
    setChar(6, 0xF0, 5); // р
    uint8_t x = 0xEA;       // к
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
      uint8_t x = (toMin) ? 0 : 1;
      x += 0x30;
      setChar(12, x, 5);
    }
#endif
    shMAX72xxMini<cs_pin, 4>::setColumn(2, 2, 0b00100100);
    if (!blink)
    {
      setChar(20, br / 10 + 0x30, 5);
      setChar(26, br % 10 + 0x30, 5);
    }
  }

  /**
   * @brief установка яркости экрана
   *
   * @param brightness значение яркости (0..15)
   */
  void setBrightness(uint8_t brightness)
  {
    brightness = (brightness <= 15) ? brightness : 15;
    for (uint8_t i = 0; i < 4; i++)
    {
      shMAX72xxMini<cs_pin, 4>::setBrightness(i, brightness);
    }
  }
};
