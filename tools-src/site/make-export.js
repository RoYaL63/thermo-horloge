// Prépare une page qui rend les écrans de la maquette à heure et météo fixes, puis récupère les images.
// node tools-src/site/make-export.js <maquette.html>   puis Chrome --dump-dom, puis node tools-src/site/save-images.js
const fs = require('fs');
const path = require('path');
const mock = fs.readFileSync(process.argv[2], 'utf8');

// Heure figée : mardi 18 novembre 2026, 18 h 42
const pre = `<script>(function(){
  const R = Date, T = new R(2026, 10, 18, 18, 42, 7).getTime();
  function F(...a) { if (!(this instanceof F)) return new R(T).toString(); return a.length ? new R(...a) : new R(T); }
  F.prototype = R.prototype; F.now = () => T; F.UTC = R.UTC; F.parse = R.parse; window.Date = F;
  try { localStorage.clear(); } catch (e) {}
})();</script>`;

const post = `<script>
setTimeout(() => {
  try {
    document.body.innerHTML = '';
    const out = [];
    S.tin = 21.4; S.tout = 9.6; S.hum = 48; S.wx = 'nuage'; S.wxd = 'eclaircies';
    S.timer = { total: 600, remain: 452, running: false }; S.ota = 0.62;
    const shot = (name, th, scr, o = {}) => {
      const cv = document.createElement('canvas'); cv.width = 480; cv.height = 640;
      S.theme = th; S.hv[th] = o.v || 0; S.ns = o.ns || 0; S.nc = o.nc == null ? 0 : o.nc; S.nb = 0.6;
      render(cv, th, scr, 9.3, true);
      out.push(name + ' ' + cv.toDataURL('image/png').split(',')[1]);
    };
    [['galaxie',0],['obscur',1],['cockpit',0],['radar',0],['holo',0],['portail',0],['assistant',0],['labo',0],['grandprix',0],['bracelet',0],['loutre',0],['grimoire',0]]
      .forEach(([t, v]) => shot('theme-' + t, t, 0, { v }));
    [['agenda',15],['pluie',12],['previsions',1],['confort',2],['sonometre',13],['minuteur',3],['ciel',4],['transports',10],
     ['reglages',6],['ecrans',11],['wifi',7],['miseajour',8],['couleurs',9],['nuitreglage',17]]
      .forEach(([n, s]) => shot('ecran-' + n, 'galaxie', s));
    [['grandprix',3],['loutre',1],['grimoire',1],['labo',2],['portail',6]].forEach(([t, s]) => shot('ecran-' + t + '-' + s, t, s));
    shot('variante-grande', 'assistant', 0, { v: 1 });
    shot('variante-tableau', 'galaxie', 0, { v: 3 });
    shot('nuit-galaxie', 'galaxie', 5, { ns: 0, nc: 0 });
    shot('nuit-grimoire', 'grimoire', 5, { ns: 0, nc: 1 });
    shot('nuit-loutre', 'loutre', 5, { ns: 0, nc: 1 });
    shot('nuit-horloge', 'portail', 5, { ns: 1, nc: 1 });
    const p = document.createElement('pre'); p.id = 'out';
    p.textContent = 'IMGS\\n' + out.join('\\n') + '\\nFIN';
    document.body.appendChild(p);
  } catch (e) { document.body.textContent = 'ERREUR ' + e.stack; }
}, 2500);
</script>`;

fs.writeFileSync(path.join(__dirname, 'export.html'), '<!doctype html><meta charset=utf-8>' + pre + mock + post);
