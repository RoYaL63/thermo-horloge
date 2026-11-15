// Son : codec ES8311 (micro et haut-parleur) en I2S, horloge tirée du signal BCLK (la carte n'a pas de broche MCLK).
// Sonomètre : niveau efficace du micro converti en dB approximatifs, à étalonner (réglage soundOffset).
// Si le codec ne répond pas, le reste de l'objet fonctionne : le sonomètre affiche « micro indisponible ».
#include <ESP_I2S.h>
#include "board.h"
#include "config.h"
#include "data.h"

namespace {
  constexpr uint8_t ES = 0x18;
  constexpr int RATE = 16000;
  I2SClass i2s;
  bool ready = false;
  int beepsLeft = 0;
  uint32_t nextBeep = 0, lastLevel = 0;

  bool wr(uint8_t reg, uint8_t v) {
    Wire.beginTransmission(ES);
    Wire.write(reg);
    Wire.write(v);
    return Wire.endTransmission() == 0;
  }

  bool codecInit() {
    if (!wr(0x00, 0x1F)) return false;      // remise à zéro
    delay(20);
    wr(0x00, 0x00);
    wr(0x45, 0x00);
    wr(0x01, 0xBF);   // horloge maître prise sur BCLK, toutes les horloges internes actives
    wr(0x02, 0x18);   // BCLK = 32 fs, multiplié par 8 -> 256 fs
    wr(0x03, 0x10);   // suréchantillonnage ADC
    wr(0x04, 0x10);   // suréchantillonnage DAC
    wr(0x05, 0x00);
    wr(0x06, 0x03);
    wr(0x07, 0x00);
    wr(0x08, 0xFF);
    wr(0x09, 0x0C);   // entrée série : I2S, 16 bits
    wr(0x0A, 0x0C);   // sortie série : I2S, 16 bits
    wr(0x0B, 0x00);
    wr(0x0C, 0x00);
    wr(0x10, 0x1F);
    wr(0x11, 0x7F);
    wr(0x00, 0x80);   // mise en marche
    wr(0x0D, 0x01);
    wr(0x0E, 0x02);
    wr(0x12, 0x00);
    wr(0x13, 0x10);
    wr(0x14, 0x1A);   // micro analogique, gain de préampli élevé
    wr(0x15, 0x40);
    wr(0x16, 0x24);
    wr(0x17, 0xBF);   // volume ADC
    wr(0x1B, 0x0A);
    wr(0x1C, 0x6A);
    wr(0x37, 0x08);
    wr(0x31, 0x00);   // DAC non muet
    wr(0x32, 0xB0);   // volume du haut-parleur, modéré
    wr(0x44, 0x08);
    return true;
  }
}

float soundDb = NAN;
float soundHist[60];

bool audioBegin() {
  for (float& v : soundHist) v = NAN;
  if (!codecInit()) { Serial.println("Codec ES8311 absent : son et sonometre desactives"); return false; }
  i2s.setPins(15, 3, 2, 20);   // BCLK, LRCK, sortie vers le codec, entrée depuis le micro
  if (!i2s.begin(I2S_MODE_STD, RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)) {
    Serial.println("I2S impossible : son et sonometre desactives");
    return false;
  }
  ready = true;
  Serial.println("Son pret");
  return true;
}

static void tone(int ms, int freq) {
  board::amp(true);
  const int n = 256;
  int16_t buf[n * 2];
  int total = RATE * ms / 1000;
  for (int done = 0; done < total; done += n) {
    for (int i = 0; i < n; i++) {
      float ph = 2 * PI * freq * (done + i) / RATE;
      float env = min(1.0f, min((done + i) / 400.0f, (total - done - i) / 400.0f));
      int16_t v = (int16_t)(sinf(ph) * 9000 * max(0.0f, env));
      buf[i * 2] = v; buf[i * 2 + 1] = v;
    }
    i2s.write((uint8_t*)buf, sizeof buf);
  }
  board::amp(false);
}

void beep(int n) {
  if (!ready || !cfg.sounds) return;
  beepsLeft = n;
  nextBeep = 0;
}

void audioLoop() {
  if (!ready) return;
  if (beepsLeft > 0 && millis() >= nextBeep) {
    tone(140, beepsLeft % 2 ? 1320 : 990);
    beepsLeft--;
    nextBeep = millis() + 120;
    return;
  }
  if (millis() - lastLevel < 1000) return;
  lastLevel = millis();
  int16_t buf[512];
  size_t got = i2s.readBytes((char*)buf, sizeof buf);
  int n = got / 2;
  if (n < 64) return;
  double mean = 0, sq = 0;
  for (int i = 0; i < n; i++) mean += buf[i];
  mean /= n;
  for (int i = 0; i < n; i++) { double v = buf[i] - mean; sq += v * v; }
  double rms = sqrt(sq / n);
  if (rms < 1) rms = 1;
  // dB relatifs à la pleine échelle, décalés vers une échelle proche des dB ambiants. Approximatif, à étalonner.
  float db = 20 * log10(rms / 32768.0) + 100 + cfg.soundOffset;
  soundDb = isnan(soundDb) ? db : soundDb * 0.6f + db * 0.4f;
  memmove(soundHist, soundHist + 1, sizeof(float) * 59);
  soundHist[59] = soundDb;
}
