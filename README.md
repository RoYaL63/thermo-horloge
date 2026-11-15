# Thermo-horloge murale

Firmware de la thermo-horloge sur la carte Waveshare **ESP32-C6-Touch-LCD-2.8**, version 1.0.0.

- Présentation et guide pour la fabriquer : dossier [`docs/`](docs/index.html), publié en GitHub Pages.
- Maquette interactive de tous les écrans : [`tools-src/site/maquette.html`](tools-src/site/maquette.html).

## Mercredi : premier branchement

1. Brancher la carte au PC avec un câble USB-C **de données**.
2. Dans le terminal, depuis ce dossier :

   ```bash
   ./build.sh auto
   ```

   La carte est trouvée toute seule et le firmware est envoyé (environ 1 minute).
   Si l'envoi échoue : débrancher, maintenir le bouton **BOOT**, rebrancher, relâcher, relancer la commande.
3. L'écran affiche **Bienvenue**. Deux façons de lui donner le WiFi :
   - **sans rien taper** : toucher « Bouton WPS » sur l'écran (ou appuyer sur BOOT), puis appuyer
     sur le bouton WPS de la box dans les 2 minutes ;
   - **avec le téléphone** : scanner le QR code (ou rejoindre le WiFi **Thermo-Horloge**), la page s'ouvre
     (sinon `192.168.4.1`), choisir le réseau, coller le mot de passe, la ville et la pièce, puis **Enregistrer**.
     La page explique comment copier le mot de passe du WiFi depuis un Android ou un iPhone.

   Aucun téléphone ne laisse une application lire ses mots de passe WiFi enregistrés : le WPS ou le
   copier-coller sont les deux façons d'éviter de le retaper.
   Après le WPS, la ville se règle ensuite sur la page web (Lyon par défaut).
4. L'objet redémarre, se connecte, prend l'heure et la météo par internet.
5. Laisser tourner 30 minutes à côté d'un thermomètre fiable, puis corriger la sonde sur la page web (Étalonnage).

Pour voir ce que la carte raconte (adresses I2C, mesures, erreurs) : `./build.sh monitor`.

## À vérifier au premier allumage

Le firmware compile, mais n'a jamais tourné sur la vraie carte. Points à contrôler, dans l'ordre :

| Point | Comment le voir | Si ça ne va pas |
|---|---|---|
| Écran allumé | quelque chose s'affiche | lire `./build.sh monitor` : « Expandeur CH32 absent » signifie un problème de bus I2C |
| Couleurs justes | le bleu est bleu, le noir est noir | couleurs inversées : passer `cfg.invert` à `false` dans `board.h` |
| Toucher au bon endroit | le moniteur affiche `Toucher x=… y=…` | page web > Étalonnage : inverser X et Y, ou les miroirs |
| Adresses I2C | ligne `I2C :` au démarrage | attendu : 0x18 (audio), 0x24 (expandeur), 0x51 (horloge), 0x58 (tactile), 0x6A ou 0x6B (mouvement), 0x70 (sonde) |
| Son et micro | bip à la fin du minuteur, écran Sonomètre | réglage du codec ES8311 à reprendre dans `audio.cpp` |
| Mémoire | ligne `Image en memoire : oui` | si « non », l'écran scintille : signaler, on réduira l'image en mémoire |

## Ce que fait le firmware

- **Premier démarrage par le téléphone** : point d'accès Thermo-Horloge, page qui s'ouvre toute seule.
- **Heure** par internet, gardée par l'horloge de la carte en cas de coupure.
- **Météo Open-Meteo** : actuelle, 4 jours, lever et coucher, UV, pluie par quart d'heure sur 2 h, qualité de l'air.
- **Mesures** : température et humidité de la pièce, corrigées de l'échauffement, courbe sur 24 h.
- **12 thèmes** : Galaxie, Côté obscur, Cockpit, Radar, Hologramme, Portail, Assistant, Labo, Grand Prix, Terminal, Loutre, Magie.
  Chacun a son accueil, sa veille et ses menus, plus deux accueils au choix : Grande heure et Tableau de bord.
- **Écrans** : Accueil, Agenda, Pluie, Prévisions, Confort, Sonomètre, Minuteur, Ciel, Transports. Chacun peut être retiré.
- **Veille de nuit** : ambiance, horloge seule ou écran éteint, rouge doux, anti-marquage.
- **Agenda** : lien privé .ics (Google Agenda, iCloud), avec répétitions simples (quotidiennes, hebdomadaires, mensuelles, annuelles).
- **Rappels** : jusqu'à 8, saisis sur la page web, avec bip et message à l'écran.
- **Transports** : Paris (API PRIM d'Île-de-France Mobilités, clé gratuite) ou Lyon (données TCL du Grand Lyon, compte gratuit).
- **Page web** sur `http://thermo-horloge.local` : tous les réglages, et mise à jour du firmware sans câble (`/update`).

## Sur l'objet

- Toucher à droite ou à gauche : écran suivant ou précédent. Barre du bas : aller directement à un écran.
- Appui long (1 s) : Réglages (Affichage, Maison, Système). Les réglages complets sont sur la page web.
- Bouton BOOT : écran suivant ; appui de 2 s : Réglages.
- La nuit, toucher l'écran le réveille une minute.

## Mettre à jour plus tard, sans câble

```bash
./build.sh
```

puis sur la page web, **Mettre à jour le firmware**, et choisir `build/thermo_horloge.ino.bin`.

## Limites connues

- Pas encore de format paysage sur la carte (il existe dans la maquette).
- Transports : les deux services demandent un compte et une clé. Le format des réponses de Lyon est à confirmer avec de vrais identifiants.
- Agenda : les exceptions aux répétitions (dates supprimées) ne sont pas prises en compte.
- Sonomètre : niveaux approximatifs tant qu'il n'est pas étalonné.
- La page web n'a pas de mot de passe : toute personne sur le même WiFi peut changer les réglages.

## Organisation

| Fichier | Rôle |
|---|---|
| `firmware/thermo_horloge/board.*` | carte : expandeur, écran, tactile, sonde, horloge, batterie |
| `config.*` | réglages enregistrés dans la carte |
| `net.cpp` | WiFi, portail, heure, météo, agenda, transports |
| `web.cpp` | page de réglages et mise à jour |
| `ui.cpp`, `homes.cpp` | écrans, thèmes, veille, menus, tactile |
| `audio.cpp` | bips et sonomètre |
| `fonts.h`, `images.h` | polices et loutres, générées par `tools-src/` |

Les polices sont rendues depuis Google Fonts (licence SIL OFL) par `tools-src/fontgen.html` puis `node tools-src/fonts2c.js`.
Les images des loutres viennent des mascottes OtterWise (`node tools-src/img2c.js`).

Matériel vérifié sur le [wiki Waveshare](https://docs.waveshare.com/ESP32-C6-Touch-LCD-2.8) et les [exemples Waveshare](https://github.com/waveshareteam/ESP32-C6-Touch-LCD-2.8).
