// Thermo-horloge murale : programme principal.
// Démarrage : écran, capteurs, horloge, WiFi (ou portail Thermo-Horloge au premier démarrage), page web, son.
// Ensuite : mesures toutes les 10 s, courbe sur 24 h, rappels, réseau et écran en continu.

#include "board.h"
#include "config.h"
#include "data.h"

static uint32_t lastClimate = 0, lastHist = 0;
static int lastRemMin = -1;

static void setFromBuild(struct tm& t) {
  char m[4];
  int d, y, hh, mm, ss;
  sscanf(__DATE__, "%3s %d %d", m, &d, &y);
  sscanf(__TIME__, "%d:%d:%d", &hh, &mm, &ss);
  const char* names = "JanFebMarAprMayJunJulAugSepOctNovDec";
  t = {};
  t.tm_mon = (strstr(names, m) - names) / 3;
  t.tm_mday = d; t.tm_year = y - 1900; t.tm_hour = hh; t.tm_min = mm; t.tm_sec = ss; t.tm_isdst = -1;
  mktime(&t);
}

static void readClimate() {
  float t, h;
  if (board::readClimate(t, h)) {
    clim.raw = t;
    float c = t - cfg.tempOffset;
    clim.t = isnan(clim.t) ? c : clim.t * 0.7f + c * 0.3f;
    // Humidité ramenée à la vraie température : même quantité d'eau dans l'air, air moins chaud donc plus humide
    auto es = [](float T) { return 6.112f * expf(17.62f * T / (243.12f + T)); };
    float hc = constrain(h * es(t) / es(c), 0.0f, 100.0f);
    clim.h = isnan(clim.h) ? hc : clim.h * 0.7f + hc * 0.3f;
  }
  board::readBattery(clim.vbat);
}

// Étalonnage automatique : juste après un branchement à froid, la sonde est à la température de la pièce.
// 40 minutes plus tard, l'écart avec la mesure stabilisée donne la correction à appliquer.
static float coldRaw = NAN;
static void calBegin() {
  if (!cfg.autoCal) return;
  float t, h;
  if (board::readClimate(t, h)) { coldRaw = t; Serial.printf("Etalonnage : mesure a froid %.2f C, resultat dans 40 min\n", t); }
}
static void calLoop() {
  if (!cfg.autoCal || isnan(coldRaw) || millis() < 40UL * 60 * 1000 || isnan(clim.raw)) return;
  float off = clim.raw - coldRaw;
  if (off < 1 || off > 20 || coldRaw > 30) {
    Serial.printf("Etalonnage rate (ecart %.1f) : carte pas assez froide au demarrage\n", off);
    uiAlert("Étalonnage", "Carte pas assez froide au départ : réessaie");
    cfg.autoCal = false;
  } else {
    cfg.tempOffset = roundf(off * 10) / 10;
    cfg.autoCal = false;
    clim.t = NAN;
    Serial.printf("Etalonnage reussi : correction %.1f C\n", cfg.tempOffset);
    uiAlert("Sonde étalonnée", "Correction : -" + String(cfg.tempOffset, 1) + " °C");
  }
  coldRaw = NAN;
  cfgSave();
}

static void pushHistory() {
  if (isnan(clim.t)) return;
  if (clim.nh < 96) clim.hist[clim.nh++] = clim.t;
  else { memmove(clim.hist, clim.hist + 1, sizeof(float) * 95); clim.hist[95] = clim.t; }
}

static void checkReminders() {
  if (!timeOk) return;
  time_t n = time(nullptr);
  struct tm t;
  localtime_r(&n, &t);
  int m = t.tm_hour * 60 + t.tm_min;
  if (m == lastRemMin) return;
  lastRemMin = m;
  if (m == 0) agendaRebuild();   // passage à minuit : demain devient aujourd'hui
  for (int i = 0; i < N_REM; i++) {
    Reminder& r = cfg.rem[i];
    if (!r.on || r.text.isEmpty() || r.h * 60 + r.m != m) continue;
    beep(4);
    uiAlert("Rappel", r.text);
    if (!r.daily) { r.on = false; cfgSave(); agendaRebuild(); }
  }
}

void setup() {
  Serial.begin(115200);
  // Processeur à 160 MHz : le passer à 80 MHz ne changeait presque rien à la chauffe de la sonde (mesuré)
  delay(300);
  Serial.println("\nThermo-horloge " FW_VERSION);
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  if (!board::begin()) Serial.println("Expandeur CH32 absent a 0x24 : ecran et retroeclairage non pilotables");
  board::scanI2C();
  cfgLoad();
  calBegin();

  struct tm t;
  if (board::rtcRead(t) && t.tm_year >= 125) timeOk = true;
  else { setFromBuild(t); board::rtcWrite(t); Serial.println("Horloge reglee sur l'heure de compilation, en attente d'internet"); }
  time_t now = mktime(&t);
  timeval tv = {now, 0};
  settimeofday(&tv, nullptr);
  timeOk = true;   // l'heure de compilation ou de l'horloge suffit en attendant internet

  netBegin();
  uiBegin();
  readClimate();
  pushHistory();
  lastHist = millis();
  webBegin();
  audioBegin();
  agendaRebuild();
}

void loop() {
  uint32_t ms = millis();
  if (ms - lastClimate > 10000) { lastClimate = ms; readClimate(); }
  if (ms - lastHist > 15UL * 60 * 1000) { lastHist = ms; pushHistory(); }

  static bool geoDone = false;
  if (!geoDone && netOnline()) {
    geoDone = true;
    if (cfg.city != "Lyon" && fabsf(cfg.lat - 45.7578f) < 0.0001f) {   // ville saisie au premier démarrage, hors ligne
      float la, lo; String label;
      if (geocodeCity(cfg.city, la, lo, label)) { cfg.lat = la; cfg.lon = lo; cfg.city = label; cfgSave(); netRefreshSoon(); }
    }
  }

  calLoop();
  checkReminders();
  netLoop();
  webLoop();
  audioLoop();
  uiLoop();
  delay(20);   // laisse le processeur au repos : moins de chaleur près de la sonde
}
