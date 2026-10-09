// DarkStudio web UI - small shared components.
// SPDX-License-Identifier: Apache-2.0
import { useEffect, useMemo, useRef } from 'react'
import { useTranslation } from 'react-i18next'
import type { Job, LogLine } from './api'
import { EMPTY_LIST, ensureLog, useStore } from './store'

export function StatusBadge({ status }: { status: Job['status'] }) {
  const { t } = useTranslation()
  const cls = status === 'succeeded' ? 'ok' : status === 'failed' ? 'err' : status === 'running' ? 'run' : status === 'stopped' ? 'warn' : ''
  return (
    <span className={`badge ${cls}`}>
      <span className="dot" />
      {t(`status.${status}`)}
    </span>
  )
}

export function Progress({ job }: { job: Job }) {
  const { current, total } = job.progress
  if (!total) return null
  const pct = Math.min(100, (100 * current) / total)
  return (
    <div className="progress" title={`${current} / ${total}`}>
      <div style={{ width: `${pct}%` }} />
    </div>
  )
}

export function ErrorText({ error }: { error?: string | null }) {
  return error ? <p className="error-text">{error}</p> : null
}

/** Live log of one job, with errors and warnings highlighted. */
export function LogView({ jobId, onlyProblems = false, search = '', height = 360, follow = true }: {
  jobId: string
  onlyProblems?: boolean
  search?: string
  height?: number | string
  follow?: boolean
}) {
  const lines = useStore((s) => s.logs[jobId] ?? (EMPTY_LIST as LogLine[]))
  const ref = useRef<HTMLDivElement>(null)

  useEffect(() => {
    void ensureLog(jobId)
  }, [jobId])

  const shown = useMemo(() => {
    const needle = search.trim().toLowerCase()
    return lines.filter((l) => (!onlyProblems || l.level !== 'info') && (!needle || l.text.toLowerCase().includes(needle)))
  }, [lines, onlyProblems, search])

  useEffect(() => {
    if (follow && ref.current) ref.current.scrollTop = ref.current.scrollHeight
  }, [shown, follow])

  return (
    <div className="log" ref={ref} style={{ height }}>
      {shown.map((l) => (
        <div key={l.n} className={l.level}>
          <span className="n">{l.n}</span>
          {l.text || ' '}
        </div>
      ))}
    </div>
  )
}

