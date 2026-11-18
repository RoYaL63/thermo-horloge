// Moteur d'écrans : thèmes, éléments communs, écrans, veille, menus, tactile.
#include <WiFi.h>
#include "ui_int.h"
#include "fonts.h"

LGFX lcd;
static LGFX_Sprite fb(&lcd);
lgfx::LovyanGFX* d = &lcd;
static bool frameOk = false, frameReleased = false;

Pal G;
Kit KT;
uint8_t TH = 0;
uint16_t GA = 0;
bool STILL = false;
uint32_t T_MS = 0;
uint8_t uiScreen = S_HOME;

const char* const JC[7] = {"DIM", "LUN", "MAR", "MER", "JEU", "VEN", "SAM"};
const char* const MC[12] = {"JANV", "FÉV", "MARS", "AVR", "MAI", "JUIN", "JUIL", "AOÛT", "SEPT", "OCT", "NOV", "DÉC"};
static const char* const JL[7] = {"dimanche", "lundi", "mardi", "mercredi", "jeudi", "vendredi", "samedi"};
static const char* const ML[12] = {"janvier", "février", "mars", "avril", "mai", "juin", "juillet", "août", "septembre", "octobre", "novembre", "décembre"};
static const char* const ROOMS[] = {"Salon", "Chambre", "Bureau", "Cuisine", "Atelier", "Entrée"};

// ---------- Polices ----------
struct FontRes { lgfx::VLWfont f; lgfx::PointerWrapper w; };
static FontRes FR[N_FONTS];
static const uint8_t* const FDATA[N_FONTS] = {F_MONO10, F_MONO13, F_NUM18, F_NUM26, F_CLOCK, F_CLOCKXL, F_MONOCLK,
                                              F_SERIF14, F_SC14, F_CINZEL, F_CINZEL26, F_BANG16, F_BANGCLK, F_HAND16};
static const size_t FLEN[N_FONTS] = {sizeof F_MONO10, sizeof F_MONO13, sizeof F_NUM18, sizeof F_NUM26, sizeof F_CLOCK,
                                     sizeof F_CLOCKXL, sizeof F_MONOCLK, sizeof F_SERIF14, sizeof F_SC14, sizeof F_CINZEL,
                                     sizeof F_CINZEL26, sizeof F_BANG16, sizeof F_BANGCLK, sizeof F_HAND16};

// ---------- Thèmes ----------
static const uint32_t PALS[N_THEMES][9] = {
  {0x04060c, 0x5fd4ff, 0xe4f4ff, 0x6a88a3, 0x1c2c40, 0xffb347, 0xff4d5a, 0x4dff9a, 0x4aa8ff},  // Galaxie
  {0x07080a, 0xff2a1f, 0xeef0f2, 0x8b919b, 0x2a2e35, 0xff8a3a, 0xffb020, 0xff2a1f, 0x8b919b},  // Côté obscur
  {0x0a0e15, 0x7de3c8, 0xf2f6fa, 0x7a8a99, 0x243240, 0xffb347, 0xff4d5a, 0x7de3c8, 0x4aa8ff},  // Cockpit
  {0x020a05, 0x5dff8a, 0xdfffe8, 0x4f8a62, 0x123a22, 0xffd84a, 0xff5a4a, 0x5dff8a, 0x5ad1ff},  // Radar
  {0x02050a, 0x6fd0ff, 0xd6f2ff, 0x4f86a8, 0x123048, 0x9fe3ff, 0xffffff, 0x6fd0ff, 0x4a8cff},  // Hologramme
  {0x070a0d, 0x58b8f0, 0xe6f1f7, 0x7b8a94, 0x232c33, 0xff9a2a, 0xff5a3a, 0x6fe0a8, 0x58b8f0},  // Portail
  {0x030912, 0x39d0ff, 0xeaf8ff, 0x5f8aa6, 0x12304a, 0xffc24a, 0xff5a4a, 0x39d0ff, 0x7aa8ff},  // Assistant
  {0xf5f2e8, 0x1f5fa8, 0x18242f, 0x6b7a88, 0xc6d3de, 0xc0392b, 0xc0392b, 0x1f8a4c, 0x1f5fa8},  // Labo
  {0x151038, 0xffd23a, 0xffffff, 0x9a90c8, 0x3a3270, 0xff4a3a, 0xff4a3a, 0x3ee07a, 0x3aa8ff},  // Grand Prix
  {0x031a0c, 0x3dff7a, 0xb8ffcc, 0x2a9a52, 0x0d4022, 0x9dffb8, 0x3dff7a, 0x3dff7a, 0x3dff7a},  // Terminal
  {0xf3ede9, 0x1f8f86, 0x3a2a1e, 0x8a7a6a, 0xddd3c8, 0xd97a1e, 0xd9483a, 0x1f8f86, 0x3a8fd0},  // Loutre
  {0xe9dbb4, 0x7a1f2b, 0x2b1d12, 0x7a6a55, 0xc9b98f, 0xa87a22, 0x7a1f2b, 0x2f6b48, 0x1f4e7a},  // Magie
};
// forme des boutons, style des lignes, interrupteurs, barres
static const Kit KITS[N_THEMES] = {
  {SH_RECT, 0, 0, 0}, {SH_CUT, 2, 1, 1}, {SH_ROUND, 1, 0, 2}, {SH_TEXT, 3, 2, 3}, {SH_BRACKET, 0, 4, 1}, {SH_ROUND, 2, 3, 1},
  {SH_CUT, 1, 4, 4}, {SH_RECT, 0, 6, 2}, {SH_PILL, 1, 3, 1}, {SH_RECT, 3, 2, 1}, {SH_PILL, 1, 7, 4}, {SH_ROUND, 0, 4, 4}};

void uiApplyTheme() {
  TH = cfg.theme % N_THEMES;
  const uint32_t* p = PALS[TH];
  G = {rgb(p[0]), rgb(p[1]), rgb(p[2]), rgb(p[3]), rgb(p[4]), rgb(p[5]), rgb(p[6]), rgb(p[7]), rgb(p[8])};
  if (cfg.colMain[TH]) G.cy = rgb(cfg.colMain[TH] & 0xFFFFFF);
  if (cfg.colSecond[TH]) G.am = rgb(cfg.colSecond[TH] & 0xFFFFFF);
  GA = cfg.colGauge[TH] ? rgb(cfg.colGauge[TH] & 0xFFFFFF) : 0;
  KT = KITS[TH];
}

bool light() { return TH == T_LABO || TH == T_LOUTRE || TH == T_MAGIE; }
uint8_t fLab() { return TH == T_MAGIE ? FSC : F10; }
uint8_t fTxt() { return TH == T_MAGIE ? FSER : TH == T_GRANDPRIX ? FBANG : TH == T_LABO ? F13 : F13; }
uint8_t fNum() { return TH == T_MAGIE ? FCIN26 : TH == T_GRANDPRIX ? FBANG : TH == T_TERMINAL ? F13 : FN18; }
uint8_t fNumL() { return TH == T_MAGIE ? FCIN26 : FN26; }
uint8_t fClk() { return TH == T_MAGIE ? FCIN : TH == T_GRANDPRIX ? FBANGCLK : (TH == T_LABO || TH == T_TERMINAL) ? FMCLK : FCLK; }

uint16_t mix(uint16_t a, uint16_t b, float t) {
  int ar = (a >> 11) & 31, ag = (a >> 5) & 63, ab = a & 31, br = (b >> 11) & 31, bg = (b >> 5) & 63, bb = b & 31;
  return ((int)(ar + (br - ar) * t) << 11) | ((int)(ag + (bg - ag) * t) << 5) | (int)(ab + (bb - ab) * t);
}

String UP(const String& s) {
  String o;
  for (size_t i = 0; i < s.length(); i++) {
    uint8_t c = s[i];
    if (c >= 'a' && c <= 'z') o += (char)(c - 32);
    else if (c == 0xC3 && i + 1 < s.length() && (uint8_t)s[i + 1] >= 0xA0 && (uint8_t)s[i + 1] <= 0xBE && (uint8_t)s[i + 1] != 0xB7) { o += (char)c; o += (char)((uint8_t)s[++i] - 0x20); }
    else if (c == 0xC5 && i + 1 < s.length() && (uint8_t)s[i + 1] == 0x93) { o += (char)c; o += (char)0x92; i++; }
    else o += (char)c;
  }
  return o;
}
String U(const String& s) { return TH == T_MAGIE ? s : UP(s); }

static void useFont(uint8_t f) { d->setFont(&FR[f].f); }

int textW(const String& s, uint8_t f) { useFont(f); return d->textWidth(s); }

void txt(const String& s0, int x, int y, uint8_t f, uint16_t col, uint8_t align, int maxw) {
  useFont(f);
  String s = s0;
  if (maxw > 0 && d->textWidth(s) > maxw) {
    while (s.length() && d->textWidth(s + "…") > maxw) {
      int n = s.length() - 1;
      while (n > 0 && ((uint8_t)s[n] & 0xC0) == 0x80) n--;
      s.remove(n);
    }
    s += "…";
  }
  d->setTextColor(col);
  d->setTextDatum(align == 1 ? textdatum_t::baseline_center : align == 2 ? textdatum_t::baseline_right : textdatum_t::baseline_left);
  d->drawString(s, x, y);
}

void glowText(const String& s, int x, int y, uint8_t f, uint16_t col, uint16_t glow, uint8_t align) {
  if (!light()) {
    uint16_t g = mix(G.bg, glow, 0.35f);
    txt(s, x - 1, y, f, g, align); txt(s, x + 1, y, f, g, align); txt(s, x, y + 1, f, g, align);
  }
  txt(s, x, y, f, col, align);
}

void ln(int x0, int y0, int x1, int y1, uint16_t col, float w) {
  if (w <= 1.05f) d->drawLine(x0, y0, x1, y1, col);
  else d->drawWideLine(x0, y0, x1, y1, w / 2, col);
}

