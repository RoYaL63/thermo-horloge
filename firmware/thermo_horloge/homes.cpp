// Accueils des thèmes (dessins originaux, repris de la maquette), variantes Grande heure et Tableau de bord, scènes de nuit.
#include "ui_int.h"
#include "images.h"

static uint32_t rnd(uint32_t i) { i = (i ^ 61) ^ (i >> 16); i *= 9; i ^= i >> 4; i *= 0x27d4eb2d; i ^= i >> 15; return i; }
static float tt() { return T_MS / 1000.0f; }
static String RM() { return UP(cfg.room); }

const uint8_t* otterImg(uint8_t sky, size_t& len) {
  switch (sky) {
    case W_SOLEIL: len = sizeof IMG_W_SOLEIL; return IMG_W_SOLEIL;
    case W_LUNE: len = sizeof IMG_W_LUNE; return IMG_W_LUNE;
    case W_ECLAIRCIES: len = sizeof IMG_W_ECLAIRCIES; return IMG_W_ECLAIRCIES;
    case W_BROUILLARD: len = sizeof IMG_W_BROUILLARD; return IMG_W_BROUILLARD;
    case W_VENT: len = sizeof IMG_W_VENT; return IMG_W_VENT;
    case W_PLUIE: len = sizeof IMG_W_PLUIE; return IMG_W_PLUIE;
    case W_ORAGE: len = sizeof IMG_W_ORAGE; return IMG_W_ORAGE;
    case W_NEIGE: len = sizeof IMG_W_NEIGE; return IMG_W_NEIGE;
    default: len = sizeof IMG_W_NUAGE; return IMG_W_NUAGE;
  }
}
const uint8_t* tempsImg(size_t& len) { len = sizeof IMG_TEMPS; return IMG_TEMPS; }
const uint8_t* zenImg(size_t& len) { len = sizeof IMG_ZEN; return IMG_ZEN; }

uint8_t variantCount() { return 3; }
String variantName(uint8_t v) { static const char* const N[] = {"classique", "grande heure", "tableau de bord"}; return N[v % 3]; }

static String status() { return isnan(clim.t) ? "mesure…" : clim.t < 18 ? "frais" : clim.t <= 24 ? "confort" : "chaud"; }
static String humS() { return isnan(clim.h) ? "--" : String((int)lroundf(clim.h)); }
static int secNow() { time_t n = time(nullptr); struct tm t; localtime_r(&n, &t); return t.tm_sec; }

// Texte avec contour sombre (Grand Prix)
static void otx(const String& s, int x, int y, uint8_t f, uint16_t fill) {
  uint16_t k = rgb(0x0d0826);
  for (int dx = -2; dx <= 2; dx += 2) for (int dy = -2; dy <= 2; dy += 2) if (dx || dy) txt(s, x + dx, y + dy, f, k, 1);
  txt(s, x, y, f, fill, 1);
}

static void planet(int x, int y, int r, float T) {
  uint32_t pal[3];
  if (T < 5) { pal[0] = 0xbfe6ff; pal[1] = 0x6fa8d6; pal[2] = 0xeaf6ff; }
  else if (T < 20) { pal[0] = 0x4fb3a1; pal[1] = 0x2c6f8f; pal[2] = 0xa8e0c8; }
  else if (T < 30) { pal[0] = 0xe0a458; pal[1] = 0xb06a2c; pal[2] = 0xf3d19a; }
  else { pal[0] = 0xff7a3d; pal[1] = 0x8a1f14; pal[2] = 0xffc46b; }
  d->fillCircle(x, y, r, rgb(pal[1]));
  for (int i = 0; i < 5; i++) {
    int yy = y - r + (i + 0.5f) * r * 0.4f;
    int hw = sqrtf(max(0.0f, (float)(r * r - (yy - y) * (yy - y))));
    int sh = STILL ? 0 : (int)(sinf(tt() * 0.3f + i * 1.7f) * 4);
    d->fillRect(x - hw + sh / 2, yy - 1, hw * 2 - abs(sh), 2 + (i % 2), rgb(i % 2 ? pal[0] : pal[2]));
  }
  d->fillArc(x, y, r - 5, r, 20, 160, mix(rgb(pal[1]), rgb(0x00000c), 0.6f));
  d->drawEllipse(x, y, r * 1.5f, r * 0.34f, rgb(0xb4dcff));
  d->fillArc(x, y, 0, r, 200, 340, rgb(pal[1]));
  for (int i = 0; i < 2; i++) { int yy = y - r + (i * 2 + 0.5f) * r * 0.4f; int hw = sqrtf(max(0.0f, (float)(r * r - (yy - y) * (yy - y)))); d->fillRect(x - hw, yy - 1, hw * 2, 2, rgb(pal[2])); }
}

// ---------- Accueils classiques ----------
static void homeGalaxie() {
  gBg(); gTop("Secteur " + cfg.room);
  glowText(HM(), 120, 78, FCLK, G.tx, G.cy);
  int s = secNow();
  for (int i = 0; i < 60; i++) d->fillRect(30 + i * 3, 87, 2, 3, i <= s ? G.cy : G.line);
  txt(DS(), 120, 106, F10, G.dim, 1);
  ln(10, 117, 230, 117, G.line);
  uint16_t col = gCol();
  txt(UP("Intérieur"), 12, 133, F10, G.dim);
  txt(UP(status() + " · hum " + humS() + "%"), 12, 146, F10, col);
  txt(tC(clim.t), 228, 145, FN26, G.tx, 2);
  saber(12, 162, 170, (clim.t - 15) / 12, col);
  ruler(58, 228, 171, G.dim, false);
  ln(10, 194, 230, 194, G.line);
  planet(46, 234, 24, isnan(wx.t) ? 12 : wx.t);
  txt(UP("Extérieur"), 92, 212, F10, G.dim);
  txt(tC(wx.t), 92, 242, FN26, G.tx);
  txt(UP(skyLabel(wx.sky, false)), 92, 257, F10, G.am);
  float T = isnan(wx.t) ? 12 : wx.t;
  txt(T < 5 ? "MONDE GLACÉ" : T < 20 ? "MONDE TEMPÉRÉ" : T < 30 ? "MONDE ARIDE" : "MONDE DE LAVE", 92, 269, F10, G.dim);
  ln(10, 278, 230, 278, G.line); ln(10, 298, 230, 298, G.line);
  String msg = "TRANSMISSION /// " + UP(advice()) + " /// HUMIDITÉ " + humS() + " % /// LEVER " + hm(wx.sunrise) + " · COUCHER " + hm(wx.sunset) + " ///";
  int w = textW(msg, F10) + 230;
  d->setClipRect(10, 279, 220, 19);
  txt(msg, 232 - (int)(tt() * 30) % w, 292, F10, G.am);
  d->clearClipRect();
}

