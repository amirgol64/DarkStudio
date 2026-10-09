// DarkStudio web UI - application shell: navigation, connection status, theme.
// SPDX-License-Identifier: Apache-2.0
import { useEffect, useMemo } from 'react'
import { applyTheme, savedTheme } from './util'
import { NavLink, Navigate, Route, Routes } from 'react-router-dom'
import { useTranslation } from 'react-i18next'
import { sortJobs, useStore } from './store'
import Devices from './pages/Devices'
import Benchmarks from './pages/Benchmarks'
import Training from './pages/Training'
import Logs from './pages/Logs'
import Annotation from './pages/Annotation'
import SettingsPage from './pages/Settings'

export default function App() {
  const { t } = useTranslation()
  const connected = useStore((s) => s.connected)
  const jobs = useStore((s) => s.jobs)
  const running = useMemo(() => sortJobs(jobs).find((j) => j.status === 'running'), [jobs])
  const errorCount = useMemo(() => sortJobs(jobs).slice(0, 10).reduce((n, j) => n + (j.status === 'failed' ? 1 : 0), 0), [jobs])

  useEffect(() => applyTheme(savedTheme()), [])

  const links = [
    { to: '/devices', label: t('nav.devices'), icon: '🖥' },
    { to: '/benchmarks', label: t('nav.benchmarks'), icon: '⏱' },
    { to: '/training', label: t('nav.training'), icon: '📈' },
    { to: '/logs', label: t('nav.logs'), icon: '📜', count: errorCount },
    { to: '/annotation', label: t('nav.annotation'), icon: '🏷' },
    { to: '/settings', label: t('nav.settings'), icon: '⚙' },
  ]

  return (
    <div className="shell">
      <aside className="sidebar">
        <div className="brand">
          <strong>{t('app.title')}</strong>
          <small>{t('app.subtitle')}</small>
        </div>
        <nav className="nav">
          {links.map((l) => (
            <NavLink key={l.to} to={l.to}>
              <span aria-hidden>{l.icon}</span>
              {l.label}
              {l.count ? <span className="count">{l.count}</span> : null}
            </NavLink>
          ))}
        </nav>
        <div className="footer">
          <span className={`badge ${connected ? 'ok' : 'err'}`}>
            <span className="dot" />
            {connected ? t('app.connected') : t('app.disconnected')}
          </span>
          {running && (
            <p style={{ marginTop: 8 }}>
              <span className="badge run"><span className="dot" />{running.title}</span>
            </p>
          )}
        </div>
      </aside>
      <main className="main">
        <Routes>
          <Route path="/" element={<Navigate to="/devices" replace />} />
          <Route path="/devices" element={<Devices />} />
          <Route path="/benchmarks" element={<Benchmarks />} />
          <Route path="/training" element={<Training />} />
          <Route path="/logs" element={<Logs />} />
          <Route path="/annotation" element={<Annotation />} />
          <Route path="/settings" element={<SettingsPage />} />
          <Route path="*" element={<Navigate to="/devices" replace />} />
        </Routes>
      </main>
    </div>
  )
}
