import { h } from 'preact';

export function FormRow({ label, hint, children }) {
  return (
    <div class="mb-4">
      <div class="flex items-center gap-2">
        <label class="min-w-[120px] text-sm text-gray-400">{label}</label>
        <div class="flex-1 flex items-center gap-2">{children}</div>
      </div>
      {hint && <p class="text-xs text-gray-500 mt-1 ml-[128px]">{hint}</p>}
    </div>
  );
}
