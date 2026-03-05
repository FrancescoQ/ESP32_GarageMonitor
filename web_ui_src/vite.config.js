import { defineConfig, loadEnv } from 'vite';
import preact from '@preact/preset-vite';
import mockApiPlugin from './dev/mock-api.js';

export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), '');
  const esp32Ip = env.VITE_ESP32_IP;
  const useMock = !esp32Ip;

  if (useMock) {
    console.log('[dev] No VITE_ESP32_IP set — using mock API');
  } else {
    console.log(`[dev] Proxying /api to http://${esp32Ip}`);
  }

  return {
    plugins: [
      preact(),
      ...(useMock ? [mockApiPlugin()] : []),
    ],

    build: {
      outDir: '../data',
      emptyOutDir: true,
      minify: 'esbuild',
      cssCodeSplit: false,
      rollupOptions: {
        output: {
          manualChunks: undefined,
          entryFileNames: 'app.js',
          assetFileNames: 'app.[ext]'
        }
      },
      target: 'es2015',
    },

    server: {
      port: 3000,
      // Only proxy when a real ESP32 IP is provided
      ...(esp32Ip ? {
        proxy: {
          '/api': {
            target: `http://${esp32Ip}`,
            changeOrigin: true
          }
        }
      } : {})
    }
  };
});
