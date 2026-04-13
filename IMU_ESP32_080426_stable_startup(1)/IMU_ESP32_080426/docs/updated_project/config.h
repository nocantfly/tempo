#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// PINOUT (CŨ - CHẠY ĐƯỢC)
#define I2C_SDA  15
#define I2C_SCL  14
#define LCD_CS   12  
#define LCD_SCK  11  
#define LCD_D0   4   
#define LCD_D1   5
#define LCD_D2   6
#define LCD_D3   7
#define LCD_RST  8   
#define BTN_BOOT 0  

#define LCD_WIDTH  368
#define LCD_HEIGHT 448

// MÀU SẮC
#define BLACK   0x0000
#define WHITE   0xFFFF
#define GREEN   0x07E0
#define BLUE    0x001F
#define RED     0xF800
#define YELLOW  0xFFE0
#define GREY    0x2104

#endif