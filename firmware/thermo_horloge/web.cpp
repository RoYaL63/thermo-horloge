// Page de réglages ouverte depuis un téléphone sur le même WiFi (ou sur le réseau Thermo-Horloge au premier démarrage).
// Adresse : http://thermo-horloge.local ou l'adresse IP affichée dans Réglages > Système.
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPUpdateServer.h>
#include <ESPmDNS.h>
#include "config.h"
#include "data.h"

static WebServer server(80);
static HTTPUpdateServer updater;
static uint32_t restartAt = 0;

static String esc(const String& s) {
  String o;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '&') o += "&amp;"; else if (c == '<') o += "&lt;"; else if (c == '>') o += "&gt;"; else if (c == '"') o += "&quot;"; else o += c;
  }
  return o;
}
static String hex6(uint32_t c) { char b[8]; snprintf(b, sizeof b, "#%06X", (unsigned)(c & 0xFFFFFF)); return b; }
static uint32_t parseHex(const String& s) { return (uint32_t)strtoul(s.c_str() + (s.startsWith("#") ? 1 : 0), nullptr, 16); }
static String chk(bool v) { return v ? " checked" : ""; }
static String sel(bool v) { return v ? " selected" : ""; }
static String opt(int v, int cur, const String& label) { return "<option value=\"" + String(v) + "\"" + sel(v == cur) + ">" + label + "</option>"; }

