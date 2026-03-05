import { h } from 'preact';
import { useState, useEffect } from 'preact/hooks';
import { fetchJSON, postJSON, noCache } from '../api';
import { PermBadge } from '../components/PermBadge';

const PERMS = [
  { key: 'STATUS', bit: 0x01 },
  { key: 'CLOSE',  bit: 0x02 },
  { key: 'OPEN',   bit: 0x04 },
  { key: 'CONFIG', bit: 0x08 },
];

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

export function Users() {
  const [users, setUsers] = useState([]);
  const [msg, setMsg] = useState(null);
  const [name, setName] = useState('');
  const [phone, setPhone] = useState('');
  const [perms, setPerms] = useState(0x01); // STATUS checked by default

  function flash(text, ok) {
    setMsg({ text, ok });
    setTimeout(() => setMsg(null), 4000);
  }

  async function load() {
    try {
      const d = await fetchJSON('/api/users');
      setUsers(d.users);
    } catch {
      flash('Failed to load users', false);
    }
  }

  useEffect(() => { load(); }, []);

  async function addUser() {
    if (!name.trim() || !phone.trim()) {
      flash('Name and phone required', false);
      return;
    }
    try {
      const d = await postJSON('/api/users', {
        name: name.trim(),
        phone: phone.trim(),
        permissions: perms
      });
      if (d.ok) {
        flash('User added', true);
        setName('');
        setPhone('');
        setPerms(0x01);
        load();
      } else {
        flash(d.error || 'Failed', false);
      }
    } catch {
      flash('Request failed', false);
    }
  }

  async function delUser(idx) {
    if (!confirm('Remove this user?')) return;
    try {
      const r = await fetch(noCache('/api/users?index=' + idx), { method: 'DELETE' });
      const d = await r.json();
      if (d.ok) {
        flash('User removed', true);
        load();
      } else {
        flash(d.error || 'Failed', false);
      }
    } catch {
      flash('Request failed', false);
    }
  }

  function togglePerm(bit) {
    setPerms(p => p ^ bit);
  }

  return (
    <div>
      <StatusMsg text={msg?.text} ok={msg?.ok} />

      <table class="w-full text-sm mb-4">
        <thead>
          <tr class="text-sky-400 text-xs">
            <th class="text-left py-1 px-2">Name</th>
            <th class="text-left py-1 px-2">Phone</th>
            <th class="text-left py-1 px-2">Permissions</th>
            <th class="w-8"></th>
          </tr>
        </thead>
        <tbody>
          {users.map((u, i) => (
            <tr key={i} class="border-t border-gray-700">
              <td class="py-2 px-2">{u.name}</td>
              <td class="py-2 px-2 text-xs">{u.phone}</td>
              <td class="py-2 px-2">
                {PERMS.map(p => (
                  <PermBadge key={p.key} name={p.key} active={u.permissions & p.bit} />
                ))}
              </td>
              <td class="py-2 px-2">
                <button
                  onClick={() => delUser(i)}
                  class="bg-red-800 hover:bg-red-700 text-white text-xs px-2 py-1 rounded cursor-pointer"
                >
                  X
                </button>
              </td>
            </tr>
          ))}
          {users.length === 0 && (
            <tr><td colspan="4" class="text-center py-4 text-gray-500">No users</td></tr>
          )}
        </tbody>
      </table>

      <h3 class="text-sky-400 text-sm font-semibold mb-3 mt-4">Add User</h3>

      <div class="space-y-2 mb-3">
        <div class="flex items-center gap-2">
          <label class="min-w-[60px] text-xs text-gray-400">Name</label>
          <input
            value={name}
            onInput={e => setName(e.target.value)}
            placeholder="e.g. Family"
            class="flex-1 bg-gray-900 border border-gray-600 rounded px-2 py-1.5 text-sm text-gray-200"
          />
        </div>
        <div class="flex items-center gap-2">
          <label class="min-w-[60px] text-xs text-gray-400">Phone</label>
          <input
            value={phone}
            onInput={e => setPhone(e.target.value)}
            placeholder="+39..."
            class="flex-1 bg-gray-900 border border-gray-600 rounded px-2 py-1.5 text-sm text-gray-200"
          />
        </div>
        <div class="flex items-center gap-2">
          <label class="min-w-[60px] text-xs text-gray-400">Perms</label>
          <div class="flex flex-wrap gap-x-3 gap-y-1">
            {PERMS.map(p => (
              <label key={p.key} class="flex items-center gap-1 text-xs text-gray-300 cursor-pointer">
                <input
                  type="checkbox"
                  checked={perms & p.bit}
                  onChange={() => togglePerm(p.bit)}
                  class="accent-green-600"
                />
                {p.key}
              </label>
            ))}
          </div>
        </div>
      </div>

      <button
        onClick={addUser}
        class="bg-green-800 hover:bg-green-700 text-white text-sm px-4 py-1.5 rounded cursor-pointer"
      >
        Add User
      </button>
    </div>
  );
}
