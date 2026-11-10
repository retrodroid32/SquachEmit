'use strict';
// Standalone Node test: no browser dependencies or network needed.
// Validates dropdown labels and SHA-bound feature notes in the LAB UI.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const root = path.resolve(__dirname, '..');
const notes = require('../tools/firmware_build_notes.json').builds;
const ids = [
  'board', 'build', 'buildDetails', 'confirm', 'notice', 'channelBadge',
  'enterLab', 'exitLab', 'installArea', 'installLocked', 'installButton',
  'downloadBin', 'serialState', 'connect', 'disconnect', 'serialLog',
  'command', 'commandForm', 'send', 'clear', 'saveLog', 'baudRate', 'customBaud', 'baudHint'
];

class Element {
  constructor(id) {
    this.id = id;
    this.value = '';
    this.textContent = '';
    this.children = [];
    this.handlers = {};
    this.dataset = {};
    this.style = {};
    this.checked = false;
    this.hidden = false;
    this.classList = {toggle() {}};
  }
  addEventListener(event, fn) { this.handlers[event] = fn; }
  replaceChildren(...nodes) {
    this.children = nodes.slice();
    this.value = nodes.length ? nodes[0].value : '';
  }
  appendChild(node) { this.children.push(node); }
  setAttribute(name, value) { this[name] = value; }
}
const elements = Object.fromEntries(ids.map(id => [id, new Element(id)]));
elements.baudRate.value = '115200';
elements.customBaud.value = '115200';
const document = {
  getElementById(id) { return elements[id]; },
  createElement() { return new Element('option'); },
  querySelectorAll() { return []; }
};
const shaRows = [
  'fb57b628ce6a2539cb0940127763744e47be19bf',
  '6c08d0de80c0206a08ba99eaab4c324400a35bff',
  'e5cf01bf40a796f50750b37b1777da0273da92f1',
  '2d6e9f3f29b51b30d1b5731bc7b220e7d020312f'
];
const builds = shaRows.map((sha, index) => ({
  id: 'lab-c5-' + sha.slice(0, 8),
  board: 'c5-waveshare-28',
  channel: 'lab',
  sha, short_sha: sha.slice(0, 8),
  title: notes[sha].title,
  version_label: notes[sha].version,
  summary: notes[sha].summary,
  highlights: notes[sha].highlights,
  notes_verified_for_sha: true,
  created_at: '2026-11-09T18:00:00Z',
  branch: 'lab/fake-for-ui-test',
  manifest: 'lab/c5-' + sha.slice(0, 8) + '/manifest.json',
  application: 'lab/c5-' + sha.slice(0, 8) + '/application.bin',
  sha256: 'a'.repeat(64)
}));
builds.push({
  ...builds[0], id: 'lab-c5-00000000', sha: '0'.repeat(40),
  short_sha: '00000000', title: 'Uncatalogued LAB firmware (review changes)',
  summary: 'No SHA-specific feature notes have been reviewed for this build.',
  highlights: [], notes_verified_for_sha: false
});
builds.push({
  ...builds[0],
  id: 'lab-cyd32-471699cb',
  board: 'cyd32-st7798',
  sha: '471699cb08ad7e458b4d56a11e1f5ef265d54ed5',
  short_sha: '471699cb',
  title: notes['471699cb08ad7e458b4d56a11e1f5ef265d54ed5'].title,
  version_label: notes['471699cb08ad7e458b4d56a11e1f5ef265d54ed5'].version,
  summary: notes['471699cb08ad7e458b4d56a11e1f5ef265d54ed5'].summary,
  highlights: notes['471699cb08ad7e458b4d56a11e1f5ef265d54ed5'].highlights,
  manifest: 'lab/cyd32-471699cb/manifest.json',
  application: 'lab/cyd32-471699cb/application.bin'
});
// Newest CYD source-port build and older catalog entry intentionally coexist.
builds.push({
  ...builds[builds.length - 1],
  id: 'lab-cyd32-caa89d82',
  sha: 'caa89d82047b73cecfbd9d9b476c7bc2fa2a14bb',
  short_sha: 'caa89d82',
  title: notes['caa89d82047b73cecfbd9d9b476c7bc2fa2a14bb'].title,
  version_label: notes['caa89d82047b73cecfbd9d9b476c7bc2fa2a14bb'].version,
  summary: notes['caa89d82047b73cecfbd9d9b476c7bc2fa2a14bb'].summary,
  highlights: notes['caa89d82047b73cecfbd9d9b476c7bc2fa2a14bb'].highlights,
  created_at: '2026-11-10T13:23:09Z',
  manifest: 'lab/cyd32-caa89d82/manifest.json',
  application: 'lab/cyd32-caa89d82/application.bin'
});
let fetches = 0;
const context = {
  document,
  location: { search: '?lab=1' },
  navigator: { serial: {} },
  window: { isSecureContext: true, addEventListener() {} },
  URLSearchParams, TextEncoder, TextDecoder,
  fetch: async () => { fetches++; return {ok: true, json: async () => ({builds})}; },
  console
};
vm.runInNewContext(fs.readFileSync(path.join(root, 'web-installer/app.js'), 'utf8'),
                   context, {timeout: 2500});
setImmediate(() => {
  assert.equal(fetches, 1);
  assert.equal(elements.board.value, 'c5-waveshare-28');
  const options = elements.build.children;
  for (let i = 0; i < shaRows.length; i++) {
    const label = options.find(option => option.value === builds[i].id)?.textContent;
    assert.ok(label, 'missing build ' + builds[i].id);
    assert.ok(label.includes(notes[shaRows[i]].title), 'not descriptive: ' + label);
    assert.ok(label.includes(shaRows[i].slice(0, 8)), 'missing short SHA');
  }
  elements.build.value = builds[0].id;
  elements.build.handlers.change();
  assert.ok(elements.buildDetails.textContent.includes('DJI'));
  assert.ok(elements.buildDetails.textContent.includes('AUTEL') ||
            elements.buildDetails.textContent.includes('Autel'));
  assert.ok(elements.buildDetails.textContent.includes(shaRows[0]));
  assert.ok(elements.buildDetails.textContent.includes('FEATURES:'));
  elements.build.value = builds[4].id;
  elements.build.handlers.change();
  assert.ok(elements.buildDetails.textContent.includes('no curated changelog') ||
            elements.buildDetails.textContent.includes('no curated') ||
            elements.buildDetails.textContent.includes('no SHA-specific') ||
            elements.buildDetails.textContent.includes('no reviewed'));
  assert.equal(elements.confirm.checked, false);
  // Switch board; only the 3.2-inch ESP32 build must be visible.
  elements.board.value = 'cyd32-st7798';
  elements.board.handlers.change();
  assert.equal(elements.build.children.length, 2); // placeholder + CYD
  elements.build.value = 'lab-cyd32-caa89d82';
  elements.build.handlers.change();
  assert.ok(elements.buildDetails.textContent.includes('ST7798'));
  assert.ok(elements.buildDetails.textContent.includes('caa89d82'));
  assert.ok(elements.buildDetails.textContent.includes('Source-derived') || elements.buildDetails.textContent.includes('source-derived'));
  assert.equal(elements.build.children[1].value, 'lab-cyd32-caa89d82');
  assert.equal(elements.confirm.checked, false);
  // Regression: when 2 CYD builds are present, only the newest is shown.
  // The newest is selected by created_at, not the catalog array's order.
  assert.equal(elements.build.children.length, 2);
  console.log('Installer build dropdown / C5 / CYD3.2 warning / SHA fallback: PASS');
});