// ---------- Données mises en forme ----------
String f1(float v) {
  if (isnan(v)) return "--,-";
  char b[12];
  snprintf(b, sizeof b, "%.1f", v);
  for (char* p = b; *p; p++) if (*p == '.') *p = ',';
  return b;
}
String tC(float v, bool unit) { return f1(v) + (unit && TH != T_MAGIE ? "°C" : "°"); }
int nowMin() { time_t n = time(nullptr); struct tm t; localtime_r(&n, &t); return t.tm_hour * 60 + t.tm_min; }
String hm(int m) { char b[8]; snprintf(b, sizeof b, "%02d:%02d", (m / 60) % 24, m % 60); return b; }
String HM() { return hm(nowMin()); }
String DS() { time_t n = time(nullptr); struct tm t; localtime_r(&n, &t); return String(JC[t.tm_wday]) + " " + t.tm_mday + " " + MC[t.tm_mon]; }
String dateLong() { time_t n = time(nullptr); struct tm t; localtime_r(&n, &t); return String(JL[t.tm_wday]) + " " + t.tm_mday + " " + ML[t.tm_mon]; }

String skyLabel(uint8_t s, bool upper) {
  static const char* const L[] = {"ciel dégagé", "nuit claire", "éclaircies", "ciel couvert", "brouillard", "grand vent", "pluie", "orage", "neige"};
  String r = L[s % 9];
  return upper ? UP(r) : r;
}

String rainMsg() {
  if (!wx.rainOk) return "Pluie : en attente";
  for (int i = 0; i < 8; i++) if (wx.rain[i] > 0.1f) return i == 0 ? (wx.sky == W_NEIGE ? "Neige en cours" : "Pluie en cours") : String(wx.sky == W_NEIGE ? "Neige" : "Pluie") + " dans " + (i * 15) + " min";
  return "Pas de pluie d'ici 2 h";
}

String advice() {
  int nm = nowMin();
  for (int i = 0; i < agenda.n; i++) {
    const Event& e = agenda.ev[i];
    if (e.day == 0 && e.min > nm && e.min - nm <= 90) return "Dans " + String(e.min - nm) + " min : " + e.title;
  }
  if (wx.rainOk) for (int i = 1; i < 8; i++) if (wx.rain[i] > 0.1f) return rainMsg();
  if (wx.ok && !isnan(wx.fmin[0]) && wx.fmin[0] <= 0) return "Risque de gel demain matin";
  if (!isnan(clim.h) && clim.h > 65) return "Air humide, pensez à aérer";
  if (!isnan(clim.h) && clim.h < 30) return "Air sec";
  if (!wx.ok && !netPortal() && WiFi.status() != WL_CONNECTED) return "WiFi : connexion en cours";
  return "Rien à signaler";
}

uint16_t gCol() {
  if (GA) return GA;
  if (isnan(clim.t)) return G.dim;
  return clim.t < 18 ? G.bl : clim.t <= 24 ? G.gn : G.rd;
}

float moonPhase() {
  double p = fmod((time(nullptr) - 947182440.0) / 86400.0 / 29.530588853, 1.0);
  return p < 0 ? p + 1 : p;
}

// ---------- Éléments communs ----------
static uint32_t rnd(uint32_t i) { i = (i ^ 61) ^ (i >> 16); i *= 9; i ^= i >> 4; i *= 0x27d4eb2d; i ^= i >> 15; return i; }

void gBg() {
  d->fillScreen(G.bg);
  uint32_t sec = T_MS / 1000;
  switch (TH) {
    case T_GALAXIE: case T_HOLO: case T_ASSISTANT: case T_PORTAIL:
      for (int i = 0; i < 40; i++) {
        bool on = STILL || ((i * 7 + sec) % 5);
        d->drawPixel(rnd(i) % W, rnd(i + 99) % H, on ? (i % 7 ? rgb(0x9fb4d0) : rgb(0xffe2b0)) : rgb(0x28324a));
      }
      if (TH == T_ASSISTANT) for (int y = 16; y < H; y += 16) for (int x = 16; x < W; x += 16) d->drawPixel(x, y, G.line);
      if (TH == T_PORTAIL) { d->fillArc(230, 330, 108, 130, 180, 270, rgb(0x12181d)); }
      break;
    case T_OBSCUR:
      for (int r = 0; r < 6; r++) d->fillArc(120, 360, 120 + r * 25, 145 + r * 25, 180, 360, mix(G.bg, rgb(0x501008), 0.5f - r * 0.08f));
      for (int i = 0; i < 16; i++) {
        uint32_t q = (T_MS / 40 + rnd(i) % 1000) % 1000;
        d->drawPixel(rnd(i + 5) % W, H - q * H / 1000, mix(G.bg, rgb(0xff5a3a), 1.0f - q / 1000.0f));
      }
      break;
    case T_COCKPIT: d->drawRoundRect(3, 3, W - 6, H - 6, 8, G.line); break;
    case T_RADAR: for (int y = 0; y < H; y += 4) d->drawFastHLine(0, y, W, mix(G.bg, G.line, 0.5f)); break;
    case T_LABO:
      for (int v = 0; v <= H; v += 8) {
        uint16_t c = v % 40 == 0 ? G.line : mix(G.bg, G.line, 0.45f);
        if (v <= W) d->drawFastVLine(v, 0, H, c);
        d->drawFastHLine(0, v, W, c);
      }
      break;
    case T_GRANDPRIX: d->fillGradientRect(0, 0, W, H, rgb(0x2b1f6e), G.bg, lgfx::fill_style_t::vertical_linear); break;
    case T_TERMINAL:
      d->fillGradientRect(0, 0, W, H, rgb(0x06301a), rgb(0x010804), lgfx::fill_style_t::radial_center);
      for (int y = 0; y < H; y += 2) d->drawFastHLine(0, y, W, rgb(0x020c06));
      break;
    case T_LOUTRE:
      for (int k = 0; k < 3; k++) for (int x = 0; x < W; x += 3) d->drawPixel(x, 302 + k * 7 + (int)(sinf(x / 20.0f + k * 2 + (STILL ? 0 : T_MS / 1600.0f)) * 1.5f), G.line);
      break;
    case T_MAGIE:
      d->fillGradientRect(0, 0, W, H, rgb(0xefe2bd), rgb(0xd9c48f), lgfx::fill_style_t::radial_center);
      d->drawRect(5, 5, W - 10, H - 10, rgb(0x3c2612));
      d->drawRect(9, 9, W - 18, H - 18, rgb(0x8a7550));
      break;
  }
}

void gTop(const String& s0) {
  String s = U(s0);
  switch (TH) {
    case T_MAGIE:
      txt(s, 120, 31, FSC, G.cy, 1);
      ln(36, 38, 112, 38, G.dim); ln(128, 38, 204, 38, G.dim);
      d->fillTriangle(120, 34, 124, 38, 120, 42, G.dim); d->fillTriangle(120, 34, 116, 38, 120, 42, G.dim);
      return;
    case T_RADAR: txt("> " + s + ((T_MS / 500) % 2 ? " _" : ""), 10, 15, F10, G.cy); ln(8, 21, 232, 21, G.line); return;
    case T_COCKPIT: d->fillRect(8, 6, 224, 16, rgb(0x1a222c)); d->drawRect(8, 6, 224, 16, rgb(0x3a4656)); txt(s, 14, 18, F10, G.tx); break;
    case T_LOUTRE: txt(s, 12, 17, F13, G.cy); ln(10, 23, 230, 23, G.line); return;
    case T_LABO: txt(s, 12, 16, F10, G.tx); ln(10, 22, 230, 22, G.tx); return;
    case T_OBSCUR:
      txt(s, 120, 15, F10, G.cy, 1);
      d->fillTriangle(10, 10, 24, 10, 10, 16, G.cy); d->fillTriangle(230, 10, 216, 10, 230, 16, G.cy);
      ln(10, 22, 230, 22, G.line);
      return;
    case T_GRANDPRIX: {
      for (int i = 0; i < 30; i++) for (int j = 0; j < 2; j++) d->fillRect(i * 8, 4 + j * 7, 8, 7, (i + j) % 2 ? rgb(0xffffff) : rgb(0x0d0826));
      int w = min(200, textW(s, FBANG) + 28);
      d->fillRoundRect(120 - w / 2, 2, w, 20, 10, rgb(0x0d0826)); d->drawRoundRect(120 - w / 2, 2, w, 20, 10, G.cy);
      txt(s, 120, 17, FBANG, G.cy, 1, w - 14);
      return;
    }
    case T_TERMINAL: {
      int w = textW(s, F13) + 16;
      ln(6, 14, 120 - w / 2, 14, G.cy); ln(120 + w / 2, 14, 234, 14, G.cy); ln(6, 14, 6, 22, G.cy); ln(234, 14, 234, 22, G.cy);
      txt(s, 120, 18, F13, G.cy, 1, 216);
      return;
    }
    default: txt(s, 10, 15, F10, G.dim);
  }
  if (TH != T_COCKPIT) { d->drawLine(8, 21, 196, 21, G.line); d->drawLine(196, 21, 202, 25, G.line); d->drawLine(202, 25, 232, 25, G.line); }
  int bars = WiFi.status() == WL_CONNECTED ? (WiFi.RSSI() > -60 ? 4 : WiFi.RSSI() > -70 ? 3 : WiFi.RSSI() > -80 ? 2 : 1) : 0;
  for (int i = 0; i < 4; i++) d->fillRect(214 + i * 4, 15 - 3 - i * 2, 3, 3 + i * 2, i < bars ? G.cy : G.line);
}

