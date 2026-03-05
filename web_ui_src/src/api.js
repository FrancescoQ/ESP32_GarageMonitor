/** Append cache-busting timestamp to a URL */
export function noCache(url) {
  return url + (url.indexOf('?') < 0 ? '?' : '&') + '_t=' + Date.now();
}

/** GET JSON with cache-busting */
export async function fetchJSON(url) {
  const r = await fetch(noCache(url));
  return r.json();
}

/** POST JSON body, return JSON */
export async function postJSON(url, body) {
  const r = await fetch(url, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(body)
  });
  return r.json();
}
