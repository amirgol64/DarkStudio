# DarkStudio web UI

React 19 + TypeScript + Vite. It's served by the C++ server (`server/`) at http://localhost:8765/.

```powershell
npm install
npm run dev      # http://localhost:5173/, forwards /api and /ws to the C++ server on :8765
npm run lint     # oxlint
npm run build    # -> dist/, served by darkstudio-server
```

| File | Purpose |
|---|---|
| `src/api.ts` | REST client and the types the server returns |
| `src/store.ts` | Live state (jobs, logs, metrics, system) fed by the `/ws/events` WebSocket |
| `src/i18n.ts` | English and Hebrew strings (Hebrew switches the layout to right-to-left) |
| `src/pages/*` | Devices, Benchmarks, Training, Logs, Annotation, Settings |

See [../docs/darkstudio-ui.md](../docs/darkstudio-ui.md) for the architecture and API.