void kShape(int x, int y, int w, int h, uint8_t sh, uint16_t stroke, uint16_t fill, bool doFill) {
  if (sh == SH_PILL || sh == SH_ROUND) {
    int r = sh == SH_PILL ? h / 2 : 5;
    if (doFill) d->fillRoundRect(x, y, w, h, r, fill);
    d->drawRoundRect(x, y, w, h, r, stroke);
  } else if (sh == SH_CUT) {
    int q = min(6, h / 3);
    if (doFill) { d->fillRect(x + q, y, w - q, h - q, fill); d->fillRect(x, y + q, w - q, h - q, fill); d->fillTriangle(x, y + q, x + q, y, x + q, y + q, fill); d->fillTriangle(x + w - q, y + h, x + w, y + h - q, x + w - q, y + h - q, fill); }
    ln(x + q, y, x + w, y, stroke); ln(x + w, y, x + w, y + h - q, stroke); ln(x + w, y + h - q, x + w - q, y + h, stroke);
    ln(x + w - q, y + h, x, y + h, stroke); ln(x, y + h, x, y + q, stroke); ln(x, y + q, x + q, y, stroke);
  } else if (sh == SH_BRACKET) {
    if (doFill) d->fillRect(x, y, w, h, fill);
    for (int k = 0; k < 4; k++) {
      int cx = k % 2 ? x + w : x, cy = k / 2 ? y + h : y, sx = k % 2 ? -1 : 1, sy = k / 2 ? -1 : 1;
      ln(cx, cy, cx + sx * 7, cy, stroke); ln(cx, cy, cx, cy + sy * 7, stroke);
    }
  } else {
    if (doFill) d->fillRect(x, y, w, h, fill);
    d->drawRect(x, y, w, h, stroke);
  }
}

void kBtn(int x, int y, int w, int h, const String& label0, bool pri, bool off) {
  String label = U(label0);
  uint8_t f = TH == T_MAGIE ? FSC : TH == T_GRANDPRIX ? FBANG : F10;
  if (KT.sh == SH_TEXT) { txt("[ " + label + " ]", x + w / 2, y + h / 2 + 4, f, pri && !off ? G.cy : off ? G.dim : G.tx, 1, w + 8); return; }
  bool solid = pri && !off && (light() || TH == T_GRANDPRIX);
  uint16_t fill = solid ? G.cy : mix(G.bg, G.cy, 0.18f);
  kShape(x, y, w, h, KT.sh, off ? G.dim : pri ? G.cy : G.dim, fill, pri && !off);
  txt(label, x + w / 2, y + h / 2 + 4, f, off ? G.dim : solid ? G.bg : G.tx, 1, w - 6);
}

void kBar(int x, int y, int w, float fr) {
  fr = constrain(fr, 0.0f, 1.0f);
  switch (KT.bar) {
    case 1: { int n = 10; float sw = w / (float)n; for (int i = 0; i < n; i++) d->fillRect(x + i * sw, y, sw - 2, 5, i < lroundf(fr * n) ? G.cy : G.line); break; }
    case 2:
      ln(x, y + 5, x + w, y + 5, G.dim);
      for (int i = 0; i <= 10; i++) ln(x + i * w / 10, y + (i % 5 ? 2 : 0), x + i * w / 10, y + 5, G.dim);
      d->fillTriangle(x + w * fr, y + 4, x + w * fr - 3, y - 2, x + w * fr + 3, y - 2, G.cy);
      break;
    case 3: { int n = lroundf(fr * 12); String s; for (int i = 0; i < 12; i++) s += i < n ? '#' : '.'; txt(s, x, y + 6, F10, G.cy); break; }
    case 4:
      d->fillRoundRect(x, y, w, 5, 2, G.line); d->fillRoundRect(x, y, max(5, (int)(w * fr)), 5, 2, G.cy);
      d->fillCircle(x + w * fr, y + 2, 4, light() ? G.cy : G.tx);
      break;
    default: d->fillRect(x, y, w, 5, G.line); d->fillRect(x, y, w * fr, 5, G.cy);
  }
}

void kTog(int x, int y, bool on) {
  int cx = x + 18, cy = y + 7;
  uint16_t col = on ? G.cy : G.dim;
  switch (KT.tg) {
    case 1: d->fillTriangle(cx, cy - 8, cx + 8, cy, cx, cy + 8, on ? G.cy : G.bg); d->fillTriangle(cx, cy - 8, cx - 8, cy, cx, cy + 8, on ? G.cy : G.bg);
            ln(cx, cy - 8, cx + 8, cy, col); ln(cx + 8, cy, cx, cy + 8, col); ln(cx, cy + 8, cx - 8, cy, col); ln(cx - 8, cy, cx, cy - 8, col); break;
    case 2: txt(on ? "[OUI]" : "[NON]", x + 36, y + 12, F10, col, 2); break;
    case 3: d->fillCircle(cx - 8, cy, 5, on ? G.line : G.am); d->fillCircle(cx + 8, cy, 5, on ? G.gn : G.line); break;
    case 4: d->drawCircle(cx, cy, 8, col); if (on) d->fillCircle(cx, cy, 4, G.cy); break;
    case 6: d->drawRect(cx - 7, cy - 7, 14, 14, G.tx); if (on) { ln(cx - 5, cy, cx - 1, cy + 5, G.am, 2); ln(cx - 1, cy + 5, cx + 8, cy - 8, G.am, 2); } break;
    case 7: d->fillRoundRect(x + 2, y - 1, 32, 17, 8, on ? G.cy : G.line); d->fillCircle(on ? x + 25 : x + 11, cy, 6, rgb(0xffffff)); break;
    default: d->drawRect(x, y, 36, 15, col); d->fillRect(on ? x + 21 : x + 3, y + 3, 13, 9, col);
  }
}

int kRow(int i, int y, int h, const String& lab, const String& val, float fr, int on) {
  int m = y + h / 2, lx = 14;
  uint16_t tc = on == 0 ? G.dim : G.tx;
  switch (KT.row) {
    case 1: kShape(10, y + 1, 220, h - 3, KT.sh == SH_PILL ? SH_ROUND : KT.sh == SH_TEXT ? SH_RECT : KT.sh, G.line, light() ? rgb(0xffffff) : mix(G.bg, G.cy, 0.06f), true); lx = 20; break;
    case 2: d->fillRect(12, m - 7, 3, 14, G.cy); ln(10, y + h - 1, 230, y + h - 1, G.line); lx = 26; break;
    case 3: txt(">", 12, m + 4, F10, G.dim); for (int x = 24; x < 230; x += 4) d->drawPixel(x, y + h - 1, G.dim); lx = 24; break;
    default: if (TH == T_MAGIE) { for (int x = 18; x < 222; x += 4) d->drawPixel(x, y + h - 1, G.dim); } else ln(10, y + h - 1, 230, y + h - 1, G.line);
  }
  int rx = KT.row == 1 ? 222 : 228;
  txt(U(lab), lx, m + 4, TH == T_MAGIE ? FSC : F13, tc, 0, 130);
  if (fr >= 0) kBar(rx - 82, m - 3, 74, fr);
  else if (val.length()) txt(TH == T_MAGIE ? val : UP(val), rx - 4, m + 4, TH == T_MAGIE ? FSER : F10, G.cy, 2, 112);
  return rx;
}

void ruler(int x0, int x1, int y, uint16_t col, bool marker) {
  for (int i = 0; i <= 24; i++) {
    int x = x0 + i * (x1 - x0) / 24, h = i % 6 == 0 ? 7 : i % 2 == 0 ? 5 : 3;
    d->drawFastVLine(x, y, h, col);
    if (i % 6 == 0) txt(String(15 + i / 2), constrain(x, x0 + 6, x1 - 6), y + 16, F10, col, 1);
  }
  if (marker && !isnan(clim.t)) {
    int x = x0 + (constrain(clim.t, 15.0f, 27.0f) - 15) / 12 * (x1 - x0);
    d->fillTriangle(x, y - 1, x - 4, y - 7, x + 4, y - 7, col);
  }
}

static void cloud(int x, int y, int s, uint16_t col, uint16_t bg) {
  d->fillCircle(x - s * 0.35f, y + s * 0.12f, s * 0.32f, col); d->fillCircle(x, y - s * 0.12f, s * 0.42f, col);
  d->fillCircle(x + s * 0.4f, y + s * 0.12f, s * 0.32f, col); d->fillRect(x - s * 0.35f, y + s * 0.1f, s * 0.75f, s * 0.34f, col);
  int k = max(1, s / 9);
  d->fillCircle(x - s * 0.35f, y + s * 0.12f, s * 0.32f - k, bg); d->fillCircle(x, y - s * 0.12f, s * 0.42f - k, bg);
  d->fillCircle(x + s * 0.4f, y + s * 0.12f, s * 0.32f - k, bg); d->fillRect(x - s * 0.35f, y + s * 0.1f, s * 0.75f, s * 0.34f - k, bg);
}

void icon(uint8_t sky, int x, int y, int s, uint16_t col, uint16_t bg) {
  switch (sky) {
    case W_SOLEIL:
      d->drawCircle(x, y, s * 0.38f, col);
      for (int i = 0; i < 8; i++) { float a = i * PI / 4; ln(x + cosf(a) * s * 0.6f, y + sinf(a) * s * 0.6f, x + cosf(a) * s * 0.85f, y + sinf(a) * s * 0.85f, col); }
      break;
    case W_LUNE: d->fillCircle(x, y, s * 0.5f, col); d->fillCircle(x + s * 0.25f, y - s * 0.12f, s * 0.42f, bg); break;
    case W_ECLAIRCIES: d->drawCircle(x - s * 0.25f, y - s * 0.25f, s * 0.3f, col); cloud(x + s * 0.1f, y + s * 0.15f, s * 0.8f, col, bg); break;
    case W_BROUILLARD: for (int i = 0; i < 3; i++) ln(x - s * 0.7f + i * 3, y - s * 0.3f + i * s * 0.3f, x + s * 0.7f - i * 3, y - s * 0.3f + i * s * 0.3f, col, 1.5f); break;
    case W_VENT: for (int i = 0; i < 3; i++) { ln(x - s * 0.7f, y - s * 0.3f + i * s * 0.3f, x + s * 0.4f, y - s * 0.3f + i * s * 0.3f, col); d->drawCircle(x + s * 0.4f, y - s * 0.4f + i * s * 0.3f, s * 0.1f + 1, col); } break;
    case W_PLUIE: case W_ORAGE: case W_NEIGE:
      cloud(x, y - s * 0.25f, s, col, bg);
      for (int i = -1; i < 2; i++) {
        if (sky == W_PLUIE) ln(x + i * s * 0.35f + 2, y + s * 0.42f, x + i * s * 0.35f - 2, y + s * 0.78f, col);
        else if (sky == W_NEIGE) d->fillCircle(x + i * s * 0.35f, y + s * 0.62f, max(1, s / 9), col);
      }
      if (sky == W_ORAGE) d->fillTriangle(x + 2, y + s * 0.3f, x - 4, y + s * 0.62f, x + 1, y + s * 0.62f, G.am), d->fillTriangle(x + 1, y + s * 0.58f, x + 5, y + s * 0.58f, x - 2, y + s * 0.95f, G.am);
      break;
    default: cloud(x, y, s, col, bg);
  }
}

