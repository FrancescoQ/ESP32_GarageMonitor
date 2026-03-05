import { h } from 'preact';

export function Toggle({ checked, onChange }) {
  return (
    <button
      type="button"
      onClick={() => onChange(!checked)}
      class={
        'relative inline-flex h-6 w-11 flex-shrink-0 rounded-full transition-colors cursor-pointer ' +
        (checked ? 'bg-green-700' : 'bg-gray-600')
      }
    >
      <span
        class={
          'inline-block h-[18px] w-[18px] rounded-full transition-transform mt-[3px] ' +
          (checked ? 'translate-x-[22px] bg-white' : 'translate-x-[3px] bg-gray-400')
        }
      />
    </button>
  );
}
