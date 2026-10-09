// DarkStudio web UI - Annotation (first version of the M1 labeling canvas): YOLO boxes on images in the
// datasets folder.  Draw, move, resize, delete, change class; saves Darknet/YOLO .txt files next to the images.
// SPDX-License-Identifier: Apache-2.0
import { useCallback, useEffect, useMemo, useRef, useState } from 'react'
import { useTranslation } from 'react-i18next'
import { Image as KImage, Layer, Rect, Stage, Text, Transformer } from 'react-konva'
import type Konva from 'konva'
import { api, type Box, type DatasetFolder, type DatasetImage } from '../api'
import { ErrorText } from '../components'
import { useAction } from '../util'

const colour = (cls: number) => `hsl(${(cls * 67) % 360} 80% 55%)`

function useHtmlImage(url: string | null) {
  // remember which URL the loaded image belongs to, so a stale image is never shown while the next one loads
  const [loaded, setLoaded] = useState<{ url: string; img: HTMLImageElement } | null>(null)
  useEffect(() => {
    if (!url) return
    const i = new window.Image()
    i.onload = () => setLoaded({ url, img: i })
    i.src = url
    return () => {
      i.onload = null
    }
  }, [url])
  return loaded && loaded.url === url ? loaded.img : null
}

function useSize(ref: React.RefObject<HTMLDivElement | null>) {
  const [size, setSize] = useState({ width: 800, height: 600 })
  useEffect(() => {
    if (!ref.current) return
    const obs = new ResizeObserver(([entry]) => setSize({ width: entry.contentRect.width, height: entry.contentRect.height }))
    obs.observe(ref.current)
    return () => obs.disconnect()
  }, [ref])
  return size
}

