// DarkStudio web UI - Training: start Darknet training from a dataset folder and a model (Normal) or with every
// option / own .data and .cfg files (Advanced), on CPU or Intel GPU (SYCL); live loss / mAP chart, speed and log.
// SPDX-License-Identifier: Apache-2.0
import { useEffect, useMemo, useState } from 'react'
import { useTranslation } from 'react-i18next'
import { CartesianGrid, ComposedChart, Legend, Line, ResponsiveContainer, Scatter, Tooltip, XAxis, YAxis } from 'recharts'
import { api, type DatasetInfo, type Job, type TrainingDataset, type TrainingMetric, type TrainingModel } from '../api'
import { EMPTY_LIST, ensureMetrics, sortJobs, upsertJob, useStore } from '../store'
import { ErrorText, Help, LogView, PageHelp, Progress, StatusBadge } from '../components'
import { PathField } from '../FileBrowser'
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

const SIZES = [224, 256, 288, 320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640]

type Mode = 'normal' | 'advanced'

function savedMode(): Mode {
  try {
    return localStorage.getItem('darkstudio.trainMode') === 'advanced' ? 'advanced' : 'normal'
  } catch {
    return 'normal'
  }
}

/** Text field for an optional number: '' means "keep the model default". */
function optionalNumber(text: string): number | undefined {
  const v = Number(text)
  return text.trim() === '' || Number.isNaN(v) ? undefined : v
}

function ModelStatus({ model, pretrained }: { model?: TrainingModel; pretrained: boolean }) {
  const { t } = useTranslation()
  if (!model) return null
  if (!model.cfg_found) return <p className="error-text">{model.cfg}: {t('devices.missing')}</p>
  if (!pretrained || !model.pretrained) return <p className="muted">{t('train.modelScratch')}</p>
  if (model.partial_found) return <p className="muted">✓ {t('train.modelReady')}</p>
  if (model.weights_found) return <p className="muted">{t('train.modelPrepare')}</p>
  return (
    <p className="error-text">
      {t('train.modelMissing')} <a href={model.weights_url} target="_blank" rel="noreferrer">{model.weights}</a>
    </p>
  )
}

/** "New training run": Normal mode picks a dataset folder and a model (the server writes .data/.cfg); Advanced adds
 *  the Darknet training options and the manual .data/.cfg mode. */