static void homeObscur() {
  gBg(); gTop("Holocron · " + cfg.room);
  const int cx = 120, cy = 116;
  float rot = tt() * 0.5f;
  for (int i = 0; i < 28; i++) {
    float a = -rot * 0.4f + i / 28.0f * 2 * PI;
    int gx = cx + cosf(a) * 78, gy = cy + sinf(a) * 78;
    for (int k = 0; k < 3; k++) { uint32_t s = i * 5 + k * 7 + 300; ln(gx + (int)(rnd(s) % 8) - 4, gy + (int)(rnd(s + 1) % 9) - 4, gx + (int)(rnd(s + 2) % 8) - 4, gy + (int)(rnd(s + 3) % 9) - 4, rgb(0x6a1a14)); }
  }
  d->drawCircle(cx, cy, 70, rgb(0x3a0d0a)); d->drawCircle(cx, cy, 86, rgb(0x3a0d0a));
  float V[5][3] = {{0, -1.15f, 0}, {-1, .75f, -1}, {1, .75f, -1}, {1, .75f, 1}, {-1, .75f, 1}};
  int P[5][2];
  for (int i = 0; i < 5; i++) {
    float x = V[i][0], y = V[i][1], z = V[i][2];
    float X = x * cosf(rot) - z * sinf(rot), Z = x * sinf(rot) + z * cosf(rot);
    float Y = y * cosf(0.3f) - Z * sinf(0.3f), Z2 = y * sinf(0.3f) + Z * cosf(0.3f), k = 40 / (1 + Z2 * 0.12f);
    P[i][0] = cx + X * k; P[i][1] = cy + Y * k;
  }
  const int E[8][2] = {{0, 1}, {0, 2}, {0, 3}, {0, 4}, {1, 2}, {2, 3}, {3, 4}, {4, 1}};
  for (auto& e : E) ln(P[e[0]][0], P[e[0]][1], P[e[1]][0], P[e[1]][1], mix(G.bg, G.cy, 0.35f), 4);
  for (auto& e : E) ln(P[e[0]][0], P[e[0]][1], P[e[1]][0], P[e[1]][1], rgb(0xff4a3a), 1.5f);
  d->fillCircle(cx, cy + 6, 4, rgb(0xffd0c8));
  glowText(HM(), 120, 234, FN26 == FN26 ? FCLK : FCLK, G.tx, G.cy);
  txt(DS(), 120, 250, F10, G.dim, 1);
  txt(UP(f1(clim.t) + "° dedans   ·   " + f1(wx.t) + "° dehors"), 120, 272, F10, G.tx, 1);
  txt(isnan(clim.t) ? "" : clim.t < 18 ? "LE FROID ÉPROUVE TA VOLONTÉ." : clim.t <= 24 ? "TON ÉQUILIBRE EST MAÎTRISÉ." : "LA CHALEUR NOURRIT TA COLÈRE.", 120, 290, F10, G.cy, 1, 220);
}

static void dial(int x, int y, int r, float v, float lo, float hi, float mn, float mj, uint16_t col, const String& lab, const String& val) {
  d->fillCircle(x, y, r, rgb(0x070b12)); d->drawCircle(x, y, r, rgb(0x2c3a4c));
  auto A = [&](float q) { return 0.75f * PI + (constrain(q, lo, hi) - lo) / (hi - lo) * 1.5f * PI; };
  for (float q = lo; q <= hi + 0.001f; q += mn) {
    float a = A(q); bool M = fabsf(q / mj - roundf(q / mj)) < 0.01f; int ri = r - (M ? 8 : 4);
    ln(x + cosf(a) * ri, y + sinf(a) * ri, x + cosf(a) * (r - 2), y + sinf(a) * (r - 2), M ? G.tx : G.dim);
  }
  float a = A(isnan(v) ? lo : v);
  ln(x, y, x + cosf(a) * (r - 10), y + sinf(a) * (r - 10), col, 2);
  d->fillCircle(x, y, 2, col);
  txt(val, x, y + 18, FN18, G.tx, 1);
  txt(lab, x, y + 28, F10, G.dim, 1);
}

static void homeCockpit() {
  d->fillScreen(rgb(0x000000));
  for (int i = 0; i < 70; i++) {
    float a = (rnd(i) % 628) / 100.0f, q = fmodf(tt() * (0.25f + (rnd(i + 40) % 50) / 100.0f) + (rnd(i + 80) % 100) / 100.0f, 1.0f);
    float r0 = 14 + q * q * 190, r1 = r0 + 4 + q * 46;
    ln(120 + cosf(a) * r0, 112 + sinf(a) * r0, 120 + cosf(a) * r1, 112 + sinf(a) * r1, mix(rgb(0), rgb(0xcfe6ff), 0.2f + q * 0.8f));
  }
  // cadre octogonal de la verrière
  uint16_t fr = rgb(0x0a0e15);
  d->fillTriangle(0, 0, 40, 14, 0, 60, fr); d->fillRect(0, 0, 40, 14, fr); d->fillRect(0, 0, 8, 210, fr);
  d->fillTriangle(240, 0, 200, 14, 240, 60, fr); d->fillRect(200, 0, 40, 14, fr); d->fillRect(232, 0, 8, 210, fr);
  d->fillTriangle(0, 170, 40, 204, 0, 204, fr); d->fillTriangle(240, 170, 200, 204, 240, 204, fr);
  d->fillRect(0, 204, 240, 116, fr);
  const int oc[9][2] = {{40, 14}, {200, 14}, {232, 60}, {232, 170}, {200, 204}, {40, 204}, {8, 170}, {8, 60}, {40, 14}};
  for (int i = 0; i < 8; i++) ln(oc[i][0], oc[i][1], oc[i + 1][0], oc[i + 1][1], rgb(0x3a4656), 2);
  glowText(HM(), 120, 122, FCLK, G.tx, G.cy);
  txt(DS(), 120, 146, F10, G.tx, 1);
  txt(UP("Cap " + cfg.room + " · " + skyLabel(wx.sky, false)), 120, 190, F10, G.am, 1, 200);
  ln(0, 212, 240, 212, rgb(0x1f3a4c));
  dial(56, 262, 38, clim.t, 15, 27, 0.5f, 3, gCol(), "INTÉRIEUR", f1(clim.t) + "°");
  dial(184, 262, 38, wx.t, -10, 40, 2.5f, 10, G.am, "EXTÉRIEUR", f1(wx.t) + "°");
  txt("HUM", 120, 228, F10, G.dim, 1);
  d->drawRect(114, 234, 12, 56, rgb(0x2c3a4c));
  float h = isnan(clim.h) ? 0 : clim.h / 100;
  d->fillRect(116, 289 - h * 53, 8, h * 53, G.cy);
  txt(humS() + "%", 120, 304, F10, G.tx, 1);
}

