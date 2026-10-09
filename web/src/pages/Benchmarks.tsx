// DarkStudio web UI - Benchmarks: OpenVINO (ov-bench) and Darknet CPU vs SYCL, with a results chart.
// SPDX-License-Identifier: Apache-2.0
import { useMemo, useState } from 'react'
import { useTranslation } from 'react-i18next'
import { Bar, BarChart, CartesianGrid, ResponsiveContainer, Tooltip, XAxis, YAxis } from 'recharts'
import { api, type Job } from '../api'
import { sortJobs, upsertJob, useStore } from '../store'
import { ErrorText, LogView, Progress, StatusBadge } from '../components'
import { duration, useAction } from '../util'

const fmt = (v: unknown, digits = 1) => (typeof v === 'number' ? v.toFixed(digits) : '–')

/** One bar per finished benchmark: label and milliseconds per image. */
function bars(jobs: Job[]) {
  const out: { label: string; ms: number }[] = []
  for (const j of jobs.filter((j) => j.status === 'succeeded').slice(0, 12).reverse()) {
    if (j.kind === 'ov-bench' && typeof j.result.total_mean_ms === 'number') {
      out.push({ label: `OpenVINO ${j.result.device} ${j.result.precision}`, ms: j.result.total_mean_ms })
    } else if (j.kind === 'compare-cpu-sycl' && typeof j.result.cpu_ms === 'number') {
      out.push({ label: 'Darknet CPU', ms: j.result.cpu_ms }, { label: 'Darknet SYCL', ms: j.result.sycl_ms })
    }
  }
  return out
}

function OvBenchDetails({ job }: { job: Job }) {
  const { t } = useTranslation()
  const r = job.result
  return (
    <div className="stack">
      <div className="row" style={{ gap: 32 }}>
        <div className="stat"><b>{fmt(r.total_mean_ms)} ms</b><span>{t('bench.total')}</span></div>
        <div className="stat"><b>{fmt(r.infer_mean_ms)} ms</b><span>{t('bench.infer')}</span></div>
        <div className="stat"><b>{fmt(r.fps, 0)}</b><span>{t('bench.fps')}</span></div>
        <div className="stat"><b>{fmt(r.compile_ms, 0)} ms</b><span>{t('bench.compile')}</span></div>
      </div>
      <table>
        <thead>
          <tr><th>Image</th><th className="num">{t('bench.infer')} ms</th><th className="num">{t('bench.total')} ms</th><th>Detections</th></tr>
        </thead>
        <tbody>
          {(r.images ?? []).map((img: any) => (
            <tr key={img.image}>
              <td>{img.image}</td>
              <td className="num">{fmt(img.infer_mean_ms)}</td>
              <td className="num">{fmt(img.total_mean_ms)}</td>
              <td>{(img.detections ?? []).map((d: any) => `${d.name} ${d.confidence}%`).join(', ') || '–'}</td>
            </tr>
          ))}
        </tbody>
      </table>
      <h3>{t('bench.images')}</h3>
      <div className="row" style={{ alignItems: 'flex-start' }}>
        {(r.images ?? []).map((img: any) => {
          const name = `${img.image.replace(/\.[^.]+$/, '')}_${r.device}.jpg`
          return <img key={name} src={api.jobImageUrl(job.id, name)} alt={img.image} style={{ height: 140, borderRadius: 6 }} loading="lazy" />
        })}
      </div>
    </div>
  )
}

