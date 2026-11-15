// Reproduction fidèle de firmware/thermo_horloge/jsonscan.h, pour tester l'algorithme sur de vraies réponses.
// node tools-src/test-jsonscan.js fichier.json
const fs = require('fs');

function JsonScan(cb) {
  const st = [{ key: '', arr: false, idx: -1 }];
  let depth = 0, inStr = false, esc = false, strDone = false, cur = '', pending = '', scal = '';
  const emit = v => {
    if (depth > 11) return;
    const t = st[depth];
    if (t.arr) { const p = st[depth > 0 ? depth - 1 : 0]; cb(p.key, p.idx, t.key, t.idx++, v); }
    else cb(t.key, t.idx, pending, -1, v);
  };
  const push = arr => {
    if (depth >= 11) { depth++; return; }
    const t = st[depth], n = {};
    if (t.arr) { n.key = t.key; n.idx = t.idx++; } else { n.key = pending; n.idx = -1; }
    n.arr = arr; if (arr) n.idx = 0;
    st[++depth] = n;
  };
  const flushScalar = () => { if (scal.length) { emit(scal); scal = ''; } };
  return c => {
    if (inStr) {
      if (esc) { esc = false; cur += c === 'n' ? ' ' : c; return; }
      if (c === '\\') { esc = true; return; }
      if (c === '"') { inStr = false; strDone = true; return; }
      if (cur.length < 240) cur += c; return;
    }
    if (' \n\r\t'.includes(c)) return;
    if (strDone) { strDone = false; if (c === ':') { pending = cur; return; } emit(cur); }
    switch (c) {
      case '"': flushScalar(); inStr = true; cur = ''; break;
      case '{': flushScalar(); push(false); break;
      case '[': flushScalar(); push(true); break;
      case '}': case ']': flushScalar(); if (depth > 0) depth--; break;
      case ',': flushScalar(); break;
      case ':': break;
      default: if (scal.length < 40) scal += c;
    }
  };
}

const text = fs.readFileSync(process.argv[2], 'utf8');
const out = { current: {}, daily: {}, minutely: [] };
const feed = JsonScan((ctx, ci, key, ki, v) => {
  if (ctx === 'current') out.current[key] = v;
  else if (ctx === 'daily' && ki >= 0) (out.daily[key] = out.daily[key] || [])[ki] = v;
  else if (ctx === 'minutely_15' && key === 'precipitation') out.minutely[ki] = v;
  else if (ctx === 'results' && ci === 0) out.current['geo_' + key] = v;
});
for (const ch of text) feed(ch);
console.log(JSON.stringify(out, null, 1));