void saber(int x, int y, int maxL, float fr, uint16_t col) {
  int L = max(8, (int)(maxL * constrain(fr, 0.0f, 1.0f)) + (STILL ? 0 : (int)(sinf(T_MS / 43.0f) * 0.7f)));
  int x0 = x + 46;
  d->fillRoundRect(x0 - 2, y - 7, L + 4, 14, 7, mix(G.bg, col, 0.35f));
  d->fillRoundRect(x0, y - 4, L, 8, 4, col);
  d->fillRoundRect(x0, y - 2, L - 1, 4, 2, rgb(0xffffff));
  d->fillRect(x, y - 5, 40, 10, rgb(0x9aa5b1));
  for (int i = 0; i < 4; i++) d->fillRect(x + 5 + i * 5, y - 5, 2, 10, rgb(0x565f6b));
  d->fillRect(x + 38, y - 7, 8, 14, rgb(0xc5ced8));
  d->fillRect(x + 28, y - 7, 4, 2, G.rd);
}

void moon(int x, int y, int r, float p, uint16_t lit, uint16_t dark, uint16_t edge) {
  d->fillCircle(x, y, r, dark);
  float k = cosf(2 * PI * p);
  bool wax = p < 0.5f;
  for (int yy = -r; yy <= r; yy++) {
    float hw = sqrtf(r * r - yy * yy), term = hw * k;
    int a, b;
    if (wax) { a = x + (int)term; b = x + (int)hw; if (k < 0) a = x + (int)term; }
    else { a = x - (int)hw; b = x - (int)term; }
    if (b > a) d->drawFastHLine(a, y + yy, b - a, lit);
  }
  d->drawCircle(x, y, r, edge);
}

void graph(int x0, int y0, int x1, int y1, const float* v, int n, uint16_t grid, uint16_t lab, uint16_t line, uint16_t dot) {
  float lo = 99, hi = -99;
  for (int i = 0; i < n; i++) if (!isnan(v[i])) { lo = min(lo, v[i]); hi = max(hi, v[i]); }
  if (lo > hi) return;
  lo = floorf(lo - 0.3f); hi = ceilf(hi + 0.3f);
  if (hi - lo < 2) hi = lo + 2;
  int step = hi - lo > 8 ? 2 : 1;
  auto X = [&](int i) { return x0 + i * (x1 - x0) / max(1, n - 1); };
  auto Y = [&](float q) { return (int)(y1 - (q - lo) / (hi - lo) * (y1 - y0)); };
  for (int q = lo; q <= hi; q += step) { ln(x0, Y(q), x1, Y(q), grid); txt(String(q) + "°", x0 - 4, Y(q) + 3, F10, lab, 2); }
  int px = -1, py = -1;
  for (int i = 0; i < n; i++) {
    if (isnan(v[i])) continue;
    int x = X(i), y = Y(v[i]);
    if (px >= 0) ln(px, py, x, y, line, 1.6f);
    px = x; py = y;
  }
  if (px >= 0) d->fillCircle(px, py, 3, dot);
}

static void sunArc(uint16_t cl, uint16_t cs) {
  int m = nowMin(), cx = 120, cy = 118;
  ln(24, cy, 216, cy, cl);
  for (int a = 180; a <= 360; a += 6) d->drawPixel(cx + cosf(a * PI / 180) * 86, cy + sinf(a * PI / 180) * 62, cl);
  if (m >= wx.sunrise && m <= wx.sunset) {
    float f = (m - wx.sunrise) / (float)max(1, wx.sunset - wx.sunrise);
    d->fillCircle(cx - 86 * cosf(PI * f), cy - 62 * sinf(PI * f), 6, cs);
  }
  d->fillCircle(34, cy, 2, cl); d->fillCircle(206, cy, 2, cl);
}

// ---------- Barre du bas ----------
static uint8_t carousel[N_CAROUSEL];
static int carN = 0;
static void buildCarousel() {
  carN = 0;
  for (uint8_t s = 0; s < N_CAROUSEL; s++) if (screenOn(s)) carousel[carN++] = s;
}
static int carIndex(uint8_t s) { for (int i = 0; i < carN; i++) if (carousel[i] == s) return i; return -1; }

static void navIcon(uint8_t s, int x, int y, uint16_t c) {
  switch (s) {
    case S_HOME: ln(x - 5, y, x, y - 5, c); ln(x, y - 5, x + 5, y, c); d->drawRect(x - 3, y - 1, 7, 6, c); break;
    case S_AGENDA: d->drawRect(x - 5, y - 4, 11, 9, c); ln(x - 5, y - 1, x + 5, y - 1, c); break;
    case S_RAIN: d->fillTriangle(x, y - 6, x - 4, y + 1, x + 4, y + 1, c); d->fillCircle(x, y + 2, 4, c); break;
    case S_FORE: d->drawCircle(x - 2, y - 2, 3, c); d->fillCircle(x + 2, y + 2, 3, c); d->fillCircle(x - 1, y + 3, 2, c); break;
    case S_COMFORT: d->drawRect(x - 1, y - 6, 3, 8, c); d->fillCircle(x, y + 3, 3, c); break;
    case S_SOUND: d->drawRect(x - 5, y - 2, 3, 4, c); d->fillTriangle(x - 2, y - 2, x + 1, y - 5, x + 1, y + 5, c); d->drawArc(x + 1, y, 4, 4, -45, 45, c); break;
    case S_TIMER: ln(x - 4, y - 5, x + 4, y - 5, c); ln(x - 4, y + 5, x + 4, y + 5, c); ln(x - 4, y - 5, x + 4, y + 5, c); ln(x + 4, y - 5, x - 4, y + 5, c); break;
    case S_SKY: d->fillCircle(x, y, 5, c); d->fillCircle(x + 3, y - 1, 4, G.bg); break;
    case S_TRANSIT: d->drawRoundRect(x - 4, y - 5, 9, 8, 2, c); d->fillCircle(x - 2, y + 4, 1, c); d->fillCircle(x + 3, y + 4, 1, c); break;
  }
}

static const char* const NSHORT[N_CAROUSEL] = {"Accueil", "Agenda", "Pluie", "Prévis.", "Confort", "Bruit", "Minut.", "Ciel", "Lignes"};

struct NavPos { uint8_t s; int x0, x1; };
static int navPos(NavPos* out) {
  if (cfg.nav == 2) {
    float w = min(24.0f, 224.0f / carN), x0 = 120 - carN * w / 2;
    for (int i = 0; i < carN; i++) out[i] = {carousel[i], (int)(x0 + i * w), (int)(x0 + (i + 1) * w)};
    return carN;
  }
  if (cfg.nav == 1) {
    int n = min(5, carN), k = max(0, carIndex(uiScreen)), st = constrain(k - 2, 0, max(0, carN - n));
    float w = 228.0f / n;
    for (int i = 0; i < n; i++) out[i] = {carousel[st + i], (int)(6 + i * w), (int)(6 + (i + 1) * w)};
    return n;
  }
  for (int i = 0; i < carN; i++) out[i] = {carousel[i], 120 - carN * 5 + i * 10, 120 - carN * 5 + (i + 1) * 10};
  return carN;
}

void navBar(int y) {
  if (carIndex(uiScreen) < 0) return;
  NavPos p[N_CAROUSEL];
  int n = navPos(p);
  uint16_t dim = TH == T_MAGIE ? G.dim : G.dim;
  if (cfg.nav == 0) {
    for (int i = 0; i < n; i++) d->fillCircle((p[i].x0 + p[i].x1) / 2, y, p[i].s == uiScreen ? 2 : 1, p[i].s == uiScreen ? G.cy : G.line);
    return;
  }
  ln(6, y - 8, 234, y - 8, G.line);
  for (int i = 0; i < n; i++) {
    bool a = p[i].s == uiScreen;
    int cx = (p[i].x0 + p[i].x1) / 2;
    if (a) d->fillRect(p[i].x0 + 2, y - 9, p[i].x1 - p[i].x0 - 4, 2, G.cy);
    if (cfg.nav == 2) navIcon(p[i].s, cx, y + 1, a ? G.cy : dim);
    else txt(TH == T_MAGIE ? NSHORT[p[i].s] : UP(NSHORT[p[i].s]), cx, y + 4, TH == T_MAGIE ? FSER : F10, a ? G.cy : dim, 1, p[i].x1 - p[i].x0 - 4);
  }
}

// ---------- Images ----------
void drawPngImg(const uint8_t* data, size_t len, int x, int y, float scale) { d->drawPng(data, len, x, y, 0, 0, 0, 0, scale, scale); }

// ---------- Écrans ----------
static struct { uint32_t total = 300000, remain = 300000, last = 0; bool running = false; } tmr;
static uint8_t stab = 0;
static uint32_t confirmAt = 0;
static int8_t confirmRow = -1;
static String toastS, alertT, alertX;
static uint32_t toastUntil = 0;

void uiToast(const String& s) { toastS = s; toastUntil = millis() + 2200; }
void uiAlert(const String& title, const String& text) { alertT = title; alertX = text; uiScreen = S_ALERT; }