static void homeRadar() {
  gBg(); gTop("Radar · " + cfg.room);
  const int x = 120, y = 98, r = 64;
  float a = tt() * 1.2f;
  int nb = wx.sky == W_PLUIE ? 12 : wx.sky == W_ORAGE ? 14 : wx.sky == W_NEIGE ? 9 : wx.sky == W_NUAGE ? 5 : 2;
  for (int q : {r, r * 2 / 3, r / 3}) d->drawCircle(x, y, q, G.line);
  ln(x - r, y, x + r, y, G.line); ln(x, y - r, x, y + r, G.line);
  for (int k = 0; k < 10; k++) { float a0 = a - 1.0f + k * 0.1f; d->fillArc(x, y, 0, r, a0 * 180 / PI, (a0 + 0.11f) * 180 / PI, mix(G.bg, G.cy, 0.03f + k * 0.03f)); }
  ln(x, y, x + cosf(a) * r, y + sinf(a) * r, G.cy, 1.5f);
  for (int i = 0; i < nb; i++) {
    float ba = (rnd(i + 300) % 628) / 100.0f, br = (0.25f + (rnd(i + 320) % 70) / 100.0f) * r;
    float dd = fmodf(fmodf(a - ba, 2 * PI) + 2 * PI, 2 * PI);
    d->fillCircle(x + cosf(ba) * br, y + sinf(ba) * br, 2, mix(G.bg, G.tx, max(0.12f, 1 - dd / 4)));
  }
  d->drawCircle(x, y, r, G.cy);
  txt(String(nb) + " ÉCHOS · " + UP(skyLabel(wx.sky, false)), x, y + r + 15, F10, G.am, 1);
  glowText(HM(), 120, 220, FCLK, G.tx, G.cy);
  txt(DS(), 120, 234, F10, G.dim, 1);
  ln(10, 241, 230, 241, G.line);
  txt("INT", 12, 259, F10, G.dim); txt(tC(clim.t), 38, 261, FN18, G.tx);
  txt("EXT", 126, 259, F10, G.dim); txt(tC(wx.t), 228, 261, FN18, G.tx, 2);
  ruler(14, 226, 274, G.cy, true);
}

static void homeHolo() {
  d->fillScreen(G.bg);
  for (int y = 36; y < 290; y += 2) { float f = (290 - y) / 254.0f; int hw = 30 + (1 - f) * 0 + f * 82; d->drawFastHLine(120 - hw, y, hw * 2, mix(G.bg, G.cy, 0.18f * (1 - f) + 0.03f)); }
  bool flick = !STILL && (rnd(T_MS / 60) % 9) == 0;
  uint16_t b = flick ? G.dim : G.cy;
  glowText(HM(), 120, 100, FCLK, flick ? G.dim : G.tx, G.cy);
  txt(DS(), 120, 122, F10, b, 1);
  int cx = 120, cy = 182, r = 34;
  d->drawCircle(cx, cy, r, b);
  for (float k : {-0.6f, 0.0f, 0.6f}) { float rx = r * sqrtf(1 - k * k); d->drawEllipse(cx, cy + k * r, rx, rx * 0.2f, b); }
  for (int j = 0; j < 3; j++) d->drawEllipse(cx, cy, max(1.0f, r * fabsf(cosf(tt() * 0.6f + j * PI / 3))), r, b);
  txt("INT", 14, 172, F10, b); txt(f1(clim.t) + "°", 14, 194, FN18, G.tx);
  txt("EXT", 226, 172, F10, b, 2); txt(f1(wx.t) + "°", 226, 194, FN18, G.tx, 2);
  txt(UP(skyLabel(wx.sky, false)) + " · HUM " + humS() + "%", 120, 236, F10, b, 1);
  ruler(36, 204, 252, b, true);
  for (int y = 36; y < 290; y += 3) d->drawFastHLine(0, y, W, mix(G.bg, rgb(0), 0.4f));
  d->fillEllipse(120, 296, 50, 9, rgb(0x1c2530)); d->drawEllipse(120, 296, 50, 9, rgb(0x3a4656)); d->fillEllipse(120, 294, 28, 4, G.cy);
  txt(UP("Projection · " + cfg.room), 120, 316, F10, G.dim, 1);
}

