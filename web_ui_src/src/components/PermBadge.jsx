import { h } from 'preact';

export function PermBadge({ name, active }) {
  return (
    <span
      class={
        'inline-block text-xs px-1.5 py-0.5 rounded mr-1 ' +
        (active
          ? 'bg-green-900 text-green-300'
          : 'bg-gray-700 text-gray-500')
      }
    >
      {name}
    </span>
  );
}
