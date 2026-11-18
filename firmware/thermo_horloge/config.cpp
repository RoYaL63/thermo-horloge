#include "config.h"
#include <Preferences.h>

Settings cfg;
const char* const THEME_NAMES[N_THEMES] = {"Galaxie", "Côté obscur", "Cockpit", "Radar", "Hologramme", "Portail",
                                            "Assistant", "Labo", "Grand Prix", "Terminal", "Loutre", "Magie"};
const char* const SCREEN_NAMES[N_CAROUSEL] = {"Accueil", "Agenda", "Pluie", "Prévisions", "Confort", "Sonomètre",
                                               "Minuteur", "Ciel", "Transports"};

static Preferences prefs;

bool screenOn(uint8_t s) { return s == S_HOME || (cfg.screens & (1 << s)); }

void cfgLoad() {
  prefs.begin("thermo", true);
  if (!prefs.isKey("ver")) { prefs.end(); return; }
  cfg.ssid = prefs.getString("ssid", "");
  cfg.pass = prefs.getString("pass", "");
  cfg.city = prefs.getString("city", cfg.city);
  cfg.lat = prefs.getFloat("lat", cfg.lat);
  cfg.lon = prefs.getFloat("lon", cfg.lon);
  cfg.room = prefs.getString("room", cfg.room);
  cfg.theme = prefs.getUChar("theme", cfg.theme) % N_THEMES;
  prefs.getBytes("variant", cfg.variant, sizeof cfg.variant);
  cfg.nav = prefs.getUChar("nav", cfg.nav);
  cfg.bri = prefs.getUChar("bri", cfg.bri);
  cfg.nightOn = prefs.getBool("nOn", cfg.nightOn);
  cfg.nightFrom = prefs.getUChar("nFrom", cfg.nightFrom);
  cfg.nightTo = prefs.getUChar("nTo", cfg.nightTo);
  cfg.nightStyle = prefs.getUChar("nStyle", cfg.nightStyle);
  cfg.nightRed = prefs.getBool("nRed", cfg.nightRed);
  cfg.nightBri = prefs.getUChar("nBri", cfg.nightBri);
  cfg.nightInfo = prefs.getBool("nInfo", cfg.nightInfo);
  cfg.nightShift = prefs.getBool("nShift", cfg.nightShift);
  cfg.tempOffset = prefs.getFloat("tOff", cfg.tempOffset);
  cfg.autoCal = prefs.getBool("aCal", false);
  cfg.soundOffset = prefs.getFloat("sOff", cfg.soundOffset);
  cfg.screens = prefs.getUShort("screens", cfg.screens);
  cfg.touchSwap = prefs.getBool("tSwap", false);
  cfg.touchInvX = prefs.getBool("tInvX", false);
  cfg.touchInvY = prefs.getBool("tInvY", false);
  prefs.getBytes("cMain", cfg.colMain, sizeof cfg.colMain);
  prefs.getBytes("cSec", cfg.colSecond, sizeof cfg.colSecond);
  prefs.getBytes("cGauge", cfg.colGauge, sizeof cfg.colGauge);
  cfg.sounds = prefs.getBool("sounds", cfg.sounds);
  cfg.transitMode = prefs.getUChar("trMode", 0);
  cfg.primKey = prefs.getString("prim", "");
  cfg.lyonUser = prefs.getString("lyU", "");
  cfg.lyonPass = prefs.getString("lyP", "");
  for (int i = 0; i < 3; i++) {
    String k = "l" + String(i);
    cfg.lines[i].label = prefs.getString((k + "lb").c_str(), "");
    cfg.lines[i].color = prefs.getUInt((k + "co").c_str(), 0x6F7780);
    cfg.lines[i].stop = prefs.getString((k + "st").c_str(), "");
    cfg.lines[i].filter = prefs.getString((k + "fi").c_str(), "");
    cfg.lines[i].stopName = prefs.getString((k + "sn").c_str(), "");
  }
  cfg.ics = prefs.getString("ics", "");
  for (int i = 0; i < N_REM; i++) {
    String k = "r" + String(i);
    uint32_t packed = prefs.getUInt((k + "p").c_str(), 0);
    cfg.rem[i].on = packed & 1;
    cfg.rem[i].daily = packed & 2;
    cfg.rem[i].h = (packed >> 8) & 0xFF;
    cfg.rem[i].m = (packed >> 16) & 0xFF;
    cfg.rem[i].text = prefs.getString((k + "t").c_str(), "");
  }
  prefs.end();
}