static void homePortail() {
  gBg(); gTop("Portail · " + cfg.room);
  const int x = 120, y = 114;
  int sec = secNow(), lit = STILL ? 5 : sec / 8 + 1;
  d->fillGradientRect(x - 56, y - 56, 112, 112, rgb(0x8fd2f7), rgb(0x0a3560), lgfx::fill_style_t::radial_center);
  d->fillArc(x, y, 56, 84, 0, 360, rgb(0x2c343a));
  d->drawCircle(x, y, 80, rgb(0x6b7882)); d->drawCircle(x, y, 58, rgb(0x6b7882));
  float rot = tt() * 0.12f;
  for (int i = 0; i < 24; i++) {
    float a = rot + i / 24.0f * 2 * PI;
    int gx = x + cosf(a) * 66, gy = y + sinf(a) * 66;
    for (int k = 0; k < 3; k++) { uint32_t s = i * 7 + k * 3 + 900; ln(gx + (int)(rnd(s) % 7) - 3, gy + (int)(rnd(s + 1) % 8) - 4, gx + (int)(rnd(s + 2) % 7) - 3, gy + (int)(rnd(s + 3) % 8) - 4, rgb(0xaab6be)); }
  }
  for (int i = 0; i < 8; i++) {
    float a = -PI / 2 + i / 8.0f * 2 * PI, ca = cosf(a), sa = sinf(a), px = -sa, py = ca;
    int cxx = x + ca * 78, cyy = y + sa * 78;
    uint16_t c = i < lit ? G.am : rgb(0x2c343a);
    d->fillTriangle(cxx - ca * 6 + px * 7, cyy - sa * 6 + py * 7, cxx - ca * 6 - px * 7, cyy - sa * 6 - py * 7, cxx + ca * 6, cyy + sa * 6, c);
  }
  for (int k = 0; k < 4; k++) { float rr = fmodf(tt() * 7 + k * 14.5f, 56); d->drawEllipse(x, y, rr, rr * 0.96f, mix(rgb(0x2f8fd6), rgb(0xffffff), 0.5f * (1 - rr / 56))); }
  d->drawCircle(x, y, 80, rgb(0x6b7882));
  otx(HM(), 120, 124, FCLK, rgb(0xffffff));
  txt(DS(), 120, 141, F10, rgb(0xe6f6ff), 1);
  ln(10, 204, 230, 204, G.line);
  txt(UP("Intérieur"), 12, 218, F10, G.dim); txt(UP(status() + " · hum " + humS() + "%"), 12, 231, F10, gCol());
  txt(tC(clim.t), 228, 227, FN26, G.tx, 2);
  ruler(14, 226, 242, G.cy, true);
  ln(10, 266, 230, 266, G.line);
  txt(UP("Extérieur"), 12, 281, F10, G.dim); txt(UP(skyLabel(wx.sky, false)), 12, 294, F10, G.am);
  txt(tC(wx.t), 228, 293, FN26, G.tx, 2);
}

static void homeAssistant() {
  gBg(); gTop("Assistant · " + cfg.room);
  const int x = 120, y = 112;
  int sec = secNow();
  float t = tt();
  for (int i = 0; i < 60; i++) { float a = -PI / 2 + i / 60.0f * 2 * PI; bool on = i == sec; int r0 = on ? 77 : 81; ln(x + cosf(a) * r0, y + sinf(a) * r0, x + cosf(a) * 85, y + sinf(a) * 85, on ? G.am : i % 5 ? G.line : G.dim, on ? 2 : 1); }
  for (int k = 0; k < 3; k++) { float a0 = t * 0.35f + k * 2 * PI / 3; d->fillArc(x, y, 71, 75, a0 * 180 / PI, (a0 + 1.5f) * 180 / PI, G.cy); }
  for (int k = 0; k < 4; k++) { float a0 = -t * 0.55f + k * PI / 2; d->fillArc(x, y, 64, 65, a0 * 180 / PI, (a0 + 0.9f) * 180 / PI, G.tx); }
  for (int a = 0; a < 360; a += 6) d->drawPixel(x + cosf(a * PI / 180) * 59.5f, y + sinf(a * PI / 180) * 59.5f, G.dim);
  float fr = isnan(clim.t) ? 0 : constrain((clim.t - 15) / 12, 0.01f, 1.0f);
  d->fillArc(x, y, 48, 53, 135, 405, G.line);
  d->fillArc(x, y, 48, 53, 135, 135 + 270 * fr, gCol());
  for (int i = 0; i <= 24; i++) { float a = (135 + i * 270 / 24.0f) * PI / 180; int r1 = i % 6 == 0 ? 40 : i % 2 == 0 ? 43 : 45; ln(x + cosf(a) * r1, y + sinf(a) * r1, x + cosf(a) * 47, y + sinf(a) * 47, i % 6 == 0 ? G.tx : G.dim); }
  txt("EN LIGNE", x, y - 18, F10, G.am, 1);
  glowText(HM(), x, y + 9, FN26, G.tx, G.cy);
  txt(DS(), x, y + 23, F10, G.dim, 1);
  for (int k = 0; k < 2; k++) {
    int px = 10 + k * 114;
    kShape(px, 208, 106, 58, SH_CUT, G.cy, mix(G.bg, G.cy, 0.07f), true);
    txt(k ? "EXTÉRIEUR" : "INTÉRIEUR", px + 8, 222, F10, G.dim);
    txt(tC(k ? wx.t : clim.t), px + 8, 246, FN18, G.tx);
    txt(k ? UP(skyLabel(wx.sky, false)) : UP(status()) + " · " + humS() + "%", px + 8, 259, F10, k ? G.am : gCol(), 0, 94);
  }
  time_t n = time(nullptr); struct tm lt; localtime_r(&n, &lt);
  String msg = String(lt.tm_hour < 5 || lt.tm_hour >= 18 ? "Bonsoir. " : "Bonjour. ") + advice() + ".";
  int k = STILL ? msg.length() : min((int)msg.length(), (int)(fmodf(t, 12) * 9));
  txt(msg.substring(0, k) + ((T_MS / 500) % 2 ? "_" : ""), 12, 288, F13, G.tx, 0, 216);
}

