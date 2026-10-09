// DarkStudio web UI - Training: prepare data, start/stop Darknet training on CPU or Intel GPU (SYCL),
// live loss / mAP chart, speed and log.
// SPDX-License-Identifier: Apache-2.0
import { useEffect, useMemo, useState } from 'react'
import { useTranslation } from 'react-i18next'
import { CartesianGrid, ComposedChart, Legend, Line, ResponsiveContainer, Scatter, Tooltip, XAxis, YAxis } from 'recharts'
import { api, type Job, type TrainingMetric } from '../api'
import { EMPTY_LIST, ensureMetrics, sortJobs, upsertJob, useStore } from '../store'
import { ErrorText, LogView, Progress, StatusBadge } from '../components'
import { duration, useAction } from '../util'

function TrainingChart({ job }: { job: Job }) {
  const { t } = useTranslation()
  const metrics = useStore((s) => s.metrics[job.id] ?? (EMPTY_LIST as TrainingMetric[]))
  useEffect(() => {
    void ensureMetrics(job.id)
  }, [job.id])

  const data = useMemo(() => {
    const rows = new Map<number, { iteration: number; loss?: number; avg_loss?: number; map?: number }>()
    for (const m of metrics) {
      const row = rows.get(m.iteration) ?? { iteration: m.iteration }
      if (m.type === 'iteration') {
        row.loss = m.loss
        row.avg_loss = m.avg_loss
      } else if (m.type === 'map') {
        row.map = m.map
      }
      rows.set(m.iteration, row)
    }
    return [...rows.values()].sort((a, b) => a.iteration - b.iteration)
  }, [metrics])

  const last = [...metrics].reverse().find((m) => m.type === 'iteration')
  const lastMap = [...metrics].reverse().find((m) => m.type === 'map')
  const speeds = metrics.filter((m) => m.type === 'iteration').slice(-20).map((m) => m.train_seconds ?? 0)
  const speed = speeds.length ? speeds.reduce((a, b) => a + b, 0) / speeds.length : undefined
  const remaining = speed && job.progress.total ? Math.max(0, job.progress.total - job.progress.current) * speed : undefined

  return (
    <div className="stack">
      <div className="row" style={{ gap: 32 }}>
        <div className="stat"><b>{job.progress.current} / {job.progress.total || '?'}</b><span>{t('train.iteration')}</span></div>
        <div className="stat"><b>{last?.loss?.toFixed(3) ?? '–'}</b><span>{t('train.loss')}</span></div>
        <div className="stat"><b>{last?.avg_loss?.toFixed(3) ?? '–'}</b><span>{t('train.avgLoss')}</span></div>
        <div className="stat"><b>{lastMap ? `${lastMap.map?.toFixed(1)}%` : '–'}</b><span>{t('train.mapLabel')}{lastMap ? ` @${lastMap.iou}` : ''}</span></div>
        <div className="stat"><b>{speed?.toFixed(2) ?? '–'} s</b><span>{t('train.speed')}</span></div>
        {job.status === 'running' && <div className="stat"><b>{remaining ? `${Math.ceil(remaining / 60)} min` : '–'}</b><span>{t('train.remaining')}</span></div>}
      </div>
      <Progress job={job} />
      <div style={{ height: 300, direction: 'ltr' }}>
        <ResponsiveContainer>
          <ComposedChart data={data} margin={{ left: 4, right: 4 }}>
            <CartesianGrid strokeDasharray="3 3" stroke="var(--border)" />
            <XAxis dataKey="iteration" type="number" domain={[0, job.progress.total || 'dataMax']} tick={{ fill: 'var(--muted)', fontSize: 11 }} />
            <YAxis yAxisId="loss" scale="log" domain={['dataMin', 'dataMax']} tickFormatter={(v) => Number(v).toPrecision(2)} tick={{ fill: 'var(--muted)', fontSize: 11 }} />
            <YAxis yAxisId="map" orientation="right" domain={[0, 100]} unit="%" tick={{ fill: 'var(--muted)', fontSize: 11 }} />
            <Tooltip contentStyle={{ background: 'var(--surface)', border: '1px solid var(--border)' }} />
            <Legend />
            <Line yAxisId="loss" dataKey="loss" name={t('train.loss')} stroke="var(--muted)" dot={false} strokeWidth={1} isAnimationActive={false} connectNulls />
            <Line yAxisId="loss" dataKey="avg_loss" name={t('train.avgLoss')} stroke="var(--accent)" dot={false} strokeWidth={2} isAnimationActive={false} connectNulls />
            <Scatter yAxisId="map" dataKey="map" name="mAP" fill="var(--ok)" isAnimationActive={false} />
          </ComposedChart>
        </ResponsiveContainer>
      </div>
    </div>
  )
}

