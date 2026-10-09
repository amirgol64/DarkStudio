// DarkStudio web UI - live state: jobs, logs, training metrics and system info, kept up to date by the
// server's WebSocket (/ws/events).
// SPDX-License-Identifier: Apache-2.0
import { useSyncExternalStore } from 'react'
import { api, type Job, type LogLine, type SystemInfo, type TrainingMetric } from './api'

const MAX_LOG_LINES = 3000

export interface State {
  connected: boolean
  system: SystemInfo
  jobs: Record<string, Job>
  logs: Record<string, LogLine[]>
  metrics: Record<string, TrainingMetric[]>
}

let state: State = { connected: false, system: { probing: true }, jobs: {}, logs: {}, metrics: {} }
const listeners = new Set<() => void>()
const loadedLogs = new Set<string>()
const loadedMetrics = new Set<string>()

function setState(update: (s: State) => State) {
  state = update(state)
  listeners.forEach((l) => l())
}

function subscribe(listener: () => void) {
  listeners.add(listener)
  return () => listeners.delete(listener)
}

/** Select part of the live state; the component re-renders when it changes.  The selector must return a part of
 * the state as-is (e.g. `s => s.jobs`), never a new array or object, or React re-renders forever; derive lists
 * with useMemo instead (see sortJobs). */
export function useStore<T>(selector: (s: State) => T): T {
  return useSyncExternalStore(subscribe, () => selector(state))
}

function appendLog(id: string, lines: LogLine[]) {
  setState((s) => {
    const existing = s.logs[id] ?? []
    const last = existing.length ? existing[existing.length - 1].n : 0
    const merged = existing.concat(lines.filter((l) => l.n > last))
    return { ...s, logs: { ...s.logs, [id]: merged.slice(-MAX_LOG_LINES) } }
  })
}

function handleEvent(event: any) {
  switch (event.type) {
    case 'job':
      setState((s) => ({ ...s, jobs: { ...s.jobs, [event.job.id]: event.job } }))
      break
    case 'log':
      // only keep lines for jobs whose log has been opened or that are running now
      appendLog(event.job_id, [{ n: event.n, text: event.line, level: event.level }])
      break
    case 'metric':
      setState((s) => ({
        ...s,
        metrics: { ...s.metrics, [event.job_id]: [...(s.metrics[event.job_id] ?? []), event.metric] },
      }))
      break
    case 'system':
      setState((s) => ({ ...s, system: { ...event.system, probing: false } }))
      break
  }
}

let socket: WebSocket | null = null
let retry = 0

function connect() {
  const url = `${location.protocol === 'https:' ? 'wss' : 'ws'}://${location.host}/ws/events`
  socket = new WebSocket(url)
  socket.onopen = () => {
    retry = 0
    setState((s) => ({ ...s, connected: true }))
    void refreshJobs()
  }
  socket.onmessage = (msg) => {
    try {
      handleEvent(JSON.parse(msg.data))
    } catch {
      // ignore malformed events
    }
  }
  socket.onclose = () => {
    setState((s) => ({ ...s, connected: false }))
    retry = Math.min(retry + 1, 6)
    setTimeout(connect, 500 * 2 ** retry) // back off up to ~30 s while the server is down
  }
}

export async function refreshJobs() {
  const jobs = await api.jobs()
  setState((s) => ({ ...s, jobs: Object.fromEntries(jobs.map((j) => [j.id, j])) }))
}

export async function refreshSystem(probe = false) {
  const system = probe ? await api.refreshSystem() : await api.system()
  setState((s) => ({ ...s, system }))
}

/** Load the log lines the UI has not seen yet (e.g. for jobs from before the page was opened). */
export async function ensureLog(id: string) {
  if (loadedLogs.has(id)) return
  loadedLogs.add(id)
  const r = await api.jobLog(id)
  appendLog(id, r.lines)
}

export async function ensureMetrics(id: string) {
  if (loadedMetrics.has(id)) return
  loadedMetrics.add(id)
  const m = await api.jobMetrics(id)
  setState((s) => ({ ...s, metrics: { ...s.metrics, [id]: m } }))
}

export function upsertJob(job: Job) {
  setState((s) => ({ ...s, jobs: { ...s.jobs, [job.id]: job } }))
}

export function startStore() {
  if (socket) return
  connect()
  void refreshSystem().catch(() => {})
}

/** Newest first (job ids start with the date and time). */
export function sortJobs(jobs: Record<string, Job>): Job[] {
  return Object.values(jobs).sort((a, b) => (a.id < b.id ? 1 : -1))
}

const EMPTY: never[] = []
/** Stable empty array for selectors such as `s => s.logs[id] ?? EMPTY_LIST`. */
export const EMPTY_LIST = EMPTY