static const char* const EL[60][2] = {
  {"n", "Neutron"}, {"H", "Hydrogène"}, {"He", "Hélium"}, {"Li", "Lithium"}, {"Be", "Béryllium"}, {"B", "Bore"}, {"C", "Carbone"}, {"N", "Azote"}, {"O", "Oxygène"}, {"F", "Fluor"},
  {"Ne", "Néon"}, {"Na", "Sodium"}, {"Mg", "Magnésium"}, {"Al", "Aluminium"}, {"Si", "Silicium"}, {"P", "Phosphore"}, {"S", "Soufre"}, {"Cl", "Chlore"}, {"Ar", "Argon"}, {"K", "Potassium"},
  {"Ca", "Calcium"}, {"Sc", "Scandium"}, {"Ti", "Titane"}, {"V", "Vanadium"}, {"Cr", "Chrome"}, {"Mn", "Manganèse"}, {"Fe", "Fer"}, {"Co", "Cobalt"}, {"Ni", "Nickel"}, {"Cu", "Cuivre"},
  {"Zn", "Zinc"}, {"Ga", "Gallium"}, {"Ge", "Germanium"}, {"As", "Arsenic"}, {"Se", "Sélénium"}, {"Br", "Brome"}, {"Kr", "Krypton"}, {"Rb", "Rubidium"}, {"Sr", "Strontium"}, {"Y", "Yttrium"},
  {"Zr", "Zirconium"}, {"Nb", "Niobium"}, {"Mo", "Molybdène"}, {"Tc", "Technétium"}, {"Ru", "Ruthénium"}, {"Rh", "Rhodium"}, {"Pd", "Palladium"}, {"Ag", "Argent"}, {"Cd", "Cadmium"}, {"In", "Indium"},
  {"Sn", "Étain"}, {"Sb", "Antimoine"}, {"Te", "Tellure"}, {"I", "Iode"}, {"Xe", "Xénon"}, {"Cs", "Césium"}, {"Ba", "Baryum"}, {"La", "Lanthane"}, {"Ce", "Cérium"}, {"Pr", "Praséodyme"}};

static void homeLabo() {
  gBg();
  time_t n = time(nullptr); struct tm lt; localtime_r(&n, &lt);
  txt("LABO · " + RM(), 12, 16, F10, G.tx);
  txt("p. " + String(lt.tm_yday + 1), 228, 16, F10, G.dim, 2);
  ln(10, 22, 230, 22, G.tx);
  int v[2] = {lt.tm_hour, lt.tm_min}, xs[2] = {18, 130};
  for (int k = 0; k < 2; k++) {
    d->fillRect(xs[k], 32, 92, 98, rgb(0xffffff)); d->drawRect(xs[k], 32, 92, 98, G.tx); d->drawRect(xs[k] + 1, 33, 90, 96, G.tx);
    char b[4]; snprintf(b, sizeof b, "%02d", v[k]);
    txt(b, xs[k] + 8, 60, FN18, G.tx);
    txt(EL[v[k]][0], xs[k] + 46, 106, FN26, G.cy, 1);
    txt(EL[v[k]][1], xs[k] + 46, 123, F10, G.dim, 1, 84);
  }
  if (STILL || lt.tm_sec % 2 == 0) { d->fillCircle(120, 70, 2, G.tx); d->fillCircle(120, 92, 2, G.tx); }
  txt(dateLong(), 120, 151, FHAND, G.am, 1);
  int X = 40 + (constrain(isnan(clim.t) ? 15 : clim.t, 15.0f, 27.0f) - 15) / 12 * 180;
  d->fillRoundRect(30, 165, 198, 10, 5, rgb(0xffffff)); d->drawRoundRect(30, 165, 198, 10, 5, G.tx);
  ln(26, 170, X, 170, gCol(), 3); d->fillCircle(26, 170, 8, gCol()); d->drawCircle(26, 170, 8, G.tx);
  ruler(40, 220, 178, G.tx, false);
  float dew = NAN;
  if (!isnan(clim.t) && clim.h > 0) { float g = logf(clim.h / 100) + 17.62f * clim.t / (243.12f + clim.t); dew = 243.12f * g / (17.62f - g); }
  String rows[3][3] = {{"T int", f1(clim.t) + " °C", f1(clim.t + 273.15f) + " K"}, {"T ext", f1(wx.t) + " °C", f1(wx.t + 273.15f) + " K"}, {"HR", humS() + " %", "rosée " + f1(dew) + " °C"}};
  for (int i = 0; i < 3; i++) { int y = 218 + i * 22; txt(rows[i][0], 14, y, F13, G.dim); txt("=", 58, y, F13, G.dim); txt(rows[i][1], 146, y, FN18, G.tx, 2); txt(rows[i][2], 228, y, F10, G.dim, 2); }
  String note = advice(); note.toLowerCase();
  txt("nb : " + note, 120, 289, FHAND, G.am, 1, 212);
}

