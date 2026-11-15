// Convertit les images PNG de tools-src/img en tableaux C (affichées par drawPng sur la carte).
const fs=require('fs'),path=require('path'),dir=path.join(__dirname,'img');
let h='// Images des loutres (PNG) générées par tools-src/img2c.js. Ne pas modifier à la main.\n#pragma once\n#include <stdint.h>\n\n';
for(const f of fs.readdirSync(dir).filter(f=>f.endsWith('.png')).sort()){const b=fs.readFileSync(path.join(dir,f)),n='IMG_'+f.replace('.png','').toUpperCase();
 h+=`const uint8_t ${n}[${b.length}] = {`;for(let i=0;i<b.length;i++)h+=(i%32?'':'\n  ')+b[i]+',';h+='\n};\n\n';console.log(n,b.length,'octets',b.readUInt32BE(16)+'x'+b.readUInt32BE(20));}
fs.writeFileSync(path.join(__dirname,'..','firmware/thermo_horloge/images.h'),h);
