// Binary wire protocol shared with GlassSDK/examples/invaders (invaders_app.c,
// invaders_core.c).
// Keep both sides in sync if you change any layout or constant here.

export const CONFIG_CHANNEL = 0x4943; // 'IC', phone -> glasses
export const EVENT_CHANNEL = 0x4945; // 'IE', glasses -> phone
export const PROTOCOL_VERSION = 1;

export const COMMAND_CONFIG = 1;
export const COMMAND_NEW_GAME = 2;

export const EVENT_STATUS = 1;

export const STATE_IDLE = 0;
export const STATE_PLAYING = 1;
export const STATE_PAUSED = 2;
export const STATE_OVER = 3;

const STATUS_NAMES = ['idle', 'playing', 'paused', 'over'];

export const MIN_SENSITIVITY = 1;
export const MAX_SENSITIVITY = 10;
export const DEFAULT_SENSITIVITY = 5;

export const MIN_FIRE_RATE = 1;
export const MAX_FIRE_RATE = 10;
export const DEFAULT_FIRE_RATE = 4;

export const FIRE_MODE_AUTO = 0;
export const FIRE_MODE_BUTTON = 1;

export const DEFAULT_CONFIG = {
  sensitivity: DEFAULT_SENSITIVITY,
  fireMode: FIRE_MODE_AUTO,
  fireRate: DEFAULT_FIRE_RATE,
};

// [version, COMMAND_CONFIG, sensitivity(1..10), fire_mode(0/1), fire_rate(1..10)]
export function encodeConfig(config) {
  return new Uint8Array([
    PROTOCOL_VERSION,
    COMMAND_CONFIG,
    config.sensitivity,
    config.fireMode === FIRE_MODE_BUTTON ? 1 : 0,
    config.fireRate,
  ]);
}

// [version, COMMAND_NEW_GAME, 0, 0, 0] (padded to the 5-byte header).
export function encodeNewGame() {
  return new Uint8Array([PROTOCOL_VERSION, COMMAND_NEW_GAME, 0, 0, 0]);
}

export function clampConfig(config) {
  const clamp = (value, min, max, fallback) => {
    const number = Number(value);
    if (!Number.isFinite(number)) return fallback;
    return Math.min(max, Math.max(min, Math.round(number)));
  };
  return {
    sensitivity: clamp(config?.sensitivity, MIN_SENSITIVITY, MAX_SENSITIVITY, DEFAULT_SENSITIVITY),
    fireMode: config?.fireMode === FIRE_MODE_BUTTON ? FIRE_MODE_BUTTON : FIRE_MODE_AUTO,
    fireRate: clamp(config?.fireRate, MIN_FIRE_RATE, MAX_FIRE_RATE, DEFAULT_FIRE_RATE),
  };
}

// Decodes a glasses -> phone event. Returns null for anything unrecognized.
export function decodeEvent(data) {
  if (!data || data.length < 8 || data[0] !== PROTOCOL_VERSION) return null;
  const event = data[1];
  if (event !== EVENT_STATUS) return null;
  const unsigned16 = (low, high) => (data[low] | (data[high] << 8)) & 0xffff;
  return {
    event,
    score: unsigned16(2, 3),
    wave: data[4],
    lives: data[5],
    running: data[6] !== 0,
    state: STATUS_NAMES[data[7]] || 'idle',
  };
}