static void homeGrandPrix() {
  gBg(); gTop("Grand Prix · " + cfg.room);
  otx(HM(), 120, 86, FBANGCLK, G.cy);
  txt(DS(), 120, 104, F10, G.tx, 1);
  const int x = 66, y = 166, r = 46;
  auto A = [](float v) { return 135 + (constrain(v, 15.0f, 27.0f) - 15) / 12 * 270; };
  d->fillCircle(x, y, r + 6, rgb(0x0d0826)); d->drawCircle(x, y, r + 6, G.tx); d->drawCircle(x, y, r + 5, G.tx);
  d->fillArc(x, y, r - 6, r - 1, A(15), A(18), G.bl); d->fillArc(x, y, r - 6, r - 1, A(18), A(24), G.gn); d->fillArc(x, y, r - 6, r - 1, A(24), A(27), G.am);
  for (int i = 0; i <= 24; i++) {
    float a = A(15 + i / 2.0f) * PI / 180; int r1 = r - (i % 6 == 0 ? 16 : i % 2 == 0 ? 12 : 10);
    ln(x + cosf(a) * r1, y + sinf(a) * r1, x + cosf(a) * (r - 7), y + sinf(a) * (r - 7), G.tx);
    if (i % 6 == 0) txt(String(15 + i / 2), x + cosf(a) * (r - 24), y + sinf(a) * (r - 24) + 3, F10, G.dim, 1);
  }
  float an = A(isnan(clim.t) ? 15 : clim.t) * PI / 180;
  ln(x, y, x + cosf(an) * (r - 10), y + sinf(an) * (r - 10), G.am, 2.5f);
  d->fillCircle(x, y, 5, G.tx);
  txt("INT", x, y + 17, F10, G.dim, 1);
  otx(tC(clim.t), x, y + r + 22, FBANG, G.tx);
  const int bx = 134, by = 122, bs = 86;
  const uint32_t rb[5] = {0xff4a3a, 0xffd23a, 0x3ee07a, 0x3aa8ff, 0xc25cff};
  int ph = STILL ? 0 : (T_MS / 300) % 5;
  for (int i = 0; i < 5; i++) d->fillRoundRect(bx + i * 2, by + i * 2, bs - i * 4, bs - i * 4, 14 - i, rgb(rb[(i + ph) % 5]));
  d->fillRoundRect(bx + 8, by + 8, bs - 16, bs - 16, 9, rgb(0x0d0826));
  txt("EXT", bx + bs / 2, by + 24, F10, G.tx, 1);
  icon(wx.sky, bx + bs / 2, by + bs / 2 + 4, 22, G.tx, rgb(0x0d0826));
  txt(UP(skyLabel(wx.sky, false)), bx + bs / 2, by + bs - 13, F10, G.cy, 1, bs - 18);
  otx(tC(wx.t), bx + bs / 2, y + r + 22, FBANG, G.tx);
  d->fillTriangle(98, 242, 142, 242, 0, 300, rgb(0x3a3550)); d->fillTriangle(142, 242, 240, 300, 0, 300, rgb(0x3a3550));
  int off = STILL ? 0 : (T_MS / 25) % 12;
  for (int s = 0; s < 2; s++) {
    int ax = s ? 142 : 98, bx2 = s ? 240 : 0;
    for (int k = 0; k < 10; k++) {
      float f0 = (k * 12 + off) / 120.0f, f1v = min(1.0f, f0 + 0.05f);
      ln(ax + (bx2 - ax) * f0, 242 + 58 * f0, ax + (bx2 - ax) * f1v, 242 + 58 * f1v, k % 2 ? G.am : rgb(0xffffff), 4);
    }
  }
  for (int k = 0; k < 5; k++) { int y0 = 242 + (k * 12 + off) % 58; ln(120, y0, 120, min(300, y0 + 5), rgb(0xffffff), 2); }
  String m = advice();
  int mw = min(212, textW(m, FBANG) + 22);
  d->fillRoundRect(120 - mw / 2, 262, mw, 22, 11, rgb(0x0d0826)); d->drawRoundRect(120 - mw / 2, 262, mw, 22, 11, G.cy);
  txt(m, 120, 278, FBANG, G.cy, 1, mw - 12);
}

static void homeTerminal() {
  gBg(); gTop("Logis · " + cfg.room);
  time_t n = time(nullptr); struct tm lt; localtime_r(&n, &lt);
  glowText(HM(), 120, 80, FMCLK, G.cy, G.cy);
  char b[32]; snprintf(b, sizeof b, "%s. %02d.%02d.%d", JC[lt.tm_wday], lt.tm_mday, lt.tm_mon + 1, lt.tm_year + 1900);
  txt(b, 120, 98, F13, G.tx, 1);
  const int x = 56, y = 160, r = 40;
  auto A = [](float v) { return (207 + (constrain(v, 15.0f, 27.0f) - 15) / 12 * 126) * PI / 180; };
  d->drawArc(x, y, r, r - 1, 207, 333, G.cy);
  for (int i = 0; i <= 24; i++) { float a = A(15 + i / 2.0f); int r1 = r - (i % 6 == 0 ? 9 : i % 2 == 0 ? 6 : 4); ln(x + cosf(a) * r1, y + sinf(a) * r1, x + cosf(a) * r, y + sinf(a) * r, G.cy); if (i % 6 == 0) txt(String(15 + i / 2), x + cosf(a) * (r + 9), y + sinf(a) * (r + 9) + 3, F10, G.dim, 1); }
  float an = A(isnan(clim.t) ? 15 : clim.t);
  ln(x, y, x + cosf(an) * (r - 4), y + sinf(an) * (r - 4), G.cy, 2); d->fillCircle(x, y, 3, G.cy);
  txt(tC(clim.t), x, y + 20, F13, G.tx, 1); txt("THERMOMÈTRE", x, y + 34, F10, G.dim, 1);
  float bars[4] = {isnan(clim.h) ? 0 : clim.h / 100, isnan(wx.t) ? 0 : constrain((wx.t + 10) / 48, 0.0f, 1.0f), isnan(clim.t) ? 0 : constrain(1 - fabsf(clim.t - 21) / 6, 0.0f, 1.0f), isnan(soundDb) ? 0 : constrain((soundDb - 30) / 60, 0.0f, 1.0f)};
  String lb[4] = {"HUMIDITÉ", "EXTÉRIEUR", "CONFORT", "BRUIT"};
  String vv[4] = {humS() + "%", f1(wx.t) + "°", UP(status()), isnan(soundDb) ? String("--") : String((int)lroundf(soundDb)) + "dB"};
  for (int i = 0; i < 4; i++) {
    int yy = 126 + i * 22;
    txt(lb[i], 114, yy, F10, G.tx); txt(vv[i], 230, yy, F10, G.cy, 2);
    for (int k = 0; k < 14; k++) d->fillRect(114 + k * 8.3f, yy + 4, 6, 5, k < lroundf(bars[i] * 14) ? G.cy : G.line);
  }
  ln(8, 222, 232, 222, G.dim);
  txt("> " + UP(advice()) + ((T_MS / 500) % 2 ? "_" : ""), 10, 240, F13, G.tx, 0, 220);
  int k = -1; if (wx.rainOk) for (int i = 0; i < 8; i++) if (wx.rain[i] > 0.1f) { k = i; break; }
  String box[3][2] = {{"CIEL", UP(skyLabel(wx.sky, false))}, {"SOLEIL", "COUCHER " + hm(wx.sunset)}, {"PLUIE", k < 0 ? "SEC" : k == 0 ? "EN COURS" : String(k * 15) + " MIN"}};
  for (int i = 0; i < 3; i++) {
    int bx = 8 + i * 78;
    d->fillRect(bx, 254, 68, 30, i == 0 ? G.cy : mix(G.bg, G.cy, 0.12f));
    txt(box[i][0], bx + 6, 266, F10, i == 0 ? G.bg : G.dim); txt(box[i][1], bx + 6, 279, F10, i == 0 ? G.bg : G.tx, 0, 58);
  }
}

