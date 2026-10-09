// DarkStudio web UI - Logs & errors: every job's output, with errors and warnings highlighted and filterable.
// SPDX-License-Identifier: Apache-2.0
import { useMemo, useState } from 'react'
import { useTranslation } from 'react-i18next'
import { sortJobs, useStore } from '../store'
import { LogView, StatusBadge } from '../components'
import { duration } from '../util'

export default function Logs() {
  const { t } = useTranslation()
  const allJobs = useStore((s) => s.jobs)
  const jobs = useMemo(() => sortJobs(allJobs), [allJobs])
  const [selected, setSelected] = useState<string | null>(null)
  const [onlyProblems, setOnlyProblems] = useState(false)
  const [search, setSearch] = useState('')
  const [follow, setFollow] = useState(true)
  const current = jobs.find((j) => j.id === selected) ?? jobs.find((j) => j.status === 'running') ?? jobs[0]

  return (
    <div className="stack">
      <div className="page-head"><h1>{t('nav.logs')}</h1></div>
      <div style={{ display: 'grid', gridTemplateColumns: 'minmax(260px, 340px) 1fr', gap: 14 }}>
        <div className="card" style={{ padding: 8, maxHeight: 'calc(100vh - 140px)', overflow: 'auto' }}>
          {jobs.length === 0 && <p className="muted" style={{ padding: 8 }}>{t('common.none')}</p>}
          <table>
            <tbody>
              {jobs.map((j) => (
                <tr key={j.id} className={`clickable ${current?.id === j.id ? 'selected' : ''}`} onClick={() => setSelected(j.id)}>
                  <td>
                    <div>{j.title}</div>
                    <div className="muted" style={{ fontSize: 12 }}>{j.started.replace('T', ' ')} · {duration(j)}</div>
                  </td>
                  <td style={{ textAlign: 'end' }}>
                    <StatusBadge status={j.status} />
                    {j.errors > 0 && <div><span className="badge err">{j.errors} {t('common.errors')}</span></div>}
                    {j.warnings > 0 && <div><span className="badge warn">{j.warnings} {t('common.warnings')}</span></div>}
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>

        <div className="card stack" style={{ minWidth: 0 }}>
          {!current ? <p className="muted">{t('logs.empty')}</p> : (
            <>
              <div className="row">
                <h2 style={{ margin: 0 }}>{current.title}</h2>
                <StatusBadge status={current.status} />
                <span className="spacer" />
                <input type="search" placeholder={t('logs.search')} value={search} onChange={(e) => setSearch(e.target.value)} />
                <select value={onlyProblems ? 'problems' : 'all'} onChange={(e) => setOnlyProblems(e.target.value === 'problems')}>
                  <option value="all">{t('logs.all')}</option>
                  <option value="problems">{t('logs.onlyProblems')}</option>
                </select>
                <label className="check"><input type="checkbox" checked={follow} onChange={(e) => setFollow(e.target.checked)} />{t('logs.follow')}</label>
              </div>
              {current.error && <p className="error-text">{current.error}</p>}
              <div className="muted mono">{current.dir}</div>
              <LogView jobId={current.id} onlyProblems={onlyProblems} search={search} follow={follow} height="calc(100vh - 260px)" />
            </>
          )}
        </div>
      </div>
    </div>
  )
}