export default function Annotation() {
  const { t } = useTranslation()
  const [folders, setFolders] = useState<DatasetFolder[]>([])
  const [folder, setFolder] = useState('')
  const [images, setImages] = useState<DatasetImage[]>([])
  const [names, setNames] = useState<string[]>([])
  const [index, setIndex] = useState(0)
  const [boxes, setBoxes] = useState<Box[]>([])
  const [selected, setSelected] = useState<number | null>(null)
  const [activeClass, setActiveClass] = useState(0)
  const [dirty, setDirty] = useState(false)
  const [drawing, setDrawing] = useState<{ x0: number; y0: number; x1: number; y1: number } | null>(null)
  const action = useAction()

  const container = useRef<HTMLDivElement>(null)
  const size = useSize(container)
  const transformer = useRef<Konva.Transformer>(null)
  const rects = useRef<Map<number, Konva.Rect>>(new Map())

  const image = images[index]
  const img = useHtmlImage(image ? api.imageUrl(image.path) : null)

  // fit the image into the canvas
  const view = useMemo(() => {
    if (!img) return { scale: 1, ox: 0, oy: 0, w: 0, h: 0 }
    const scale = Math.min(size.width / img.width, size.height / img.height)
    return { scale, ox: (size.width - img.width * scale) / 2, oy: (size.height - img.height * scale) / 2, w: img.width * scale, h: img.height * scale }
  }, [img, size])

  useEffect(() => {
    void action.run(async () => {
      const list = await api.datasets()
      setFolders(list)
      // open the folder with the most labeled images (dataset roots often only hold charts or screenshots)
      const best = [...list].sort((a, b) => b.with_boxes - a.with_boxes || b.images - a.images)[0]
      if (best && !folder) setFolder(best.path)
    })
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [])

  useEffect(() => {
    if (!folder) return
    void action.run(async () => {
      const r = await api.datasetImages(folder)
      setImages(r.images)
      setNames(r.names)
      setIndex(0)
    })
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [folder])

  useEffect(() => {
    if (!image) return
    setSelected(null)
    void action.run(async () => {
      const r = await api.labels(image.path)
      setBoxes(r.boxes)
      if (r.names.length) setNames(r.names)
      setDirty(false)
    })
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [image?.path])

  useEffect(() => {
    const node = selected === null ? null : rects.current.get(selected)
    transformer.current?.nodes(node ? [node] : [])
    transformer.current?.getLayer()?.batchDraw()
  }, [selected, boxes])

  const save = useCallback(async () => {
    if (!image || !dirty) return
    await api.saveLabels(image.path, boxes)
    setDirty(false)
    setImages((list) => list.map((i) => (i.path === image.path ? { ...i, labeled: boxes.length > 0 || i.labeled } : i)))
  }, [image, boxes, dirty])

  const go = useCallback(
    (next: number) => {
      if (next < 0 || next >= images.length) return
      void action.run(async () => {
        await save() // auto-save before switching images
        setIndex(next)
      })
    },
    // eslint-disable-next-line react-hooks/exhaustive-deps
    [images.length, save],
  )

  const update = (i: number, b: Box) => {
    setBoxes((list) => list.map((old, k) => (k === i ? b : old)))
    setDirty(true)
  }

  // keyboard shortcuts
  useEffect(() => {
    const onKey = (e: KeyboardEvent) => {
      if ((e.target as HTMLElement)?.tagName === 'INPUT' || (e.target as HTMLElement)?.tagName === 'SELECT') return
      if (e.key === 's' && (e.ctrlKey || e.metaKey)) {
        e.preventDefault()
        void action.run(save)
      } else if (e.key === 'Delete' || e.key === 'Backspace') {
        if (selected !== null) {
          setBoxes((list) => list.filter((_, k) => k !== selected))
          setSelected(null)
          setDirty(true)
        }
      } else if (e.key === 'ArrowRight' || e.key === 'ArrowDown') {
        go(index + 1)
      } else if (e.key === 'ArrowLeft' || e.key === 'ArrowUp') {
        go(index - 1)
      } else if (/^[1-9]$/.test(e.key)) {
        const cls = Number(e.key) - 1
        if (cls < Math.max(names.length, 1)) {
          setActiveClass(cls)
          if (selected !== null) update(selected, { ...boxes[selected], class: cls })
        }
      } else if (e.key === 'Escape') {
        setSelected(null)
      }
    }
    window.addEventListener('keydown', onKey)
    return () => window.removeEventListener('keydown', onKey)
  })

  // drawing a new box
  const pointer = (e: Konva.KonvaEventObject<MouseEvent>) => e.target.getStage()!.getPointerPosition()!
  const onDown = (e: Konva.KonvaEventObject<MouseEvent>) => {
    const name = e.target.name()
    if (name !== 'image' && e.target !== e.target.getStage()) return
    setSelected(null)
    const p = pointer(e)
    setDrawing({ x0: p.x, y0: p.y, x1: p.x, y1: p.y })
  }
  const onMove = (e: Konva.KonvaEventObject<MouseEvent>) => {
    if (!drawing) return
    const p = pointer(e)
    setDrawing({ ...drawing, x1: p.x, y1: p.y })
  }
  const onUp = () => {
    if (!drawing || !img) return
    const clamp = (v: number, lo: number, hi: number) => Math.min(hi, Math.max(lo, v))
    const x0 = clamp(Math.min(drawing.x0, drawing.x1), view.ox, view.ox + view.w)
    const x1 = clamp(Math.max(drawing.x0, drawing.x1), view.ox, view.ox + view.w)
    const y0 = clamp(Math.min(drawing.y0, drawing.y1), view.oy, view.oy + view.h)
    const y1 = clamp(Math.max(drawing.y0, drawing.y1), view.oy, view.oy + view.h)
    setDrawing(null)
    if (x1 - x0 < 4 || y1 - y0 < 4) return
    const b: Box = {
      class: activeClass,
      x: ((x0 + x1) / 2 - view.ox) / view.w,
      y: ((y0 + y1) / 2 - view.oy) / view.h,
      w: (x1 - x0) / view.w,
      h: (y1 - y0) / view.h,
    }
    setBoxes((list) => [...list, b])
    setSelected(boxes.length)
    setDirty(true)
  }

  /** Read a moved/resized rectangle back into a normalized YOLO box. */
  const fromNode = (node: Konva.Rect, cls: number): Box => {
    const w = Math.max(2, node.width() * node.scaleX())
    const h = Math.max(2, node.height() * node.scaleY())
    node.scaleX(1)
    node.scaleY(1)
    const clamp01 = (v: number) => Math.min(1, Math.max(0, v))
    return {
      class: cls,
      x: clamp01((node.x() + w / 2 - view.ox) / view.w),
      y: clamp01((node.y() + h / 2 - view.oy) / view.h),
      w: clamp01(w / view.w),
      h: clamp01(h / view.h),
    }
  }

  const classCount = Math.max(names.length, ...boxes.map((b) => b.class + 1), 1)
  const current = folders.find((f) => f.path === folder)

  return (
    <div className="stack" style={{ gap: 10 }}>
      <div className="page-head" style={{ marginBottom: 0 }}>
        <div>
          <h1>{t('nav.annotation')}</h1>
          <p>{t('annot.help')}</p>
        </div>
        <div className="row">
          <label className="field">{t('annot.folder')}
            <select value={folder} onChange={(e) => setFolder(e.target.value)}>
              {folders.map((f) => (
                <option key={f.path} value={f.path}>{f.path} ({f.labeled}/{f.images})</option>
              ))}
            </select>
          </label>
          {dirty && <span className="badge warn">{t('annot.unsaved')}</span>}
          <button disabled={!dirty || action.busy} onClick={() => action.run(save)}>{t('common.save')}</button>
        </div>
      </div>
      <ErrorText error={action.error} />
      {folders.length === 0 && !action.busy && <p className="muted">{t('annot.noFolder')}</p>}

      <div className="annot">
        <div className="card list" style={{ padding: 6 }}>
          <div className="muted" style={{ padding: '2px 8px 6px' }}>{t('annot.images')}: {images.length} · {current?.labeled ?? 0} {t('annot.labeled')}</div>
          {images.map((im, i) => (
            <button key={im.path} className={i === index ? 'active' : ''} onClick={() => go(i)}>
              <span style={{ overflow: 'hidden', textOverflow: 'ellipsis' }}>{im.name}</span>
              {im.labeled && <span aria-hidden>✓</span>}
            </button>
          ))}
        </div>

        <div className="canvas" ref={container}>
          <Stage width={size.width} height={size.height} onMouseDown={onDown} onMouseMove={onMove} onMouseUp={onUp}>
            <Layer>
              {img && <KImage name="image" image={img} x={view.ox} y={view.oy} width={view.w} height={view.h} />}
              {boxes.map((b, i) => {
                const x = view.ox + (b.x - b.w / 2) * view.w
                const y = view.oy + (b.y - b.h / 2) * view.h
                return (
                  <Rect
                    key={i}
                    ref={(node) => {
                      if (node) rects.current.set(i, node)
                      else rects.current.delete(i)
                    }}
                    x={x}
                    y={y}
                    width={b.w * view.w}
                    height={b.h * view.h}
                    stroke={colour(b.class)}
                    strokeWidth={selected === i ? 3 : 2}
                    fill={selected === i ? `${colour(b.class).replace(')', ' / 0.15)')}` : undefined}
                    draggable={selected === i}
                    onMouseDown={(e) => {
                      e.cancelBubble = true
                      setSelected(i)
                    }}
                    onDragEnd={(e) => update(i, fromNode(e.target as Konva.Rect, b.class))}
                    onTransformEnd={(e) => update(i, fromNode(e.target as Konva.Rect, b.class))}
                  />
                )
              })}
              {boxes.map((b, i) => (
                <Text
                  key={`label-${i}`}
                  x={view.ox + (b.x - b.w / 2) * view.w + 2}
                  y={view.oy + (b.y - b.h / 2) * view.h - 16}
                  text={names[b.class] ?? `class ${b.class}`}
                  fontSize={13}
                  fill={colour(b.class)}
                  listening={false}
                />
              ))}
              {drawing && (
                <Rect
                  x={Math.min(drawing.x0, drawing.x1)}
                  y={Math.min(drawing.y0, drawing.y1)}
                  width={Math.abs(drawing.x1 - drawing.x0)}
                  height={Math.abs(drawing.y1 - drawing.y0)}
                  stroke={colour(activeClass)}
                  dash={[6, 4]}
                  listening={false}
                />
              )}
              <Transformer ref={transformer} rotateEnabled={false} keepRatio={false} ignoreStroke />
            </Layer>
          </Stage>
        </div>

        <div className="card classes">
          <h3>{t('annot.classes')}</h3>
          {Array.from({ length: classCount }, (_, cls) => (
            <button
              key={cls}
              className={cls === activeClass ? 'active' : ''}
              onClick={() => {
                setActiveClass(cls)
                if (selected !== null) update(selected, { ...boxes[selected], class: cls })
              }}
            >
              <span className="swatch" style={{ background: colour(cls) }} />
              <span className="muted">{cls < 9 ? cls + 1 : ''}</span>
              {names[cls] ?? `class ${cls}`}
              <span className="spacer" />
              <span className="muted">{boxes.filter((b) => b.class === cls).length}</span>
            </button>
          ))}
          <p className="muted" style={{ marginTop: 12 }}>{boxes.length} {t('annot.boxes')}</p>
          {selected !== null && (
            <button
              className="danger"
              onClick={() => {
                setBoxes((list) => list.filter((_, k) => k !== selected))
                setSelected(null)
                setDirty(true)
              }}
            >
              {t('annot.delete')}
            </button>
          )}
        </div>
      </div>
    </div>
  )
}