static void homeLoutre() {
  gBg(); gTop("OtterWise · " + cfg.room);
  glowText(HM(), 120, 66, FCLK, G.tx, G.cy);
  txt(DS(), 120, 85, F10, G.dim, 1);
  size_t len;
  const uint8_t* im = otterImg(wx.sky, len);
  drawPngImg(im, len, 4, 112, 1.0f);
  txt(UP("Intérieur"), 128, 112, F10, G.dim);
  txt(tC(clim.t), 128, 140, FN26, G.tx);
  txt(UP(status() + " · " + humS() + "%"), 128, 155, F10, gCol());
  ln(128, 166, 230, 166, G.line);
  txt(UP("Extérieur"), 128, 184, F10, G.dim);
  txt(tC(wx.t), 128, 212, FN26, G.tx);
  txt(UP(skyLabel(wx.sky, false)), 128, 227, F10, G.am);
  ruler(14, 226, 274, G.cy, true);
}

static void flask(int x) {
  const int top = 130, bot = 226, by = 240;
  float fr = isnan(clim.t) ? 0 : constrain((clim.t - 15) / 12, 0.0f, 1.0f);
  int ly = bot - fr * (bot - top - 6);
  uint16_t col = GA ? GA : isnan(clim.t) ? G.dim : clim.t < 18 ? rgb(0x3b5b9a) : clim.t <= 24 ? rgb(0x2f7d55) : rgb(0xa32a2a);
  d->fillRect(x - 5, top, 10, bot - top + 6, rgb(0xf6efd9));
  d->fillCircle(x, by, 15, col);
  d->fillRect(x - 5, ly, 10, by - ly, col);
  d->drawFastVLine(x - 6, top, by - 13 - top, G.tx); d->drawFastVLine(x + 6, top, by - 13 - top, G.tx);
  d->drawCircle(x, by, 15, G.tx);
  d->fillRect(x - 7, top - 7, 14, 8, rgb(0x8a5a2c)); d->drawRect(x - 7, top - 7, 14, 8, G.tx);
  for (int i = 0; i <= 24; i++) { int y = bot - i * (bot - top - 6) / 24; ln(x + 8, y, x + (i % 6 == 0 ? 16 : i % 2 == 0 ? 13 : 11), y, G.tx); if (i % 6 == 0) txt(String(15 + i / 2), x + 19, y + 3, FSER, G.dim); }
}

static void homeMagie() {
  gBg();
  txt("· " + cfg.room + " ·", 120, 27, FSC, G.cy, 1);
  txt(HM(), 120, 80, FCIN, G.tx, 1);
  txt(dateLong(), 120, 102, FSER, G.dim, 1);
  ln(50, 114, 112, 114, G.dim); ln(128, 114, 190, 114, G.dim);
  flask(40);
  txt("Au dedans", 86, 140, FSER, G.cy); txt(f1(clim.t) + "°", 86, 172, FCIN26, G.tx);
  txt("humidité " + humS() + " %", 86, 188, FSER, G.dim);
  ln(86, 198, 224, 198, G.line);
  txt("Au dehors", 86, 216, FSER, G.cy); txt(f1(wx.t) + "°", 86, 248, FCIN26, G.tx);
  txt(skyLabel(wx.sky, false), 86, 264, FSER, G.dim);
  icon(wx.sky, 200, 232, 16, G.tx, G.bg);
  static const char* const M[] = {"Beau temps pour sortir le balai.", "Nuit claire, les étoiles veillent.", "Quelques éclaircies, rien d'inquiétant.", "Ciel couvert, rien d'inquiétant.", "Brume épaisse, gardez la lanterne.", "Le vent souffle, tenez vos chapeaux.", "Prenez une cape, la pluie arrive.", "L'orage gronde, restez au château.", "La neige arrive, gardez l'âtre allumé."};
  String m = M[wx.sky % 9];
  int k = STILL ? m.length() : min((int)m.length(), (int)(fmodf(tt(), 14) * 7));
  txt(m.substring(0, k), 120, 291, FSER, G.tx, 1, 206);
}

// ---------- Variantes ----------
void homeBig() {
  gBg(); gTop(cfg.room);
  if (TH == T_GRANDPRIX) otx(HM(), 120, 136, FBANGCLK, G.cy);
  else if (TH == T_MAGIE) txt(HM(), 120, 134, FCIN, G.tx, 1);
  else glowText(HM(), 120, 134, TH == T_LABO || TH == T_TERMINAL ? FMCLK : FCLKXL, G.tx, G.cy);
  txt(TH == T_MAGIE ? dateLong() : UP(dateLong()), 120, 162, TH == T_MAGIE ? FSER : F13, G.dim, 1);
  kBar(40, 176, 160, secNow() / 60.0f);
  txt(U("Intérieur"), 62, 214, fLab(), G.dim, 1); txt(U("Extérieur"), 178, 214, fLab(), G.dim, 1);
  ln(120, 204, 120, 256, G.line);
  txt(f1(clim.t) + "°", 62, 246, fNumL(), gCol(), 1); txt(f1(wx.t) + "°", 178, 246, fNumL(), G.tx, 1);
  txt(U(advice()), 120, 284, TH == T_MAGIE ? FSER : F10, G.am, 1, 220);
}

