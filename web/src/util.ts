// DarkStudio web UI - helpers shared by the pages (kept out of component files for React fast refresh).
// SPDX-License-Identifier: Apache-2.0
import { useState } from 'react'
import type { Job } from './api'

export function applyTheme(theme: string) {
  if (theme === 'light' || theme === 'dark') document.documentElement.dataset.theme = theme
  else delete document.documentElement.dataset.theme
  try {
    localStorage.setItem('darkstudio.theme', theme)
  } catch {
    // storage unavailable: the theme still applies for this page view
  }
}

export function savedTheme(): string {
  try {
    return localStorage.getItem('darkstudio.theme') ?? 'system'
  } catch {
    return 'system'
  }
}

export function duration(job: Job): string {
  if (!job.started) return '–'
  const end = job.finished ? new Date(job.finished) : new Date()
  const s = Math.max(0, Math.round((end.getTime() - new Date(job.started).getTime()) / 1000))
  if (s < 60) return `${s} s`
  if (s < 3600) return `${Math.floor(s / 60)} min ${s % 60} s`
  return `${Math.floor(s / 3600)} h ${Math.floor((s % 3600) / 60)} min`
}

/** Run an async action and remember its error for display. */
export function useAction() {
  const [busy, setBusy] = useState(false)
  const [error, setError] = useState<string | null>(null)
  const run = async (fn: () => Promise<unknown>) => {
    setBusy(true)
    setError(null)
    try {
      await fn()
    } catch (e) {
      setError(e instanceof Error ? e.message : String(e))
    } finally {
      setBusy(false)
    }
  }
  return { busy, error, run, setError }
}