function NewTrainingRun({ disabled, onStart }: { disabled: boolean; onStart: (params: Record<string, unknown>) => void }) {
  const { t } = useTranslation()
  const [mode, setModeState] = useState<Mode>(savedMode)
  const [datasets, setDatasets] = useState<TrainingDataset[]>([])
  const [models, setModels] = useState<TrainingModel[]>([])
  const [loadError, setLoadError] = useState<string | null>(null)

  const [backend, setBackend] = useState<'sycl' | 'cpu'>('sycl')
  const [map, setMap] = useState(true)
  const [dataset, setDataset] = useState('')
  const [modelId, setModelId] = useState('yolov4-tiny')
  const [width, setWidth] = useState(416)
  const [height, setHeight] = useState(416)
  const [iterations, setIterations] = useState<number | null>(null) // null = follow the suggestion
  // advanced
  const [batch, setBatch] = useState(64)
  const [subdivisions, setSubdivisions] = useState(1)
  const [learningRate, setLearningRate] = useState('')
  const [validPercent, setValidPercent] = useState(10)
  const [pretrained, setPretrained] = useState(true)
  const [customWeights, setCustomWeights] = useState('')
  const [mosaic, setMosaic] = useState(false)
  const [flip, setFlip] = useState(true)
  const [hue, setHue] = useState('')
  const [saturation, setSaturation] = useState('')
  const [exposure, setExposure] = useState('')
  const [angle, setAngle] = useState('')
  const [manual, setManual] = useState(false)
  const [data, setData] = useState('')
  const [cfg, setCfg] = useState('')
  const [weights, setWeights] = useState('')

  useEffect(() => {
    Promise.all([api.trainingDatasets(), api.trainingModels()])
      .then(([d, m]) => {
        setDatasets(d)
        setModels(m)
        // preselect the folder with the most boxes that has class names
        const best = [...d].filter((x) => x.names.length > 0).sort((a, b) => b.with_boxes - a.with_boxes)[0]
        if (best) setDataset((cur) => cur || best.path)
      })
      .catch((e) => setLoadError(e instanceof Error ? e.message : String(e)))
  }, [])

  const setMode = (m: Mode) => {
    setModeState(m)
    try {
      localStorage.setItem('darkstudio.trainMode', m)
    } catch {
      // storage unavailable: the choice lasts for this page view
    }
  }

  const advanced = mode === 'advanced'
  const isManual = advanced && manual
  const [current, setCurrent] = useState<DatasetInfo | null>(null)
  const [datasetError, setDatasetError] = useState<string | null>(null)
  useEffect(() => {
    const path = dataset.trim()
    let cancelled = false
    const timer = setTimeout(() => {
      if (!path) {
        setCurrent(null)
        setDatasetError(null)
        return
      }
      api.datasetInfo(path)
        .then((info) => { if (!cancelled) { setCurrent(info); setDatasetError(null) } })
        .catch((e) => { if (!cancelled) { setCurrent(null); setDatasetError(e instanceof Error ? e.message : String(e)) } })
    }, 300)
    return () => {
      cancelled = true
      clearTimeout(timer)
    }
  }, [dataset])
  const model = models.find((m) => m.id === modelId)
  const classes = current?.names.length ?? 0
  const suggested = Math.max(6000, 2000 * classes)
  const maxBatches = iterations ?? suggested

  const start = () => {
    if (isManual) {
      onStart({ backend, map, data, cfg, weights })
      return
    }
    const params: Record<string, unknown> = { backend, map, dataset, model: modelId, width, height: advanced ? height : width, max_batches: maxBatches }
    if (advanced) {
      Object.assign(params, {
        batch, subdivisions, valid_percent: validPercent, pretrained, mosaic, flip,
        learning_rate: optionalNumber(learningRate), hue: optionalNumber(hue), saturation: optionalNumber(saturation),
        exposure: optionalNumber(exposure), angle: optionalNumber(angle),
      })
      if (customWeights.trim()) params.weights = customWeights.trim()
    }
    onStart(params)
  }

  const canStart = isManual ? true : !!current && classes > 0 && !!model?.cfg_found

  return (
    <div className="card">
      <div className="row" style={{ marginBottom: 12 }}>
        <h2 style={{ margin: 0 }}>{t('train.new')}</h2>
        <span className="spacer" />
        <div className="segmented" role="group" aria-label={t('train.mode')}>
          <button type="button" className={!advanced ? 'active' : ''} onClick={() => setMode('normal')}>{t('train.normal')}</button>
          <button type="button" className={advanced ? 'active' : ''} onClick={() => setMode('advanced')}>{t('train.advanced')}</button>
        </div>
      </div>
      <ErrorText error={loadError} />

      {advanced && (
        <label className="check" style={{ marginBottom: 12 }}>
          <input type="checkbox" checked={manual} onChange={(e) => setManual(e.target.checked)} />
          {t('train.manual')}
          <Help text={t('train.tips.manual')} anchor="#46-use-your-own-data-and-cfg-files" />
        </label>
      )}

      {!isManual && (
        <>
          <div className="form-grid">
            <label className="field">
              <span>{t('train.dataset')}<Help text={t('train.tips.dataset')} anchor="#41-put-your-images-in-datasets" /></span>
              <PathField mode="dir" kind="dataset" start="datasets" title={t('files.titleDataset')} list="train-datasets"
                placeholder={t('train.choose')} value={dataset} onChange={(v) => { setDataset(v); setIterations(null) }} />
              <datalist id="train-datasets">
                {datasets.map((d) => <option key={d.path} value={d.path} />)}
              </datalist>
            </label>
            <label className="field">
              <span>{t('train.model')}<Help text={t('train.tips.model')} anchor="#44-choose-the-model-and-pretrained-weights" /></span>
              <select value={modelId} onChange={(e) => setModelId(e.target.value)}>
                {models.map((m) => <option key={m.id} value={m.id}>{m.name}</option>)}
              </select>
            </label>
          </div>
          {datasets.length === 0 && !loadError && !dataset && <p className="muted">{t('train.noDatasets')}</p>}
          <ErrorText error={datasetError} />
          {current && (
            <p className="muted">
              {t('train.datasetInfo', { images: current.labeled, classes })}
              {classes > 0 ? `: ${current.names.slice(0, 8).join(', ')}${classes > 8 ? '…' : ''}` : <span className="error-text"> · {t('train.noNames')}</span>}
            </p>
          )}
          <ModelStatus model={model} pretrained={pretrained} />

          <div className="form-grid">
            {advanced ? (
              <>
                <label className="field">
                  <span>{t('train.width')}<Help text={t('train.tips.size')} anchor="#45-advanced-options" /></span>
                  <input type="number" min={32} max={2048} step={32} value={width} onChange={(e) => setWidth(Number(e.target.value))} />
                </label>
                <label className="field">{t('train.height')}
                  <input type="number" min={32} max={2048} step={32} value={height} onChange={(e) => setHeight(Number(e.target.value))} />
                </label>
              </>
            ) : (
              <label className="field">
                <span>{t('train.size')}<Help text={t('train.tips.size')} anchor="#43-start-a-training-run-normal" /></span>
                <select value={width} onChange={(e) => { setWidth(Number(e.target.value)); setHeight(Number(e.target.value)) }}>
                  {SIZES.map((s) => <option key={s} value={s}>{s} × {s}</option>)}
                </select>
              </label>
            )}
            <label className="field">
              <span>{t('train.iterations')}<Help text={t('train.tips.iterations')} anchor="#43-start-a-training-run-normal" /></span>
              <input type="number" min={1} max={2000000} value={maxBatches} onChange={(e) => setIterations(Number(e.target.value))} />
            </label>
            <label className="field">
              <span>{t('train.backend')}<Help text={t('train.tips.device')} /></span>
              <select value={backend} onChange={(e) => setBackend(e.target.value as 'sycl' | 'cpu')}>
                <option value="sycl">Intel GPU (SYCL)</option>
                <option value="cpu">CPU</option>
              </select>
            </label>
          </div>
          {classes > 0 && maxBatches !== suggested && (
            <p className="muted">
              {t('train.suggested', { n: suggested })}{' '}
              <button type="button" className="secondary" style={{ padding: '1px 8px' }} onClick={() => setIterations(null)}>{t('train.useSuggested')}</button>
            </p>
          )}

          {advanced && (
            <>
              <fieldset className="group">
                <legend>{t('train.advanced')}</legend>
                <div className="form-grid">
                  <label className="field">
                    <span>{t('train.batch')}<Help text={t('train.tips.batch')} anchor="#45-advanced-options" /></span>
                    <input type="number" min={1} max={256} value={batch} onChange={(e) => setBatch(Number(e.target.value))} />
                  </label>
                  <label className="field">
                    <span>{t('train.subdivisions')}<Help text={t('train.tips.subdivisions')} anchor="#45-advanced-options" /></span>
                    <input type="number" min={1} max={256} value={subdivisions} onChange={(e) => setSubdivisions(Number(e.target.value))} />
                  </label>
                  <label className="field">
                    <span>{t('train.learningRate')}<Help text={t('train.tips.learningRate')} anchor="#45-advanced-options" /></span>
                    <input type="number" min={0} step={0.0001} placeholder="0.00261" value={learningRate} onChange={(e) => setLearningRate(e.target.value)} />
                  </label>
                  <label className="field">
                    <span>{t('train.validPercent')}<Help text={t('train.tips.validPercent')} anchor="#45-advanced-options" /></span>
                    <input type="number" min={0} max={50} value={validPercent} onChange={(e) => setValidPercent(Number(e.target.value))} />
                  </label>
                </div>
                <label className="check" style={{ marginBottom: 8 }}>
                  <input type="checkbox" checked={pretrained} onChange={(e) => setPretrained(e.target.checked)} />
                  {t('train.pretrained')}
                  <Help text={t('train.tips.pretrained')} anchor="#44-choose-the-model-and-pretrained-weights" />
                </label>
                <label className="field" style={{ marginBottom: 12 }}>
                  <span>{t('train.customWeights')}<Help text={t('train.tips.customWeights')} anchor="#44-choose-the-model-and-pretrained-weights" /></span>
                  <PathField mode="file" kind="weights" start="models" title={t('files.titleWeights')} accept={['.weights', '.conv.']}
                    value={customWeights} onChange={setCustomWeights} />
                </label>
              </fieldset>

              <fieldset className="group">
                <legend>{t('train.augmentation')}<Help text={t('train.tips.augmentation')} anchor="#45-advanced-options" /></legend>
                <div className="row" style={{ marginBottom: 10 }}>
                  <label className="check"><input type="checkbox" checked={mosaic} onChange={(e) => setMosaic(e.target.checked)} />{t('train.mosaic')}</label>
                  <label className="check"><input type="checkbox" checked={flip} onChange={(e) => setFlip(e.target.checked)} />{t('train.flip')}</label>
                </div>
                <div className="form-grid">
                  <label className="field">{t('train.hue')}<input type="number" min={0} max={0.5} step={0.01} placeholder="0.1" value={hue} onChange={(e) => setHue(e.target.value)} /></label>
                  <label className="field">{t('train.saturation')}<input type="number" min={1} max={3} step={0.1} placeholder="1.5" value={saturation} onChange={(e) => setSaturation(e.target.value)} /></label>
                  <label className="field">{t('train.exposure')}<input type="number" min={1} max={3} step={0.1} placeholder="1.5" value={exposure} onChange={(e) => setExposure(e.target.value)} /></label>
                  <label className="field">{t('train.angle')}<input type="number" min={0} max={180} placeholder="0" value={angle} onChange={(e) => setAngle(e.target.value)} /></label>
                </div>
              </fieldset>
            </>
          )}
        </>
      )}

      {isManual && (
        <div className="stack" style={{ gap: 8, marginBottom: 12 }}>
          <label className="field">{t('train.data')}
            <PathField mode="file" kind="data" start="datasets" title={t('files.titleData')} accept={['.data']}
              placeholder="build/train-test/legogears/LegoGears.data" value={data} onChange={setData} />
          </label>
          <label className="field">{t('train.cfg')}
            <PathField mode="file" kind="cfg" start="datasets" title={t('files.titleCfg')} accept={['.cfg']}
              placeholder="build/train-test/legogears/LegoGears.cfg" value={cfg} onChange={setCfg} />
          </label>
          <label className="field">{t('train.weights')}
            <PathField mode="file" kind="weights" start="models" title={t('files.titleWeights')} accept={['.weights', '.conv.']}
              value={weights} onChange={setWeights} />
          </label>
          <div className="form-grid">
            <label className="field">{t('train.backend')}
              <select value={backend} onChange={(e) => setBackend(e.target.value as 'sycl' | 'cpu')}>
                <option value="sycl">Intel GPU (SYCL)</option>
                <option value="cpu">CPU</option>
              </select>
            </label>
          </div>
        </div>
      )}

      <label className="check" style={{ marginBottom: 12 }}>
        <input type="checkbox" checked={map} onChange={(e) => setMap(e.target.checked)} />
        {t('train.map')}
        <Help text={t('train.tips.map')} anchor="#5-follow-the-run-evaluate-it-find-the-weights" />
      </label>
      {backend === 'cpu' && <p className="muted">⚠ {t('train.cpuWarning')}</p>}
      <button disabled={disabled || !canStart} onClick={start}>{t('common.start')}</button>
    </div>
  )
}