void homeDash() {
  gBg(); gTop("Tableau · " + cfg.room);
  txt(HM(), 12, 62, TH == T_MAGIE ? FCIN26 : FN26, G.tx);
  time_t n = time(nullptr); struct tm lt; localtime_r(&n, &lt);
  txt(String(JC[lt.tm_wday]) + " " + lt.tm_mday, 228, 48, F10, G.dim, 2);
  txt(String(MC[lt.tm_mon]) + " " + (lt.tm_year + 1900), 228, 62, F10, G.dim, 2);
  int k = -1; if (wx.rainOk) for (int i = 0; i < 8; i++) if (wx.rain[i] > 0.1f) { k = i; break; }
  int nm = nowMin(); const Event* nx = nullptr;
  for (int i = 0; i < agenda.n; i++) if (agenda.ev[i].day == 0 && agenda.ev[i].min >= nm) { nx = &agenda.ev[i]; break; }
  const char* const MN[] = {"nouvelle", "croissant", "1er quartier", "gibbeuse", "pleine", "gibbeuse", "dern. quartier", "croissant"};
  float p = moonPhase();
  String aq = wx.aqi < 0 ? "--" : String(wx.aqi);
  String aqs = wx.aqi < 0 ? "en attente" : wx.aqi <= 20 ? "très bon" : wx.aqi <= 40 ? "bon" : wx.aqi <= 60 ? "moyen" : wx.aqi <= 80 ? "médiocre" : "mauvais";
  String tiles[8][3] = {
    {"Intérieur", tC(clim.t), humS() + " % humidité"}, {"Extérieur", tC(wx.t), skyLabel(wx.sky, false)},
    {"Pluie", k < 0 ? "sec" : k == 0 ? "en cours" : "dans " + String(k * 15) + " min", k < 0 ? "rien d'ici 2 h" : "pas de 15 min"},
    {"Air dehors", aq, aqs + " · indice européen"},
    {"Soleil", hm(wx.sunset), "coucher · lever " + hm(wx.sunrise)},
    {transit.n ? "Ligne " + transit.d[0].line : String("Transports"), transit.n && transit.d[0].m1 >= 0 ? String(transit.d[0].m1) + " min" : String("--"), transit.n ? transit.d[0].stop : String("à régler")},
    {"Agenda", nx ? hm(nx->min) : String("libre"), nx ? nx->title : String("rien de prévu")},
    {"Lune", String((int)lroundf((1 - cosf(2 * PI * p)) / 2 * 100)) + " %", MN[(int)lroundf(p * 8) % 8]}};
  for (int i = 0; i < 8; i++) {
    int x = 10 + (i % 2) * 112, y = 76 + (i / 2) * 52;
    kShape(x, y, 107, 45, KT.sh == SH_PILL ? SH_ROUND : KT.sh == SH_TEXT ? SH_RECT : KT.sh, G.line, light() ? rgb(0xffffff) : mix(G.bg, G.cy, 0.06f), true);
    txt(U(tiles[i][0]), x + 8, y + 13, fLab(), G.dim, 0, 92);
    txt(i == 0 || i == 1 ? tiles[i][1] : U(tiles[i][1]), x + 8, y + 32, TH == T_MAGIE ? FSER : FN18, i == 0 ? gCol() : G.tx, 0, 92);
    txt(U(tiles[i][2]), x + 8, y + 42, F10, G.cy, 0, 92);
  }
}

void drawHome() {
  uint8_t v = cfg.variant[TH] % 3;
  if (v == 1) { homeBig(); return; }
  if (v == 2) { homeDash(); return; }
  switch (TH) {
    case T_GALAXIE: homeGalaxie(); break;
    case T_OBSCUR: homeObscur(); break;
    case T_COCKPIT: homeCockpit(); break;
    case T_RADAR: homeRadar(); break;
    case T_HOLO: homeHolo(); break;
    case T_PORTAIL: homePortail(); break;
    case T_ASSISTANT: homeAssistant(); break;
    case T_LABO: homeLabo(); break;
    case T_GRANDPRIX: homeGrandPrix(); break;
    case T_TERMINAL: homeTerminal(); break;
    case T_LOUTRE: homeLoutre(); break;
    case T_MAGIE: homeMagie(); break;
  }
}

// ---------- Scènes de nuit ----------
void nightScene(uint16_t nc) {
  float t = tt();
  switch (TH) {
    case T_MAGIE: {
      d->fillRect(114, 270, 12, 30, rgb(0x2a261e)); d->fillRect(106, 299, 28, 3, rgb(0x241e10));
      int fl = STILL ? 0 : (int)(sinf(t * 9) * 1.2f);
      d->fillTriangle(120, 246 + fl, 126, 266, 114, 266, rgb(0x6a4a18)); d->fillCircle(120, 264, 5, rgb(0x6a4a18));
      return;
    }
    case T_LOUTRE: { size_t len; const uint8_t* im = zenImg(len); drawPngImg(im, len, 70, 196, 0.83f); return; }
    case T_GRANDPRIX: for (int i = 0; i < 5; i++) d->fillCircle(64 + i * 28, 262, 7, i == 4 && sinf(t) > 0 ? nc : rgb(0x1a1626)); return;
    case T_PORTAIL: d->fillArc(120, 420, 170, 188, 216, 324, rgb(0x14191d)); d->fillCircle(120, 240, 5, nc); return;
    case T_ASSISTANT: d->fillArc(120, 112, 85, 87, t * 11, t * 11 + 70, nc); d->drawArc(120, 112, 92, 92, -t * 9, -t * 9 + 35, nc); return;
    case T_TERMINAL: for (int y = 0; y < H; y += 2) d->drawFastHLine(0, y, W, rgb(0x020a04)); return;
    case T_LABO: { const int P[6][2] = {{30, 230}, {62, 256}, {96, 242}, {132, 270}, {170, 252}, {204, 278}}; for (int i = 0; i < 5; i++) ln(P[i][0], P[i][1], P[i + 1][0], P[i + 1][1], nc); return; }
    case T_OBSCUR: {
      float r = t * 0.15f; int px[3], py[3]; const float tri[3][2] = {{0, -1}, {0.9f, 0.6f}, {-0.9f, 0.6f}};
      for (int i = 0; i < 3; i++) { px[i] = 120 + tri[i][0] * cosf(r) * 34; py[i] = 262 + tri[i][1] * 34; }
      for (int i = 0; i < 3; i++) ln(px[i], py[i], px[(i + 1) % 3], py[(i + 1) % 3], nc);
      return;
    }
    default:
      for (int i = 0; i < 30; i++) d->drawPixel((int)(rnd(i) % W + t * 0.6f * (0.2f + (rnd(i + 9) % 100) / 100.0f)) % W, rnd(i + 50) % H, rgb(0x1e2430));
      d->fillCircle(200, 330, 90, rgb(0x04070c));
      d->drawArc(200, 330, 90, 89, 190, 280, nc);
  }
}