export default function Training() {
  const { t } = useTranslation()
  const allJobs = useStore((s) => s.jobs)
  const jobs = useMemo(() => sortJobs(allJobs).filter((j) => j.kind === 'train' || j.kind === 'map' || j.kind === 'prepare-legogears'), [allJobs])
  const [backend, setBackend] = useState<'sycl' | 'cpu'>('sycl')
  const [data, setData] = useState('')
  const [cfg, setCfg] = useState('')
  const [weights, setWeights] = useState('')
  const [map, setMap] = useState(true)
  const [maxBatches, setMaxBatches] = useState(1000)
  const [iou, setIou] = useState('0.50')
  const [selected, setSelected] = useState<string | null>(null)
  const [onlyProblems, setOnlyProblems] = useState(false)
  const action = useAction()

  const running = jobs.find((j) => j.status === 'running')
  const current = jobs.find((j) => j.id === selected) ?? running ?? jobs[0]

  const start = (kind: 'train' | 'map' | 'prepare-legogears', params: Record<string, unknown>) =>
    action.run(async () => {
      const job = await api.startJob(kind, params)
      upsertJob(job)
      setSelected(job.id)
    })

  const evaluate = (job: Job) => {
    const cfgName = String(job.params.cfg ?? 'LegoGears.cfg').split(/[\\/]/).pop()!.replace(/\.cfg$/, '')
    start('map', {
      backend: job.params.backend ?? 'sycl',
      data: job.params.data ?? '',
      cfg: job.params.cfg ?? '',
      // Darknet only writes *_best.weights when mAP is calculated during training
      weights: `${job.dir}/${cfgName}_${job.params.map === false ? 'final' : 'best'}.weights`,
      iou,
    })
  }

  return (
    <div className="stack">
      <div className="page-head"><h1>{t('nav.training')}</h1></div>
      <ErrorText error={action.error} />

      <div className="grid">
        <div className="card">
          <h2>{t('train.new')}</h2>
          <div className="form-grid">
            <label className="field">{t('train.backend')}
              <select value={backend} onChange={(e) => setBackend(e.target.value as 'sycl' | 'cpu')}>
                <option value="sycl">Intel GPU (SYCL)</option>
                <option value="cpu">CPU</option>
              </select>
            </label>
            <label className="check"><input type="checkbox" checked={map} onChange={(e) => setMap(e.target.checked)} />{t('train.map')}</label>
          </div>
          <div className="stack" style={{ gap: 8, marginBottom: 12 }}>
            <label className="field">{t('train.data')}<input type="text" dir="ltr" className="mono" placeholder="build/train-test/legogears/LegoGears.data" value={data} onChange={(e) => setData(e.target.value)} /></label>
            <label className="field">{t('train.cfg')}<input type="text" dir="ltr" className="mono" placeholder="build/train-test/legogears/LegoGears.cfg" value={cfg} onChange={(e) => setCfg(e.target.value)} /></label>
            <label className="field">{t('train.weights')}<input type="text" dir="ltr" className="mono" value={weights} onChange={(e) => setWeights(e.target.value)} /></label>
          </div>
          {backend === 'cpu' && <p className="muted">⚠ {t('train.cpuWarning')}</p>}
          <button disabled={action.busy || !!running} onClick={() => start('train', { backend, data, cfg, weights, map })}>{t('common.start')}</button>
        </div>

        <div className="card">
          <h2>{t('train.prepare')}</h2>
          <p className="muted">{t('train.prepareHelp')}</p>
          <div className="form-grid">
            <label className="field">{t('train.maxBatches')}
              <input type="number" min={1} max={100000} value={maxBatches} onChange={(e) => setMaxBatches(Number(e.target.value))} />
            </label>
          </div>
          <button className="secondary" disabled={action.busy || !!running} onClick={() => start('prepare-legogears', { max_batches: maxBatches })}>{t('common.start')}</button>
        </div>
      </div>

      <div className="card">
        <h2>{t('train.runs')}</h2>
        {jobs.length === 0 ? <p className="muted">{t('common.none')}</p> : (
          <table>
            <thead><tr><th>{t('logs.job')}</th><th>{t('common.status')}</th><th>{t('common.started')}</th><th>{t('common.duration')}</th><th className="num">{t('train.progress')}</th><th className="num">mAP</th></tr></thead>
            <tbody>
              {jobs.map((j) => (
                <tr key={j.id} className={`clickable ${current?.id === j.id ? 'selected' : ''}`} onClick={() => setSelected(j.id)}>
                  <td>{j.title}</td>
                  <td><StatusBadge status={j.status} /></td>
                  <td>{j.started.replace('T', ' ')}</td>
                  <td>{duration(j)}</td>
                  <td className="num">{j.progress.total ? `${j.progress.current} / ${j.progress.total}` : '–'}</td>
                  <td className="num">{typeof j.result.last_map === 'number' ? `${j.result.last_map.toFixed(1)}%` : '–'}</td>
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
            {current.kind === 'train' && current.status === 'succeeded' && (
              <>
                <label className="field" style={{ flexDirection: 'row', alignItems: 'center' }}>{t('train.iou')}
                  <select value={iou} onChange={(e) => setIou(e.target.value)}><option>0.50</option><option>0.75</option></select>
                </label>
                <button className="secondary" disabled={action.busy || !!running} onClick={() => evaluate(current)}>{t('train.evaluate')}</button>
              </>
            )}
            {current.status === 'running' && <button className="danger" onClick={() => action.run(() => api.stopJob(current.id))}>{t('common.stop')}</button>}
          </div>
          <ErrorText error={current.error} />
          {current.kind === 'train' && <TrainingChart job={current} />}
          <div className="row" style={{ margin: '12px 0 6px' }}>
            <span className="muted">{current.lines} {t('common.lines')} · {current.errors} {t('common.errors')} · {current.warnings} {t('common.warnings')}</span>
            <span className="spacer" />
            <label className="check"><input type="checkbox" checked={onlyProblems} onChange={(e) => setOnlyProblems(e.target.checked)} />{t('logs.onlyProblems')}</label>
          </div>
          <LogView jobId={current.id} onlyProblems={onlyProblems} height={240} />
        </div>
      )}
    </div>
  )
}
