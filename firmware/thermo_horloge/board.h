// Carte Waveshare ESP32-C6-Touch-LCD-2.8 : brochage et pilotes bas niveau.
// Valeurs reprises du wiki et des exemples Waveshare (docs.waveshare.com/ESP32-C6-Touch-LCD-2.8).
#pragma once
#include <Arduino.h>
#include <Wire.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <time.h>

namespace pin {
  constexpr int I2C_SDA = 6, I2C_SCL = 7;          // bus commun : expandeur, tactile, SHTC3, RTC, IMU, codec
  constexpr int LCD_SCLK = 0, LCD_MOSI = 1, LCD_CS = 11, LCD_DC = 10;
  constexpr int SD_CS = 23;                         // carte SD sur le même bus SPI : à garder désélectionnée
  constexpr int TP_INT = 18;
  constexpr int BOOT = 9;                           // bouton BOOT, utilisable comme bouton
}

namespace addr {
  constexpr uint8_t EXIO = 0x24;    // CH32V003 : resets, rétroéclairage, mesure batterie
  constexpr uint8_t TOUCH = 0x58;   // CST3530
  constexpr uint8_t SHTC3 = 0x70;   // température / humidité
  constexpr uint8_t RTC = 0x51;     // PCF85063A
}

// Écran ST7789 240 x 320 en SPI. Le reset passe par l'expandeur, pas par une broche.
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_SPI _bus;
public:
  LGFX() {
    {
      auto cfg = _bus.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;   // Waveshare monte à 80 MHz ; 40 MHz d'abord, plus sûr
      cfg.freq_read = 16000000;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = pin::LCD_SCLK;
      cfg.pin_mosi = pin::LCD_MOSI;
      cfg.pin_miso = -1;
      cfg.pin_dc = pin::LCD_DC;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs = pin::LCD_CS;
      cfg.pin_rst = -1;
      cfg.pin_busy = -1;
      cfg.panel_width = 240;
      cfg.panel_height = 320;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.readable = false;
      cfg.invert = true;           // l'exemple Waveshare active l'inversion (INVON)
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = true;
      _panel.config(cfg);
    }
    setPanel(&_panel);
  }
};

namespace board {
  bool begin();                         // I2C, expandeur, reset écran et tactile
  void scanI2C();                       // liste les adresses présentes sur le port série
  void backlight(uint8_t percent);      // 0 à 100
  void amp(bool on);                    // amplificateur du haut-parleur
  bool readBattery(float& volts);

  bool readClimate(float& tempC, float& humidity);   // SHTC3, valeurs brutes (sans correction)

  bool rtcRead(struct tm& t);
  bool rtcWrite(const struct tm& t);

  bool touchRead(int16_t& x, int16_t& y);            // vrai si un doigt est posé
}