export default function Training() {
  const { t } = useTranslation()
  const allJobs = useStore((s) => s.jobs)
  const jobs = useMemo(() => sortJobs(allJobs).filter((j) => j.kind === 'train' || j.kind === 'map' || j.kind === 'prepare-legogears'), [allJobs])
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
    // runs started from a dataset folder keep their generated .data/.cfg in the run folder (job.result)
    const data = String(job.result.data ?? job.params.data ?? '')
    const cfg = String(job.result.cfg ?? job.params.cfg ?? '')
    const cfgName = (cfg || 'LegoGears.cfg').split(/[\\/]/).pop()!.replace(/\.cfg$/, '')
    start('map', {
      backend: job.params.backend ?? 'sycl',
      data,
      cfg,
      // Darknet only writes *_best.weights when mAP is calculated during training
      weights: `${job.dir}/${cfgName}_${job.params.map === false ? 'final' : 'best'}.weights`,
      iou,
    })
  }

  return (
    <div className="stack">
      <div className="page-head"><h1>{t('nav.training')}</h1></div>
      <PageHelp page="training" anchor="#4-train-on-your-own-dataset" />
      <ErrorText error={action.error} />

      <div className="grid train-top">
        <NewTrainingRun disabled={action.busy || !!running} onStart={(params) => start('train', params)} />

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
          {typeof current.result.train_images === 'number' && (
            <p className="muted">
              {t('train.generated', { train: current.result.train_images, valid: current.result.valid_images, classes: current.result.classes })}
              {' · '}<span className="mono" dir="ltr">{current.dir}</span>
            </p>
          )}
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
