// DarkStudio web UI - small shared components.
// SPDX-License-Identifier: Apache-2.0
import { useEffect, useMemo, useRef, useState } from 'react'
import { useTranslation } from 'react-i18next'
import type { Job, LogLine } from './api'
import { EMPTY_LIST, ensureLog, useStore } from './store'
import { GUIDE_URL } from './util'

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


/** A small "?" next to a field: click it for a short tip and a link to the matching part of the guide. */
export function Help({ text, anchor }: { text: string; anchor?: string }) {
  const { t } = useTranslation()
  const [open, setOpen] = useState(false)
  const ref = useRef<HTMLSpanElement>(null)

  useEffect(() => {
    if (!open) return
    const onDown = (e: MouseEvent) => {
      if (!ref.current?.contains(e.target as Node)) setOpen(false)
    }
    const onKey = (e: KeyboardEvent) => {
      if (e.key === 'Escape') setOpen(false)
    }
    document.addEventListener('mousedown', onDown)
    document.addEventListener('keydown', onKey)
    return () => {
      document.removeEventListener('mousedown', onDown)
      document.removeEventListener('keydown', onKey)
    }
  }, [open])

  return (
    <span className="help" ref={ref}>
      <button
        type="button"
        className="help-btn"
        aria-label={t('help.what')}
        aria-expanded={open}
        onClick={(e) => {
          e.preventDefault() // the "?" often sits inside a <label>: don't move the focus to its input
          setOpen((o) => !o)
        }}
      >
        ?
      </button>
      {open && (
        <span className="help-pop" role="tooltip">
          {text}
          {anchor && (
            <a href={`${GUIDE_URL}${anchor}`} target="_blank" rel="noreferrer">
              {t('help.more')}
            </a>
          )}
        </span>
      )}
    </span>
  )
}

function readOpen(page: string, fallback: boolean): boolean {
  try {
    const v = localStorage.getItem(`darkstudio.help.${page}`)
    return v === null ? fallback : v === 'open'
  } catch {
    return fallback
  }
}

/** "How to use this page": a short list of steps at the top of a page, folded away once the user closes it. */
export function PageHelp({ page, anchor, defaultOpen = true }: { page: string; anchor?: string; defaultOpen?: boolean }) {
  const { t } = useTranslation()
  const [open, setOpen] = useState(() => readOpen(page, defaultOpen))
  const steps = t(`help.pages.${page}`, { returnObjects: true }) as string[]

  const toggle = (next: boolean) => {
    setOpen(next)
    try {
      localStorage.setItem(`darkstudio.help.${page}`, next ? 'open' : 'closed')
    } catch {
      // storage unavailable: the choice lasts for this page view
    }
  }

  return (
    <details className="page-help" open={open} onToggle={(e) => toggle((e.target as HTMLDetailsElement).open)}>
      <summary>
        <span className="help-btn" aria-hidden>?</span>
        {t('help.title')}
      </summary>
      <ol>
        {Array.isArray(steps) && steps.map((s, i) => <li key={i}>{s}</li>)}
      </ol>
      {anchor && (
        <a href={`${GUIDE_URL}${anchor}`} target="_blank" rel="noreferrer">
          {t('help.guide')}
        </a>
      )}
    </details>
  )
}
