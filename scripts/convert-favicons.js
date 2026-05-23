const sharp = require('sharp');
const fs = require('fs');
const path = require('path');

(async function(){
  try{
    const svgPath = path.join(__dirname, '..', 'img', 'favicon.svg');
    if(!fs.existsSync(svgPath)){
      console.error('SVG source not found:', svgPath);
      process.exit(1);
    }

    const outDir = path.join(__dirname, '..', 'img');
    const buffer = fs.readFileSync(svgPath);

    const targets = [
      { name: 'favicon-16.png', size: 16 },
      { name: 'favicon-32.png', size: 32 },
      { name: 'apple-touch-icon.png', size: 180 }
    ];

    for(const t of targets){
      const outPath = path.join(outDir, t.name);
      await sharp(buffer)
        .resize(t.size, t.size, { fit: 'contain', background: { r:0, g:0, b:0, alpha:0 } })
        .png({ quality: 100, compressionLevel: 9 })
        .toFile(outPath);
      console.log('Wrote', outPath);
    }

    console.log('\nAll favicons exported.');
  }catch(err){
    console.error(err);
    process.exit(1);
  }
})();
