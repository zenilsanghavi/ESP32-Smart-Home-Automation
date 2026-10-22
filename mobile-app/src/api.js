// Talks to the ESP32 web server from Relay.txt:
//   GET /status                    -> { states: [bool, bool, bool, bool] }
//   GET /relay?ch=<1-4|all>&state=<on|off> -> same shape

const TIMEOUT_MS = 3000;

async function request(host, path) {
  const controller = new AbortController();
  const timer = setTimeout(() => controller.abort(), TIMEOUT_MS);
  try {
    const res = await fetch(`http://${host}${path}`, { signal: controller.signal });
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const data = await res.json();
    if (!Array.isArray(data.states)) throw new Error('Unexpected response');
    return data.states;
  } finally {
    clearTimeout(timer);
  }
}

export function getStatus(host) {
  return request(host, '/status');
}

// channel: 1-4 or 'all'
export function setRelay(host, channel, on) {
  return request(host, `/relay?ch=${channel}&state=${on ? 'on' : 'off'}`);
}
