// DarkStudio web UI - entry point.
// SPDX-License-Identifier: Apache-2.0
import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import { HashRouter } from 'react-router-dom'
import './index.css'
import i18n, { hasLocalLanguage, setLanguage } from './i18n'
import App from './App'
import { startStore } from './store'
import { api } from './api'
import { applyTheme, savedTheme } from './util'

setLanguage(i18n.language)
startStore()

// a browser without its own choice follows the language and theme saved on the server (Settings page)
void api.settings().then((s) => {
  if (!hasLocalLanguage && s.language !== i18n.language) setLanguage(s.language)
  if (savedTheme() === 'system' && s.theme !== 'system') applyTheme(s.theme)
}).catch(() => {})

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <HashRouter>
      <App />
    </HashRouter>
  </StrictMode>,
)