void cfgSave() {
  prefs.begin("thermo", false);
  prefs.putUChar("ver", 1);
  prefs.putString("ssid", cfg.ssid);
  prefs.putString("pass", cfg.pass);
  prefs.putString("city", cfg.city);
  prefs.putFloat("lat", cfg.lat);
  prefs.putFloat("lon", cfg.lon);
  prefs.putString("room", cfg.room);
  prefs.putUChar("theme", cfg.theme);
  prefs.putBytes("variant", cfg.variant, sizeof cfg.variant);
  prefs.putUChar("nav", cfg.nav);
  prefs.putUChar("bri", cfg.bri);
  prefs.putBool("nOn", cfg.nightOn);
  prefs.putUChar("nFrom", cfg.nightFrom);
  prefs.putUChar("nTo", cfg.nightTo);
  prefs.putUChar("nStyle", cfg.nightStyle);
  prefs.putBool("nRed", cfg.nightRed);
  prefs.putUChar("nBri", cfg.nightBri);
  prefs.putBool("nInfo", cfg.nightInfo);
  prefs.putBool("nShift", cfg.nightShift);
  prefs.putFloat("tOff", cfg.tempOffset);
  prefs.putBool("aCal", cfg.autoCal);
  prefs.putFloat("sOff", cfg.soundOffset);
  prefs.putUShort("screens", cfg.screens);
  prefs.putBool("tSwap", cfg.touchSwap);
  prefs.putBool("tInvX", cfg.touchInvX);
  prefs.putBool("tInvY", cfg.touchInvY);
  prefs.putBytes("cMain", cfg.colMain, sizeof cfg.colMain);
  prefs.putBytes("cSec", cfg.colSecond, sizeof cfg.colSecond);
  prefs.putBytes("cGauge", cfg.colGauge, sizeof cfg.colGauge);
  prefs.putBool("sounds", cfg.sounds);
  prefs.putUChar("trMode", cfg.transitMode);
  prefs.putString("prim", cfg.primKey);
  prefs.putString("lyU", cfg.lyonUser);
  prefs.putString("lyP", cfg.lyonPass);
  for (int i = 0; i < 3; i++) {
    String k = "l" + String(i);
    prefs.putString((k + "lb").c_str(), cfg.lines[i].label);
    prefs.putUInt((k + "co").c_str(), cfg.lines[i].color);
    prefs.putString((k + "st").c_str(), cfg.lines[i].stop);
    prefs.putString((k + "fi").c_str(), cfg.lines[i].filter);
    prefs.putString((k + "sn").c_str(), cfg.lines[i].stopName);
  }
  prefs.putString("ics", cfg.ics);
  for (int i = 0; i < N_REM; i++) {
    String k = "r" + String(i);
    uint32_t packed = (cfg.rem[i].on ? 1 : 0) | (cfg.rem[i].daily ? 2 : 0) | (cfg.rem[i].h << 8) | (cfg.rem[i].m << 16);
    prefs.putUInt((k + "p").c_str(), packed);
    prefs.putString((k + "t").c_str(), cfg.rem[i].text);
  }
  prefs.end();
}

void cfgForgetWifi() {
  cfg.ssid = "";
  cfg.pass = "";
  cfgSave();
}

void cfgFactory() {
  prefs.begin("thermo", false);
  prefs.clear();
  prefs.end();
  cfg = Settings();
}