static void scrAgenda() {
  gBg(); gTop("Agenda · aujourd'hui");
  int nm = nowMin();
  const Event* nx = nullptr;
  for (int i = 0; i < agenda.n; i++) if (agenda.ev[i].day == 0 && agenda.ev[i].min >= nm) { nx = &agenda.ev[i]; break; }
  if (!nx) for (int i = 0; i < agenda.n; i++) if (agenda.ev[i].day == 1) { nx = &agenda.ev[i]; break; }
  kShape(10, 32, 220, 58, KT.sh == SH_PILL ? SH_ROUND : KT.sh == SH_TEXT ? SH_RECT : KT.sh, G.cy, mix(G.bg, G.cy, 0.08f), true);
  if (nx) {
    int dm = nx->day * 1440 + nx->min - nm;
    String when = nx->min < 0 ? (nx->day ? "demain" : "aujourd'hui") : dm < 60 ? "dans " + String(dm) + " min" : "dans " + String(dm / 60) + " h " + (dm % 60 < 10 ? "0" : "") + String(dm % 60);
    txt(U("Prochain · " + when), 20, 48, fLab(), G.cy);
    txt(nx->title, 20, 70, TH == T_MAGIE ? FSER : FN18, G.tx, 0, 200);
    txt(U((nx->min >= 0 ? hm(nx->min) + " · " : String("")) + (nx->reminder ? "rappel" : "agenda")), 20, 84, fLab(), G.dim);
  } else {
    txt(U("Rien de prévu"), 20, 60, fTxt(), G.tx);
    txt(cfg.ics.isEmpty() ? U("Ajoute ton agenda sur la page web") : U(agenda.err.length() ? agenda.err : "Journée libre"), 20, 78, fLab(), G.dim, 0, 200);
  }
  int row = 0;
  for (int i = 0; i < agenda.n && row < 6; i++) {
    const Event& e = agenda.ev[i];
    if (e.day != 0) continue;
    int y = 98 + row * 26;
    bool past = e.min >= 0 && e.min < nm;
    txt(e.min < 0 ? "--:--" : hm(e.min), 14, y + 17, F13, past ? G.dim : G.cy);
    txt(U(e.title), 62, y + 17, fTxt(), past ? G.dim : G.tx, 0, 120);
    if (past) ln(62, y + 13, 62 + min(120, textW(U(e.title), fTxt())), y + 13, G.dim);
    txt(U(e.reminder ? "rappel" : "agenda"), 228, y + 17, F10, e.reminder ? G.am : G.dim, 2);
    ln(10, y + 25, 230, y + 25, G.line);
    row++;
  }
  for (int i = 0; i < agenda.n; i++) if (agenda.ev[i].day == 1) {
    txt(U("Demain " + (agenda.ev[i].min >= 0 ? hm(agenda.ev[i].min) : String("")) + " · " + agenda.ev[i].title), 14, 268, fLab(), G.dim, 0, 214);
    break;
  }
  navBar();
}

static void scrRain() {
  gBg(); gTop(TH == T_OBSCUR ? "Balayage orbital" : "Pluie · 2 heures");
  float sum = 0;
  for (float v : wx.rain) sum += v / 4;
  txt(U(rainMsg()), 120, 58, TH == T_MAGIE ? FSER : FN18, G.tx, 1, 220);
  txt(U(sum > 0 ? "Cumul " + f1(sum) + " mm · " + skyLabel(wx.sky, false) : "Ciel : " + skyLabel(wx.sky, false)), 120, 76, fLab(), G.dim, 1, 220);
  const int x0 = 30, x1 = 226, y0 = 96, y1 = 232;
  float bw = (x1 - x0) / 8.0f;
  for (float q : {1.0f, 2.5f}) {
    int y = y1 - q / 4 * (y1 - y0);
    for (int x = x0; x < x1; x += 5) d->drawFastHLine(x, y, 2, G.line);
    txt(U(q < 2 ? "faible" : "forte"), x1, y - 3, F10, G.dim, 2);
  }
  ln(x0, y1, x1, y1, G.dim);
  for (int i = 0; i < 8; i++) {
    float q = wx.rain[i];
    int h = q > 0 ? max(2, (int)(min(q, 4.0f) / 4 * (y1 - y0))) : 0;
    if (h) d->fillRect(x0 + i * bw + 3, y1 - h, bw - 6, h, q < 1 ? G.dim : q < 2.5f ? G.cy : G.am);
  }
  ln(x0, y0 - 6, x0, y1, G.am, 1.5f);
  const char* lb[] = {"maint.", "+30", "+1 h", "+1 h 30", "+2 h"};
  for (int i = 0; i < 5; i++) txt(U(lb[i]), x0 + i * 2 * bw, y1 + 14, F10, G.dim, i == 0 ? 0 : i == 4 ? 2 : 1);
  uint16_t cs[] = {G.dim, G.cy, G.am};
  const char* ls[] = {"faible", "modérée", "forte"};
  for (int i = 0; i < 3; i++) { d->fillRect(24 + i * 72, 265, 10, 8, cs[i]); txt(U(ls[i]), 39 + i * 72, 273, F10, G.dim); }
  txt(U(wx.rainOk ? "pas de 15 min · Open-Meteo" : "données en attente"), 120, 292, F10, G.dim, 1);
  navBar();
}

static void scrFore() {
  gBg(); gTop(TH == T_MAGIE ? "Almanach" : TH == T_OBSCUR ? "Directives · 4 cycles" : TH == T_GRANDPRIX ? "4 prochaines courses" : "Prévisions · 4 jours");
  size_t len;
  for (int i = 0; i < 4; i++) {
    int y = 30 + i * 60;
    if (TH == T_LOUTRE) { const uint8_t* im = otterImg(wx.fsky[i], len); drawPngImg(im, len, 8, y + 6, 0.40f); }
    else icon(wx.fsky[i], 28, y + 27, 14, G.cy, G.bg);
    txt(U(String(JC[wx.fwday[i]]) + " " + wx.fmday[i]), 58, y + 20, F13, G.tx);
    txt(U(skyLabel(wx.fsky[i], false)), 58, y + 34, F10, G.dim);
    String mm = isnan(wx.fmin[i]) ? "--" : String((int)lroundf(wx.fmin[i])) + "° / " + String((int)lroundf(wx.fmax[i])) + "°";
    txt(mm, 228, y + 21, fNum(), G.tx, 2);
    txt(U("Pluie " + String(wx.fpop[i]) + "%"), 228, y + 34, F10, wx.fpop[i] > 40 ? G.cy : G.dim, 2);
    d->fillRect(58, y + 44, 170, 3, G.line);
    if (!isnan(wx.fmin[i])) {
      auto X = [](float v) { return 58 + (constrain(v, -5.0f, 35.0f) + 5) / 40 * 170; };
      d->fillRect(X(wx.fmin[i]), y + 44, max(2.0f, X(wx.fmax[i]) - X(wx.fmin[i])), 3, G.am);
    }
    ln(10, y + 59, 230, y + 59, G.line);
  }
  txt(U(wx.ok ? "Source Open-Meteo · " + cfg.city : "Météo en attente du WiFi"), 120, 290, F10, G.dim, 1, 220);
  navBar();
}

static void scrComfort() {
  gBg(); gTop(TH == T_OBSCUR || TH == T_GALAXIE ? "Systèmes vitaux" : TH == T_LABO ? "Hygrométrie" : "Confort");
  float hu = isnan(clim.h) ? 0 : clim.h;
  d->fillArc(62, 82, 28, 34, 135, 405, G.line);
  d->fillArc(62, 82, 28, 34, 135, 135 + 270 * hu / 100, G.cy);
  txt(String((int)lroundf(hu)) + "%", 62, 89, fNum(), G.tx, 1);
  txt(U("Humidité"), 62, 130, fLab(), G.dim, 1);
  float lo = 99, hi = -99;
  for (int i = 0; i < clim.nh; i++) { lo = min(lo, clim.hist[i]); hi = max(hi, clim.hist[i]); }
  txt(U("Intérieur"), 118, 54, fLab(), G.dim);
  txt(tC(clim.t), 118, 84, fNumL(), G.tx);
  if (clim.nh) txt(U("min " + f1(lo) + "  max " + f1(hi)), 118, 100, F10, G.dim);
  bool ok = hu >= 35 && hu <= 65 && clim.t >= 18 && clim.t <= 24;
  kShape(118, 109, ok ? 70 : 92, 16, SH_RECT, ok ? G.gn : G.am, G.bg, false);
  txt(U(ok ? "nominal" : "à surveiller"), 124, 121, F10, ok ? G.gn : G.am);
  ln(10, 140, 230, 140, G.line);
  txt(U("Température · 24 h"), 12, 156, fLab(), G.dim);
  if (clim.nh >= 2) graph(34, 166, 224, 252, clim.hist, clim.nh, G.line, G.dim, gCol(), G.tx);
  else txt(U("Courbe en cours : un point par quart d'heure"), 120, 210, F10, G.dim, 1, 210);
  txt(U("-24 h"), 34, 266, F10, G.dim); txt(U("maint."), 224, 266, F10, G.dim, 2);
  ln(10, 276, 230, 276, G.line);
  float dew = NAN;
  if (!isnan(clim.t) && clim.h > 0) { float g = logf(clim.h / 100) + 17.62f * clim.t / (243.12f + clim.t); dew = 243.12f * g / (17.62f - g); }
  txt(U("Rosée " + f1(dew) + "° · sonde " + f1(clim.raw) + " corr -" + f1(cfg.tempOffset)), 120, 292, F10, G.am, 1, 220);
  navBar();
}

