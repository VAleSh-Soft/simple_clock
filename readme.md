# simple_clock v2.1
Простые часы на модуле реального времени (RTC) **DS3231**. Календаря нет. В базовом варианте (без будильника) помещается в **ATmega8**/**ATmega88** (возможно, без загрузчика), но проверить работу на этих контроллерах возможности не было.

В часах могут быть использованы либо семисегментый индикатор на базе драйвера **TM1637**, либо экраны на базе драверов **MAX7219** или **MAX7221**, как в виде семисегментного индикатора на четыре цифры, так и светодиодных матриц 8х8 светодиодов (четыре модуля). Для выбора требуемого экрана нужно раскомментировать одну из нижеследующих строк (закомментировав остальные) в файле **header_file.h**.
- `#define TM1637_DISPLAY`
- `#define MAX72XX_7SEGMENT_DISPLAY`
- `#define MAX72XX_MATRIX_DISPLAY`

Часы управляются тремя кнопками: **Set** - вход в режим настроек и сохранение изменений; **Up** - увеличение текущих значений; **Down** - уменьшение текущих значений.

Вход в настройки текущего времени выполняется длинным кликом кнопкой **Set**.

## Дополнительные возможности

### Будильник

Для использования будильника нужно раскомментировать строку `#define USE_ALARM` в файле **header_file.h**.

Состояние будильника показывает светодиод - если будильник включен, светодиод светится, если будильник сработал, светодиод мигает.

Сигнал сработавшего будильника отключается кликом любой кнопки.

В режим настройки будильника по умолчанию можно перейти по двойному клику кнопкой **Set**. Или можно настроить переход в этот режим по одиночному клику кнопкой **Set**, для этого нужно раскомментировать строку `#define USE_ONECLICK_TO_SET_ALARM` в файле **header_file.h**. Включение/выключение будильника выполняется кнопками **Up** или **Down**. После включения будильника следующий клик кнопкой **Set** переводит в режим настройки времени его срабатывания.

### Автоматическое управление яркостью экрана

Предусмотрена возможность снижения яркости экрана при низком освещении по данным датчика света - фоторезистора типа **GL5528**, подключенного к пину **A3**. Для этого нужно раскомментировать строку `#define USE_LIGHT_SENSOR` в файле **header_file.h**.

Схема подключения датчика:

![scheme0001](/docs/0001.jpg "Схема подключения датчика")

Если использование датчика света не предполагается, индикатор всегда будет работать с максимальной (т.е. заданной в строке `#define MAX_DISPLAY_BRIGHTNESS 7`) яркостью. 

### Вывод на экран текущей температуры

Для вывода на экран текущей температуры нужно раскомментировать строку `#define USE_TEMP_DATA` в файле **header_file.h**. В этом случае в режиме отображения текущего времени клик кнопкой **Up** будет выводить на несколько секунд на экран температуру.

Температура по умолчанию берется из внутреннего датчика микросхемы **DS3231**, однако есть возможность использования внешних датчиков.

 - Датчик **DS18b20**. Для этого нужно раскомментировать строку `#define USE_DS18B20` в файле **header_file.h**.

Схема подключения датчика DS18b20:

![scheme0002](/docs/0002.jpg "Схема подключения датчика DS18b20")

Кроме датчика **DS18b20** можно использовать датчики **DS18s20**, **DS1820**, **DS1822**.

- NTC термистор. Для этого нужно раскомментировать строку `#define USE_NTC` в файле **header_file.h**. 


Схема подключения датчика:

![scheme0003](/docs/0003.jpg "Схема подключения датчика")

Для использования термистора нужно знать его сопротивление при комнатной (25 градусов Цельсия) температуре и точное сопротивление резистора R3, и то, и другое - в Омах. Также нужно знать бета-коэффициент термистора - он отражает изменение сопротивления термистора при изменении температуры; значение коэффициента можно узнать у производителя датчика или рассчитать (см. формулу расчета температуры в файле **ntc.h**)

### Подключение модулей

Часы построены с использованием модуля **DS3231** и **Arduino Pro Mini** на базе **ATmega168p**. В качестве экрана используются либо семисегментные индикаторы на драйверах **TM1637** или **MAX7219/MAX7221**, либо светодиодная матрица 8х8х4 на драйверах **MAX7219/MAX7221**. В качестве источника звука использован пассивный пьезоэлектрический излучатель.

Пины для подключения экранов, RTC, кнопок, пищалки, светодиода и датчиков температуры и света определены в файле **header_file.h**

### Использованные сторонние библиотеки

**shButton.h** - https://github.com/VAleSh-Soft/shButton <br>
**shTaskManager.h** - https://github.com/VAleSh-Soft/shTaskManager <br>

Для работы с модулем **DS3231** используется библиотека<br>
**DS3231.h** - https://github.com/NorthernWidget/DS3231 <br>

Для работы с экранами используются библиотеки<br>
**TM1637Display.h** - https://github.com/avishorp/TM1637 <br>
или<br>
**shMAX72xxMini.h** - https://github.com/VAleSh-Soft/shMAX72xxMini <br>

для работы с датчиком **DS18b20** используется библиотека<br>
**OneWire.h** - https://github.com/PaulStoffregen/OneWire


Если возникнут вопросы, пишите на valesh-soft@yandex.ru 
