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
    normal: 'Normal', advanced: 'Advanced', mode: 'Options', dataset: 'Dataset folder', choose: 'Choose…',
    datasetInfo: '{{images}} labeled images · {{classes}} classes', noNames: 'no .names file',
    noDatasets: 'No labeled folders in datasets/ yet. Label images on the Annotation page first.',
    model: 'Model', size: 'Image size', width: 'Width', height: 'Height', iterations: 'Iterations',
    suggested: 'Darknet suggests {{n}}', useSuggested: 'Use',
    batch: 'Batch', subdivisions: 'Subdivisions', learningRate: 'Learning rate', validPercent: 'Validation images (%)',
    pretrained: 'Start from pretrained weights (COCO)', customWeights: 'Weights file (optional, replaces the pretrained weights)',
    augmentation: 'Augmentation', mosaic: 'Mosaic', flip: 'Horizontal flip', hue: 'Hue', saturation: 'Saturation',
    exposure: 'Exposure', angle: 'Rotation (degrees)', manual: 'Use my own .data and .cfg files',
    modelReady: 'Pretrained weights ready', modelPrepare: 'Pretrained weights are prepared on the first run',
    modelMissing: 'COCO weights not downloaded:', modelScratch: 'No pretrained weights: trains from scratch and needs more data',
    generated: '{{train}} training / {{valid}} validation images · {{classes}} classes',
    tips: {
      dataset: 'A folder in datasets/ with your images, their YOLO .txt labels and a .names file (one class per line). Only labeled images are used.',
      model: 'The network to train. YOLOv4-tiny is fast and a good start. YOLOv4-tiny-3L finds small objects better. YOLOv7-tiny has no pretrained weights here yet.',
      size: 'The network input size. Images are resized to it. Smaller is faster; larger finds small objects better. Must be a multiple of 32.',
      iterations: 'How many training steps (max_batches). Darknet suggests 2000 per class and at least 6000. Use a few hundred for a quick test.',
      device: 'Intel GPU (SYCL) is about 27× faster than the CPU on the Iris Xe.',
      map: 'Measures accuracy (mAP) on the validation images during training and keeps the best weights as *_best.weights.',
      batch: 'Images per training step. 64 is the Darknet default.',
      subdivisions: 'Splits each batch into smaller pieces to save memory. Raise it (2, 4, 8) if training runs out of memory.',
      learningRate: 'How big each learning step is. Leave it empty to keep the model default; lower it if the loss becomes nan.',
      validPercent: 'Share of the labeled images kept aside to measure mAP. They are not used for training. 0 measures on the training images.',
      pretrained: 'Reuse layers already trained on the COCO dataset. Training is much faster and better on small datasets.',
      customWeights: 'Continue from your own weights, for example _best.weights of an earlier run with the same model and classes.',
      augmentation: 'Random changes Darknet applies to the training images so the model generalizes. Leave a field empty to keep the model default.',
      manual: 'Train from a .data and .cfg file you wrote yourself (see the guide). DarkStudio does not change them.',
    },
  },
  logs: { job: 'Job', filter: 'Filter', all: 'All lines', onlyProblems: 'Errors & warnings', search: 'Search…', follow: 'Follow', empty: 'Select a job to see its log' },
  annot: {
    folder: 'Dataset folder', images: 'Images', classes: 'Classes', help: 'Drag on the image to draw a box. Click a box to select it; Delete removes it. Keys 1–9 pick the class, ←/→ change image, Ctrl+S saves.',
    unsaved: 'Unsaved changes', noFolder: 'No images found in the datasets folder', labeled: 'labeled', boxes: 'boxes', delete: 'Delete box',
  },
  help: {
    what: 'What is this?', more: 'More in the guide →', title: 'How to use this page', guide: 'Open the full guide →',
    pages: {
      devices: [
        'Check that each backend (Darknet CPU, Darknet SYCL on the Intel GPU, OpenVINO) is green.',
        'A red backend shows why underneath: usually a wrong path or a missing DLL folder.',
        'Fix paths in Settings → Paths, save, then press Detect again here.',
      ],
      benchmarks: [
        'OpenVINO benchmark: inference speed of yolov4-tiny on the GPU or CPU, with annotated sample images.',
        'Darknet CPU vs SYCL: checks that the Intel GPU gives the same detections as the CPU, and how much faster it is.',
        'Only one job runs at a time, so results are not distorted by training.',
      ],
      training: [
        'Put your images and a .names file (one class per line) in a folder under datasets/, and label them on the Annotation page.',
        'In New training run, choose that folder and a model. YOLOv4-tiny with pretrained weights is a good start.',
        'Keep Intel GPU (SYCL) and mAP on, set the iterations (a few hundred for a test), and press Start.',
        'Watch the loss drop and mAP rise. When it finishes, press Evaluate mAP. The weights are in the run folder.',
        'Switch to Advanced for batch, learning rate, augmentation, validation split, or your own .data/.cfg files.',
      ],
      logs: [
        'Pick a job to see its full output, live.',
        'Errors are red and warnings yellow. Filter to problems only, or search.',
      ],
      annotation: [
        'Pick a folder from datasets/. Add a .names file there so the classes have names.',
        'Drag to draw a box. Click a box to select, move or resize it. Delete removes it.',
        'Keys 1–9 choose the class. ←/→ change image (saves automatically). Ctrl+S saves now.',
        'Visit and save images with nothing in them too: an empty label teaches the model what is not an object.',
      ],
      settings: [
        'Paths: where DarkStudio finds Darknet, oneAPI, OpenVINO, models and datasets.',
        'Defaults: the OpenVINO device and precision. Keep the model cache on for fast GPU startup.',
        'Press Save. Theme and language apply right away.',
      ],
    },
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
    normal: 'רגיל', advanced: 'מתקדם', mode: 'אפשרויות', dataset: 'תיקיית מאגר', choose: 'בחירה…',
    datasetInfo: '{{images}} תמונות מתויגות · {{classes}} מחלקות', noNames: 'אין קובץ names.',
    noDatasets: 'עדיין אין תיקיות מתויגות ב-datasets/. תייגו תמונות בעמוד התיוג תחילה.',
    model: 'מודל', size: 'גודל תמונה', width: 'רוחב', height: 'גובה', iterations: 'איטרציות',
    suggested: 'Darknet ממליץ על {{n}}', useSuggested: 'שימוש',
    batch: 'אצווה', subdivisions: 'חלוקות משנה', learningRate: 'קצב למידה', validPercent: 'תמונות אימות (%)',
    pretrained: 'התחלה ממשקולות מאומנות מראש (COCO)', customWeights: 'קובץ משקולות (רשות, מחליף את המשקולות המאומנות מראש)',
    augmentation: 'הגברת נתונים', mosaic: 'פסיפס', flip: 'היפוך אופקי', hue: 'גוון', saturation: 'רוויה',
    exposure: 'חשיפה', angle: 'סיבוב (מעלות)', manual: 'שימוש בקובצי data. ו-cfg. שלי',
    modelReady: 'המשקולות המאומנות מראש מוכנות', modelPrepare: 'המשקולות המאומנות מראש יוכנו בהרצה הראשונה',
    modelMissing: 'משקולות COCO לא הורדו:', modelScratch: 'אין משקולות מאומנות מראש: האימון מתחיל מאפס ודורש יותר נתונים',
    generated: '{{train}} תמונות אימון / {{valid}} תמונות אימות · {{classes}} מחלקות',
    tips: {
      dataset: 'תיקייה ב-datasets/ עם התמונות שלכם, קובצי התיוג (txt בפורמט YOLO) וקובץ names (מחלקה בכל שורה). רק תמונות מתויגות משמשות לאימון.',
      model: 'הרשת שתאומן. YOLOv4-tiny מהירה ומתאימה להתחלה. YOLOv4-tiny-3L מוצאת עצמים קטנים טוב יותר. ל-YOLOv7-tiny עדיין אין כאן משקולות מאומנות מראש.',
      size: 'גודל הקלט של הרשת; התמונות מוקטנות אליו. קטן יותר מהיר יותר, גדול יותר מוצא עצמים קטנים טוב יותר. חייב להיות כפולה של 32.',
      iterations: 'כמה צעדי אימון (max_batches). Darknet ממליץ על 2000 לכל מחלקה ולפחות 6000. לבדיקה מהירה מספיקות כמה מאות.',
      device: 'המעבד הגרפי של Intel (SYCL) מהיר פי 27 בערך מהמעבד ב-Iris Xe.',
      map: 'מודד דיוק (mAP) על תמונות האימות במהלך האימון ושומר את המשקולות הטובות ביותר בקובץ best.weights.',
      batch: 'מספר התמונות בכל צעד אימון. ברירת המחדל של Darknet היא 64.',
      subdivisions: 'מחלק כל אצווה לחלקים קטנים יותר כדי לחסוך זיכרון. הגדילו (2, 4, 8) אם נגמר הזיכרון.',
      learningRate: 'גודל כל צעד למידה. השאירו ריק כדי לשמור על ברירת המחדל של המודל; הקטינו אם ההפסד הופך ל-nan.',
      validPercent: 'החלק מהתמונות המתויגות שנשמר בצד למדידת mAP ולא משמש לאימון. 0 מודד על תמונות האימון.',
      pretrained: 'שימוש חוזר בשכבות שכבר אומנו על מאגר COCO. האימון מהיר וטוב הרבה יותר במאגרים קטנים.',
      customWeights: 'המשך מהמשקולות שלכם, למשל best.weights מהרצה קודמת עם אותו מודל ואותן מחלקות.',
      augmentation: 'שינויים אקראיים ש-Darknet מחיל על תמונות האימון כדי שהמודל יכליל. השאירו שדה ריק כדי לשמור על ברירת המחדל.',
      manual: 'אימון מקובצי data ו-cfg שכתבתם בעצמכם (ראו המדריך). DarkStudio לא משנה אותם.',
    },
  },
  logs: { job: 'משימה', filter: 'סינון', all: 'כל השורות', onlyProblems: 'שגיאות ואזהרות', search: 'חיפוש…', follow: 'מעקב', empty: 'בחרו משימה כדי לראות את היומן שלה' },
  annot: {
    folder: 'תיקיית מאגר', images: 'תמונות', classes: 'מחלקות', help: 'גררו על התמונה כדי לצייר תיבה. לחיצה על תיבה בוחרת אותה; Delete מוחק. מקשים 1–9 בוחרים מחלקה, ←/→ מחליפים תמונה, Ctrl+S שומר.',
    unsaved: 'שינויים שלא נשמרו', noFolder: 'לא נמצאו תמונות בתיקיית המאגרים', labeled: 'מתויגות', boxes: 'תיבות', delete: 'מחיקת תיבה',
  },
  help: {
    what: 'מה זה?', more: 'עוד במדריך ←', title: 'איך משתמשים בעמוד הזה', guide: 'פתיחת המדריך המלא ←',
    pages: {
      devices: [
        'בדקו שכל מנוע (Darknet על המעבד, Darknet SYCL על המעבד הגרפי של Intel, OpenVINO) מסומן בירוק.',
        'מתחת למנוע אדום מופיעה הסיבה: בדרך כלל נתיב שגוי או תיקיית DLL חסרה.',
        'תקנו נתיבים בהגדרות ← נתיבים, שמרו ולחצו כאן על "זיהוי מחדש".',
      ],
      benchmarks: [
        'מדד OpenVINO: מהירות ההסקה של yolov4-tiny על המעבד הגרפי או על המעבד, עם תמונות דוגמה מסומנות.',
        'Darknet מעבד מול SYCL: בודק שהמעבד הגרפי של Intel נותן אותן תוצאות כמו המעבד, ובכמה הוא מהיר יותר.',
        'רק משימה אחת רצה בכל פעם, כך שאימון לא משבש את התוצאות.',
      ],
      training: [
        'שימו את התמונות וקובץ names (מחלקה בכל שורה) בתיקייה תחת datasets/, ותייגו אותן בעמוד התיוג.',
        'ב"אימון חדש", בחרו את התיקייה ומודל. YOLOv4-tiny עם משקולות מאומנות מראש היא התחלה טובה.',
        'השאירו את המעבד הגרפי של Intel (SYCL) ואת חישוב ה-mAP, קבעו את מספר האיטרציות (כמה מאות לבדיקה) ולחצו "התחלה".',
        'עקבו אחרי ירידת ההפסד ועליית ה-mAP. בסיום לחצו "הערכת mAP". המשקולות נשמרות בתיקיית ההרצה.',
        'עברו ל"מתקדם" בשביל אצווה, קצב למידה, הגברת נתונים, חלוקת אימות או קובצי data/cfg משלכם.',
      ],
      logs: [
        'בחרו משימה כדי לראות את כל הפלט שלה בזמן אמת.',
        'שגיאות באדום ואזהרות בצהוב. אפשר להציג רק בעיות או לחפש.',
      ],
      annotation: [
        'בחרו תיקייה מ-datasets/. הוסיפו בה קובץ names כדי שלמחלקות יהיו שמות.',
        'גררו כדי לצייר תיבה. לחיצה על תיבה בוחרת אותה, ואז אפשר להזיז אותה או לשנות את גודלה. Delete מוחק.',
        'מקשים 1–9 בוחרים מחלקה. ←/→ מחליפים תמונה (נשמר אוטומטית). Ctrl+S שומר מיד.',
        'בקרו ושמרו גם תמונות בלי עצמים: תיוג ריק מלמד את המודל מה אינו עצם.',
      ],
      settings: [
        'נתיבים: היכן DarkStudio מוצא את Darknet, oneAPI, OpenVINO, המודלים והמאגרים.',
        'ברירות מחדל: ההתקן והדיוק של OpenVINO. השאירו את מטמון המודלים פעיל להפעלה מהירה של המעבד הגרפי.',
        'לחצו "שמירה". ערכת הנושא והשפה מוחלות מיד.',
      ],
    },
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
