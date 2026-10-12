// DarkStudio web UI - "Browse…" window: pick a folder or a file on this machine through the server's file list
// (GET /api/files).  A web page can't read real paths from the operating system's own file dialog, so the server
// lists the folders and the page shows them.
// SPDX-License-Identifier: Apache-2.0
import { useCallback, useEffect, useMemo, useRef, useState } from 'react'
import { createPortal } from 'react-dom'
import { useTranslation } from 'react-i18next'
import { api, type FileListing } from './api'

type Mode = 'dir' | 'file'

/** Folder part of a path ("C:\a\b.cfg" -> "C:\a"); '' when there is none. */
function folderOf(path: string): string {
  const i = Math.max(path.lastIndexOf('\\'), path.lastIndexOf('/'))
  return i > 0 ? path.slice(0, i) : ''
}

function isAbsolute(path: string): boolean {
  return /^[a-zA-Z]:[\\/]/.test(path) || path.startsWith('/') || path.startsWith('\\\\')
}

function remembered(kind: string): string {
  try {
    return localStorage.getItem(`darkstudio.browse.${kind}`) ?? ''
  } catch {
    return ''
  }
}

function remember(kind: string, folder: string) {
  try {
    localStorage.setItem(`darkstudio.browse.${kind}`, folder)
  } catch {
    // storage unavailable: the next window starts at the default folder
  }
}

