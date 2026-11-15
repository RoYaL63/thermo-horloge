// Enregistre les images rendues par export.html (récupérées avec chrome --dump-dom) dans docs/img.
const fs = require('fs');
const path = require('path');
const t = fs.readFileSync(path.join(__dirname, 'export.dom.html'), 'utf8');
if (t.includes('ERREUR')) { console.error(t.slice(t.indexOf('ERREUR'), t.indexOf('ERREUR') + 400)); process.exit(1); }
const body = t.slice(t.indexOf('IMGS') + 5, t.lastIndexOf('FIN'));   // le dernier : « FIN » peut apparaître dans les données
let n = 0;
for (const l of body.trim().split('\n')) {
  const [k, v] = l.trim().split(' ');
  fs.writeFileSync(path.join(__dirname, '../../docs/img', k + '.png'), Buffer.from(v, 'base64'));
  n++;
}
console.log(n, 'images enregistrées');
