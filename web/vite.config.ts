// DarkStudio web UI - Vite config.
// SPDX-License-Identifier: Apache-2.0
import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// During development ("npm run dev") the UI runs on :5173 and forwards API and WebSocket calls to the
// C++ server on :8765.  In production the C++ server serves the built files from web/dist itself.
export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
    proxy: {
      '/api': 'http://127.0.0.1:8765',
      '/ws': { target: 'ws://127.0.0.1:8765', ws: true },
    },
  },
  build: {
    chunkSizeWarningLimit: 1500,
  },
})