static const char CSS[] PROGMEM = R"(<style>
:root{--bg:#f4f5f7;--card:#fff;--fg:#151a22;--muted:#5d6775;--line:#d9dee5;--acc:#0c6f80}
@media(prefers-color-scheme:dark){:root{--bg:#0f1217;--card:#181c23;--fg:#e8ecf1;--muted:#97a2b1;--line:#2a303a;--acc:#5cc8d8}}
body{margin:0;background:var(--bg);color:var(--fg);font:15px/1.45 system-ui,sans-serif;padding:16px}
main{max-width:620px;margin:0 auto;display:flex;flex-direction:column;gap:14px}
h1{font-size:22px;margin:4px 0}h2{font-size:15px;margin:0 0 8px;text-transform:uppercase;letter-spacing:.06em;color:var(--muted)}
section{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:14px;display:flex;flex-direction:column;gap:10px}
label{display:flex;flex-direction:column;gap:4px;font-size:13px;color:var(--muted)}
.row{display:flex;flex-wrap:wrap;gap:10px}.row>label{flex:1 1 140px}
.ck{flex-direction:row;align-items:center;gap:8px;color:var(--fg);font-size:14px}
input,select{font:inherit;color:var(--fg);background:var(--bg);border:1px solid var(--line);border-radius:8px;padding:8px;min-width:0}
input[type=color]{padding:2px;height:36px;width:56px}input[type=checkbox]{width:18px;height:18px;accent-color:var(--acc)}
button,.btn{font:600 15px system-ui;background:var(--acc);color:#fff;border:0;border-radius:10px;padding:12px 16px;text-align:center;text-decoration:none}
.sec{background:transparent;color:var(--acc);border:1px solid var(--acc)}
.hint{font-size:13px;color:var(--muted);margin:0}.stat{font-variant-numeric:tabular-nums}
.sticky{position:sticky;bottom:0;padding:10px 0;background:var(--bg)}
</style>)";

static void sendPage() {
  Serial.printf("Page de reglages demandee (memoire %u)\n", ESP.getFreeHeap());
  String h;
  h.reserve(16000);
  h += F("<!doctype html><html lang=fr><meta charset=utf-8><meta name=viewport content='width=device-width,initial-scale=1'><title>Thermo-horloge</title>");
  h += FPSTR(CSS);
  h += F("<main><h1>Thermo-horloge</h1>");
  h += "<p class='hint stat'>Dedans " + (isnan(clim.t) ? String("--") : String(clim.t, 1)) + " °C · humidité " + (isnan(clim.h) ? String("--") : String((int)clim.h)) +
       " % · dehors " + (isnan(wx.t) ? String("--") : String(wx.t, 1)) + " °C · version " FW_VERSION "</p>";
  h += F("<form method=post action=/save>");

  // WiFi
  h += F("<section><h2>WiFi</h2>");
  if (netPortal()) {
    h += F("<p class=hint>Choisis le WiFi de la maison. L'objet redémarre et s'y connecte.</p><label>Réseau<select name=ssid>");
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_FAILED) { WiFi.scanNetworks(true); n = 0; }
    for (int i = 0; i < n; i++) h += "<option" + sel(WiFi.SSID(i) == cfg.ssid) + ">" + esc(WiFi.SSID(i)) + "</option>";
    h += F("</select></label><label>Ou nom du réseau à la main<input name=ssid2 autocomplete=off></label>");
    h += F("<label>Mot de passe<input id=pw name=pass type=password autocomplete=current-password></label>"
           "<label class=ck><input type=checkbox onclick=\"pw.type=this.checked?'text':'password'\"> Afficher le mot de passe</label>"
           "<details><summary>Copier le mot de passe depuis ce téléphone</summary>"
           "<p class=hint><b>Android</b> : Paramètres &gt; Wi-Fi &gt; ton réseau &gt; Partager. Le mot de passe s'affiche sous le QR code : appui long pour le copier, puis colle-le ici.</p>"
           "<p class=hint><b>iPhone</b> (iOS 16 ou plus) : Réglages &gt; Wi-Fi &gt; (i) à côté de ton réseau &gt; Mot de passe &gt; Copier, puis colle-le ici.</p></details>"
           "<p class=hint>Sans téléphone : touche « Bouton WPS » sur l'écran de l'objet, puis appuie sur le bouton WPS de ta box.</p>");
  } else {
    h += "<p class=hint>Connecté à " + esc(cfg.ssid) + " (" + String(netRssi()) + " dBm). Pour changer de réseau : Réglages &gt; Système &gt; Oublier le WiFi.</p>";
  }
  h += F("</section>");

  // Lieu
  h += "<section><h2>Lieu</h2><div class=row><label>Ville (pour la météo)<input name=city value=\"" + esc(cfg.city) + "\"></label>";
  h += "<label>Pièce<input name=room value=\"" + esc(cfg.room) + "\" maxlength=14></label></div>";
  h += "<p class=hint>Position : " + String(cfg.lat, 3) + ", " + String(cfg.lon, 3) + "</p></section>";

  // Affichage
  h += F("<section><h2>Affichage</h2><div class=row><label>Thème<select name=theme>");
  for (int i = 0; i < N_THEMES; i++) h += opt(i, cfg.theme, THEME_NAMES[i]);
  h += F("</select></label><label>Accueil<select name=variant>");
  h += opt(0, cfg.variant[cfg.theme], "Classique") + opt(1, cfg.variant[cfg.theme], "Grande heure") + opt(2, cfg.variant[cfg.theme], "Tableau de bord");
  h += F("</select></label></div><div class=row><label>Luminosité (%)<input name=bri type=number min=10 max=100 value=");
  h += String(cfg.bri) + "></label><label>Barre du bas<select name=nav>" + opt(0, cfg.nav, "Points") + opt(1, cfg.nav, "Onglets") + opt(2, cfg.nav, "Icônes") + "</select></label></div>";
  uint8_t t = cfg.theme;
  h += "<p class=hint>Couleurs du thème « " + String(THEME_NAMES[t]) + " ». Décoche pour garder la couleur d'origine.</p><div class=row>";
  const char* cn[3] = {"Principale", "Secondaire", "Lame et jauges"};
  uint32_t* cv[3] = {cfg.colMain, cfg.colSecond, cfg.colGauge};
  const char* ck[3] = {"cm", "cs", "cg"};
  for (int i = 0; i < 3; i++)
    h += "<label class=ck><input type=checkbox name=" + String(ck[i]) + "on" + chk(cv[i][t] != 0) + "><input type=color name=" + ck[i] + " value=" + hex6(cv[i][t] ? cv[i][t] : 0x5fd4ff) + "> " + cn[i] + "</label>";
  h += F("</div></section>");

  // Nuit
  h += "<section><h2>Mode nuit</h2><label class=ck><input type=checkbox name=nOn" + chk(cfg.nightOn) + "> Veille automatique</label><div class=row>";
  h += "<label>De (heure)<input name=nFrom type=number min=0 max=23 value=" + String(cfg.nightFrom) + "></label>";
  h += "<label>À (heure)<input name=nTo type=number min=0 max=23 value=" + String(cfg.nightTo) + "></label>";
  h += "<label>Style<select name=nStyle>" + opt(0, cfg.nightStyle, "Ambiance") + opt(1, cfg.nightStyle, "Horloge seule") + opt(2, cfg.nightStyle, "Écran éteint") + "</select></label>";
  h += "<label>Luminosité de nuit (%)<input name=nBri type=number min=1 max=60 value=" + String(cfg.nightBri) + "></label></div>";
  h += "<label class=ck><input type=checkbox name=nRed" + chk(cfg.nightRed) + "> Rouge doux (gêne moins le sommeil)</label>";
  h += "<label class=ck><input type=checkbox name=nInfo" + chk(cfg.nightInfo) + "> Infos de demain</label>";
  h += "<label class=ck><input type=checkbox name=nShift" + chk(cfg.nightShift) + "> Anti-marquage (décalage d'un pixel par minute)</label></section>";

  // Écrans
  h += F("<section><h2>Écrans affichés</h2><div class=row>");
  for (int s = 1; s < N_CAROUSEL; s++) h += "<label class=ck><input type=checkbox name=scr" + String(s) + chk(screenOn(s)) + "> " + SCREEN_NAMES[s] + "</label>";
  h += "</div><label class=ck><input type=checkbox name=sounds" + chk(cfg.sounds) + "> Sons (rappels, minuteur)</label></section>";

  // Agenda et rappels
  h += "<section><h2>Agenda et rappels</h2><label>Lien privé de l'agenda (.ics)<input name=ics value=\"" + esc(cfg.ics) + "\" placeholder='https://calendar.google.com/calendar/ical/…/basic.ics'></label>";
  h += F("<p class=hint>Google Agenda : Paramètres de l'agenda &gt; Adresse secrète au format iCal. iCloud : partager l'agenda en public et copier le lien.</p>");
  for (int i = 0; i < N_REM; i++) {
    const Reminder& r = cfg.rem[i];
    char tm[6]; snprintf(tm, sizeof tm, "%02d:%02d", r.h, r.m);
    h += "<div class=row><label class=ck><input type=checkbox name=r" + String(i) + "on" + chk(r.on) + "></label>";
    h += "<label>Heure<input type=time name=r" + String(i) + "t value=" + tm + "></label>";
    h += "<label>Rappel<input name=r" + String(i) + "x maxlength=40 value=\"" + esc(r.text) + "\"></label>";
    h += "<label class=ck><input type=checkbox name=r" + String(i) + "d" + chk(r.daily) + "> chaque jour</label></div>";
  }
  h += F("</section>");

  // Transports
  h += "<section><h2>Transports</h2><label>Réseau<select name=trMode>" + opt(0, cfg.transitMode, "Aucun") + opt(1, cfg.transitMode, "Paris (Île-de-France Mobilités, PRIM)") + opt(2, cfg.transitMode, "Lyon (TCL, data.grandlyon.com)") + "</select></label>";
  h += "<label>Paris : clé d'API PRIM<input name=prim value=\"" + esc(cfg.primKey) + "\" autocomplete=off></label>";
  h += "<div class=row><label>Lyon : identifiant Grand Lyon<input name=lyU value=\"" + esc(cfg.lyonUser) + "\" autocomplete=off></label>";
  h += "<label>Lyon : mot de passe<input name=lyP type=password value=\"" + esc(cfg.lyonPass) + "\" autocomplete=off></label></div>";
  h += F("<p class=hint>Pour chaque ligne : nom affiché, couleur, identifiant de l'arrêt (Paris : STIF:StopPoint:Q:…: ; Lyon : identifiant TCL), filtre de ligne, nom de l'arrêt.</p>");
  for (int i = 0; i < 3; i++) {
    const TransitLine& L = cfg.lines[i];
    String k = "l" + String(i);
    h += "<div class=row><label>Ligne<input name=" + k + "lb maxlength=6 value=\"" + esc(L.label) + "\"></label>";
    h += "<label class=ck><input type=color name=" + k + "co value=" + hex6(L.color) + "></label>";
    h += "<label>Arrêt (identifiant)<input name=" + k + "st value=\"" + esc(L.stop) + "\"></label>";
    h += "<label>Filtre ligne<input name=" + k + "fi value=\"" + esc(L.filter) + "\"></label>";
    h += "<label>Nom de l'arrêt<input name=" + k + "sn maxlength=20 value=\"" + esc(L.stopName) + "\"></label></div>";
  }
  if (transit.err.length()) h += "<p class=hint>Dernière erreur : " + esc(transit.err) + "</p>";
  h += F("</section>");

  // Sonde et tactile
  h += "<section><h2>Étalonnage</h2><div class=row><label>Correction de la sonde (°C retranchés)<input name=tOff type=number step=0.1 value=" + String(cfg.tempOffset, 1) + "></label>";
  h += "<label>Correction du sonomètre (dB)<input name=sOff type=number step=1 value=" + String(cfg.soundOffset, 0) + "></label></div>";
  h += "<p class=hint>Sonde brute : " + (isnan(clim.raw) ? String("--") + "" : String(clim.raw, 2)) + " °C.</p>";
  h += "<label class=ck><input type=checkbox name=aCal" + chk(cfg.autoCal) + "> Étalonnage automatique au prochain démarrage à froid</label>";
  h += "<p class=hint>Coche, enregistre, puis débranche l'horloge au moins 30 minutes à l'endroit où elle vivra. Rebranche-la : elle mesure la pièce à froid, puis calcule seule sa correction en 40 minutes.</p>";
  h += "<p class=hint>Si le toucher tombe à côté :</p><div class=row><label class=ck><input type=checkbox name=tSwap" + chk(cfg.touchSwap) + "> inverser X et Y</label>";
  h += "<label class=ck><input type=checkbox name=tInvX" + chk(cfg.touchInvX) + "> miroir horizontal</label>";
  h += "<label class=ck><input type=checkbox name=tInvY" + chk(cfg.touchInvY) + "> miroir vertical</label></div></section>";

  h += F("<div class=sticky><button style='width:100%'>Enregistrer</button></div></form>");
  h += F("<section><h2>Système</h2><a class='btn sec' href=/update>Mettre à jour le firmware (fichier .bin)</a>"
         "<form method=post action=/reboot><button class=sec style='width:100%'>Redémarrer</button></form></section></main></html>");
  uint32_t t0 = millis();
  server.send(200, "text/html; charset=utf-8", h);
  Serial.printf("Page envoyee : %u octets en %u ms\n", h.length(), millis() - t0);
}

static int toInt(const char* n, int def, int lo, int hi) { return server.hasArg(n) && server.arg(n).length() ? constrain(server.arg(n).toInt(), lo, hi) : def; }

static void handleSave() {
  bool wifiChanged = false;
  if (netPortal()) {
    String s = server.arg("ssid2").length() ? server.arg("ssid2") : server.arg("ssid");
    if (s.length()) { cfg.ssid = s; cfg.pass = server.arg("pass"); wifiChanged = true; }
  }
  String city = server.arg("city");
  city.trim();
  if (city.length() && city != cfg.city) {
    float la, lo; String label;
    if (geocodeCity(city, la, lo, label)) { cfg.lat = la; cfg.lon = lo; cfg.city = label; }
    else cfg.city = city;   // hors ligne (premier démarrage) : la position sera cherchée après connexion
  }
  if (server.arg("room").length()) cfg.room = server.arg("room");
  uint8_t oldTheme = cfg.theme;
  cfg.theme = toInt("theme", cfg.theme, 0, N_THEMES - 1);
  if (cfg.theme == oldTheme) cfg.variant[cfg.theme] = toInt("variant", cfg.variant[cfg.theme], 0, 2);
  cfg.bri = toInt("bri", cfg.bri, 10, 100);
  cfg.nav = toInt("nav", cfg.nav, 0, 2);
  const char* ck[3] = {"cm", "cs", "cg"};
  uint32_t* cv[3] = {cfg.colMain, cfg.colSecond, cfg.colGauge};
  if (cfg.theme == oldTheme)
    for (int i = 0; i < 3; i++) cv[i][oldTheme] = server.hasArg(String(ck[i]) + "on") ? (0x1000000 | parseHex(server.arg(ck[i]))) : 0;
  cfg.nightOn = server.hasArg("nOn");
  cfg.nightFrom = toInt("nFrom", cfg.nightFrom, 0, 23);
  cfg.nightTo = toInt("nTo", cfg.nightTo, 0, 23);
  cfg.nightStyle = toInt("nStyle", cfg.nightStyle, 0, 2);
  cfg.nightBri = toInt("nBri", cfg.nightBri, 1, 60);
  cfg.nightRed = server.hasArg("nRed");
  cfg.nightInfo = server.hasArg("nInfo");
  cfg.nightShift = server.hasArg("nShift");
  uint16_t sc = 1;
  for (int s = 1; s < N_CAROUSEL; s++) if (server.hasArg("scr" + String(s))) sc |= (1 << s);
  cfg.screens = sc;
  cfg.sounds = server.hasArg("sounds");
  cfg.ics = server.arg("ics"); cfg.ics.trim();
  for (int i = 0; i < N_REM; i++) {
    String k = "r" + String(i);
    Reminder& r = cfg.rem[i];
    r.on = server.hasArg(k + "on");
    r.daily = server.hasArg(k + "d");
    String tm = server.arg(k + "t");
    if (tm.length() >= 5) { r.h = constrain(tm.substring(0, 2).toInt(), 0, 23); r.m = constrain(tm.substring(3, 5).toInt(), 0, 59); }
    r.text = server.arg(k + "x");
  }
  cfg.transitMode = toInt("trMode", cfg.transitMode, 0, 2);
  cfg.primKey = server.arg("prim"); cfg.primKey.trim();
  cfg.lyonUser = server.arg("lyU"); cfg.lyonPass = server.arg("lyP");
  for (int i = 0; i < 3; i++) {
    String k = "l" + String(i);
    TransitLine& L = cfg.lines[i];
    L.label = server.arg(k + "lb"); L.label.trim();
    L.color = parseHex(server.arg(k + "co"));
    L.stop = server.arg(k + "st"); L.stop.trim();
    L.filter = server.arg(k + "fi"); L.filter.trim();
    L.stopName = server.arg(k + "sn");
  }
  if (server.arg("tOff").length()) cfg.tempOffset = constrain(server.arg("tOff").toFloat(), -5.0f, 20.0f);
  cfg.autoCal = server.hasArg("aCal");
  if (server.arg("sOff").length()) cfg.soundOffset = constrain(server.arg("sOff").toFloat(), -40.0f, 40.0f);
  cfg.touchSwap = server.hasArg("tSwap");
  cfg.touchInvX = server.hasArg("tInvX");
  cfg.touchInvY = server.hasArg("tInvY");
  cfgSave();
  uiApplyTheme();
  agendaRebuild();
  netRefreshSoon();
  if (wifiChanged) {
    server.send(200, "text/html; charset=utf-8",
                "<!doctype html><meta charset=utf-8><meta name=viewport content='width=device-width'><body style='font:16px system-ui;padding:20px'>"
                "<h2>C'est noté</h2><p>La thermo-horloge redémarre et se connecte à « " + esc(cfg.ssid) + " ».</p>"
                "<p>Reviens sur ton WiFi habituel. La page de réglages sera ensuite sur <b>http://thermo-horloge.local</b> "
                "ou à l'adresse affichée dans Réglages &gt; Système.</p>");
    restartAt = millis() + 2500;
    return;
  }
  uiToast("Réglages enregistrés");
  server.sendHeader("Location", "/");
  server.send(303);
}

void webBegin() {
  server.on("/", HTTP_GET, sendPage);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/reboot", HTTP_POST, []() {
    server.send(200, "text/html; charset=utf-8", "<meta charset=utf-8><body style='font:16px system-ui;padding:20px'>Redémarrage…");
    restartAt = millis() + 1500;
  });
  server.on("/api", HTTP_GET, []() {
    String j = "{\"in\":" + String(isnan(clim.t) ? 0 : clim.t, 1) + ",\"hum\":" + String(isnan(clim.h) ? 0 : clim.h, 0) +
               ",\"raw\":" + String(isnan(clim.raw) ? 0 : clim.raw, 2) + ",\"out\":" + String(isnan(wx.t) ? 0 : wx.t, 1) + ",\"version\":\"" FW_VERSION "\"}";
    server.send(200, "application/json", j);
  });
  // Adresses que les téléphones et ordinateurs testent pour détecter un portail : on les renvoie vers la page
  static const char* const PROBES[] = {"/generate_204", "/gen_204", "/hotspot-detect.html", "/library/test/success.html",
                                       "/connecttest.txt", "/ncsi.txt", "/success.txt", "/canonical.html", "/redirect", "/mobile/status.php"};
  for (const char* p : PROBES) server.on(p, []() {
    Serial.printf("Test de portail : %s %s\n", server.hostHeader().c_str(), server.uri().c_str());
    if (netPortal() || (WiFi.getMode() & WIFI_AP)) {
      server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/");
      server.sendHeader("Cache-Control", "no-store");
      server.send(302, "text/html", "<a href=\"http://" + WiFi.softAPIP().toString() + "/\">Réglages de la thermo-horloge</a>");
    } else server.send(204);
  });
  server.onNotFound([]() {
    Serial.printf("Requete inconnue : %s %s\n", server.hostHeader().c_str(), server.uri().c_str());
    if (netPortal() || WiFi.getMode() & WIFI_AP) { server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/"); server.send(302); }
    else server.send(404, "text/plain", "Page introuvable");
  });
  updater.setup(&server, "/update");
  server.begin();
  if (netPortal()) WiFi.scanNetworks(true);
  else MDNS.begin("thermo-horloge");
}

void webLoop() {
  server.handleClient();
  static uint32_t beat = 0;
  if (millis() - beat > 15000) {   // signe de vie dans le journal série
    beat = millis();
    Serial.printf("[%lus] memoire %u, telephones connectes %d, WiFi %s\n", millis() / 1000, ESP.getFreeHeap(),
                  WiFi.softAPgetStationNum(), WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString().c_str() : "portail");
  }
  if (restartAt && millis() > restartAt) ESP.restart();
}
