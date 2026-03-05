# Garage Monitor Web UI

Preact + Tailwind CSS frontend for the garage monitoring system's setup page. Built with Vite, outputs minified files to `data/` for LittleFS upload to the ESP32.

## Quick Start

```bash
npm install
npm run dev      # Dev server with mock data at http://localhost:3000
npm run build    # Production build → ../data/
```

## Development

### Mock API (default)

Running `npm run dev` without configuration serves mock API responses — 3 sample users, working settings CRUD, and simulated sensor readings that update every 2 seconds. All state is in-memory and resets on restart.

### Real ESP32

To proxy API calls to a real ESP32, create `.env.development.local`:

```
VITE_ESP32_IP=192.168.4.1
```

The dev server will proxy all `/api/*` requests to the ESP32 instead of using mock data.

## Production Build

```bash
npm run build
```

Outputs to `../data/`:
- `index.html` — minimal shell
- `app.js` — minified JS bundle
- `app.css` — purged Tailwind CSS

Upload to ESP32 with `pio run -t uploadfs`.

## Structure

```
web_ui_src/
  index.html              Entry point
  vite.config.js          Build config + dev proxy/mock selection
  tailwind.config.js      Tailwind purge paths
  postcss.config.js       PostCSS plugins
  dev/
    mock-api.js           Mock API Vite plugin (dev only, not in build)
  src/
    app.jsx               Root component, tab routing
    styles.css             Tailwind directives
    api.js                 fetchJSON/postJSON helpers
    components/
      Toggle.jsx           Toggle switch
      FormRow.jsx          Label + input + hint layout
      PermBadge.jsx        Permission badge
    views/
      Users.jsx            User list + add user form
      Settings.jsx         All settings with toggles/inputs
      Diagnostics.jsx      Live sensor readings (2s auto-refresh)
```

## API Endpoints (unchanged)

| Method | Path | Description |
|--------|------|-------------|
| GET | `/api/users` | List authorized users |
| POST | `/api/users` | Add user `{name, phone, permissions}` |
| DELETE | `/api/users?index=N` | Remove user by index |
| GET | `/api/settings` | Get current settings |
| POST | `/api/settings` | Update settings |
| GET | `/api/diagnostics` | Live sensor readings |