static void scrSound() {
  gBg(); gTop(TH == T_OBSCUR ? "Surveillance audio" : "Sonomètre");
  if (isnan(soundDb)) {
    txt(U("Micro indisponible"), 120, 140, fTxt(), G.tx, 1);
    txt(U("Le codec audio n'a pas répondu au démarrage"), 120, 162, F10, G.dim, 1, 220);
    navBar(); return;
  }
  auto dc = [](float q) { return q < 60 ? G.cy : q < 75 ? G.am : G.rd; };
  static const char* const REF[] = {"chuchotement", "pièce calme", "pluie fine", "conversation", "télévision forte", "aspirateur", "tondeuse"};
  int ri = constrain((int)lroundf((soundDb - 30) / 10), 0, 6);
  txt(String((int)lroundf(soundDb)), 112, 86, fClk() == FCIN ? FCIN : FCLK, G.tx, 2);
  txt("dB", 118, 86, fNum(), G.dim);
  txt(U(REF[ri]), 120, 104, fLab(), dc(soundDb), 1);
  float pk = 0, sum = 0; int n = 0;
  for (float v : soundHist) if (!isnan(v)) { pk = max(pk, v); sum += v; n++; }
  for (int i = 0; i < 30; i++) { int q = 30 + i * 2; d->fillRect(14 + i * 7.1f, 114, 5, 16, q <= soundDb ? dc(q) : G.line); }
  if (n) { int px = 14 + (constrain(pk, 30.0f, 90.0f) - 30) / 2 * 7.1f; ln(px, 111, px, 133, G.tx, 1.5f); }
  for (int q = 30; q <= 90; q += 15) txt(String(q), constrain(16 + (q - 30) / 2 * 7.1f, 20.0f, 222.0f), 146, F10, G.dim, 1);
  const int gx0 = 30, gx1 = 226, gy0 = 162, gy1 = 228;
  auto Y = [&](float q) { return (int)(gy1 - (constrain(q, 30.0f, 90.0f) - 30) / 60 * (gy1 - gy0)); };
  for (int q : {40, 60, 80}) { ln(gx0, Y(q), gx1, Y(q), G.line); txt(String(q), gx0 - 4, Y(q) + 3, F10, G.dim, 2); }
  int px = -1, py = -1;
  for (int i = 0; i < 60; i++) { if (isnan(soundHist[i])) continue; int x = gx0 + i * (gx1 - gx0) / 59, y = Y(soundHist[i]); if (px >= 0) ln(px, py, x, y, G.cy, 1.4f); px = x; py = y; }
  txt(U("-1 min"), gx0, gy1 + 12, F10, G.dim); txt(U("maint."), gx1, gy1 + 12, F10, G.dim, 2);
  txt(U("Moyenne"), 24, 260, F10, G.dim); txt(n ? String((int)lroundf(sum / n)) + " dB" : "--", 24, 278, fNum(), G.tx);
  txt(U("Pic"), 216, 260, F10, G.dim, 2); txt(n ? String((int)lroundf(pk)) + " dB" : "--", 216, 278, fNum(), dc(pk), 2);
  txt(U("micro de la carte · à étalonner"), 120, 292, F10, G.dim, 1);
  navBar();
}

static void scrTimer() {
  gBg(); gTop(TH == T_GRANDPRIX ? "Feux de départ" : TH == T_MAGIE ? "Sablier" : TH == T_OBSCUR || TH == T_GALAXIE ? "Compte à rebours" : "Minuteur");
  uint32_t rem = tmr.remain;
  bool done = rem == 0;
  float fr = tmr.total ? rem / (float)tmr.total : 0;
  uint16_t col = fr < 0.1f ? G.rd : G.cy;
  if (TH == T_GRANDPRIX) {
    d->fillRoundRect(24, 38, 192, 52, 10, rgb(0x0d0826)); d->drawRoundRect(24, 38, 192, 52, 10, G.tx);
    int lit = (int)ceilf(fr * 5);
    for (int i = 0; i < 5; i++) { d->fillCircle(46 + i * 37, 64, 13, rgb(0x2a2440)); if (done || i < lit) d->fillCircle(46 + i * 37, 64, 11, done ? G.gn : G.am); }
  } else if (TH == T_LOUTRE) {
    size_t len = 0;
    const uint8_t* im = tempsImg(len);
    drawPngImg(im, len, 4, 34, 1.0f);
  } else {
    for (int i = 0; i < 60; i++) {
      float a = -PI / 2 + i / 60.0f * 2 * PI;
      bool on = i / 60.0f < fr;
      int r1 = i % 5 ? 68 : 71;
      ln(120 + cosf(a) * 60, 122 + sinf(a) * 60, 120 + cosf(a) * r1, 122 + sinf(a) * r1, on ? col : G.line, on ? 2 : 1);
    }
  }
  int s = (rem + 999) / 1000;
  char b[8];
  snprintf(b, sizeof b, "%02d:%02d", s / 60, s % 60);
  int ty = TH == T_GRANDPRIX ? 156 : TH == T_LOUTRE ? 112 : 135, tx = TH == T_LOUTRE ? 176 : 120;
  txt(b, tx, ty, TH == T_MAGIE ? FCIN26 : TH == T_GRANDPRIX ? FBANGCLK : FN26, done && (T_MS / 500) % 2 ? G.rd : G.tx, 1);
  txt(U(done ? "terminé" : tmr.running ? "en cours" : "en pause"), 120, 212, fLab(), done ? G.rd : G.am, 1);
  kBtn(12, 226, 64, 36, "-1 min", false);
  kBtn(84, 226, 72, 36, done ? "relancer" : tmr.running ? "pause" : "lancer", true);
  kBtn(164, 226, 64, 36, "+1 min", false);
  txt(U(cfg.sounds ? "sonnerie à la fin" : "sons coupés"), 120, 290, F10, G.dim, 1);
  navBar();
}

static void scrSky() {
  gBg(); gTop(TH == T_MAGIE ? "Carte du ciel" : TH == T_LABO ? "Astronomie" : TH == T_GALAXIE || TH == T_OBSCUR ? "Carte stellaire" : "Soleil et lune");
  sunArc(G.dim, G.am);
  txt(U("lever " + hm(wx.sunrise)), 24, 136, F13, G.tx);
  txt(U("coucher " + hm(wx.sunset)), 216, 136, F13, G.tx, 2);
  int dd = wx.sunset - wx.sunrise;
  txt(U("jour " + String(dd / 60) + " h " + (dd % 60 < 10 ? "0" : "") + String(dd % 60)), 120, 152, F10, G.dim, 1);
  ln(10, 164, 230, 164, G.line);
  float p = moonPhase();
  moon(60, 218, 30, p, light() ? rgb(0xfdf6e0) : rgb(0xdfe9f2), light() ? rgb(0x8a8070) : rgb(0x111926), G.dim);
  static const char* const MN[] = {"nouvelle lune", "premier croissant", "premier quartier", "gibbeuse croissante", "pleine lune", "gibbeuse décroissante", "dernier quartier", "dernier croissant"};
  txt(U("Lune"), 106, 198, fLab(), G.dim);
  txt(U(MN[(int)lroundf(p * 8) % 8]), 106, 216, F13, G.tx, 0, 124);
  txt(U("éclairée " + String((int)lroundf((1 - cosf(2 * PI * p)) / 2 * 100)) + " %"), 106, 232, F13, G.cy);
  txt(U("pleine dans " + String((int)lroundf(fmodf(1.5f - p, 1.0f) * 29.53f)) + " j"), 106, 248, F10, G.dim);
  if (!isnan(wx.uv)) txt(U("UV max " + String((int)lroundf(wx.uv))), 106, 264, F10, wx.uv >= 6 ? G.am : G.dim);
  ln(10, 272, 230, 272, G.line);
  txt(U("ciel " + skyLabel(wx.sky, false)), 120, 290, F10, G.am, 1);
  navBar();
}

static void scrTransit() {
  gBg(); gTop("Transports · " + String(cfg.transitMode == 1 ? "Paris" : cfg.transitMode == 2 ? "Lyon" : "à régler"));
  if (!cfg.transitMode || !transit.n) {
    txt(U("Aucune ligne réglée"), 120, 120, fTxt(), G.tx, 1);
    txt(U("Sur ton téléphone : http://" + netIp()), 120, 146, F10, G.cy, 1, 220);
    txt(U("rubrique Transports : clé, arrêts, lignes"), 120, 164, F10, G.dim, 1, 220);
    if (transit.err.length()) txt(U(transit.err), 120, 190, F10, G.am, 1, 220);
    navBar(); return;
  }
  txt(U("Mes lignes"), 14, 42, fLab(), G.dim);
  for (int i = 0; i < transit.n; i++) {
    const Departure& r = transit.d[i];
    int y = 50 + i * 56;
    ln(10, y, 230, y, G.line);
    uint16_t bc = rgb(r.color);
    d->fillRoundRect(14, y + 14, 40, 24, 4, bc);
    bool darkTxt = ((r.color >> 16) & 0xFF) * 0.3 + ((r.color >> 8) & 0xFF) * 0.59 + (r.color & 0xFF) * 0.11 > 170;
    txt(r.line, 34, y + 31, r.line.length() > 3 ? F10 : FN18, darkTxt ? rgb(0x111111) : rgb(0xffffff), 1, 38);
    txt(U(r.dest.length() ? r.dest : "direction"), 62, y + 25, F10, G.tx, 0, 100);
    txt(U("depuis " + r.stop), 62, y + 38, F10, G.dim, 0, 100);
    txt(r.m1 < 0 ? "--" : r.m1 < 2 ? U("proche") : String(r.m1) + U(" min"), 228, y + 26, fNum(), r.m1 >= 0 && r.m1 < 3 ? G.am : G.tx, 2);
    if (r.m2 >= 0) txt(U("puis " + String(r.m2) + " min"), 228, y + 40, F10, G.dim, 2);
  }
  if (transit.err.length()) txt(U(transit.err), 120, 250, F10, G.am, 1, 220);
  txt(U(cfg.transitMode == 1 ? "Source Île-de-France Mobilités" : "Source TCL, données du Grand Lyon"), 120, 290, F10, G.dim, 1);
  navBar();
}

// Veille de nuit
static bool nightNow() {
  if (!cfg.nightOn || !timeOk) return false;
  int h = nowMin() / 60;
  return cfg.nightFrom > cfg.nightTo ? (h >= cfg.nightFrom || h < cfg.nightTo) : (h >= cfg.nightFrom && h < cfg.nightTo);
}

