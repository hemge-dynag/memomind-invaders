import { createGMPlugin } from './vendor/gm-plugin-web-sdk.esm.js';
import {
  CONFIG_CHANNEL,
  EVENT_CHANNEL,
  FIRE_MODE_AUTO,
  FIRE_MODE_BUTTON,
  DEFAULT_CONFIG,
  clampConfig,
  decodeEvent,
  encodeConfig,
  encodeNewGame,
} from './protocol.js';
import { detectLocale, getStrings } from './translations.js';

const S = getStrings(detectLocale());

const STORAGE_KEY = 'invaders';

const gm = createGMPlugin();

const statusEl = document.querySelector('#status');
const sensitivityInput = document.querySelector('#sensitivity');
const sensitivityValue = document.querySelector('#sensitivity-value');
const fireRateInput = document.querySelector('#fire-rate');
const fireRateValue = document.querySelector('#fire-rate-value');
const fireAutoButton = document.querySelector('#fire-auto');
const fireButtonButton = document.querySelector('#fire-button');
const newGameButton = document.querySelector('#new-game');
const scoreEl = document.querySelector('#score');
const waveEl = document.querySelector('#wave');
const livesEl = document.querySelector('#lives');
const gameStateEl = document.querySelector('#game-state');

let config = { ...DEFAULT_CONFIG };
let connected = false;
let game = { score: 0, wave: 1, lives: 3, state: 'idle' };

function setStatus(text, state = '') {
  statusEl.textContent = text;
  statusEl.className = `status ${state}`.trim();
}

function applyStrings() {
  document.documentElement.lang = detectLocale();
  const text = (selector, value) => {
    document.querySelector(selector).textContent = value;
  };
  document.title = S.appTitle;
  text('#app-title', S.appTitle);
  text('#section-controls', S.sectionControls);
  text('#sensitivity-label', S.sensitivityLabel);
  text('#sensitivity-hint', S.sensitivityHint);
  text('#fire-mode-label', S.fireModeLabel);
  text('#fire-auto', S.fireModeAuto);
  text('#fire-button', S.fireModeButton);
  text('#fire-rate-label', S.fireRateLabel);
  text('#new-game', S.newGame);
  text('#section-game', S.sectionGame);
  text('#score-label', S.score);
  text('#wave-label', S.wave);
  text('#lives-label', S.lives);
  text('#hint', S.hint);
  setStatus(S.statusStarting);
}

function stateLabel(state) {
  if (state === 'playing') return S.statePlaying;
  if (state === 'paused') return S.statePaused;
  if (state === 'over') return S.stateOver;
  return S.stateIdle;
}

async function loadState() {
  const result = await gm.storage.get(STORAGE_KEY);
  if (result.value && result.value.config) {
    config = clampConfig({ ...config, ...result.value.config });
  }
}

async function persistState() {
  await gm.storage.set(STORAGE_KEY, { config });
}

function renderControls() {
  sensitivityInput.value = String(config.sensitivity);
  sensitivityValue.textContent = String(config.sensitivity);
  fireRateInput.value = String(config.fireRate);
  fireRateValue.textContent = String(config.fireRate);
  const automatic = config.fireMode === FIRE_MODE_AUTO;
  fireAutoButton.classList.toggle('active', automatic);
  fireButtonButton.classList.toggle('active', !automatic);
  fireAutoButton.setAttribute('aria-checked', automatic ? 'true' : 'false');
  fireButtonButton.setAttribute('aria-checked', automatic ? 'false' : 'true');
  sensitivityInput.disabled = !connected;
  fireRateInput.disabled = !connected;
  fireAutoButton.disabled = !connected;
  fireButtonButton.disabled = !connected;
  newGameButton.disabled = !connected;
}

function renderGame() {
  scoreEl.textContent = String(game.score);
  waveEl.textContent = String(game.wave);
  livesEl.textContent = String(game.lives);
  gameStateEl.textContent = stateLabel(game.state);
}

async function sendConfig() {
  if (!connected) return;
  try {
    await gm.plugin.sendMessage(CONFIG_CHANNEL, encodeConfig(config));
  } catch (error) {
    setStatus(error.message || String(error), 'error');
  }
}

async function onConfigChanged() {
  await persistState();
  renderControls();
  await sendConfig();
}

sensitivityInput.addEventListener('input', () => {
  config.sensitivity = Number(sensitivityInput.value);
  sensitivityValue.textContent = sensitivityInput.value;
  void onConfigChanged();
});

fireRateInput.addEventListener('input', () => {
  config.fireRate = Number(fireRateInput.value);
  fireRateValue.textContent = fireRateInput.value;
  void onConfigChanged();
});

function setFireMode(mode) {
  if (config.fireMode === mode) return;
  config.fireMode = mode;
  void onConfigChanged();
}

fireAutoButton.addEventListener('click', () => setFireMode(FIRE_MODE_AUTO));
fireButtonButton.addEventListener('click', () => setFireMode(FIRE_MODE_BUTTON));

newGameButton.addEventListener('click', async () => {
  if (!connected) return;
  try {
    await gm.plugin.sendMessage(CONFIG_CHANNEL, encodeNewGame());
  } catch (error) {
    setStatus(error.message || String(error), 'error');
  }
});

const offMessages = gm.plugin.onMessage((message) => {
  if (message.channel !== EVENT_CHANNEL) return;
  const decoded = decodeEvent(message.data);
  if (!decoded || decoded.event !== EVENT_STATUS) return;
  game = {
    score: decoded.score,
    wave: decoded.wave || 1,
    lives: decoded.lives,
    state: decoded.state,
  };
  renderGame();
});

async function start() {
  try {
    await gm.ready();
    await loadState();
    applyStrings();
    renderControls();
    renderGame();
    const info = await gm.device.getInfo();
    connected = Boolean(info.connected);
    setStatus(connected ? S.statusConnected : S.statusDisconnected, connected ? 'ready' : 'error');
    renderControls();
    await sendConfig();
    await gm.device.subscribeEvents(['connection']);
    gm.device.onConnection((event) => {
      connected = Boolean(event.connected);
      setStatus(connected ? S.statusConnected : S.statusDisconnected, connected ? 'ready' : 'error');
      renderControls();
      if (connected) void sendConfig();
    });
  } catch (error) {
    setStatus(error.message || String(error), 'error');
  }
}

window.addEventListener('pagehide', () => {
  offMessages();
  gm.close();
});

void start();