function formatSize(bytes: number): string {
  if (bytes < 1024) return `${bytes} B`
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(0)} KiB`
  return `${(bytes / 1024 / 1024).toFixed(1)} MiB`
}

/** Does a file name match one of the patterns (".cfg", or ".conv." for "yolov4-tiny.conv.29")? */
function accepted(name: string, accept?: string[]): boolean {
  if (!accept || accept.length === 0) return true
  const n = name.toLowerCase()
  return accept.some((a) => (a.endsWith('.') ? n.includes(a) : n.endsWith(a)))
}

export function FileBrowser({ mode, title, kind, initial, start, accept, onPick, onClose }: {
  mode: Mode
  title: string
  /** remembers the last folder per kind, e.g. "dataset", "cfg", "weights" */
  kind: string
  /** the field's current value: the window opens there when it is an absolute path */
  initial?: string
  /** shortcut id to open when nothing else is known: datasets, models, runs, root, home */
  start?: string
  accept?: string[]
  onPick: (path: string) => void
  onClose: () => void
}) {
  const { t } = useTranslation()
  const [listing, setListing] = useState<FileListing | null>(null)
  const [typed, setTyped] = useState('')
  const [error, setError] = useState<string | null>(null)
  const [selected, setSelected] = useState<string | null>(null)
  const [showAll, setShowAll] = useState(false)
  const [busy, setBusy] = useState(false)
  const dialog = useRef<HTMLDivElement>(null)

  const load = useCallback(async (path: string) => {
    setBusy(true)
    setError(null)
    try {
      const l = await api.files(path)
      setListing(l)
      setTyped(l.path)
      setSelected(null)
      return true
    } catch (e) {
      setError(e instanceof Error ? e.message : String(e))
      return false
    } finally {
      setBusy(false)
    }
  }, [])

  // open at: the field's value, the last folder used for this kind, the default shortcut, or the drive list
  useEffect(() => {
    void (async () => {
      const value = initial?.trim() ?? ''
      const first = await api.files('').catch(() => null)
      const shortcut = first?.shortcuts.find((s) => s.id === start)
      // a relative value (e.g. "legogears/set1") is relative to the start folder, normally datasets/
      const sep = shortcut?.path.includes('\\') ? '\\' : '/'
      const absolute = !value ? '' : isAbsolute(value) ? value : shortcut ? `${shortcut.path}${sep}${value.replace(/[\\/]/g, sep)}` : ''
      const candidates = [absolute && (mode === 'dir' ? absolute : folderOf(absolute)), remembered(kind)].filter(Boolean)
      for (const c of candidates) {
        if (await load(c)) return
      }
      if (!shortcut || !(await load(shortcut.path))) await load('')
      setError(null)
    })()
    dialog.current?.focus()
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [])

  useEffect(() => {
    const onKey = (e: KeyboardEvent) => {
      if (e.key === 'Escape') onClose()
    }
    document.addEventListener('keydown', onKey)
    return () => document.removeEventListener('keydown', onKey)
  }, [onClose])

  const entries = useMemo(
    () => (listing?.entries ?? []).filter((e) => e.dir || (mode === 'file' && (showAll || accepted(e.name, accept)))),
    [listing, mode, showAll, accept],
  )

  const pick = (path: string) => {
    remember(kind, mode === 'dir' ? path : folderOf(path))
    onPick(path)
    onClose()
  }

  const summary = listing?.summary
  const atDrives = listing?.path === ''

  // rendered into <body>: the window is often opened from inside a <label>, which would otherwise take its clicks
  return createPortal(
    <div className="modal-backdrop" onMouseDown={(e) => e.target === e.currentTarget && onClose()}>
      <div className="modal" role="dialog" aria-modal="true" aria-label={title} tabIndex={-1} ref={dialog}>
        <div className="row" style={{ marginBottom: 10 }}>
          <h2 style={{ margin: 0 }}>{title}</h2>
          <span className="spacer" />
          <button type="button" className="secondary" aria-label={t('files.close')} onClick={onClose}>✕</button>
        </div>

        <form className="row path-bar" onSubmit={(e) => { e.preventDefault(); void load(typed.trim()) }}>
          <button type="button" className="secondary" title={t('files.up')} disabled={!listing || atDrives}
            onClick={() => listing && void load(listing.parent)}>↑</button>
          <input type="text" dir="ltr" className="mono" value={typed} placeholder={t('files.computer')}
            onChange={(e) => setTyped(e.target.value)} spellCheck={false} />
          <button type="submit" className="secondary">{t('files.go')}</button>
        </form>

        <div className="row shortcuts">
          <button type="button" className={`chip ${atDrives ? 'active' : ''}`} onClick={() => void load('')}>{t('files.computer')}</button>
          {listing?.shortcuts.map((s) => (
            <button type="button" key={s.id} className={`chip ${listing.path === s.path ? 'active' : ''}`} title={s.path}
              onClick={() => void load(s.path)}>{t(`files.shortcut.${s.id}`)}</button>
          ))}
        </div>

        {error && <p className="error-text">{error}</p>}

        <div className="file-list" aria-busy={busy}>
          {entries.length === 0 && !busy && <p className="muted" style={{ padding: 10 }}>{t('files.empty')}</p>}
          {entries.map((e) => (
            <button
              type="button"
              key={e.path}
              className={`file-row ${selected === e.path ? 'active' : ''}`}
              onClick={() => (e.dir ? void load(e.path) : setSelected(e.path))}
              onDoubleClick={() => !e.dir && pick(e.path)}
            >
              <span aria-hidden>{e.dir ? '📁' : '📄'}</span>
              <span className="name"><bdi>{e.name}</bdi></span>
              {!e.dir && <span className="muted size">{formatSize(e.size)}</span>}
            </button>
          ))}
        </div>
        {listing?.truncated && <p className="muted">{t('files.truncated')}</p>}

        <div className="row" style={{ marginTop: 12 }}>
          {mode === 'dir' ? (
            <span className="muted">
              {summary && !atDrives && t('files.summary', { images: summary.images, labeled: summary.labeled })}
              {summary && summary.names.length > 0 && ` · ${t('files.names', { names: summary.names.join(', ') })}`}
            </span>
          ) : (
            <>
              {accept && accept.length > 0 && (
                <label className="check"><input type="checkbox" checked={showAll} onChange={(e) => setShowAll(e.target.checked)} />{t('files.showAll')}</label>
              )}
              <span className="muted mono" dir="ltr">{selected ? selected.split(/[\\/]/).pop() : ''}</span>
            </>
          )}
          <span className="spacer" />
          <button type="button" className="secondary" onClick={onClose}>{t('files.cancel')}</button>
          {mode === 'dir' ? (
            <button type="button" disabled={!listing || atDrives} onClick={() => listing && pick(listing.path)}>{t('files.selectFolder')}</button>
          ) : (
            <button type="button" disabled={!selected} onClick={() => selected && pick(selected)}>{t('files.select')}</button>
          )}
        </div>
      </div>
    </div>,
    document.body,
  )
}

/** A path text box with a "Browse…" button that opens the file browser window. */
export function PathField({ value, onChange, mode, kind, title, start, accept, placeholder, list }: {
  value: string
  onChange: (path: string) => void
  mode: Mode
  kind: string
  title: string
  start?: string
  accept?: string[]
  placeholder?: string
  /** id of a <datalist> with suggestions */
  list?: string
}) {
  const { t } = useTranslation()
  const [open, setOpen] = useState(false)
  return (
    <span className="path-field">
      <input type="text" dir="ltr" className="mono" value={value} placeholder={placeholder} list={list}
        onChange={(e) => onChange(e.target.value)} spellCheck={false} />
      <button type="button" className="secondary" onClick={(e) => { e.preventDefault(); setOpen(true) }}>{t('files.browse')}</button>
      {open && (
        <FileBrowser mode={mode} kind={kind} title={title} initial={value} start={start} accept={accept}
          onPick={onChange} onClose={() => setOpen(false)} />
      )}
    </span>
  )
}