static void scrNight(bool woke) {
  d->fillScreen(rgb(0x000000));
  uint16_t nc = cfg.nightRed ? rgb(0xb0201c) : (TH == T_MAGIE ? rgb(0xc89a3a) : G.cy);
  int m = nowMin(), dx = cfg.nightShift ? (m * 7) % 11 - 5 : 0, dy = cfg.nightShift ? (m * 5) % 9 - 4 : 0;
  uint16_t c1 = mix(rgb(0), nc, 0.85f), c2 = mix(rgb(0), nc, 0.55f), c3 = mix(rgb(0), nc, 0.3f);
  if (cfg.nightStyle == 2 && !woke) { d->fillCircle(120 + dx * 3, 300 + dy, 2, c3); return; }
  if (cfg.nightStyle == 0) nightScene(c3);
  uint8_t big = fClk() == FBANGCLK ? FBANGCLK : fClk();
  if (cfg.nightStyle == 1) {
    txt(HM(), 120 + dx, 170 + dy, big == FCLK ? FCLKXL : big, c1, 1);
    txt(TH == T_MAGIE ? dateLong() : DS(), 120 + dx, 196 + dy, TH == T_MAGIE ? FSER : F10, c2, 1);
    return;
  }
  txt(HM(), 120 + dx, 104 + dy, big, c1, 1);
  txt(TH == T_MAGIE ? dateLong() : DS() + " · " + UP(cfg.room), 120 + dx, 126 + dy, TH == T_MAGIE ? FSER : F10, c2, 1);
  if (cfg.nightInfo) {
    ln(96 + dx, 146 + dy, 144 + dx, 146 + dy, c3);
    String l1 = U("Demain · " + skyLabel(wx.fsky[0], false) + " · " + (isnan(wx.fmin[0]) ? String("--") : String((int)lroundf(wx.fmin[0])) + "° / " + String((int)lroundf(wx.fmax[0])) + "°"));
    String l2 = U("Lever " + hm(wx.sunrise));
    for (int i = 0; i < agenda.n; i++) if (agenda.ev[i].day == 1 && agenda.ev[i].min >= 0) { l2 += U("  ·  1er rdv " + hm(agenda.ev[i].min)); break; }
    String l3 = U("Dedans " + f1(clim.t) + "°  ·  dehors " + f1(wx.t) + "°");
    uint8_t f = TH == T_MAGIE ? FSER : F10;
    txt(l1, 120 + dx, 170 + dy, f, c2, 1, 220); txt(l2, 120 + dx, 190 + dy, f, c2, 1, 220); txt(l3, 120 + dx, 210 + dy, f, c2, 1, 220);
  }
  txt(U("Veille jusqu'à " + String(cfg.nightTo) + " h · touche pour réveiller"), 120 + dx, 312, F10, mix(rgb(0), nc, 0.18f), 1, 220);
}

// Réglages sur l'objet (les réglages complets sont sur la page web)
struct SRow { String lab, val; float fr; };
static int settingsRows(SRow* r) {
  int n = 0;
  if (stab == 0) {
    static const char* const NS[] = {"ambiance", "horloge seule", "écran éteint"};
    static const char* const NV[] = {"points", "onglets", "icônes"};
    r[n++] = {"Thème", THEME_NAMES[TH], -1};
    r[n++] = {"Accueil", variantName(cfg.variant[TH]), -1};
    r[n++] = {"Luminosité", "", cfg.bri / 100.0f};
    r[n++] = {"Mode nuit", cfg.nightOn ? String(NS[cfg.nightStyle % 3]) : String("désactivé"), -1};
    r[n++] = {"Couleur de nuit", cfg.nightRed ? "rouge doux" : "du thème", -1};
    r[n++] = {"Barre du bas", NV[cfg.nav % 3], -1};
  } else if (stab == 1) {
    int on = 0;
    for (int s = 0; s < N_CAROUSEL; s++) on += screenOn(s);
    r[n++] = {"Pièce", cfg.room, -1};
    r[n++] = {"Ville", cfg.city, -1};
    r[n++] = {"Écrans", String(on) + " affichés", -1};
    r[n++] = {"Sons", cfg.sounds ? "activés" : "coupés", -1};
    r[n++] = {"Sonde", "corr. -" + f1(cfg.tempOffset) + "°", -1};
  } else {
    r[n++] = {"Réglages web", netIp(), -1};
    r[n++] = {"WiFi", netOnline() ? cfg.ssid : String("non connecté"), -1};
    r[n++] = {"Version", FW_VERSION, -1};
    r[n++] = {"Redémarrer", confirmRow == 3 ? "touche encore" : "", -1};
    r[n++] = {"Oublier le WiFi", confirmRow == 4 ? "touche encore" : "", -1};
    r[n++] = {"Réglages usine", confirmRow == 5 ? "touche encore" : "", -1};
  }
  return n;
}

static void scrSettings() {
  gBg(); gTop("Réglages");
  const char* tabs[] = {"Affichage", "Maison", "Système"};
  for (int i = 0; i < 3; i++) kBtn(10 + i * 74, 28, 70, 22, tabs[i], i == stab);
  SRow r[7];
  int n = settingsRows(r);
  for (int i = 0; i < n; i++) kRow(i, 56 + i * 30, 30, r[i].lab, r[i].val, r[i].fr);
  kBtn(12, 278, 88, 26, "< retour", false);
  kBtn(128, 278, 100, 26, "Page web", true);   // QR code vers la page de réglages, avec l'adresse actuelle
}

static void scrScreens() {
  gBg(); gTop("Écrans affichés");
  kRow(0, 38, 24, "Accueil", "toujours");
  for (int s = 1; s <= N_CAROUSEL; s++) {
    bool on = s < N_CAROUSEL ? screenOn(s) : cfg.nightOn;
    int y = 62 + (s - 1) * 23;
    int x = kRow(s, y, 23, s < N_CAROUSEL ? SCREEN_NAMES[s] : "Nuit automatique", "", -1, on);
    kTog(x - 46, y + 4, on);
  }
  kBtn(12, 278, 88, 26, "< retour", false);
}

static void scrInfo() {
  gBg(); gTop("Réglages complets");
  String url = "http://" + netIp() + "/";
  d->fillRect(66, 36, 108, 108, rgb(0xffffff));
  d->qrcode(url.c_str(), 70, 40, 100, 3);
  txt(U("Sur ton téléphone, même WiFi :"), 120, 166, F10, G.dim, 1);
  txt(url, 120, 186, F13, G.cy, 1, 220);
  txt("thermo-horloge.local", 120, 204, F10, G.dim, 1);
  txt(U("Ville, couleurs, agenda, rappels,"), 120, 228, F10, G.tx, 1);
  txt(U("transports, mise à jour"), 120, 242, F10, G.tx, 1);
  kBtn(12, 278, 88, 26, "< retour", false);
}

static void scrPortal() {
  gBg(); gTop("Bienvenue");
  uint8_t ws = netWpsState();
  if (ws != 0 && ws != 3) {   // WPS en cours ou réussi
    txt(U("Connexion par WPS"), 120, 60, fLab(), G.am, 1);
    if (ws == 4) {
      txt(U("Connecté à"), 120, 130, F13, G.dim, 1);
      txt(netWpsSsid(), 120, 158, FN18, G.tx, 1, 220);
      txt(U("Redémarrage…"), 120, 200, F13, G.cy, 1);
      return;
    }
    if (ws == 1) {
      txt(U("Appuie sur le bouton WPS"), 120, 120, F13, G.tx, 1);
      txt(U("de ta box internet"), 120, 140, F13, G.tx, 1);
      int left = netWpsLeft();
      d->drawArc(120, 200, 30, 27, -90, -90 + 360 * left / 120, G.cy);
      txt(String(left) + " s", 120, 206, FN18, G.tx, 1);
      txt(U("Le bouton porte souvent deux flèches"), 120, 258, F10, G.dim, 1, 220);
      txt(U("ou le mot WPS. Pas de mot de passe à taper."), 120, 272, F10, G.dim, 1, 220);
    } else {
      txt(U("Box trouvée, connexion…"), 120, 160, F13, G.tx, 1);
    }
    return;
  }
  d->fillRect(70, 32, 100, 100, rgb(0xffffff));
  d->qrcode("WIFI:T:nopass;S:Thermo-Horloge;;", 74, 36, 92, 2);
  txt(U("Premier démarrage"), 120, 148, fLab(), G.am, 1);
  const char* steps[3][2] = {{"Rejoins le réseau", "Thermo-Horloge"}, {"Dans le navigateur, tape", "http://192.168.4.1"}, {"Choisis ton WiFi", "et ta ville"}};
  for (int i = 0; i < 3; i++) {
    int y = 158 + i * 36;
    kShape(14, y, 22, 26, KT.sh == SH_TEXT || KT.sh == SH_BRACKET ? SH_RECT : KT.sh, G.cy, G.bg, false);
    txt(String(i + 1), 25, y + 19, FN18, G.cy, 1);
    txt(U(steps[i][0]), 46, y + 11, F10, G.dim);
    txt(steps[i][1], 46, y + 25, F13, G.tx);
  }
  if (ws == 3) txt(U("WPS sans réponse : réessaie"), 14, 280, F10, G.am, 0, 104);
  else txt(U("ou sans téléphone :"), 14, 280, F10, G.dim, 0, 104);
  kBtn(124, 268, 104, 30, "Bouton WPS", true);
}

static void scrAlert() {
  gBg();
  kShape(14, 70, 212, 160, KT.sh == SH_TEXT ? SH_RECT : KT.sh == SH_PILL ? SH_ROUND : KT.sh, G.cy, mix(G.bg, G.cy, 0.1f), true);
  txt(U(alertT), 120, 104, fLab(), G.cy, 1, 200);
  txt(alertX, 120, 140, TH == T_MAGIE ? FSER : FN18, G.tx, 1, 200);
  txt(HM(), 120, 176, fNum(), G.dim, 1);
  kBtn(70, 192, 100, 28, "ok", true);
}

// ---------- Dessin ----------
static uint32_t lastDraw = 0, lastTouch = 0, wakeUntil = 0;
static bool asleep = false;

