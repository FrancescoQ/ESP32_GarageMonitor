/**
 * Vite plugin that serves mock /api/* responses during development.
 * Active when VITE_ESP32_IP is not set in .env.development.local.
 */

// In-memory mock state
let users = [
  { name: 'Francesco', phone: '+393401234567', permissions: 0x0F },
  { name: 'Maria', phone: '+393407654321', permissions: 0x03 },
  { name: 'Neighbor', phone: '+393409999999', permissions: 0x01 },
];

let settings = {
  door_alert_min: 10,
  sms_poll_ms: 5000,
  deep_sleep: false,
  fwd_unknown: true,
  notify_reboot: true,
  auto_reboot: true,
  reboot_days: 7,
  reboot_hour: 4,
};

// Simulated sensor state (changes slightly each read)
function getDiagnostics() {
  const doorOpen = Math.random() > 0.7;
  const temp = 18 + Math.random() * 8;
  const hum = 55 + Math.random() * 20;
  const press = 1010 + Math.random() * 15;
  const water = Math.random() > 0.95;
  const uptimeMin = Math.floor((Date.now() % 86400000) / 60000);
  const h = Math.floor(uptimeMin / 60);
  const m = uptimeMin % 60;

  return {
    door: doorOpen ? `OPEN (${Math.floor(Math.random() * 30)} min)` : 'CLOSED',
    temperature: `${temp.toFixed(1)} C`,
    humidity: `${hum.toFixed(1)} %`,
    pressure: `${press.toFixed(0)} hPa`,
    water: water ? 'ALERT' : 'OK',
    uptime: `${h}h ${m}m`,
  };
}

/** Read request body as JSON */
function readBody(req) {
  return new Promise((resolve) => {
    let body = '';
    req.on('data', chunk => { body += chunk; });
    req.on('end', () => {
      try { resolve(JSON.parse(body)); }
      catch { resolve({}); }
    });
  });
}

/** Send JSON response */
function json(res, data, status = 200) {
  res.writeHead(status, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify(data));
}

/** Parse query string from URL */
function query(url) {
  const q = url.split('?')[1];
  if (!q) return {};
  return Object.fromEntries(new URLSearchParams(q));
}

export default function mockApiPlugin() {
  return {
    name: 'mock-api',
    configureServer(server) {
      server.middlewares.use(async (req, res, next) => {
        const path = req.url.split('?')[0];

        // GET /api/users
        if (path === '/api/users' && req.method === 'GET') {
          return json(res, { users });
        }

        // POST /api/users
        if (path === '/api/users' && req.method === 'POST') {
          const body = await readBody(req);
          if (!body.name || !body.phone) {
            return json(res, { ok: false, error: 'Name and phone required' }, 400);
          }
          users.push({
            name: body.name,
            phone: body.phone,
            permissions: body.permissions || 0x01
          });
          return json(res, { ok: true });
        }

        // DELETE /api/users?index=N
        if (path === '/api/users' && req.method === 'DELETE') {
          const q = query(req.url);
          const idx = parseInt(q.index);
          if (isNaN(idx) || idx < 0 || idx >= users.length) {
            return json(res, { ok: false, error: 'Invalid index' }, 400);
          }
          users.splice(idx, 1);
          return json(res, { ok: true });
        }

        // GET /api/settings
        if (path === '/api/settings' && req.method === 'GET') {
          return json(res, settings);
        }

        // POST /api/settings
        if (path === '/api/settings' && req.method === 'POST') {
          const body = await readBody(req);
          settings = { ...settings, ...body };
          return json(res, { ok: true });
        }

        // GET /api/diagnostics
        if (path === '/api/diagnostics' && req.method === 'GET') {
          return json(res, getDiagnostics());
        }

        next();
      });
    }
  };
}
