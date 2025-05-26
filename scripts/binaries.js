const os = require('os');
const path = require('path');
const abi = require('node-abi');
const libc = require('detect-libc');

function getModuleName() {
  const stdlib = libc.familySync();
  const platform = process.env['BUILD_PLATFORM'] || os.platform();
  const arch = process.env['BUILD_ARCH'] || os.arch();
  const identifier = [platform, arch, stdlib, abi.getAbi(process.versions.node, 'node')].filter(Boolean).join('-');
  return `stack-trace-${identifier}.node`;
}

const source = path.join(__dirname, '..', 'build', 'Release', 'stack-trace.node');
const target = path.join(__dirname, '..', 'lib', getModuleName());

module.exports.source = source;
module.exports.target = target;
module.exports.getModuleName = getModuleName;
