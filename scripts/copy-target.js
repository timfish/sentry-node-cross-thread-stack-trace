const fs = require('fs');
const path = require('path');
const binaries = require('./binaries.js');

const build = path.resolve(__dirname, '..', 'lib');
if (!fs.existsSync(build)) {
  fs.mkdirSync(build, { recursive: true });
}

const source = binaries.source;
const target = binaries.target;

if (!fs.existsSync(source)) {
  console.log('Source file does not exist:', source);
  process.exit(1);
} else {
  if (fs.existsSync(target)) {
    console.log('Target file already exists, overwriting it');
    fs.unlinkSync(target);
  }
  console.log('Copying', source, 'to', target);
  fs.copyFileSync(source, target);
}