function CompareDetails({ job }: { job: Job }) {
  const { t } = useTranslation()
  const r = job.result
  const ok = r.class_mismatches === 0 && r.worst_box_diff === 0
  return (
    <div className="stack">
      <div className="row" style={{ gap: 32 }}>
        <div className="stat"><b>{fmt(r.cpu_ms)} ms</b><span>Darknet CPU</span></div>
        <div className="stat"><b>{fmt(r.sycl_ms)} ms</b><span>Darknet SYCL</span></div>
        <div className="stat"><b>{fmt(r.speedup)}×</b><span>{t('bench.speedup')}</span></div>
        <div className="stat"><b className={ok ? '' : 'error-text'}>{r.class_mismatches ?? '–'}</b><span>{t('bench.mismatches')}</span></div>
        <div className="stat"><b>{fmt(r.worst_probability_diff, 4)}</b><span>{t('bench.probDiff')}</span></div>
        <div className="stat"><b>{r.worst_box_diff ?? '–'} px</b><span>{t('bench.boxDiff')}</span></div>
      </div>
      <table>
        <thead><tr><th>Image</th><th>Object</th><th className="num">CPU</th><th className="num">SYCL</th><th className="num">diff</th><th className="num">box</th></tr></thead>
        <tbody>
          {(r.detections ?? []).map((d: any, i: number) => (
            <tr key={i}>
              <td>{d.image}</td><td>{d.name ?? `count ${d.count_cpu} / ${d.count_sycl}`}</td>
              <td className="num">{fmt(d.cpu)}%</td><td className="num">{fmt(d.sycl)}%</td>
              <td className="num">{fmt(d.diff, 4)}</td><td className="num">{d.box_diff ?? '–'} px</td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  )
}

export default function Benchmarks() {
  const { t } = useTranslation()
  const allJobs = useStore((s) => s.jobs)
  const jobs = useMemo(() => sortJobs(allJobs).filter((j) => j.kind === 'ov-bench' || j.kind === 'compare-cpu-sycl'), [allJobs])
  const [device, setDevice] = useState('GPU')
  const [precision, setPrecision] = useState('f16')
  const [iterations, setIterations] = useState(50)
  const [repeat, setRepeat] = useState(3)
  const [selected, setSelected] = useState<string | null>(null)
  const action = useAction()

  const running = jobs.find((j) => j.status === 'running')
  const current = jobs.find((j) => j.id === selected) ?? running ?? jobs[0]
  const chart = useMemo(() => bars(jobs), [jobs])

  const start = (kind: 'ov-bench' | 'compare-cpu-sycl', params: Record<string, unknown>) =>
    action.run(async () => {
      const job = await api.startJob(kind, params)
      upsertJob(job)
      setSelected(job.id)
    })

  return (
    <div className="stack">
      <div className="page-head"><h1>{t('nav.benchmarks')}</h1></div>
      <ErrorText error={action.error} />

      <div className="grid">
        <div className="card">
          <h2>{t('bench.openvino')}</h2>
          <p className="muted">{t('bench.openvinoHelp')}</p>
          <div className="form-grid">
            <label className="field">{t('bench.device')}
              <select value={device} onChange={(e) => setDevice(e.target.value)}>
                <option>GPU</option><option>CPU</option><option>AUTO</option>
              </select>
            </label>
            <label className="field">{t('bench.precision')}
              <select value={precision} onChange={(e) => setPrecision(e.target.value)}>
                <option value="f16">FP16</option><option value="f32">FP32</option>
              </select>
            </label>
            <label className="field">{t('bench.iterations')}
              <input type="number" min={1} max={1000} value={iterations} onChange={(e) => setIterations(Number(e.target.value))} />
            </label>
          </div>
          <button disabled={action.busy || !!running} onClick={() => start('ov-bench', { device, precision, iterations })}>{t('common.start')}</button>
        </div>

        <div className="card">
          <h2>{t('bench.compare')}</h2>
          <p className="muted">{t('bench.compareHelp')}</p>
          <div className="form-grid">
            <label className="field">{t('bench.repeat')}
              <input type="number" min={2} max={20} value={repeat} onChange={(e) => setRepeat(Number(e.target.value))} />
            </label>
          </div>
          <button disabled={action.busy || !!running} onClick={() => start('compare-cpu-sycl', { repeat })}>{t('common.start')}</button>
        </div>
      </div>

      {chart.length > 0 && (
        <div className="card">
          <h2>{t('bench.chart')}</h2>
          <div style={{ height: 260, direction: 'ltr' }}>
            <ResponsiveContainer>
              <BarChart data={chart} margin={{ left: 8, right: 8 }}>
                <CartesianGrid strokeDasharray="3 3" stroke="var(--border)" />
                <XAxis dataKey="label" tick={{ fill: 'var(--muted)', fontSize: 11 }} interval={0} />
                <YAxis tick={{ fill: 'var(--muted)', fontSize: 11 }} unit=" ms" />
                <Tooltip contentStyle={{ background: 'var(--surface)', border: '1px solid var(--border)' }} formatter={(v) => `${Number(v).toFixed(1)} ms`} />
                <Bar dataKey="ms" fill="var(--accent)" radius={[4, 4, 0, 0]} maxBarSize={80} isAnimationActive={false} />
              </BarChart>
            </ResponsiveContainer>
          </div>
        </div>
      )}

      <div className="card">
        <h2>{t('bench.history')}</h2>
        {jobs.length === 0 ? <p className="muted">{t('common.none')}</p> : (
          <table>
            <thead><tr><th>{t('logs.job')}</th><th>{t('common.status')}</th><th>{t('common.started')}</th><th>{t('common.duration')}</th><th className="num">{t('bench.perImage')}</th></tr></thead>
            <tbody>
              {jobs.map((j) => (
                <tr key={j.id} className={`clickable ${current?.id === j.id ? 'selected' : ''}`} onClick={() => setSelected(j.id)}>
                  <td>{j.title}</td>
                  <td><StatusBadge status={j.status} /></td>
                  <td>{j.started.replace('T', ' ')}</td>
                  <td>{duration(j)}</td>
                  <td className="num">{fmt(j.kind === 'ov-bench' ? j.result.total_mean_ms : j.result.sycl_ms)}</td>
                </tr>
              ))}
            </tbody>
          </table>
        )}
      </div>

      {current && (
        <div className="card">
          <div className="row" style={{ marginBottom: 10 }}>
            <h2 style={{ margin: 0 }}>{current.title}</h2>
            <StatusBadge status={current.status} />
            <span className="spacer" />
            {current.status === 'running' && <button className="danger" onClick={() => action.run(() => api.stopJob(current.id))}>{t('common.stop')}</button>}
          </div>
          {current.status === 'running' && <Progress job={current} />}
          <ErrorText error={current.error} />
          {current.status === 'succeeded' && current.kind === 'ov-bench' && <OvBenchDetails job={current} />}
          {current.status === 'succeeded' && current.kind === 'compare-cpu-sycl' && <CompareDetails job={current} />}
          {current.status !== 'succeeded' && <LogView jobId={current.id} height={260} />}
        </div>
      )}
    </div>
  )
}
