#include "board.h"

namespace {
  // Expandeur CH32V003 (registres d'après Board_IO.h de Waveshare)
  constexpr uint8_t REG_MODE = 0x02, REG_OUTPUT = 0x03, REG_PWM = 0x05, REG_ADC = 0x06;
  constexpr uint8_t EXIO_TP_RST = 0, EXIO_LCD_RST = 1, EXIO_PA_CTRL = 3;
  uint8_t exioOut = 0, exioMode = 0;

  bool write8(uint8_t dev, uint8_t reg, uint8_t val) {
    Wire.beginTransmission(dev);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
  }

  bool readN(uint8_t dev, uint8_t reg, uint8_t* buf, size_t n) {
    Wire.beginTransmission(dev);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(dev, (uint8_t)n) != n) return false;
    for (size_t i = 0; i < n; i++) buf[i] = Wire.read();
    return true;
  }

  void exioSet(uint8_t bit, bool high) {
    if (!(exioMode & (1 << bit))) { exioMode |= (1 << bit); write8(addr::EXIO, REG_MODE, exioMode); }
    if (high) exioOut |= (1 << bit); else exioOut &= ~(1 << bit);
    write8(addr::EXIO, REG_OUTPUT, exioOut);
  }

  // CST3530 : registres sur 32 bits, envoyés poids fort en premier
  bool touchReg(uint32_t reg, uint8_t* buf, size_t n) {
    Wire.beginTransmission(addr::TOUCH);
    for (int s = 24; s >= 0; s -= 8) Wire.write((uint8_t)(reg >> s));
    if (n == 0) return Wire.endTransmission(true) == 0;
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(addr::TOUCH, (uint8_t)n) != n) return false;
    for (size_t i = 0; i < n; i++) buf[i] = Wire.read();
    return true;
  }
  constexpr uint32_t TOUCH_DATA = 0xD0070000, TOUCH_END = 0xD00002AB;

  uint8_t bcd2dec(uint8_t v) { return (v >> 4) * 10 + (v & 0x0F); }
  uint8_t dec2bcd(uint8_t v) { return ((v / 10) << 4) | (v % 10); }

  bool shtCmd(uint16_t cmd) {
    Wire.beginTransmission(addr::SHTC3);
    Wire.write(cmd >> 8);
    Wire.write(cmd & 0xFF);
    return Wire.endTransmission() == 0;
  }
  uint8_t crc8(const uint8_t* d, int n) {
    uint8_t c = 0xFF;
    for (int i = 0; i < n; i++) { c ^= d[i]; for (int b = 0; b < 8; b++) c = (c & 0x80) ? (c << 1) ^ 0x31 : (c << 1); }
    return c;
  }
}

namespace board {

bool begin() {
  pinMode(pin::SD_CS, OUTPUT);
  digitalWrite(pin::SD_CS, HIGH);
  pinMode(pin::TP_INT, INPUT);
  Wire.begin(pin::I2C_SDA, pin::I2C_SCL);
  Wire.setClock(400000);

  // Resets relâchés, ampli audio coupé, puis direction des broches
  exioOut = (1 << EXIO_TP_RST) | (1 << EXIO_LCD_RST);
  exioMode = (1 << EXIO_TP_RST) | (1 << EXIO_LCD_RST) | (1 << EXIO_PA_CTRL);
  bool ok = write8(addr::EXIO, REG_OUTPUT, exioOut) && write8(addr::EXIO, REG_MODE, exioMode);
  if (!ok) return false;

  exioSet(EXIO_LCD_RST, false); delay(50);
  exioSet(EXIO_LCD_RST, true);  delay(120);
  exioSet(EXIO_TP_RST, false);  delay(100);
  exioSet(EXIO_TP_RST, true);   delay(300);
  return true;
}

void scanI2C() {
  Serial.print("I2C :");
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) Serial.printf(" 0x%02X", a);
  }
  Serial.println();
}

void backlight(uint8_t percent) {
  if (percent > 100) percent = 100;
  write8(addr::EXIO, REG_PWM, (uint8_t)(percent * 255 / 100));
}

void amp(bool on) { exioSet(EXIO_PA_CTRL, on); }

bool readBattery(float& volts) {
  uint32_t sum = 0;
  for (int i = 0; i < 8; i++) {
    uint8_t b[2];
    if (!readN(addr::EXIO, REG_ADC, b, 2)) return false;
    uint16_t raw = b[0] | (b[1] << 8);
    if (raw > 1023) return false;
    sum += raw;
    delay(5);
  }
  volts = ((sum + 4) / 8) * 3.3f / 1023.0f * 3.0f;
  return true;
}

bool readClimate(float& tempC, float& humidity) {
  if (!shtCmd(0x3517)) return false;     // réveil
  delayMicroseconds(300);
  if (!shtCmd(0x7866)) return false;     // mesure, température d'abord, mode normal
  delay(15);
  uint8_t d[6];
  if (Wire.requestFrom(addr::SHTC3, (uint8_t)6) != 6) return false;
  for (int i = 0; i < 6; i++) d[i] = Wire.read();
  shtCmd(0xB098);                        // retour en veille : le capteur chauffe moins
  if (crc8(d, 2) != d[2] || crc8(d + 3, 2) != d[5]) return false;
  tempC = -45.0f + 175.0f * ((d[0] << 8) | d[1]) / 65536.0f;
  humidity = 100.0f * ((d[3] << 8) | d[4]) / 65536.0f;
  return true;
}

bool rtcRead(struct tm& t) {
  uint8_t r[7];
  if (!readN(addr::RTC, 0x04, r, 7)) return false;
  if (r[0] & 0x80) return false;         // oscillateur arrêté : heure non fiable
  t.tm_sec = bcd2dec(r[0] & 0x7F);
  t.tm_min = bcd2dec(r[1] & 0x7F);
  t.tm_hour = bcd2dec(r[2] & 0x3F);
  t.tm_mday = bcd2dec(r[3] & 0x3F);
  t.tm_wday = r[4] & 0x07;
  t.tm_mon = bcd2dec(r[5] & 0x1F) - 1;
  t.tm_year = bcd2dec(r[6]) + 100;
  t.tm_isdst = -1;
  return true;
}

bool rtcWrite(const struct tm& t) {
  Wire.beginTransmission(addr::RTC);
  Wire.write(0x04);
  Wire.write(dec2bcd(t.tm_sec));
  Wire.write(dec2bcd(t.tm_min));
  Wire.write(dec2bcd(t.tm_hour));
  Wire.write(dec2bcd(t.tm_mday));
  Wire.write(t.tm_wday);
  Wire.write(dec2bcd(t.tm_mon + 1));
  Wire.write(dec2bcd(t.tm_year - 100));
  return Wire.endTransmission() == 0;
}

bool touchRead(int16_t& x, int16_t& y) {
  uint8_t b[9];
  if (!touchReg(TOUCH_DATA, b, 9)) return false;
  uint8_t n = b[3] & 0x0F;
  bool ok = n > 0 && n <= 5 && (b[8] & 0xF0) != 0;
  if (ok) {
    x = ((b[7] & 0x0F) << 8) + b[4];
    y = ((b[7] & 0xF0) << 4) + b[5];
  }
  touchReg(TOUCH_END, nullptr, 0);
  return ok;
}

}  // namespace board
