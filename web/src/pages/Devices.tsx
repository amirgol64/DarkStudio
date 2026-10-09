// DarkStudio web UI - Devices & backends: CPU, Darknet CPU / SYCL builds, OpenVINO devices, required files.
// SPDX-License-Identifier: Apache-2.0
import { useTranslation } from 'react-i18next'
import { formatBytes, type Backend } from '../api'
import { useStore, refreshSystem } from '../store'
import { ErrorText } from '../components'
import { useAction } from '../util'

function BackendCard({ b }: { b: Backend }) {
  const { t } = useTranslation()
  const info = b.info ?? {}
  const devices: { name: string; detail?: string }[] =
    b.id === 'openvino'
      ? (info.devices ?? []).map((d: any) => ({ name: d.name, detail: d.id }))
      : (info.gpus ?? []).map((g: any) => ({ name: g.name, detail: `${g.info} · ${g.memory}` }))

  return (
    <div className={`card ${b.available ? 'ok' : 'bad'}`}>
      <div className="row" style={{ marginBottom: 10 }}>
        <h2 style={{ margin: 0 }}>{b.name}</h2>
        <span className="spacer" />
        <span className={`badge ${b.available ? 'ok' : 'err'}`}>
          <span className="dot" />
          {b.available ? t('common.available') : t('common.unavailable')}
        </span>
      </div>
      <dl className="kv">
        {(info.version || info.sycl_version) && (
          <>
            <dt>{t('devices.version')}</dt>
            <dd className="mono">{[info.version, info.sycl_version && `SYCL ${info.sycl_version}`].filter(Boolean).join(' · ')}</dd>
          </>
        )}
        {devices.length > 0 && (
          <>
            <dt>{b.id === 'openvino' ? t('devices.devices') : t('devices.gpus')}</dt>
            <dd>
              {devices.map((d) => (
                <div key={d.name + d.detail}>
                  <b>{d.name}</b> <span className="muted">{d.detail}</span>
                </div>
              ))}
            </dd>
          </>
        )}
        {b.id === 'darknet-cpu' && info.backend === 'cpu' && (
          <>
            <dt>{t('devices.cpu')}</dt>
            <dd>AVX2 + OpenMP</dd>
          </>
        )}
      </dl>
      <ErrorText error={b.error} />
      {b.output?.length > 0 && (
        <details style={{ marginTop: 8 }}>
          <summary className="muted">{t('devices.output')} ({b.probe_seconds?.toFixed(1)} s)</summary>
          <div className="log" style={{ maxHeight: 200, marginTop: 6 }}>
            {b.output.map((line, i) => (
              <div key={i}>{line}</div>
            ))}
          </div>
        </details>
      )}
    </div>
  )
}

export default function Devices() {
  const { t } = useTranslation()
  const system = useStore((s) => s.system)
  const action = useAction()

  return (
    <div className="stack">
      <div className="page-head">
        <div>
          <h1>{t('nav.devices')}</h1>
          <p>{system.probing ? t('devices.probing') : system.cpu?.name}</p>
        </div>
        <button className="secondary" disabled={action.busy || system.probing} onClick={() => action.run(() => refreshSystem(true))}>
          {t('devices.reprobe')}
        </button>
      </div>
      <ErrorText error={action.error} />

      {system.cpu && (
        <div className="card">
          <div className="row" style={{ gap: 32 }}>
            <div className="stat"><b>{system.cpu.name ?? '–'}</b><span>{t('devices.cpu')} · {system.cpu.os}</span></div>
            <div className="stat"><b>{system.cpu.threads}</b><span>{t('devices.threads')}</span></div>
            <div className="stat"><b>{formatBytes(system.cpu.ram_bytes)}</b><span>{t('devices.ram')}</span></div>
          </div>
        </div>
      )}

      <div className="grid">
        {(system.backends ?? []).map((b) => (
          <BackendCard key={b.id} b={b} />
        ))}
      </div>

      {system.files && (
        <div className="card">
          <h2>{t('devices.files')}</h2>
          <table>
            <tbody>
              {system.files.map((f) => (
                <tr key={f.what}>
                  <td>{f.what}</td>
                  <td className="mono">{f.path}</td>
                  <td>
                    <span className={`badge ${f.exists ? 'ok' : 'err'}`}>
                      <span className="dot" />
                      {f.exists ? 'OK' : t('devices.missing')}
                    </span>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}
    </div>
  )
}
