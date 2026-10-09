// DarkStudio web UI - translations (English and Hebrew).  Hebrew switches the layout to right-to-left.
// SPDX-License-Identifier: Apache-2.0
import i18n from 'i18next'
import { initReactI18next } from 'react-i18next'

const en = {
  app: { title: 'DarkStudio', subtitle: 'Darknet · Intel GPU · OpenVINO', connected: 'Connected', disconnected: 'Server offline – reconnecting…' },
  nav: { devices: 'Devices', benchmarks: 'Benchmarks', training: 'Training', logs: 'Logs & errors', annotation: 'Annotation', settings: 'Settings' },
  common: {
    refresh: 'Refresh', start: 'Start', stop: 'Stop', save: 'Save', saved: 'Saved', loading: 'Loading…', none: 'Nothing yet',
    available: 'Available', unavailable: 'Not available', error: 'Error', errors: 'errors', warnings: 'warnings', lines: 'lines',
    status: 'Status', started: 'Started', duration: 'Duration', open: 'Open', details: 'Details',
  },
  status: { queued: 'Queued', running: 'Running', succeeded: 'Succeeded', failed: 'Failed', stopped: 'Stopped' },
  devices: {
    cpu: 'CPU', threads: 'threads', ram: 'RAM', probing: 'Detecting devices…', reprobe: 'Detect again',
    files: 'Required files', gpus: 'GPUs', devices: 'Devices', version: 'Version', output: 'Probe output', missing: 'missing',
  },
  bench: {
    openvino: 'OpenVINO benchmark', openvinoHelp: 'yolov4-tiny (ONNX) on the 6 Darknet sample images',
    compare: 'Darknet CPU vs SYCL', compareHelp: 'Checks that the Intel GPU gives the same detections as the CPU, and how much faster it is',
    device: 'Device', precision: 'Precision', iterations: 'Iterations per image', repeat: 'Passes',
    history: 'Results', perImage: 'ms per image', fps: 'FPS', infer: 'inference', total: 'total', compile: 'compile',
    mismatches: 'class mismatches', probDiff: 'worst probability difference', boxDiff: 'worst box difference', speedup: 'speed-up',
    chart: 'Milliseconds per image (lower is better)', images: 'Annotated images',
  },
  train: {
    new: 'New training run', backend: 'Device', data: 'Data file', cfg: 'Network (.cfg)', weights: 'Start from weights (optional)',
    map: 'Calculate mAP during training', prepare: 'Prepare LEGO Gears', prepareHelp: 'Downloads the test dataset and writes a training cfg',
    maxBatches: 'Iterations', runs: 'Training runs', iteration: 'Iteration', loss: 'Loss', avgLoss: 'Average loss',
    mapLabel: 'mAP', speed: 'Seconds per iteration', remaining: 'Remaining', progress: 'Progress', cpuWarning:
      'Training on the CPU is ~27× slower than on the Iris Xe (about 30 s per iteration).', evaluate: 'Evaluate mAP', iou: 'IoU threshold',
  },
  logs: { job: 'Job', filter: 'Filter', all: 'All lines', onlyProblems: 'Errors & warnings', search: 'Search…', follow: 'Follow', empty: 'Select a job to see its log' },
  annot: {
    folder: 'Dataset folder', images: 'Images', classes: 'Classes', help: 'Drag on the image to draw a box. Click a box to select it; Delete removes it. Keys 1–9 pick the class, ←/→ change image, Ctrl+S saves.',
    unsaved: 'Unsaved changes', noFolder: 'No images found in the datasets folder', labeled: 'labeled', boxes: 'boxes', delete: 'Delete box',
  },
  settings: {
    paths: 'Paths', defaults: 'Defaults', appearance: 'Appearance', language: 'Language', theme: 'Theme',
    system: 'System', light: 'Light', dark: 'Dark', cache: 'OpenVINO model cache (12.6 s → 0.12 s GPU startup)',
    darknet_cpu_bin: 'Darknet CPU (bin)', darknet_sycl_bin: 'Darknet SYCL (bin)', oneapi_bin: 'oneAPI runtime (bin)',
    openvino_bin: 'OpenVINO runtime (bin)', openvino_tbb_bin: 'OpenVINO TBB (bin)', ov_bench: 'ov-bench executable',
    compare_script: 'CPU vs SYCL script', models_dir: 'Models folder', datasets_dir: 'Datasets folder', web_dir: 'Web UI folder',
    readOnly: 'Fixed for this server', root: 'DarkStudio folder', work_dir: 'Work folder',
  },
}

