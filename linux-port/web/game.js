/* Original engine and assets stay local to this browser. */
import { SaveStore } from './saves.js';
const canvas = document.querySelector('#canvas');
const status = document.querySelector('#status');
const start = document.querySelector('#start');
const archive = document.querySelector('#archive');
const videos = document.querySelector('#videos');
const log = document.querySelector('#log');
const storage = document.querySelector('#storage');
const saveSlot = document.querySelector('#save-slot');
const exportSave = document.querySelector('#export-save');
const importSave = document.querySelector('#import-save');
const saves = new SaveStore();
let pendingSaves = Promise.resolve(), restoring = false;
let engine, running = false;
window.titbitFrames = 0;
const logLimit = 120000;
let logHistory = '', logPending = [], logPendingSize = 0, logTimer;
function message(text) { status.textContent = text; }
function flushLog() {
  logTimer = undefined;
  logHistory = (logHistory + logPending.join('')).slice(-logLimit);
  logPending = []; logPendingSize = 0;
  log.textContent = logHistory;
  // Reading scrollHeight forces layout. It is useful only while the user is
  // actually reading the journal, and must never run inside the engine frame.
  if (log.closest('details').open) log.scrollTop = log.scrollHeight;
}
function report(text) {
  text = String(text);
  if (['begin draw', 'EndDraw'].includes(text.trim())) return;
  const line = `${text}\n`.slice(-logLimit);
  logPending.push(line); logPendingSize += line.length;
  // Keep the pending batch bounded even during very verbose scene loading.
  while (logPendingSize > logLimit) {
    const excess = logPendingSize - logLimit, first = logPending[0];
    if (first.length <= excess) logPendingSize -= logPending.shift().length;
    else { logPending[0] = first.slice(excess); logPendingSize -= excess; }
  }
  if (logTimer === undefined) logTimer = setTimeout(flushLog, 100);
}
function failure(error) { running = false; console.error(error); report(error.stack || error); message(`Ошибка: ${error.message || error}`); }
function frame() {
  if (!running) return;
  try { engine._titbit_web_frame(); window.titbitFrames++; requestAnimationFrame(frame); }
  catch (error) { failure(error); }
}
async function play(bytes, localVideos = false) {
  start.disabled = true;
  try {
    if (bytes.length < 16 || bytes[0] !== 101 || bytes[1] !== 122 || bytes[2] !== 100 || bytes[3] !== 255) {
      throw new Error('Выбранный файл не является архивом QUEST.ezd');
    }
    engine.FS.mkdir('/game');
    engine.FS.writeFile('/game/QUEST.ezd', bytes, { canOwn: true });
    if (localVideos) {
      const listing = await fetch('/local-game/videos.json');
      if (!listing.ok) throw new Error('Не удалось получить список видеофайлов');
      for (const name of await listing.json()) {
        if (!/^[\w.-]+\.avi$/i.test(name)) throw new Error('Некорректное имя видеофайла');
        message(`Чтение ${name}…`);
        const response = await fetch(`/local-game/${encodeURIComponent(name)}`);
        if (!response.ok) throw new Error(`Не удалось прочитать ${name}`);
        engine.FS.writeFile(`/${name.toLowerCase()}`, new Uint8Array(await response.arrayBuffer()), { canOwn: true });
      }
    } else {
      for (const file of videos.files) {
        if (!/^[\w.-]+\.avi$/i.test(file.name)) continue;
        engine.FS.writeFile(`/${file.name.toLowerCase()}`, new Uint8Array(await file.arrayBuffer()), { canOwn: true });
      }
    }
    const bootstrap = await fetch('browser_start.scv');
    if (!bootstrap.ok) throw new Error('Не удалось загрузить сценарий запуска');
    engine.FS.writeFile('/game/browser_start.scv', await bootstrap.text());
    message('Загрузка первой сцены…');
    await new Promise(resolve => requestAnimationFrame(resolve));
    const result = engine.ccall('titbit_web_initialize', 'number', ['string'], ['sumerki_entrance.XML']);
    if (result) throw new Error(`Инициализация движка: ${result}`);
    document.querySelector('#files').hidden = true;
    running = true; canvas.focus({ preventScroll: true }); window.scrollTo(0, 0); requestAnimationFrame(frame);
    message('Управление мышью. Меню — Escape.');
  } catch (error) { failure(error); }
}
archive.addEventListener('change', () => { start.disabled = !engine || !archive.files.length; });
start.addEventListener('click', async () => {
  try { await play(new Uint8Array(await archive.files[0].arrayBuffer())); }
  catch (error) { failure(error); }
});
canvas.addEventListener('contextmenu', event => event.preventDefault());
exportSave.addEventListener('click', async () => {
  try {
    await pendingSaves;
    const url = URL.createObjectURL(await saves.exportFile(Number(saveSlot.value)));
    const link = document.createElement('a');link.href = url;link.download = `slot${saveSlot.value}.sav`;link.click();
    setTimeout(() => URL.revokeObjectURL(url), 1000);
  } catch (error) { storage.textContent = error.message; }
});
importSave.addEventListener('change', async () => {
  if (!importSave.files.length) return;
  try {
    await pendingSaves;
    const slot = Number(saveSlot.value);
    await saves.importFile(slot, importSave.files[0]);
    restoring = true;
    try { engine.FS.writeFile(`/slot${slot}.sav`, await saves.read(slot)); }
    finally { restoring = false; }
    storage.textContent = `Слот ${slot} импортирован. Выберите его в игровом меню загрузки.`;
  } catch (error) { storage.textContent = error.message; }
  finally { importSave.value = ''; }
});
createTitbit({ canvas, print: report, printErr: report, onPhase: message,
  onAbort: reason => failure(new Error(String(reason))),
  onAudioError: error => failure(error)
}).then(async module => {
  engine = module; window.titbit = module;
  // Hydrate the exact filenames used by the unmodified game scripts before
  // they inspect their save slots. The original serializer still owns bytes.
  for (const { slot } of await saves.list()) {
    if (slot >= 1 && slot <= 6) engine.FS.writeFile(`/slot${slot}.sav`, await saves.read(slot));
  }
  const close = engine.FS.close.bind(engine.FS);
  engine.FS.close = stream => {
    const path = stream.path;
    const writable = (stream.flags & 3) !== 0;
    const result = close(stream);
    const match = /(?:^|\/)slot([1-6])\.sav$/i.exec(path);
    if (match && writable && !restoring) {
      const slot = Number(match[1]), bytes = engine.FS.readFile(path);
      storage.textContent = `Сохранение слота ${slot}…`;
      pendingSaves = pendingSaves.catch(() => {}).then(() => saves.write(slot, bytes)).then(() => {
        storage.textContent = `Слот ${slot} сохранён в браузере.`;
      }).catch(error => { storage.textContent = `Ошибка сохранения: ${error.message}`;throw error; });
      // Keep failures visible without an unhandled rejection in the frame loop.
      pendingSaves.catch(() => {});
    }
    return result;
  };
  exportSave.disabled = importSave.disabled = false;
  start.disabled = !archive.files.length; message('Выберите QUEST.ezd и нажмите «Играть».');
  // Explicit development server option; the archive is never part of the build.
  if (new URLSearchParams(location.search).has('local')) {
    message('Чтение локального архива…');
    const response = await fetch('/local-game/QUEST.ezd');
    if (!response.ok) throw new Error('Локальный архив недоступен');
    await play(new Uint8Array(await response.arrayBuffer()), true);
  }
}).catch(failure);
