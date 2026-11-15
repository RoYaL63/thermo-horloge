// Outils de dessin partagés entre les écrans (ui.cpp) et les accueils des thèmes (homes.cpp).
#pragma once
#include "board.h"
#include "config.h"
#include "data.h"

constexpr int W = 240, H = 320;

enum FontId : uint8_t { F10, F13, FN18, FN26, FCLK, FCLKXL, FMCLK, FSER, FSC, FCIN, FCIN26, FBANG, FBANGCLK, FHAND, N_FONTS };

struct Pal { uint16_t bg, cy, tx, dim, line, am, rd, gn, bl; };
struct Kit { uint8_t sh, row, tg, bar; };
enum Shape : uint8_t { SH_RECT, SH_CUT, SH_ROUND, SH_PILL, SH_TEXT, SH_BRACKET };

extern lgfx::LovyanGFX* d;     // cible de dessin (image en mémoire, ou écran direct en secours)
extern Pal G;
extern Kit KT;
extern uint8_t TH;              // thème courant
extern uint16_t GA;             // couleur de jauge choisie, 0 = automatique
extern bool STILL;
extern uint32_t T_MS;           // temps d'animation

constexpr uint16_t rgb(uint32_t c) { return ((c >> 8) & 0xF800) | ((c >> 5) & 0x07E0) | ((c >> 3) & 0x001F); }
uint16_t mix(uint16_t a, uint16_t b, float t);
bool light();                   // thème clair (Labo, Loutre, Magie)

// Polices par rôle, selon le thème
uint8_t fLab();                 // petites étiquettes
uint8_t fTxt();                 // texte courant
uint8_t fNum();                 // nombres moyens
uint8_t fNumL();                // grands nombres
uint8_t fClk();                 // heure
String U(const String& s);      // majuscules (accents compris) si le thème les utilise
String UP(const String& s);     // majuscules toujours

void txt(const String& s, int x, int y, uint8_t f, uint16_t col, uint8_t align = 0, int maxw = 0);
int textW(const String& s, uint8_t f);
void ln(int x0, int y0, int x1, int y1, uint16_t col, float w = 1);
void glowText(const String& s, int x, int y, uint8_t f, uint16_t col, uint16_t glow, uint8_t align = 1);

// Éléments communs
void gBg();
void gTop(const String& s);
void navBar(int y = 309);
void kShape(int x, int y, int w, int h, uint8_t sh, uint16_t stroke, uint16_t fill, bool doFill);
void kBtn(int x, int y, int w, int h, const String& label, bool pri, bool off = false);
void kBar(int x, int y, int w, float fr);
void kTog(int x, int y, bool on);
int kRow(int i, int y, int h, const String& lab, const String& val, float fr = -1, int on = -1);
void ruler(int x0, int x1, int y, uint16_t col, bool marker);
void icon(uint8_t sky, int x, int y, int s, uint16_t col, uint16_t bg);
void saber(int x, int y, int maxL, float fr, uint16_t col);
void moon(int x, int y, int r, float p, uint16_t lit, uint16_t dark, uint16_t edge);
void graph(int x0, int y0, int x1, int y1, const float* v, int n, uint16_t grid, uint16_t lab, uint16_t line, uint16_t dot);

// Données mises en forme
String f1(float v);                         // 21,4
String tC(float v, bool unit = true);       // 21,4°C ou 21,4°
String HM();
String DS();                                // DIM 15 NOV
String dateLong();                          // dimanche 15 novembre
String hm(int minutes);
String skyLabel(uint8_t s, bool upper);
String advice();
String rainMsg();
int nowMin();
float moonPhase();
uint16_t gCol();                            // couleur de jauge selon la température
extern const char* const JC[7];
extern const char* const MC[12];

// Accueils (homes.cpp)
void drawHome();
void homeBig();
void homeDash();
uint8_t variantCount();
String variantName(uint8_t v);
void nightScene(uint16_t nc);
void drawPngImg(const uint8_t* data, size_t len, int x, int y, float scale = 1.0f);
const uint8_t* otterImg(uint8_t sky, size_t& len);
const uint8_t* tempsImg(size_t& len);
const uint8_t* zenImg(size_t& len);
