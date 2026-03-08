import { h } from 'preact';
import { useState, useEffect } from 'preact/hooks';
import { fetchJSON, postJSON } from '../api';
import { Toggle } from '../components/Toggle';
import { FormRow } from '../components/FormRow';

function StatusMsg({ text, ok }) {
  if (!text) return null;
  return (
    <div class={
      'text-sm px-3 py-2 rounded mb-3 ' +
      (ok ? 'bg-green-900 text-green-300' : 'bg-red-900 text-red-300')
    }>
      {text}
    </div>
  );
}

export function Settings() {
  const [s, setS] = useState(null);
  const [msg, setMsg] = useState(null);

  function flash(text, ok) {
    setMsg({ text, ok });
    setTimeout(() => setMsg(null), 4000);
  }

  async function load() {
    try {
      const d = await fetchJSON('/api/settings');
      setS(d);
    } catch {
      flash('Failed to load settings', false);
    }
  }

  useEffect(() => { load(); }, []);

  function update(key, val) {
    setS(prev => ({ ...prev, [key]: val }));
  }

  async function save() {
    try {
      const d = await postJSON('/api/settings', s);
      if (d.ok) {
        flash('Settings saved', true);
      } else {
        flash(d.error || 'Failed', false);
      }
    } catch {
      flash('Request failed', false);
    }
  }

  if (!s) return <div class="text-gray-500 text-sm">Loading...</div>;

  return (
    <div>
      <StatusMsg text={msg?.text} ok={msg?.ok} />

      <FormRow label="Door alert" hint="SMS alert if door stays open longer than this">
        <input
          type="number"
          min="1"
          max="1440"
          value={s.door_alert_min}
          onInput={e => update('door_alert_min', parseInt(e.target.value))}
          class="w-20 bg-gray-900 border border-gray-600 rounded px-2 py-1.5 text-sm text-gray-200"
        />
        <span class="text-xs text-gray-400">min</span>
      </FormRow>

      <FormRow label="SMS poll" hint="How often to check for incoming SMS">
        <input
          type="number"
          min="1000"
          max="60000"
          step="1000"
          value={s.sms_poll_ms}
          onInput={e => update('sms_poll_ms', parseInt(e.target.value))}
          class="w-24 bg-gray-900 border border-gray-600 rounded px-2 py-1.5 text-sm text-gray-200"
        />
        <span class="text-xs text-gray-400">ms</span>
      </FormRow>

      <FormRow label="Deep sleep" hint="Enable low-power deep sleep between checks (Phase 5)">
        <Toggle checked={s.deep_sleep} onChange={v => update('deep_sleep', v)} />
      </FormRow>

      <FormRow label="Fwd unknown SMS" hint="Forward SMS from unknown numbers to admin users">
        <Toggle checked={s.fwd_unknown} onChange={v => update('fwd_unknown', v)} />
      </FormRow>

      <h3 class="text-sky-400 text-sm font-semibold mb-3 mt-2">Environment Alerts</h3>

      <FormRow label="Env alerts" hint="SMS admins when temperature/humidity exceeds thresholds">
        <Toggle checked={s.env_alert} onChange={v => update('env_alert', v)} />
      </FormRow>

      <FormRow label="Temp min" hint="Alert below this temperature (-50 to 80)">
        <input
          type="number"
          min="-50"
          max="80"
          step="0.5"
          value={s.temp_min}
          onInput={e => update('temp_min', parseFloat(e.target.value))}
          class="w-20 bg-gray-900 border border-gray-600 rounded px-2 py-1.5 text-sm text-gray-200"
        />
        <span class="text-xs text-gray-400">&deg;C</span>
      </FormRow>

      <FormRow label="Temp max" hint="Alert above this temperature (-50 to 80)">
        <input
          type="number"
          min="-50"
          max="80"
          step="0.5"
          value={s.temp_max}
          onInput={e => update('temp_max', parseFloat(e.target.value))}
          class="w-20 bg-gray-900 border border-gray-600 rounded px-2 py-1.5 text-sm text-gray-200"
        />
        <span class="text-xs text-gray-400">&deg;C</span>
      </FormRow>

      <FormRow label="Humidity max" hint="Alert above this humidity level (0-100)">
        <input
          type="number"
          min="0"
          max="100"
          step="1"
          value={s.hum_max}
          onInput={e => update('hum_max', parseFloat(e.target.value))}
          class="w-20 bg-gray-900 border border-gray-600 rounded px-2 py-1.5 text-sm text-gray-200"
        />
        <span class="text-xs text-gray-400">%</span>
      </FormRow>

      <h3 class="text-sky-400 text-sm font-semibold mb-3 mt-2">Auto Reboot</h3>

      <FormRow label="Auto reboot" hint="Periodically restart the system for reliability">
        <Toggle checked={s.auto_reboot} onChange={v => update('auto_reboot', v)} />
      </FormRow>

      <FormRow label="Reboot every" hint="Days between automatic reboots (1-30)">
        <input
          type="number"
          min="1"
          max="30"
          value={s.reboot_days}
          onInput={e => update('reboot_days', parseInt(e.target.value))}
          class="w-20 bg-gray-900 border border-gray-600 rounded px-2 py-1.5 text-sm text-gray-200"
        />
        <span class="text-xs text-gray-400">days</span>
      </FormRow>

      <FormRow label="Reboot hour" hint="Hour of day to reboot (0-23), or -1 to reboot as soon as interval elapses">
        <input
          type="number"
          min="-1"
          max="23"
          value={s.reboot_hour}
          onInput={e => update('reboot_hour', parseInt(e.target.value))}
          class="w-20 bg-gray-900 border border-gray-600 rounded px-2 py-1.5 text-sm text-gray-200"
        />
        <span class="text-xs text-gray-400">(-1=any)</span>
      </FormRow>

      <FormRow label="Reboot SMS notify" hint="SMS admins before and after a scheduled reboot">
        <Toggle checked={s.notify_reboot} onChange={v => update('notify_reboot', v)} />
      </FormRow>

      <button
        onClick={save}
        class="bg-blue-800 hover:bg-blue-700 text-white text-sm px-4 py-1.5 rounded mt-2 cursor-pointer"
      >
        Save Settings
      </button>
    </div>
  );
}
