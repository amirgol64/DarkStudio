// DarkStudio web UI - REST client and the types returned by the C++ server (server/src/*.cpp).
// SPDX-License-Identifier: Apache-2.0

export type JobStatus = 'queued' | 'running' | 'succeeded' | 'failed' | 'stopped'
export type JobKind = 'ov-bench' | 'compare-cpu-sycl' | 'train' | 'prepare-legogears' | 'map'
export type LogLevel = 'info' | 'warning' | 'error'

export interface Job {
  id: string
  kind: JobKind
  title: string
  params: Record<string, unknown>
  status: JobStatus
  created: string
  started: string
  finished: string
  exit_code: number
  error: string
  dir: string
  lines: number
  errors: number
  warnings: number
  result: Record<string, any>
  progress: { current: number; total: number }
}

export interface LogLine {
  n: number
  text: string
  level: LogLevel
}

export interface TrainingMetric {
  type: 'iteration' | 'map'
  iteration: number
  loss?: number
  avg_loss?: number
  rate?: number
  train_seconds?: number
  load_ms?: number
  last_map?: number | null
  best_map?: number | null
  map?: number
  iou?: number
}

export interface Backend {
  id: 'darknet-cpu' | 'darknet-sycl' | 'openvino'
  name: string
  available: boolean
  error: string
  probe_seconds: number
  output: string[]
  info: Record<string, any>
}

export interface SystemInfo {
  probing: boolean
  cpu?: { name?: string; threads?: number; ram_bytes?: number; ram_free_bytes?: number; os?: string }
  backends?: Backend[]
  files?: { what: string; path: string; exists: boolean }[]
}

export interface Settings {
  root: string
  work_dir: string
  web_dir: string
  darknet_cpu_bin: string
  darknet_sycl_bin: string
  oneapi_bin: string
  openvino_bin: string
  openvino_tbb_bin: string
  ov_bench: string
  compare_script: string
  models_dir: string
  datasets_dir: string
  default_device: string
  default_precision: 'f16' | 'f32'
  openvino_cache: boolean
  language: string
  theme: 'system' | 'light' | 'dark'
}

export interface DatasetFolder {
  path: string
  images: number
  labeled: number
  with_boxes: number
  names: string[]
}

export interface DatasetImage {
  path: string
  name: string
  labeled: boolean
}

/** YOLO box: normalized centre x/y and width/height. */
export interface Box {
  class: number
  x: number
  y: number
  w: number
  h: number
}

export class ApiError extends Error {
  status: number
  constructor(status: number, message: string) {
    super(message)
    this.status = status
  }
}

async function request<T>(method: string, url: string, body?: unknown): Promise<T> {
  const resp = await fetch(url, {
    method,
    headers: body === undefined ? undefined : { 'Content-Type': 'application/json' },
    body: body === undefined ? undefined : JSON.stringify(body),
  })
  const text = await resp.text()
  const data = text ? JSON.parse(text) : null
  if (!resp.ok) {
    throw new ApiError(resp.status, data?.error ?? resp.statusText)
  }
  return data as T
}

const q = encodeURIComponent

export const api = {
  system: () => request<SystemInfo>('GET', '/api/system'),
  refreshSystem: () => request<SystemInfo>('POST', '/api/system/refresh'),
  settings: () => request<Settings>('GET', '/api/settings'),
  saveSettings: (s: Partial<Settings>) => request<Settings>('PUT', '/api/settings', s),
  jobs: () => request<Job[]>('GET', '/api/jobs'),
  startJob: (kind: JobKind, params: Record<string, unknown>) => request<Job>('POST', '/api/jobs', { kind, params }),
  stopJob: (id: string) => request<Job>('POST', `/api/jobs/${q(id)}/stop`),
  jobLog: (id: string, since = 0) =>
    request<{ job_id: string; total: number; lines: LogLine[] }>('GET', `/api/jobs/${q(id)}/log?since=${since}`),
  jobMetrics: (id: string) => request<TrainingMetric[]>('GET', `/api/jobs/${q(id)}/metrics`),
  jobImageUrl: (id: string, name: string) => `/api/jobs/${q(id)}/image?name=${q(name)}`,
  datasets: () => request<DatasetFolder[]>('GET', '/api/datasets'),
  datasetImages: (folder: string) =>
    request<{ folder: string; names: string[]; images: DatasetImage[] }>('GET', `/api/datasets/images?folder=${q(folder)}`),
  imageUrl: (path: string) => `/api/datasets/file?path=${q(path)}`,
  labels: (image: string) => request<{ image: string; names: string[]; boxes: Box[] }>('GET', `/api/datasets/labels?image=${q(image)}`),
  saveLabels: (image: string, boxes: Box[]) =>
    request<{ image: string; names: string[]; boxes: Box[] }>('PUT', `/api/datasets/labels?image=${q(image)}`, { boxes }),
}

export function formatBytes(bytes?: number): string {
  if (!bytes) return '–'
  const units = ['B', 'KiB', 'MiB', 'GiB', 'TiB']
  let i = 0
  let v = bytes
  while (v >= 1024 && i < units.length - 1) {
    v /= 1024
    i++
  }
  return `${v.toFixed(1)} ${units[i]}`
}
