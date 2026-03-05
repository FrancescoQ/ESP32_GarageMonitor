import { h, render } from 'preact';
import { useState } from 'preact/hooks';
import { Users } from './views/Users';
import { Settings } from './views/Settings';
import { Diagnostics } from './views/Diagnostics';
import './styles.css';

const TABS = [
  { id: 'users', label: 'Users', component: Users },
  { id: 'settings', label: 'Settings', component: Settings },
  { id: 'diag', label: 'Diagnostics', component: Diagnostics },
];

function App() {
  const [tab, setTab] = useState('users');
  const View = TABS.find(t => t.id === tab)?.component || Users;

  return (
    <div class="max-w-xl mx-auto p-4">
      <h1 class="text-xl font-semibold text-sky-400 mb-3">Garage Monitor Setup</h1>

      <div class="flex gap-1">
        {TABS.map(t => (
          <button
            key={t.id}
            onClick={() => setTab(t.id)}
            class={
              'px-4 py-2 text-sm rounded-t-md border border-b-0 cursor-pointer ' +
              (tab === t.id
                ? 'bg-slate-800 text-sky-400 border-gray-600'
                : 'bg-gray-800 text-gray-400 border-gray-700 hover:text-gray-200')
            }
          >
            {t.label}
          </button>
        ))}
      </div>

      <div class="bg-slate-800 rounded-b-md rounded-tr-md p-4 border border-gray-600">
        <View />
      </div>
    </div>
  );
}

render(<App />, document.getElementById('app'));
