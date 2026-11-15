// Convertit les polices VLW produites par fontgen.html en tableaux C, et dessine un aperçu pour vérifier le format.
// node tools-src/fonts2c.js
const fs = require('fs');
const path = require('path');
const root = path.join(__dirname, '..');
const dom = fs.readFileSync(path.join(__dirname, 'fonts.dom.html'), 'utf8');
const body = dom.slice(dom.indexOf('FONTS') + 6, dom.indexOf('FIN'));
const fonts = body.trim().split('\n').map(l => { const [n, b] = l.trim().split(' '); return [n, Buffer.from(b, 'base64')]; });

let h = '// Polices lissées (format VLW) générées par tools-src/fontgen.html. Ne pas modifier à la main.\n#pragma once\n#include <stdint.h>\n\n';
for (const [n, buf] of fonts) {
  h += `const uint8_t F_${n}[${buf.length}] = {`;
  for (let i = 0; i < buf.length; i++) h += (i % 32 ? '' : '\n  ') + buf[i] + ',';
  h += '\n};\n\n';
}
fs.writeFileSync(path.join(root, 'firmware/thermo_horloge/fonts.h'), h);

// Aperçu : décode chaque police comme le ferait l'afficheur et écrit une image PGM
function decode(buf) {
  const r = o => buf.readInt32BE(o);
  const n = r(0), size = r(8), asc = r(16), desc = r(20);
  const gl = new Map(); let off = 24 + n * 28;
  for (let i = 0; i < n; i++) {
    const o = 24 + i * 28, g = { cp: r(o), h: r(o + 4), w: r(o + 8), adv: r(o + 12), dY: r(o + 16), dX: r(o + 20), off };
    off += g.w * g.h; gl.set(g.cp, g);
  }
  return { n, size, asc, desc, gl };
}
const W = 900, lineH = 92, H = lineH * fonts.length, img = Buffer.alloc(W * H, 20);
fonts.forEach(([name, buf], row) => {
  const f = decode(buf);
  const sample = f.gl.has(65) ? 'Éé 21,4°C Prévisions àç' : '19:47 21,4°';
  let x = 6; const base = row * lineH + Math.min(lineH - 8, f.asc + 4);
  for (const ch of sample) {
    const g = f.gl.get(ch.codePointAt(0)); if (!g) { x += 6; continue; }
    for (let yy = 0; yy < g.h; yy++) for (let xx = 0; xx < g.w; xx++) {
      const px = x + g.dX + xx, py = base - g.dY + yy;
      if (px >= 0 && px < W && py >= row * lineH && py < (row + 1) * lineH) img[py * W + px] = Math.max(img[py * W + px], buf[g.off + yy * g.w + xx]);
    }
    x += g.adv;
  }
  console.log(name.padEnd(9), 'glyphes', String(f.n).padStart(3), 'taille', f.size, 'octets', buf.length);
});
fs.writeFileSync(path.join(__dirname, 'fonts-preview.pgm'), Buffer.concat([Buffer.from(`P5 ${W} ${H} 255\n`), img]));
