#include "VHITEK_Var.h"
#include "logo.h"

#define LCD_RESET 4  // RST on LCD
#define LCD_CS 2     // RS on LCD
#define LCD_CLOCK 19 // E on LCD
#define LCD_DATA 21  // R/W on LCD
#define U8_Width 128
#define U8_Height 64
namespace VhitekVending
{
#ifdef VHITEK_KEYPAD
    U8G2_ST7920_128X64_F_HW_SPI u8g2(U8G2_R2, /* CS=*/LCD_CS, /* reset=*/LCD_RESET);
#else
    U8G2_ST7920_128X64_F_HW_SPI u8g2(U8G2_R0, /* CS=*/LCD_CS, /* reset=*/LCD_RESET);
#endif

    namespace Display
    {

        bool disableWorking = false;
        bool viewProductName = false;
        void begin()
        {
            SPI.begin(LCD_CLOCK, -1, LCD_DATA, LCD_CS);
            u8g2.setBusClock(600000);
            u8g2.begin();
            u8g2.enableUTF8Print();
            u8g2.setFont(u8g2_font_lubBI14_tn);
            u8g2.setFontDirection(0);
            u8g2.clearBuffer();
            u8g2.drawXBM(0, 0, 128, 64, logo_vhitek_bits);
            u8g2.sendBuffer();
            delay(1000);
            Serial.println("Display Module Initialized");
        }
    }
}