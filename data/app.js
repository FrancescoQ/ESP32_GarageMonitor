/**
 * Garage Monitor Setup - Client-side JavaScript
 *
 * Handles tab switching, user management, settings CRUD,
 * and live diagnostics polling via fetch() API calls.
 */

// Permission bit flags (must match Config.h)
const P = {
  STATUS: 0x01,
  CLOSE:  0x02,
  OPEN:   0x04,
  CONFIG: 0x08
};

/**
 * Switch active tab and load its data
 * @param {string} id - Tab panel ID ('users', 'settings', 'diag')
 */
function showTab(id) {
  var tabIds = ['users', 'settings', 'diag'];
  document.querySelectorAll('.tab').forEach(function(t, i) {
    t.classList.toggle('active', tabIds[i] === id);
  });
  document.querySelectorAll('.panel').forEach(function(p) {
    p.classList.toggle('active', p.id === id);
  });
  if (id === 'users') loadUsers();
  if (id === 'settings') loadSettings();
  if (id === 'diag') loadDiag();
}

/**
 * Show a temporary status message
 * @param {string} elId - Message element ID
 * @param {string} text - Message text
 * @param {boolean} ok - true for success, false for error
 */
function msg(elId, text, ok) {
  var el = document.getElementById(elId);
  el.className = 'msg ' + (ok ? 'ok' : 'err');
  el.textContent = text;
  setTimeout(function() { el.textContent = ''; }, 4000);
}

/**
 * Render permission badges for a user
 * @param {number} p - Permission bitmask
 * @returns {string} HTML string of permission badges
 */
function permBadges(p) {
  return Object.entries(P).map(function(entry) {
    var k = entry[0], v = entry[1];
    return '<span class="perm ' + (p & v ? 'on' : 'off') + '">' + k + '</span>';
  }).join('');
}

// =========================================================================
// Users
// =========================================================================

/** Load and display the user list */
async function loadUsers() {
  try {
    var r = await fetch('/api/users');
    var d = await r.json();
    var tb = document.getElementById('userList');
    tb.innerHTML = d.users.map(function(u, i) {
      return '<tr>' +
        '<td>' + u.name + '</td>' +
        '<td style="font-size:.8em">' + u.phone + '</td>' +
        '<td>' + permBadges(u.permissions) + '</td>' +
        '<td><button class="btn-del" onclick="delUser(' + i + ')">X</button></td>' +
        '</tr>';
    }).join('');
  } catch (e) {
    msg('userMsg', 'Failed to load users', false);
  }
}

/** Add a new user from the form inputs */
async function addUser() {
  var name = document.getElementById('newName').value.trim();
  var phone = document.getElementById('newPhone').value.trim();
  if (!name || !phone) {
    msg('userMsg', 'Name and phone required', false);
    return;
  }

  var perms = 0;
  if (document.getElementById('pS').checked) perms |= P.STATUS;
  if (document.getElementById('pC').checked) perms |= P.CLOSE;
  if (document.getElementById('pO').checked) perms |= P.OPEN;
  if (document.getElementById('pCfg').checked) perms |= P.CONFIG;

  try {
    var r = await fetch('/api/users', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ name: name, phone: phone, permissions: perms })
    });
    var d = await r.json();
    if (d.ok) {
      msg('userMsg', 'User added', true);
      loadUsers();
      document.getElementById('newName').value = '';
      document.getElementById('newPhone').value = '';
    } else {
      msg('userMsg', d.error || 'Failed', false);
    }
  } catch (e) {
    msg('userMsg', 'Request failed', false);
  }
}

/**
 * Delete a user by index
 * @param {number} idx - User index from the list
 */
async function delUser(idx) {
  if (!confirm('Remove this user?')) return;
  try {
    var r = await fetch('/api/users?index=' + idx, { method: 'DELETE' });
    var d = await r.json();
    if (d.ok) {
      msg('userMsg', 'User removed', true);
      loadUsers();
    } else {
      msg('userMsg', d.error || 'Failed', false);
    }
  } catch (e) {
    msg('userMsg', 'Request failed', false);
  }
}

// =========================================================================
// Settings
// =========================================================================

/** Load current settings into the form */
async function loadSettings() {
  try {
    var r = await fetch('/api/settings');
    var d = await r.json();
    document.getElementById('sDoorAlert').value = d.door_alert_min;
    document.getElementById('sSmsPoll').value = d.sms_poll_ms;
    document.getElementById('sDeepSleep').checked = d.deep_sleep;
    document.getElementById('sFwdUnknown').checked = d.fwd_unknown;
  } catch (e) {
    msg('setMsg', 'Failed to load settings', false);
  }
}

/** Save settings from the form */
async function saveSettings() {
  try {
    var body = {
      door_alert_min: parseInt(document.getElementById('sDoorAlert').value),
      sms_poll_ms: parseInt(document.getElementById('sSmsPoll').value),
      deep_sleep: document.getElementById('sDeepSleep').checked,
      fwd_unknown: document.getElementById('sFwdUnknown').checked
    };
    var r = await fetch('/api/settings', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body)
    });
    var d = await r.json();
    if (d.ok) {
      msg('setMsg', 'Settings saved', true);
    } else {
      msg('setMsg', d.error || 'Failed', false);
    }
  } catch (e) {
    msg('setMsg', 'Request failed', false);
  }
}

// =========================================================================
// Diagnostics
// =========================================================================

var diagTimer = null;

/** Load live diagnostics and auto-refresh every 2s while tab is active */
async function loadDiag() {
  try {
    var r = await fetch('/api/diagnostics');
    var d = await r.json();
    var el = document.getElementById('diagContent');
    el.innerHTML =
      '<div class="diag-row"><span class="diag-label">Door</span><span class="diag-val">' + d.door + '</span></div>' +
      '<div class="diag-row"><span class="diag-label">Temperature</span><span class="diag-val">' + d.temperature + '</span></div>' +
      '<div class="diag-row"><span class="diag-label">Humidity</span><span class="diag-val">' + d.humidity + '</span></div>' +
      '<div class="diag-row"><span class="diag-label">Pressure</span><span class="diag-val">' + d.pressure + '</span></div>' +
      '<div class="diag-row"><span class="diag-label">Water</span><span class="diag-val">' + d.water + '</span></div>' +
      '<div class="diag-row"><span class="diag-label">Uptime</span><span class="diag-val">' + d.uptime + '</span></div>';
  } catch (e) {
    document.getElementById('diagContent').textContent = 'Failed to load';
  }
  clearTimeout(diagTimer);
  if (document.getElementById('diag').classList.contains('active')) {
    diagTimer = setTimeout(loadDiag, 2000);
  }
}

// =========================================================================
// Init
// =========================================================================

// Load users tab on page load
loadUsers();
