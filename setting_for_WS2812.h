#pragma once

#include <FastLED.h>

// ===== настройки адресных светодиодов ==============

// здесь укажите используемый вами тип светодиодов; 
// возможные варианты: CHIPSET_LPD6803; CHIPSET_LPD8806; CHIPSET_WS2801; CHIPSET_WS2803; CHIPSET_SM16716; CHIPSET_P9813; CHIPSET_APA102; CHIPSET_SK9822; CHIPSET_DOTSTAR; CHIPSET_NEOPIXEL; CHIPSET_SM16703; CHIPSET_TM1829; CHIPSET_TM1812; CHIPSET_TM1809; CHIPSET_TM1804; CHIPSET_TM1803; CHIPSET_UCS1903; CHIPSET_UCS1903B; CHIPSET_UCS1904; CHIPSET_UCS2903; CHIPSET_WS2812; CHIPSET_WS2852; CHIPSET_WS2812B; CHIPSET_GS1903; CHIPSET_SK6812; CHIPSET_SK6822; CHIPSET_APA106; CHIPSET_PL9823; CHIPSET_WS2811; CHIPSET_WS2813; CHIPSET_APA104; CHIPSET_WS2811_400; CHIPSET_GE8822; CHIPSET_GW6205; CHIPSET_GW6205_400; CHIPSET_LPD1886; CHIPSET_LPD1886_8BIT
#define CHIPSET_WS2812B

// укажите порядок следования цветов в используемых вами светододах; 
// наиболее часто используются варианты RGB (красный, зеленый, синий)  GRB (зеленый, красный, синий)
#define EORDER GRB  

// использовать аппаратный SPI для управления светодиодами для чипов с четырехпроводным управлением
// #define USE_HARDWARE_SPI   

// пины для подключения матрицы 
#define DISPLAY_DIN_PIN 10 // пин для подключения экрана - DIN
#define DISPLAY_CLK_PIN 11 // пин для подключения экрана - CLK (для четырехпроводных схем)
