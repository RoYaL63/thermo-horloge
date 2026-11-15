// WiFi, portail de premier démarrage, heure internet, météo Open-Meteo, agenda .ics et transports.
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <DNSServer.h>
#include <esp_sntp.h>
#include <esp_wps.h>
#include "board.h"
#include "config.h"
#include "data.h"
#include "jsonscan.h"

Climate clim;
Weather wx;
Transit transit;
Agenda agenda;
bool timeOk = false;

static DNSServer dns;
static bool portal = false, apUp = false, ntpSynced = false;
static uint32_t staStart = 0, nextWx = 0, nextAg = 0, nextTr = 0, nextRtc = 0;
static Event icsEv[14];
static int icsN = 0;
extern uint8_t uiScreen;   // écran affiché (ui.cpp)

bool netPortal() { return portal; }
bool netOnline() { return WiFi.status() == WL_CONNECTED; }
String netIp() { return netOnline() ? WiFi.localIP().toString() : WiFi.softAPIP().toString(); }
int netRssi() { return netOnline() ? WiFi.RSSI() : 0; }
void netRefreshSoon() { nextWx = nextAg = nextTr = 0; }

static void startAp() {
  if (apUp) return;
  WiFi.mode(cfg.ssid.length() ? WIFI_AP_STA : WIFI_AP);
  WiFi.softAP("Thermo-Horloge");
  dns.start(53, "*", WiFi.softAPIP());
  apUp = true;
  Serial.printf("Point d'acces Thermo-Horloge, page %s\n", WiFi.softAPIP().toString().c_str());
}

static void stopAp() {
  if (!apUp) return;
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  apUp = false;
}

// ---------- WPS : la box donne son WiFi quand on appuie sur son bouton WPS, sans mot de passe à taper ----------
static volatile uint8_t wpsState = 0;   // 0 rien, 1 en attente de la box, 2 reçu, 3 échec, 4 connecté
static uint32_t wpsAt = 0;
static String wpsSsid;

uint8_t netWpsState() { return wpsState; }
String netWpsSsid() { return wpsSsid; }
int netWpsLeft() { return wpsState == 1 ? max(0, 120 - (int)((millis() - wpsAt) / 1000)) : 0; }

static void onWifiEvent(WiFiEvent_t e, arduino_event_info_t) {   // appelé depuis une autre tâche : on ne fait que noter
  if (e == ARDUINO_EVENT_WPS_ER_SUCCESS) wpsState = 2;
  else if ((e == ARDUINO_EVENT_WPS_ER_FAILED || e == ARDUINO_EVENT_WPS_ER_TIMEOUT) && wpsState == 1) wpsState = 3;
}

void netWpsStart() {
  if (wpsState == 1 || wpsState == 2) return;
  WiFi.mode(WIFI_AP_STA);   // le point d'accès reste ouvert pour le téléphone, la station écoute la box
  esp_wifi_wps_disable();
  esp_wps_config_t c;
  memset(&c, 0, sizeof c);
  c.wps_type = WPS_TYPE_PBC;
  snprintf(c.factory_info.manufacturer, sizeof c.factory_info.manufacturer, "Thermo-horloge");
  snprintf(c.factory_info.model_number, sizeof c.factory_info.model_number, "%s", CONFIG_IDF_TARGET);
  snprintf(c.factory_info.model_name, sizeof c.factory_info.model_name, "Thermo-horloge");
  snprintf(c.factory_info.device_name, sizeof c.factory_info.device_name, "Thermo-horloge");
  if (esp_wifi_wps_enable(&c) != ESP_OK) { wpsState = 3; return; }
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 0, 0)
  esp_err_t err = esp_wifi_wps_start();
#else
  esp_err_t err = esp_wifi_wps_start(0);
#endif
  if (err != ESP_OK) { esp_wifi_wps_disable(); wpsState = 3; return; }
  wpsState = 1;
  wpsAt = millis();
  Serial.println("WPS : appuie sur le bouton WPS de la box");
}

