// Réglages enregistrés dans la mémoire de la carte (NVS), modifiables à l'écran ou depuis la page web.
#pragma once
#include <Arduino.h>

#define FW_VERSION "1.0.0"

enum ThemeId : uint8_t { T_GALAXIE, T_OBSCUR, T_COCKPIT, T_RADAR, T_HOLO, T_PORTAIL, T_ASSISTANT,
                         T_LABO, T_GRANDPRIX, T_TERMINAL, T_LOUTRE, T_MAGIE, N_THEMES };
extern const char* const THEME_NAMES[N_THEMES];

// Écrans du carrousel, dans l'ordre d'affichage
enum ScreenId : uint8_t { S_HOME, S_AGENDA, S_RAIN, S_FORE, S_COMFORT, S_SOUND, S_TIMER, S_SKY, S_TRANSIT, N_CAROUSEL,
                          S_NIGHT = N_CAROUSEL, S_SETTINGS, S_SCREENS, S_INFO, S_PORTAL, S_ALERT };
extern const char* const SCREEN_NAMES[N_CAROUSEL];

struct TransitLine {
  String label;          // ex. "D", "14", "RER A"
  uint32_t color = 0x6F7780;
  String stop;           // Paris : MonitoringRef (STIF:StopPoint:Q:...:) ; Lyon : identifiant d'arrêt
  String filter;         // Paris : morceau de LineRef ; Lyon : code de ligne tel que dans les données
  String stopName;       // nom affiché
};

struct Reminder {
  bool on = false;
  uint8_t h = 8, m = 0;
  bool daily = true;
  String text;
};

constexpr int N_REM = 8;

struct Settings {
  String ssid, pass;
  String city = "Lyon";
  float lat = 45.7578f, lon = 4.8320f;
  String room = "Salon";
  uint8_t theme = T_GALAXIE;
  uint8_t variant[N_THEMES] = {0};      // 0 classique, 1 grande heure, 2 tableau de bord
  uint8_t nav = 2;                      // barre du bas : 0 points, 1 onglets, 2 icônes
  uint8_t bri = 70;                     // luminosité de jour, en %
  bool nightOn = true;
  uint8_t nightFrom = 23, nightTo = 7;
  uint8_t nightStyle = 0;               // 0 ambiance, 1 horloge seule, 2 écran éteint
  bool nightRed = true;
  uint8_t nightBri = 12;
  bool nightInfo = true, nightShift = true;
  float tempOffset = 2.1f;              // échauffement de la sonde (étalonnage automatique à froid ou à la main)
  bool autoCal = false;                 // étalonnage automatique demandé pour le prochain démarrage à froid
  float soundOffset = 0;                // correction du sonomètre, en dB
  uint16_t screens = 0x01FF;            // un bit par écran du carrousel
  bool touchSwap = false, touchInvX = false, touchInvY = false;
  uint32_t colMain[N_THEMES] = {0}, colSecond[N_THEMES] = {0}, colGauge[N_THEMES] = {0};  // 0 = couleur d'origine, sinon 0x1RRGGBB
  bool sounds = true;
  uint8_t transitMode = 0;              // 0 aucun, 1 Paris (PRIM), 2 Lyon (Grand Lyon)
  String primKey, lyonUser, lyonPass;
  TransitLine lines[3];
  String ics;                           // lien privé de l'agenda (.ics)
  Reminder rem[N_REM];
};

extern Settings cfg;

void cfgLoad();
void cfgSave();
void cfgForgetWifi();
void cfgFactory();
bool screenOn(uint8_t s);