const he: typeof en = {
  app: { title: 'DarkStudio', subtitle: 'Darknet · מעבד גרפי של Intel · OpenVINO', connected: 'מחובר', disconnected: 'השרת לא זמין – מתחבר מחדש…' },
  nav: { devices: 'התקנים', benchmarks: 'מדדי ביצועים', training: 'אימון', logs: 'יומן ושגיאות', annotation: 'תיוג', settings: 'הגדרות' },
  common: {
    refresh: 'רענון', start: 'התחלה', stop: 'עצירה', save: 'שמירה', saved: 'נשמר', loading: 'טוען…', none: 'אין עדיין נתונים',
    available: 'זמין', unavailable: 'לא זמין', error: 'שגיאה', errors: 'שגיאות', warnings: 'אזהרות', lines: 'שורות',
    status: 'מצב', started: 'התחיל', duration: 'משך', open: 'פתיחה', details: 'פרטים',
  },
  status: { queued: 'בתור', running: 'רץ', succeeded: 'הצליח', failed: 'נכשל', stopped: 'נעצר' },
  devices: {
    cpu: 'מעבד', threads: 'תהליכונים', ram: 'זיכרון', probing: 'מזהה התקנים…', reprobe: 'זיהוי מחדש',
    files: 'קבצים נדרשים', gpus: 'מעבדים גרפיים', devices: 'התקנים', version: 'גרסה', output: 'פלט הבדיקה', missing: 'חסר',
  },
  bench: {
    openvino: 'מדד ביצועים OpenVINO', openvinoHelp: 'yolov4-tiny (ONNX) על 6 תמונות הדוגמה של Darknet',
    compare: 'Darknet: מעבד מול SYCL', compareHelp: 'בודק שהמעבד הגרפי של Intel נותן אותן תוצאות כמו המעבד, ובכמה הוא מהיר יותר',
    device: 'התקן', precision: 'דיוק', iterations: 'חזרות לכל תמונה', repeat: 'מעברים',
    history: 'תוצאות', perImage: 'מ"ש לתמונה', fps: 'FPS', infer: 'הסקה', total: 'סה"כ', compile: 'הידור',
    mismatches: 'אי-התאמות מחלקה', probDiff: 'הפרש הסתברות מרבי', boxDiff: 'הפרש תיבה מרבי', speedup: 'האצה',
    chart: 'מילישניות לתמונה (נמוך יותר עדיף)', images: 'תמונות מסומנות',
  },
  train: {
    new: 'אימון חדש', backend: 'התקן', data: 'קובץ נתונים', cfg: 'רשת (.cfg)', weights: 'התחלה ממשקולות (רשות)',
    map: 'חישוב mAP במהלך האימון', prepare: 'הכנת LEGO Gears', prepareHelp: 'מוריד את מאגר הבדיקה וכותב קובץ cfg לאימון',
    maxBatches: 'איטרציות', runs: 'הרצות אימון', iteration: 'איטרציה', loss: 'הפסד', avgLoss: 'הפסד ממוצע',
    mapLabel: 'mAP', speed: 'שניות לאיטרציה', remaining: 'נותר', progress: 'התקדמות', cpuWarning:
      'אימון על המעבד איטי פי ~27 מאשר על ה-Iris Xe (כ-30 שניות לאיטרציה).', evaluate: 'הערכת mAP', iou: 'סף IoU',
  },
  logs: { job: 'משימה', filter: 'סינון', all: 'כל השורות', onlyProblems: 'שגיאות ואזהרות', search: 'חיפוש…', follow: 'מעקב', empty: 'בחרו משימה כדי לראות את היומן שלה' },
  annot: {
    folder: 'תיקיית מאגר', images: 'תמונות', classes: 'מחלקות', help: 'גררו על התמונה כדי לצייר תיבה. לחיצה על תיבה בוחרת אותה; Delete מוחק. מקשים 1–9 בוחרים מחלקה, ←/→ מחליפים תמונה, Ctrl+S שומר.',
    unsaved: 'שינויים שלא נשמרו', noFolder: 'לא נמצאו תמונות בתיקיית המאגרים', labeled: 'מתויגות', boxes: 'תיבות', delete: 'מחיקת תיבה',
  },
  settings: {
    paths: 'נתיבים', defaults: 'ברירות מחדל', appearance: 'מראה', language: 'שפה', theme: 'ערכת נושא',
    system: 'מערכת', light: 'בהיר', dark: 'כהה', cache: 'מטמון מודלים של OpenVINO (הפעלה של 0.12 ש׳ במקום 12.6 ש׳)',
    darknet_cpu_bin: 'Darknet מעבד (bin)', darknet_sycl_bin: 'Darknet SYCL (bin)', oneapi_bin: 'oneAPI (bin)',
    openvino_bin: 'OpenVINO (bin)', openvino_tbb_bin: 'OpenVINO TBB (bin)', ov_bench: 'קובץ ov-bench',
    compare_script: 'סקריפט השוואה', models_dir: 'תיקיית מודלים', datasets_dir: 'תיקיית מאגרים', web_dir: 'תיקיית ממשק',
    readOnly: 'קבוע עבור שרת זה', root: 'תיקיית DarkStudio', work_dir: 'תיקיית עבודה',
  },
}

export const languages = [
  { code: 'en', label: 'English', dir: 'ltr' as const },
  { code: 'he', label: 'עברית', dir: 'rtl' as const },
]

let stored: string | null = null
try {
  stored = localStorage.getItem('darkstudio.language')
} catch {
  // storage unavailable (private mode): fall back to the server setting or English
}
const saved = stored ?? 'en'

/** True when this browser has its own language choice (otherwise the server's setting is used). */
export const hasLocalLanguage = stored !== null

void i18n.use(initReactI18next).init({
  resources: { en: { translation: en }, he: { translation: he } },
  lng: saved,
  fallbackLng: 'en',
  interpolation: { escapeValue: false },
})

export function setLanguage(code: string) {
  void i18n.changeLanguage(code)
  try {
    localStorage.setItem('darkstudio.language', code)
  } catch {
    // ignore
  }
  const dir = languages.find((l) => l.code === code)?.dir ?? 'ltr'
  document.documentElement.lang = code
  document.documentElement.dir = dir
}

export default i18n
