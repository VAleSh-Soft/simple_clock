# simple_clock v4.4
Простые модульные часы на модуле реального времени (RTC) **DS3231**. В базовом варианте (на семисегментном экране, без календаря, будильника и датчиков освещения и температуры) помещается в **ATmega8**/**ATmega88** (возможно, без загрузчика), но проверить работу на этих контроллерах возможности не было.

Модульность заключается в возможности выбора на этапе компиляции как используемого для вывода времени экрана, так и дополнительных опций, как то: будильник, календарь, регулировка яркости экрана по датчику освещенности и вывод температуры, в том числе с использованием внешних датчиков разных типов.

- [Используемые экраны](#используемые-экраны)
- [Управление](#управление)
- [Дополнительные возможности](#дополнительные-возможности)
  - [Будильник](#будильник)
  - [Календарь](#календарь)
  - [Отображение секундного столбца](#отображение-секундного-столбца)
  - [Автоматическое управление яркостью экрана](#автоматическое-управление-яркостью-экрана)
  - [Регулировка уровней минимального и максимального уровней яркости экрана](#регулировка-уровней-минимального-и-максимального-уровней-яркости-экрана)
  - [Вывод на экран текущей температуры](#вывод-на-экран-текущей-температуры)
    - [Внешние датчики температуры](#внешние-датчики-температуры)
  - [Кириллица](#кириллица)
- [Подключение модулей](#подключение-модулей)
- [Использованные сторонние библиотеки](#использованные-сторонние-библиотеки)

## Используемые экраны

В часах могут быть использованы либо семисегментый индикатор на базе драйвера **TM1637**, либо экраны на базе драверов **MAX7219** или **MAX7221**, как в виде семисегментного индикатора на четыре цифры, так и светодиодных матриц 8х8 светодиодов (четыре модуля), либо матрица 8х32, составленная из адресных светодиодов. Для выбора требуемого экрана нужно раскомментировать одну из нижеследующих строк (закомментировав остальные) в файле **header_file.h**.
- `#define TM1637_DISPLAY`
- `#define MAX72XX_7SEGMENT_DISPLAY`
- `#define MAX72XX_MATRIX_DISPLAY`
- `#define WS2812_MATRIX_DISPLAY`

Для матриц на основе адресных светодиодов поддерживаются все виды чипов, поддерживаемых библиотекой **FastLED**, нужный тип выбирается в `setup()`; матрица может быть построена как построчно (`BY_LINE`), так и по столбцам (`BY_COLUMN`).

## Управление

Часы управляются тремя кнопками: **Set** - вход в режим настроек и сохранение изменений; **Up** - увеличение текущих значений; **Down** - уменьшение текущих значений.

Вход в настройки текущего времени выполняется длинным кликом кнопкой **Set**.

## Дополнительные возможности

### Будильник

Для использования будильника нужно раскомментировать строку `#define USE_ALARM` в файле **header_file.h**.

Состояние будильника показывает светодиод - если будильник включен, светодиод светится, если будильник сработал, светодиод мигает.

Сигнал сработавшего будильника отключается кликом любой кнопки.

В режим настройки будильника по умолчанию можно перейти по двойному клику кнопкой **Set**. Или можно настроить переход в этот режим по одиночному клику кнопкой **Set**, для этого нужно раскомментировать строку `#define USE_ONECLICK_TO_SET_ALARM` в файле **header_file.h**. Включение/выключение будильника выполняется кнопками **Up** или **Down**. После включения будильника следующий клик кнопкой **Set** переводит в режим настройки времени его срабатывания.

### Календарь

Для использования календаря нужно раскомментировать строку `#define USE_CALENDAR` в файле **header_file.h**. В этом случае в режиме отображения времени клик кнопкой **Down** будет выводить на экран текущую дату: для семисегментных индикаторов последовательно по две секунды: день и месяц, год, для матричных индикаторов добавляется текстовый вывод дня недели.

Для матричных экранов доступен вывод даты в виде бегущей строки, для этого нужно раскомментировать строку `#define USE_TICKER_FOR_DATE` в файле **matrix_data.h**

В режим настройки даты часы переходят по короткому клику кнопкой **Set** из режима настройки минут или по длинному клику кнопкой **Set** из режима отбражения календаря.

### Отображение секундного столбца

Для матричных экранов доступна возможность отображения в крайнем правом столбце экрана столбца светодиодов, отображающих количество текущих секунд в минуте. Первые полминуты каждые пять секунд световой столбец увеличивается на один светодиод снизу вверх, со второй половины минуты световой столбец каждые пять секунд уменьшается на один светодиод снизу вверх.

Для использования этой возможности нужно раскомментировать строку `#define SHOW_SECOND_COLUMN` в файле **header_file.h**.

### Автоматическое управление яркостью экрана

Предусмотрена возможность снижения яркости экрана при низком освещении по данным датчика света - фоторезистора типа **GL5528**, подключенного к пину **A3**. Для этого нужно раскомментировать строку `#define USE_LIGHT_SENSOR` в файле **header_file.h**.

Схема подключения датчика:

![scheme0001](/docs/0001.jpg "Схема подключения датчика")

Если использование датчика света не предполагается, индикатор всегда будет работать с максимальной яркостью.

### Регулировка уровней минимального и максимального уровней яркости экрана

Для того, чтобы иметь возможность регулировать яркость экрана, нужно раскомментировать строку `#define USE_SET_BRIGHTNESS_MODE` в файле **header_file.h**. В этом случае по удержанию одновременно нажатыми кнопок **Up** и **Down** часы будут переходить в режим настройки яркости. При использовании датчика света можно будет настраивать как минимальный, так и максимальный уровни, иначе можно будет настроить только максимальный уровень яркости экрана.

Кнопка **Set** в этом режиме сохраняет введенные данные и переключает режимы, кнопками **Up** и **Down** настраивается желаемый уровень. Для экранов на основе драйвера **TM1637** яркость может иметь значение 1..7, для экранов на основе драйвера **MAX72xx** - 0..15, для матриц на основе адресных светодиодов - 1..25.

 Настройки будут сохранены в EEPROM.

### Вывод на экран текущей температуры

Для вывода на экран текущей температуры нужно раскомментировать строку `#define USE_TEMP_DATA` в файле **header_file.h**. В этом случае в режиме отображения текущего времени клик кнопкой **Up** будет выводить на экран температуру на несколько секунд.

#### Внешние датчики температуры
<hr>

Температура по умолчанию берется из внутреннего датчика микросхемы **DS3231**, однако есть возможность использования внешних датчиков.

1. Датчик **DS18b20**. Для этого нужно раскомментировать строку `#define USE_DS18B20` в файле **header_file.h**.

Схема подключения датчика DS18b20:

![scheme0002](/docs/0002.jpg "Схема подключения датчика DS18b20")

Кроме датчика **DS18b20** можно использовать датчики **DS18s20**, **DS1820**, **DS1822**.

2. NTC термистор. Для этого нужно раскомментировать строку `#define USE_NTC` в файле **header_file.h**.

Схема подключения датчика:

![scheme0003](/docs/0003.jpg "Схема подключения датчика")

Для использования термистора нужно знать его сопротивление при комнатной (25 градусов Цельсия) температуре и точное сопротивление резистора R3 (и то, и другое - в Омах). Также нужно знать бета-коэффициент термистора - он отражает изменение сопротивления термистора при изменении температуры; значение коэффициента можно узнать у производителя датчика или рассчитать (см. описание в файле **ntc.h**)

### Кириллица

Для матричных экранов возможен вывод данных как на латинице, так и русскими буквами. Для использования кириллицы нужно раскомментировать строку `#define USE_RU_LANGUAGE` в файле **matrix_data.h**. В этом случае при выводе будут использоваться русские названия дней недели и русские буквенные обозначения в настройках.

## Подключение модулей

Часы построены с использованием RTC модуля **DS3231** и **Arduino Pro Mini** на базе **ATmega168p**. В качестве экрана используются либо семисегментные индикаторы на драйверах **TM1637** или **MAX7219/MAX7221**, либо светодиодная матрица 8х8х4 на драйверах **MAX7219/MAX7221**, либо светодиодная матрица 8х32, построенная на адресных светодиодах. В качестве источника звука будильника использован пассивный пьезоэлектрический излучатель.

Пины для подключения экранов, RTC, кнопок, пищалки, светодиода и датчиков температуры и света определены в файле **header_file.h**

## Использованные сторонние библиотеки

**shButton.h** - https://github.com/VAleSh-Soft/shButton<br>
**shTaskManager.h** - https://github.com/VAleSh-Soft/shTaskManager<br>

Для работы с модулем **DS3231** используется библиотека<br>
**DS3231.h** - https://github.com/NorthernWidget/DS3231<br>

Для работы с экранами используются библиотеки<br>
**TM1637Display.h** - https://github.com/avishorp/TM1637<br>
или<br>
**shMAX72xxMini.h** - https://github.com/VAleSh-Soft/shMAX72xxMini<br>
или <br>
**FastLED.h** - https://github.com/FastLED/FastLED<br>

для работы с датчиком **DS18b20** используется библиотека<br>
**OneWire.h** - https://github.com/PaulStoffregen/OneWire

Если возникнут вопросы, пишите на valesh-soft@yandex.ru 