static void drawFrame() {
  T_MS = millis();
  STILL = false;
  bool night = nightNow() && millis() > wakeUntil && uiScreen != S_ALERT && uiScreen != S_PORTAL;
  if (night != asleep) { asleep = night; board::backlight(night ? cfg.nightBri : cfg.bri); }
  if (night) { scrNight(false); return; }
  switch (uiScreen) {
    case S_HOME: drawHome(); navBar(); break;
    case S_AGENDA: scrAgenda(); break;
    case S_RAIN: scrRain(); break;
    case S_FORE: scrFore(); break;
    case S_COMFORT: scrComfort(); break;
    case S_SOUND: scrSound(); break;
    case S_TIMER: scrTimer(); break;
    case S_SKY: scrSky(); break;
    case S_TRANSIT: scrTransit(); break;
    case S_SETTINGS: scrSettings(); break;
    case S_SCREENS: scrScreens(); break;
    case S_INFO: scrInfo(); break;
    case S_PORTAL: scrPortal(); break;
    case S_ALERT: scrAlert(); break;
    default: drawHome();
  }
  if (millis() < toastUntil) {
    kShape(20, 134, 200, 42, SH_ROUND, G.cy, G.bg, true);
    txt(U(toastS), 120, 160, F13, G.tx, 1, 186);
  }
}

// L'image en mémoire n'est libérée qu'en dernier recours : la recréer peut échouer si la mémoire est morcelée,
// et l'écran dessiné en direct clignote. Avec ~130 Ko libres, une requête chiffrée tient sans la libérer.
void uiReleaseFrame() {
  if (frameOk && ESP.getMaxAllocHeap() < 60000) {
    Serial.printf("Memoire basse (%u) : image ecran liberee\n", ESP.getMaxAllocHeap());
    fb.deleteSprite(); frameOk = false; d = &lcd; frameReleased = true;
  }
}

void uiRestoreFrame() {
  if (!frameReleased) return;
  frameReleased = false;
  if (!frameOk && fb.createSprite(W, H)) { frameOk = true; d = &fb; }
  else Serial.println("Image ecran non recreee : affichage direct");
}

// ---------- Tactile et bouton ----------
static void go(int dir) {
  buildCarousel();
  int k = carIndex(uiScreen);
  uiScreen = carousel[k < 0 ? 0 : (k + dir + carN) % carN];
}

static void saveLater();

static void tapSettings(int x, int y) {
  if (y >= 26 && y <= 52) { stab = constrain((x - 10) / 74, 0, 2); confirmRow = -1; return; }
  if (y < 56 || y >= 266) return;
  int i = (y - 56) / 30;
  if (stab == 0) {
    if (i == 0) { cfg.theme = (cfg.theme + 1) % N_THEMES; uiApplyTheme(); }
    else if (i == 1) cfg.variant[TH] = (cfg.variant[TH] + 1) % variantCount();
    else if (i == 2) { cfg.bri = cfg.bri >= 100 ? 30 : cfg.bri >= 70 ? 100 : cfg.bri >= 50 ? 70 : 50; board::backlight(cfg.bri); }
    else if (i == 3) { if (!cfg.nightOn) { cfg.nightOn = true; cfg.nightStyle = 0; } else if (cfg.nightStyle < 2) cfg.nightStyle++; else cfg.nightOn = false; }
    else if (i == 4) cfg.nightRed = !cfg.nightRed;
    else if (i == 5) cfg.nav = (cfg.nav + 1) % 3;
  } else if (stab == 1) {
    if (i == 0) { int k = 0; for (int j = 0; j < 6; j++) if (cfg.room == ROOMS[j]) k = j + 1; cfg.room = ROOMS[k % 6]; }
    else if (i == 1 || i == 4) uiToast("À changer sur la page web");
    else if (i == 2) uiScreen = S_SCREENS;
    else if (i == 3) cfg.sounds = !cfg.sounds;
  } else {
    if (i == 0) uiScreen = S_INFO;
    else if (i >= 3 && i <= 5) {
      if (confirmRow == i && millis() - confirmAt < 4000) {
        if (i == 3) { cfgSave(); ESP.restart(); }
        if (i == 4) { cfgForgetWifi(); ESP.restart(); }
        if (i == 5) { cfgFactory(); ESP.restart(); }
      }
      confirmRow = i; confirmAt = millis();
    }
  }
  saveLater();
}

static uint32_t saveAt = 0;
static void saveLater() { saveAt = millis() + 3000; }

static void timerTap(int x) {
  if (x >= 84 && x <= 156) {
    if (tmr.remain == 0) { tmr.remain = tmr.total; tmr.running = true; }
    else tmr.running = !tmr.running;
    tmr.last = millis();
  } else {
    int dm = x < 84 ? -60000 : 60000;
    if (tmr.running) { tmr.remain = max<int32_t>(1000, (int32_t)tmr.remain + dm); tmr.total = max(tmr.total, tmr.remain); }
    else { int32_t v = constrain(((int32_t)tmr.remain + 30000) / 60000 * 60000 + dm, 60000, 5940000); tmr.remain = tmr.total = v; }
  }
}

static void onTap(int x, int y) {
  lastDraw = 0;   // redessin immédiat après un toucher
  lastTouch = millis();
  if (asleep) { wakeUntil = millis() + (cfg.nightStyle == 2 ? 8000 : 60000); if (cfg.nightStyle != 2) uiScreen = S_HOME; board::backlight(40); asleep = false; return; }
  if (uiScreen == S_ALERT) { uiScreen = S_HOME; return; }
  if (uiScreen == S_PORTAL) { if (x >= 120 && y >= 262 && (netWpsState() == 0 || netWpsState() == 3)) netWpsStart(); return; }
  bool back = x < 104 && y >= 274;
  if (uiScreen == S_SETTINGS) { if (back) { uiScreen = S_HOME; cfgSave(); } else if (x >= 124 && y >= 274) uiScreen = S_INFO; else tapSettings(x, y); return; }
  if (uiScreen == S_SCREENS || uiScreen == S_INFO) {
    if (back) { uiScreen = S_SETTINGS; return; }
    if (uiScreen == S_SCREENS && y >= 62) {
      int s = (y - 62) / 23 + 1;
      if (s < N_CAROUSEL) cfg.screens ^= (1 << s);
      else if (s == N_CAROUSEL) cfg.nightOn = !cfg.nightOn;
      buildCarousel(); saveLater();
    }
    return;
  }
  if (y >= 294 && cfg.nav != 0) {
    NavPos p[N_CAROUSEL];
    int n = navPos(p);
    for (int i = 0; i < n; i++) if (x >= p[i].x0 && x < p[i].x1) { uiScreen = p[i].s; return; }
  }
  if (uiScreen == S_TIMER && y >= 222 && y <= 266) { timerTap(x); return; }
  go(x < 120 ? -1 : 1);
}

static void touchPoll() {
  static bool down = false, longFired = false;
  static uint32_t t0 = 0;
  static int16_t sx, sy;
  static uint32_t lastPoll = 0;
  if (millis() - lastPoll < 25) return;
  lastPoll = millis();
  int16_t x, y;
  bool now = board::touchRead(x, y);
  if (now) {
    if (cfg.touchSwap) { int16_t t = x; x = y; y = t; }
    if (cfg.touchInvX) x = W - 1 - x;
    if (cfg.touchInvY) y = H - 1 - y;
    if (!down) { down = true; longFired = false; t0 = millis(); sx = x; sy = y; Serial.printf("Toucher x=%d y=%d\n", x, y); }
    if (!longFired && millis() - t0 > 900 && uiScreen < N_CAROUSEL && !asleep) { longFired = true; uiScreen = S_SETTINGS; stab = 0; lastTouch = millis(); lastDraw = 0; }
  } else if (down) {
    down = false;
    if (!longFired && millis() - t0 < 900) onTap(sx, sy);
  }
}

static void bootPoll() {
  static bool down = false;
  static uint32_t t0 = 0;
  bool now = digitalRead(pin::BOOT) == LOW;
  if (now && !down) { down = true; t0 = millis(); }
  if (!now && down) {
    down = false;
    lastTouch = millis();
    lastDraw = 0;
    if (asleep) { onTap(120, 160); return; }
    if (uiScreen == S_PORTAL) { netWpsStart(); return; }   // bouton BOOT au premier démarrage : WPS
    if (millis() - t0 > 1500) { uiScreen = S_SETTINGS; stab = 0; }
    else if (uiScreen < N_CAROUSEL) go(1);
    else uiScreen = S_HOME;
  }
}

// ---------- Boucle ----------
void uiBegin() {
  pinMode(pin::BOOT, INPUT_PULLUP);
  lcd.init();
  lcd.setRotation(0);
  lcd.fillScreen(0);
  for (int i = 0; i < N_FONTS; i++) { FR[i].w.set(FDATA[i], FLEN[i]); FR[i].f.loadFont(&FR[i].w); }
  uiApplyTheme();
  buildCarousel();
  frameOk = fb.createSprite(W, H) != nullptr;
  d = frameOk ? (lgfx::LovyanGFX*)&fb : (lgfx::LovyanGFX*)&lcd;
  Serial.printf("Image en memoire : %s, memoire libre %u\n", frameOk ? "oui" : "non", ESP.getFreeHeap());
  if (netPortal()) uiScreen = S_PORTAL;
  board::backlight(cfg.bri);
}

void uiLoop() {
  touchPoll();
  bootPoll();
  if (tmr.running) {
    uint32_t dt = millis() - tmr.last;
    tmr.last = millis();
    tmr.remain = tmr.remain > dt ? tmr.remain - dt : 0;
    if (!tmr.remain) { tmr.running = false; beep(6); uiAlert("Minuteur", "Temps écoulé"); }
  }
  if (saveAt && millis() > saveAt) { saveAt = 0; cfgSave(); }
  if (uiScreen >= S_SETTINGS && uiScreen != S_PORTAL && uiScreen != S_ALERT && millis() - lastTouch > 60000) { uiScreen = S_HOME; cfgSave(); }
  if (!frameOk && !frameReleased) {
    static uint32_t retry = 0;
    if (millis() - retry > 10000) { retry = millis(); uiRestoreFrame(); }
  }
  uint32_t period = asleep ? 1000 : 200;   // 5 images par seconde (mesuré : ralentir ne réduisait la chauffe que de quelques dixièmes)
  if (millis() - lastDraw < period) return;
  lastDraw = millis();
  buildCarousel();
  drawFrame();
  if (frameOk) fb.pushSprite(0, 0);
}
