/* Небольшая библиотека для работы с датчиками температуры - NTC термисторами;
   Работает с одним датчиком, выдает температуру в градусах Цельсия в формате int16_t.

   Методы библиотеки:

   NTCSensor temp_sensor(_sensor_pin, _resistor_std, _balance_resistor, _beta = 3950) - конструктора класса.
   Список аргументов:

    _sensor_pin - аналоговый пин, куда подключен датчик;
    _resistor_std - сопротивление датчика при комнатной температуре (25 градусов Цельсия) в Омах;
    _balance_resistor - сопротивление второго резистора делителя напряжения, в Омах;
    _beta - бета-коэффициент датчика, см. данные производителя; если данных производителя нет, коэффициент можно расчитать, исходя из бета-формулы расчета температуры, которую можно легко найти в интернете.

   int16_t getTemp() - получение температуры с датчика;

   void setADCbitDepth(bit_depth) - установка разрядности АЦП используемого микроконтроллера; для Ардуино UNO, Nano, Pro Mini bit_depth = 10; (это значение по умолчанию)
*/
#pragma once
#include <Arduino.h>

class NTCSensor
{
private:
  uint16_t balance;
  uint16_t resistor_room_temp;
  uint32_t max_adc = 1023;
  uint16_t beta;
  uint32_t room_temp = 29815;
  int16_t cur_temp;
  uint8_t sensor_pin;

public:
  NTCSensor(uint8_t _sensor_pin,
            uint16_t _resistor_std,
            uint16_t _balance_resistor,
            uint16_t _beta = 3950)
  {
    sensor_pin = _sensor_pin;
    balance = _balance_resistor;
    resistor_room_temp = _resistor_std;
    beta = _beta;
  }

  /**
   * @brief получение температуры с датчика
   *
   * @return uint16_t
   */
  uint16_t getTemp()
  {
    static uint16_t adcAverage = analogRead(sensor_pin);

    adcAverage = (adcAverage * 2 + analogRead(sensor_pin)) / 3;
    // определяем текущее сопротивление термистора
    uint16_t rThermistor = balance * ((max_adc * 100 / adcAverage) - 100) / 100;
    // получаем температуру в сотых долях градусов Кельвина
    uint16_t tKelvin = (beta * room_temp) /
                       ((beta + (room_temp * log((float)rThermistor / resistor_room_temp)) / 100));
    // преобразовать кельвины в цельсии (в сотых долях градусов)
    uint16_t temp = tKelvin - 27315;
    int16_t tCelsius = (temp % 100 < 50) ? temp / 100 : temp / 100 + 1;

    return (tCelsius);
  }

  /**
   * @brief установка разрядности АЦП используемого микроконтроллера; для Ардуино UNO, Nano, Pro Mini bit_depth = 10
   *
   * @param bit_depth разрядность, бит
   */
  void setADCbitDepth(uint8_t bit_depth)
  {
    uint32_t d = 1;
    for (uint8_t i = 0; i < bit_depth; i++)
    {
      d *= 2;
    }
    max_adc = d - 1;
  }
};
