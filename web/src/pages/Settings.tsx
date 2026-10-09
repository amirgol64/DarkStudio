// DarkStudio web UI - Settings: tool paths, benchmark defaults, OpenVINO cache, theme and language.
// SPDX-License-Identifier: Apache-2.0
import { useEffect, useState } from 'react'
import { useTranslation } from 'react-i18next'
import { api, type Settings } from '../api'
import { languages, setLanguage } from '../i18n'
import { applyTheme, useAction } from '../util'
import { ErrorText } from '../components'

const pathKeys = [
  'darknet_cpu_bin', 'darknet_sycl_bin', 'oneapi_bin', 'openvino_bin', 'openvino_tbb_bin',
  'ov_bench', 'compare_script', 'models_dir', 'datasets_dir', 'web_dir',
] as const

export default function SettingsPage() {
  const { t, i18n } = useTranslation()
  const [settings, setSettings] = useState<Settings | null>(null)
  const [saved, setSaved] = useState(false)
  const action = useAction()

  useEffect(() => {
    void action.run(async () => setSettings(await api.settings()))
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [])

  if (!settings) return <><h1>{t('nav.settings')}</h1><ErrorText error={action.error} /><p className="muted">{t('common.loading')}</p></>

  const set = <K extends keyof Settings>(key: K, value: Settings[K]) => {
    setSettings({ ...settings, [key]: value })
    setSaved(false)
  }

  const save = () =>
    action.run(async () => {
      const s = await api.saveSettings(settings)
      setSettings(s)
      applyTheme(s.theme)
      setLanguage(s.language)
      setSaved(true)
    })

  return (
    <div className="stack">
      <div className="page-head">
        <h1>{t('nav.settings')}</h1>
        <div className="row">
          {saved && <span className="badge ok"><span className="dot" />{t('common.saved')}</span>}
          <button disabled={action.busy} onClick={save}>{t('common.save')}</button>
        </div>
      </div>
      <ErrorText error={action.error} />

      <div className="card">
        <h2>{t('settings.appearance')}</h2>
        <div className="form-grid">
          <label className="field">{t('settings.language')}
            <select value={settings.language} onChange={(e) => { set('language', e.target.value); setLanguage(e.target.value) }}>
              {languages.map((l) => <option key={l.code} value={l.code}>{l.label}</option>)}
            </select>
          </label>
          <label className="field">{t('settings.theme')}
            <select value={settings.theme} onChange={(e) => { set('theme', e.target.value as Settings['theme']); applyTheme(e.target.value) }}>
              <option value="system">{t('settings.system')}</option>
              <option value="light">{t('settings.light')}</option>
              <option value="dark">{t('settings.dark')}</option>
            </select>
          </label>
        </div>
        <p className="muted">{i18n.language === 'he' ? 'RTL' : 'LTR'}</p>
      </div>

      <div className="card">
        <h2>{t('settings.defaults')}</h2>
        <div className="form-grid">
          <label className="field">{t('bench.device')}
            <select value={settings.default_device} onChange={(e) => set('default_device', e.target.value)}>
              <option>GPU</option><option>CPU</option><option>AUTO</option>
            </select>
          </label>
          <label className="field">{t('bench.precision')}
            <select value={settings.default_precision} onChange={(e) => set('default_precision', e.target.value as 'f16' | 'f32')}>
              <option value="f16">FP16</option><option value="f32">FP32</option>
            </select>
          </label>
        </div>
        <label className="check"><input type="checkbox" checked={settings.openvino_cache} onChange={(e) => set('openvino_cache', e.target.checked)} />{t('settings.cache')}</label>
      </div>

      <div className="card">
        <h2>{t('settings.paths')}</h2>
        <div className="stack" style={{ gap: 10 }}>
          {pathKeys.map((key) => (
            <label key={key} className="field">{t(`settings.${key}`)}
              <input type="text" className="mono" value={settings[key]} onChange={(e) => set(key, e.target.value)} spellCheck={false} dir="ltr" />
            </label>
          ))}
          <label className="field">{t('settings.root')} <span className="muted">({t('settings.readOnly')})</span>
            <input type="text" className="mono" value={settings.root} readOnly dir="ltr" />
          </label>
          <label className="field">{t('settings.work_dir')} <span className="muted">({t('settings.readOnly')})</span>
            <input type="text" className="mono" value={settings.work_dir} readOnly dir="ltr" />
          </label>
        </div>
      </div>
    </div>
  )
}
