import { h } from 'preact';
import { useState, useEffect, useRef } from 'preact/hooks';
import { fetchJSON } from '../api';

const FIELDS = [
  { key: 'door', label: 'Door' },
  { key: 'temperature', label: 'Temperature' },
  { key: 'humidity', label: 'Humidity' },
  { key: 'pressure', label: 'Pressure' },
  { key: 'water', label: 'Water' },
  { key: 'uptime', label: 'Uptime' },
];

export function Diagnostics() {
  const [data, setData] = useState(null);
  const [error, setError] = useState(false);
  const timer = useRef(null);

  async function load() {
    try {
      const d = await fetchJSON('/api/diagnostics');
      setData(d);
      setError(false);
    } catch {
      setError(true);
    }
    timer.current = setTimeout(load, 2000);
  }

  useEffect(() => {
    load();
    return () => clearTimeout(timer.current);
  }, []);

  if (error) return <div class="text-red-400 text-sm">Failed to load</div>;
  if (!data) return <div class="text-gray-500 text-sm">Loading...</div>;

  return (
    <div>
      {FIELDS.map(f => (
        <div key={f.key} class="flex justify-between py-2 border-b border-gray-700 last:border-0">
          <span class="text-sm text-gray-400">{f.label}</span>
          <span class="text-sm font-mono text-sky-400">{data[f.key]}</span>
        </div>
      ))}
    </div>
  );
}