static void wpsLoop() {
  if (wpsState == 1 && millis() - wpsAt > 125000) { esp_wifi_wps_disable(); wpsState = 3; }
  if (wpsState == 2) {
    esp_wifi_wps_disable();
    WiFi.begin();                 // identifiants reçus de la box, gardés par la puce WiFi
    wpsState = 5;
    wpsAt = millis();
  }
  if (wpsState == 5) {
    if (WiFi.status() == WL_CONNECTED) {
      wpsSsid = WiFi.SSID();
      cfg.ssid = wpsSsid;
      cfg.pass = WiFi.psk();
      cfgSave();
      wpsState = 4;
      wpsAt = millis();
      Serial.printf("WPS : connecte a %s\n", wpsSsid.c_str());
    } else if (millis() - wpsAt > 25000) wpsState = 3;
  }
  if (wpsState == 4 && millis() - wpsAt > 3000) ESP.restart();
}

void netBegin() {
  WiFi.onEvent(onWifiEvent);
  WiFi.persistent(false);
  WiFi.setHostname("thermo-horloge");
  if (cfg.ssid.isEmpty()) {
    portal = true;
    startAp();
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(cfg.ssid.c_str(), cfg.pass.c_str());
  staStart = millis();
  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.google.com");
}

// ---------- HTTP ----------
static String urlEncode(const String& s) {
  String o;
  for (size_t i = 0; i < s.length(); i++) {
    uint8_t c = s[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.') o += (char)c;
    else { char b[4]; snprintf(b, sizeof b, "%%%02X", c); o += b; }
  }
  return o;
}

// GET qui envoie la réponse dans un Stream (lecture au fil de l'eau). Libère l'image écran pendant une requête chiffrée.
static int httpGet(const String& url, Stream& sink, const char* hk = nullptr, const char* hv = nullptr,
                   const char* user = nullptr, const char* pw = nullptr) {
  bool tls = url.startsWith("https");
  if (tls) uiReleaseFrame();
  int code = -1;
  {
    HTTPClient http;
    WiFiClient plain;
    WiFiClientSecure secure;
    bool ok;
    if (tls) { secure.setInsecure(); ok = http.begin(secure, url); }
    else ok = http.begin(plain, url);
    if (ok) {
      http.setTimeout(12000);
      http.setConnectTimeout(8000);
      http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
      http.setUserAgent("thermo-horloge/" FW_VERSION);
      if (hk) http.addHeader(hk, hv);
      if (user) http.setAuthorization(user, pw);
      code = http.GET();
      if (code == 200) http.writeToStream(&sink);
      http.end();
    }
  }
  if (tls) uiRestoreFrame();
  return code;
}

// ---------- Météo ----------
static uint8_t skyFromCode(int code, bool day, float wind) {
  uint8_t s;
  if (code == 0) s = day ? W_SOLEIL : W_LUNE;
  else if (code == 1) s = day ? W_ECLAIRCIES : W_LUNE;
  else if (code == 2) s = day ? W_ECLAIRCIES : W_NUAGE;
  else if (code == 3) s = W_NUAGE;
  else if (code == 45 || code == 48) s = W_BROUILLARD;
  else if ((code >= 71 && code <= 77) || code == 85 || code == 86) s = W_NEIGE;
  else if (code >= 95) s = W_ORAGE;
  else if (code >= 51) s = W_PLUIE;
  else s = W_NUAGE;
  if (wind >= 45 && (s == W_SOLEIL || s == W_ECLAIRCIES || s == W_NUAGE || s == W_LUNE)) s = W_VENT;
  return s;
}

static int hhmm(const String& iso) {   // "2026-11-15T07:58" -> minutes
  int t = iso.indexOf('T');
  if (t < 0) return -1;
  return iso.substring(t + 1, t + 3).toInt() * 60 + iso.substring(t + 4, t + 6).toInt();
}

static bool weatherUpdate() {
  char url[400];
  snprintf(url, sizeof url,
           "http://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
           "&current=temperature_2m,weather_code,is_day,wind_speed_10m"
           "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max,sunrise,sunset,uv_index_max"
           "&minutely_15=precipitation&forecast_minutely_15=8&timezone=Europe%%2FParis&forecast_days=5",
           cfg.lat, cfg.lon);
  Weather w = wx;
  int code = -1;
  bool gotCurrent = false;
  int dcode[5] = {3, 3, 3, 3, 3};
  JsonScan::Cb onWx = [&](const String& ctx, int, const String& key, int ki, const String& v) {
    if (ctx == "current") {
      if (key == "temperature_2m") { w.t = v.toFloat(); gotCurrent = true; }
      else if (key == "weather_code") code = v.toInt();
      else if (key == "is_day") w.day = v.toInt() == 1;
      else if (key == "wind_speed_10m") w.wind = v.toFloat();
    } else if (ctx == "daily" && ki >= 0 && ki < 5) {
      if (key == "weather_code") dcode[ki] = v.toInt();
      else if (ki >= 1 && key == "temperature_2m_max") w.fmax[ki - 1] = v.toFloat();
      else if (ki >= 1 && key == "temperature_2m_min") w.fmin[ki - 1] = v.toFloat();
      else if (ki >= 1 && key == "precipitation_probability_max") w.fpop[ki - 1] = v == "null" ? 0 : v.toInt();
      else if (ki == 0 && key == "sunrise") w.sunrise = hhmm(v);
      else if (ki == 0 && key == "sunset") w.sunset = hhmm(v);
      else if (ki == 0 && key == "uv_index_max") w.uv = v == "null" ? NAN : v.toFloat();
      else if (ki >= 1 && key == "time") {
        struct tm t = {};
        t.tm_year = v.substring(0, 4).toInt() - 1900; t.tm_mon = v.substring(5, 7).toInt() - 1; t.tm_mday = v.substring(8, 10).toInt();
        t.tm_hour = 12; t.tm_isdst = -1;
        mktime(&t);
        w.fwday[ki - 1] = t.tm_wday; w.fmday[ki - 1] = t.tm_mday;
      }
    } else if (ctx == "minutely_15" && key == "precipitation" && ki >= 0 && ki < 8) {
      w.rain[ki] = v == "null" ? 0 : v.toFloat() * 4;   // mm par quart d'heure -> mm/h
      w.rainOk = true;
    }
  };
  code = -1;
  int http;
  {
    JsonScan js(onWx);
    http = httpGet(url, js);
  }
  if (http != 200 || !gotCurrent) {   // si le HTTP simple est refusé, on repasse en HTTPS avec un lecteur neuf
    String su = String(url);
    su.replace("http://", "https://");
    JsonScan js2(onWx);
    http = httpGet(su, js2);
  }
  if (http != 200 || !gotCurrent) { Serial.printf("Meteo : echec (%d)\n", http); return false; }
  w.sky = skyFromCode(code, w.day, w.wind);
  for (int i = 0; i < 4; i++) w.fsky[i] = skyFromCode(dcode[i + 1], true, 0);
  w.ok = true;
  w.at = millis();

  snprintf(url, sizeof url, "http://air-quality-api.open-meteo.com/v1/air-quality?latitude=%.4f&longitude=%.4f&current=european_aqi", cfg.lat, cfg.lon);
  JsonScan aq([&](const String& ctx, int, const String& key, int, const String& v) {
    if (ctx == "current" && key == "european_aqi" && v != "null") w.aqi = v.toInt();
  });
  httpGet(url, aq);
  wx = w;
  Serial.printf("Meteo : %.1f C, ciel %d, air %d\n", wx.t, wx.sky, wx.aqi);
  return true;
}

bool geocodeCity(const String& name, float& lat, float& lon, String& label) {
  if (!netOnline()) return false;
  String url = "http://geocoding-api.open-meteo.com/v1/search?count=1&language=fr&format=json&name=" + urlEncode(name);
  bool found = false;
  String nm, adm;
  JsonScan js([&](const String& ctx, int ci, const String& key, int, const String& v) {
    if (ctx != "results" || ci != 0) return;
    if (key == "latitude") { lat = v.toFloat(); found = true; }
    else if (key == "longitude") lon = v.toFloat();
    else if (key == "name") nm = v;
    else if (key == "admin1") adm = v;
  });
  if (httpGet(url, js) != 200 || !found) return false;
  label = nm;
  return true;
}

// ---------- Agenda (.ics) ----------
static time_t utcToLocal(int y, int mo, int d, int h, int mi) {
  // jours depuis 1970 (algorithme civil), puis conversion en heure locale par localtime
  y -= mo <= 2;
  int era = (y >= 0 ? y : y - 399) / 400;
  unsigned yoe = (unsigned)(y - era * 400);
  unsigned doy = (153 * (mo + (mo > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  long days = era * 146097L + (long)doe - 719468L;
  return (time_t)days * 86400 + h * 3600 + mi * 60;
}

class IcsSink : public Stream {
 public:
  int todayY, todayM, todayD, todayW;
  size_t write(uint8_t c) override {
    if (c == '\r') return 1;
    if (c == '\n') { nl_ = true; return 1; }
    if (nl_) {
      nl_ = false;
      if (c == ' ' || c == '\t') return 1;   // ligne repliée : on continue la précédente
      line(cur_);
      cur_ = "";
    }
    if (cur_.length() < 300) cur_ += (char)c;
    return 1;
  }
  size_t write(const uint8_t* b, size_t n) override { for (size_t i = 0; i < n; i++) write(b[i]); return n; }
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
  void flush() override { if (cur_.length()) line(cur_); cur_ = ""; }

 private:
  String cur_;
  bool nl_ = false, in_ = false;
  String start_, summary_, rrule_;

  static String val(const String& l) { int i = l.indexOf(':'); return i < 0 ? "" : l.substring(i + 1); }

  void line(const String& l) {
    if (l.startsWith("BEGIN:VEVENT")) { in_ = true; start_ = summary_ = rrule_ = ""; return; }
    if (!in_) return;
    if (l.startsWith("DTSTART")) start_ = l;
    else if (l.startsWith("SUMMARY")) summary_ = val(l);
    else if (l.startsWith("RRULE")) rrule_ = val(l);
    else if (l.startsWith("END:VEVENT")) { in_ = false; finish(); }
  }

  void finish() {
    if (icsN >= 14 || start_.isEmpty()) return;
    String v = val(start_);
    if (v.length() < 8) return;
    int y = v.substring(0, 4).toInt(), mo = v.substring(4, 6).toInt(), d = v.substring(6, 8).toInt();
    int mins = -1;
    if (v.length() >= 13 && v[8] == 'T') {
      int h = v.substring(9, 11).toInt(), mi = v.substring(11, 13).toInt();
      if (v.endsWith("Z")) {
        time_t t = utcToLocal(y, mo, d, h, mi);
        struct tm lt;
        localtime_r(&t, &lt);
        y = lt.tm_year + 1900; mo = lt.tm_mon + 1; d = lt.tm_mday; h = lt.tm_hour; mi = lt.tm_min;
      }
      mins = h * 60 + mi;
    }
    for (uint8_t off = 0; off < 2; off++) {
      struct tm day = {};
      day.tm_year = todayY - 1900; day.tm_mon = todayM - 1; day.tm_mday = todayD + off; day.tm_hour = 12; day.tm_isdst = -1;
      mktime(&day);
      bool match = (y == day.tm_year + 1900 && mo == day.tm_mon + 1 && d == day.tm_mday);
      if (!match && rrule_.length()) match = recurs(y, mo, d, day);
      if (match && icsN < 14) {
        String s = summary_;
        s.replace("\\,", ","); s.replace("\\;", ";"); s.replace("\\n", " ");
        icsEv[icsN++] = {off, (int16_t)mins, s, false};
      }
    }
  }

  // Répétitions simples : quotidienne, hebdomadaire, mensuelle, annuelle (anniversaires)
  bool recurs(int y, int mo, int d, const struct tm& day) {
    struct tm st = {};
    st.tm_year = y - 1900; st.tm_mon = mo - 1; st.tm_mday = d; st.tm_hour = 12; st.tm_isdst = -1;
    time_t ts = mktime(&st), td = mktime(const_cast<struct tm*>(&day));
    if (td < ts) return false;
    int u = rrule_.indexOf("UNTIL=");
    if (u >= 0) {
      String un = rrule_.substring(u + 6, u + 14);
      long until = un.toInt(), cur = (day.tm_year + 1900) * 10000L + (day.tm_mon + 1) * 100 + day.tm_mday;
      if (cur > until) return false;
    }
    if (rrule_.indexOf("FREQ=DAILY") >= 0) return true;
    if (rrule_.indexOf("FREQ=YEARLY") >= 0) return st.tm_mon == day.tm_mon && st.tm_mday == day.tm_mday;
    if (rrule_.indexOf("FREQ=MONTHLY") >= 0) return st.tm_mday == day.tm_mday;
    if (rrule_.indexOf("FREQ=WEEKLY") >= 0) {
      static const char* const DN[] = {"SU", "MO", "TU", "WE", "TH", "FR", "SA"};
      int b = rrule_.indexOf("BYDAY=");
      if (b >= 0) return rrule_.indexOf(DN[day.tm_wday], b) >= 0;
      return st.tm_wday == day.tm_wday;
    }
    return false;
  }
};

void agendaRebuild() {
  Agenda a;
  a.ok = agenda.ok; a.err = agenda.err; a.at = agenda.at;
  for (int i = 0; i < icsN && a.n < 14; i++) a.ev[a.n++] = icsEv[i];
  for (int i = 0; i < N_REM && a.n < 14; i++) {
    const Reminder& r = cfg.rem[i];
    if (!r.on || r.text.isEmpty()) continue;
    a.ev[a.n++] = {0, (int16_t)(r.h * 60 + r.m), r.text, true};
    if (r.daily && a.n < 14) a.ev[a.n++] = {1, (int16_t)(r.h * 60 + r.m), r.text, true};
  }
  for (int i = 1; i < a.n; i++)
    for (int j = i; j > 0; j--) {
      const Event& p = a.ev[j - 1]; const Event& q = a.ev[j];
      if (p.day * 2000 + p.min > q.day * 2000 + q.min) std::swap(a.ev[j - 1], a.ev[j]); else break;
    }
  agenda = a;
}

static void agendaUpdate() {
  if (cfg.ics.isEmpty()) { icsN = 0; agenda.ok = false; agenda.err = ""; agendaRebuild(); return; }
  time_t now = time(nullptr);
  struct tm lt;
  localtime_r(&now, &lt);
  IcsSink sink;
  sink.todayY = lt.tm_year + 1900; sink.todayM = lt.tm_mon + 1; sink.todayD = lt.tm_mday; sink.todayW = lt.tm_wday;
  int keep = icsN;
  icsN = 0;
  String url = cfg.ics;
  if (url.startsWith("webcal://")) url = "https://" + url.substring(9);
  int code = httpGet(url, sink);
  sink.flush();
  if (code != 200) { icsN = keep; agenda.err = "agenda injoignable (" + String(code) + ")"; Serial.println(agenda.err); }
  else { agenda.ok = true; agenda.err = ""; agenda.at = millis(); Serial.printf("Agenda : %d evenements\n", icsN); }
  agendaRebuild();
}

// ---------- Transports ----------
static int minutesUntil(time_t t) { return (int)((t - time(nullptr)) / 60); }

static void addDeparture(Transit& t, int lineIdx, int mins, const String& dest) {
  if (mins < 0 || mins > 120) return;
  Departure& d = t.d[lineIdx];
  if (d.m1 < 0 || mins < d.m1) { d.m2 = d.m1; d.m1 = mins; if (dest.length()) d.dest = dest; }
  else if (d.m2 < 0 || mins < d.m2) d.m2 = mins;
}

static void transitUpdate() {
  Transit t;
  t.n = 0;
  for (int i = 0; i < 3; i++) {
    const TransitLine& L = cfg.lines[i];
    if (L.label.isEmpty() || L.stop.isEmpty()) continue;
    Departure& d = t.d[t.n];
    d.line = L.label; d.color = L.color; d.stop = L.stopName;
    int idx = t.n++;
    if (cfg.transitMode == 1) {
      String url = "https://prim.iledefrance-mobilites.fr/marketplace/stop-monitoring?MonitoringRef=" + urlEncode(L.stop);
      String line, dest;
      JsonScan js([&](const String& ctx, int, const String& key, int, const String& v) {
        if (ctx == "LineRef" && key == "value") { line = v; dest = ""; }
        else if (ctx == "DestinationName" && key == "value") dest = v;
        else if ((key == "ExpectedDepartureTime" || key == "ExpectedArrivalTime") && v.length() >= 16) {
          if (L.filter.length() && line.indexOf(L.filter) < 0) return;
          time_t tt = utcToLocal(v.substring(0, 4).toInt(), v.substring(5, 7).toInt(), v.substring(8, 10).toInt(),
                                 v.substring(11, 13).toInt(), v.substring(14, 16).toInt());
          addDeparture(t, idx, minutesUntil(tt), dest);
        }
      });
      int code = httpGet(url, js, "apikey", cfg.primKey.c_str());
      if (code != 200) t.err = "PRIM : erreur " + String(code);
    } else if (cfg.transitMode == 2) {
      String url = "https://download.data.grandlyon.com/ws/rdata/tcl_sytral.tclpassagearret/all.json?maxfeatures=60&field=id&value=" + urlEncode(L.stop);
      String ligne, dir, heure;
      int cur = -1;
      auto flushRow = [&]() {
        if (heure.length() >= 16 && (L.filter.isEmpty() || ligne == L.filter)) {
          struct tm tt = {};
          tt.tm_year = heure.substring(0, 4).toInt() - 1900; tt.tm_mon = heure.substring(5, 7).toInt() - 1;
          tt.tm_mday = heure.substring(8, 10).toInt(); tt.tm_hour = heure.substring(11, 13).toInt();
          tt.tm_min = heure.substring(14, 16).toInt(); tt.tm_isdst = -1;
          addDeparture(t, idx, minutesUntil(mktime(&tt)), dir);
        }
        ligne = dir = heure = "";
      };
      JsonScan js([&](const String& ctx, int ci, const String& key, int, const String& v) {
        if (ctx != "values") return;
        if (ci != cur) { if (cur >= 0) flushRow(); cur = ci; }
        if (key == "ligne") ligne = v;
        else if (key == "direction") dir = v;
        else if (key == "heurepassage") heure = v;
      });
      int code = httpGet(url, js, nullptr, nullptr, cfg.lyonUser.c_str(), cfg.lyonPass.c_str());
      if (cur >= 0) flushRow();
      if (code != 200) t.err = "Grand Lyon : erreur " + String(code);
    }
  }
  t.ok = t.n > 0 && t.err.isEmpty();
  t.at = millis();
  transit = t;
}

// ---------- Boucle ----------
void netLoop() {
  if (apUp) dns.processNextRequest();
  wpsLoop();
  if (portal) return;

  bool on = netOnline();
  if (!on && !apUp && millis() - staStart > 90000) startAp();            // WiFi introuvable : on rouvre le portail
  if (on && apUp && millis() - staStart > 90000) stopAp();
  if (!on) {
    static uint32_t retry = 0;
    if (millis() - retry > 30000) { retry = millis(); WiFi.reconnect(); }
    return;
  }

  time_t now = time(nullptr);
  struct tm lt;
  localtime_r(&now, &lt);
  // Heure internet reçue : on recale l'horloge de la carte tout de suite, puis toutes les 6 h
  if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED || (ntpSynced && lt.tm_year + 1900 >= 2025)) {
    if (!ntpSynced) { ntpSynced = true; nextRtc = 0; Serial.println("Heure internet recue"); agendaRebuild(); }
    timeOk = true;
    if (millis() >= nextRtc) { board::rtcWrite(lt); nextRtc = millis() + 6UL * 3600 * 1000; }
  }
  if (!timeOk) return;

  uint32_t ms = millis();
  if (ms >= nextWx) { nextWx = ms + (weatherUpdate() ? 15UL * 60 * 1000 : 60UL * 1000); return; }
  if (ms >= nextAg) { agendaUpdate(); nextAg = ms + 30UL * 60 * 1000; return; }
  if (cfg.transitMode && ms >= nextTr) {
    transitUpdate();
    nextTr = ms + (uiScreen == S_TRANSIT ? 45UL : 180UL) * 1000;
    return;
  }
}
