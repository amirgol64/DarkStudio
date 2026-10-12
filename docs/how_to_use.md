# How to use DarkStudio

This guide starts from a fresh clone and ends with a model trained on your own images. It covers what the app can do today (UI v0 plus the training setup). The same help is also in the app: a **?** next to each training option, and a **How to use this page** panel on every page.

- [1. Install and start](#1-install-and-start)
- [2. Check your machine (Devices)](#2-check-your-machine-devices)
- [3. Try it out: benchmarks and the LEGO Gears training demo](#3-try-it-out-benchmarks-and-the-lego-gears-training-demo)
- [4. Train on your own dataset](#4-train-on-your-own-dataset)
- [5. Follow the run, evaluate it, find the weights](#5-follow-the-run-evaluate-it-find-the-weights)
- [6. Logs, settings and troubleshooting](#6-logs-settings-and-troubleshooting)

## 1. Install and start

1. Get the code with its submodules:

   ```sh
   git clone --recurse-submodules https://github.com/amirgol64/DarkStudio.git
   ```

2. Build the engines once. Follow these guides in order:
   - [build-windows.md](build-windows.md): vcpkg, Darknet for CPU (`build/install/`), DarkHelp.
   - [intel-gpu.md](intel-gpu.md): OpenVINO and `ov-bench` (Intel GPU inference), then oneAPI and the SYCL build of Darknet (`build/install-sycl/`, Intel GPU training).
   - Drogon (the server's web framework) and Node.js, as listed in [darkstudio-ui.md](darkstudio-ui.md#run-it).

   You don't need every backend. With only the CPU build, the app still works, and the Intel GPU options show as unavailable.

3. Start the app from the repository folder:

   ```bat
   darkstudio.bat
   ```

   The first run builds the server and the web UI. After that it starts in a few seconds and opens **http://localhost:8765/**. Use `darkstudio.bat --rebuild` after pulling new code. The server only listens on your own machine.

4. To stop it, close the server's console window.

## 2. Check your machine (Devices)

Open **Devices** first. It shows:

- CPU and RAM.
- Each backend (**Darknet CPU**, **Darknet SYCL** on the Intel GPU, **OpenVINO**) with its version and a green or red status. When one is red, the probe output underneath says why, usually a wrong path or a missing DLL folder.
- A list of the files the jobs need. Any missing file is marked.

Fix any wrong path in **Settings → Paths**, save, and press **Detect again** on the Devices page.

## 3. Try it out: benchmarks and the LEGO Gears training demo

**Benchmarks.** Run the OpenVINO benchmark (pick GPU or CPU and FP16 or FP32) to see inference speed and annotated sample images. Run **Darknet CPU vs SYCL** to check that the Intel GPU build gives the same detections as the CPU build.

**Training demo.** This is the quickest way to see a full training run before preparing your own data:

1. Go to **Training → Prepare LEGO Gears** and press **Start**. It downloads the small LEGO Gears dataset (CC BY-NC-SA, for local testing only) and writes the training files to `build/train-test/legogears/`.
2. In **New training run**, choose the dataset folder `legogears/LegoGears_v2`, the model **YOLOv4-tiny**, a few hundred iterations and **Intel GPU (SYCL)**, then press **Start**. (Alternatively, use **Advanced → Use my own .data and .cfg files** with empty paths to train on the files written in step 1.)
3. Watch the loss chart drop. With **Calculate mAP during training** turned on, mAP points appear as training goes on.

## 4. Train on your own dataset

Darknet trains from **images with YOLO label files**, a **`.data` file** that says where they are, and a **`.cfg` file** that describes the network. It can also start from **pretrained weights**, which makes training much faster and better on small datasets. In **Normal** mode, DarkStudio writes the `.data` and `.cfg` files and prepares the pretrained weights for you. You only provide labeled images and the class names.

Every field on the Training page has a small **?** next to it. Click it for a short explanation and a link to the matching part of this guide. Each page also has a **How to use this page** panel at the top that you can fold away.

### 4.1 Put your images in `datasets/`

Create a folder for the project, for example `datasets/fruit/`, and copy your images into it (`.jpg`, `.png` or `.bmp`; sub-folders are fine). The Annotation page only sees folders inside `datasets/`. The Training page can also train a folder anywhere on your PC (pick it with **Browse…**), but keeping datasets in `datasets/` lets you label them in the app too.

Add a **class names file** in that folder, one class per line, for example `datasets/fruit/fruit.names`:

```
apple
banana
orange
```

The order matters: the first line is class 0. The Training page needs this file. Without it, the Annotation page also shows only a single "class 0". A `.names` file in a parent folder covers all the folders below it.

### 4.2 Label the images (Annotation)

Open **Annotation** and pick your folder.

- Drag on the image to draw a box. The new box gets the active class.
- Click a box to select it. Drag to move it, use the handles to resize it, and press **Delete** to remove it.
- Keys **1–9** choose the class, or click a class in the list on the right. With a box selected, this changes its class.
- **←/→** go to the previous or next image. Changes are saved automatically when you switch images. **Ctrl+S** saves now.

Each image gets a `.txt` file next to it, in Darknet/YOLO format (`class x_center y_center width height`, from 0 to 1). An image with nothing to label still needs to be visited and saved, which leaves an empty `.txt`. Darknet treats it as a "negative" example, and that helps reduce false detections. **Only images that have a `.txt` file are used for training.**

Already have YOLO-format labels from another tool? Copy the images and their `.txt` files into the folder and they show up as labeled.

How much data: aim for at least a few hundred boxes per class, with images that look like what the model will see later (lighting, distance, background).

### 4.3 Start a training run (Normal)

In **Training → New training run**, keep **Normal** selected:

| Field | What to choose |
|---|---|
| **Dataset folder** | Your folder. Type it (for example `fruit`, relative to `datasets/`), pick a suggestion from the list, or press **Browse…** to choose any folder (see [Choosing folders and files](#choosing-folders-and-files)). Below the field, the page shows the number of labeled images (including sub-folders) and the class names it found |
| **Model** | **YOLOv4-tiny** to start. See [4.4](#44-choose-the-model-and-pretrained-weights) |
| **Image size** | The network input size. Every image is resized to it. 416 × 416 is a good default. Smaller is faster, larger finds small objects better |
| **Iterations** | How many training steps. It starts at Darknet's suggestion (2000 per class, at least 6000). For a first test, a few hundred is enough to see whether the loss goes down. The page shows the seconds per iteration and the time left once a run starts |
| **Device** | **Intel GPU (SYCL)**. It is about 27× faster than the CPU on the Iris Xe |
| **Calculate mAP during training** | Leave it on. Darknet then checks the validation images regularly and keeps the best model as `*_best.weights` |

#### Choosing folders and files

Every path on the Training page (the dataset folder, a weights file, and your own `.data` and `.cfg` files) has a **Browse…** button. It opens a window that lists the folders and files on this PC:

- Click a folder to open it. **↑** goes up one folder, and **This PC** lists the drives. You can also type or paste a path and press **Go**.
- The buttons under the path jump to the **Datasets**, **Models**, **Training runs** and **DarkStudio** folders, and to your **Home** folder.
- For a folder: open it and press **Select this folder**. The bottom line shows how many images are directly in the open folder, how many are labeled, and any `.names` file there.
- For a file: only matching files are listed (for example `.cfg`), unless you tick **Show all files**. Click a file and press **Select**, or double-click it.
- The window opens at the field's current path, or else at the last folder you used for that kind of file.

(A web page can't read real paths from Windows' own file dialog, so DarkStudio's server lists the folders instead. It only lists names and never sends file contents.)

Press **Start**. DarkStudio then writes these files into the run's folder (`build/darkstudio/jobs/<run id>/`):

- `train.txt` and `valid.txt`: every labeled image, with every 10th image set aside for validation (mAP).
- `<folder>.names` and `<folder>.data`.
- `<folder>.cfg`: the model's cfg with your image size, iterations, `steps` at 80% and 90%, `classes=` in every `[yolo]` section, and `filters=(classes + 5) × masks` in the layer before each `[yolo]` section.

Before starting, it checks that every class id in the labels exists in the `.names` file. The run page then shows how many training and validation images were used.

Only one job runs at a time, so a benchmark can't slow the training down. Press **Stop** to end a run early. The checkpoints saved so far (`_last.weights`, `_best.weights`) are kept.

### 4.4 Choose the model and pretrained weights

| Model | When to use it | Pretrained weights |
|---|---|---|
| **YOLOv4-tiny** | Fast. The best start on the Iris Xe | COCO, first 29 layers |
| **YOLOv4-tiny-3L** | A third detection head that finds small objects better. A little slower | COCO, first 29 layers (shared with YOLOv4-tiny) |
| **YOLOv7-tiny** | A newer tiny network | None yet: trains from scratch, which needs more data and iterations |

With pretrained weights, your network starts from layers already trained on the 80-class COCO dataset, and only learns your classes on top. DarkStudio makes this "partial" weights file (`models/pretrained/yolov4-tiny.conv.29`) automatically on the first run, with `darknet partial`. It needs the COCO `yolov4-tiny.weights` in `models/pretrained/`. [build-windows.md](build-windows.md) downloads it; model files aren't stored in git. Under the model field, the page shows whether the weights are ready, will be prepared, or are missing (with a download link).

Don't use the full `yolov4-tiny.weights` directly as the start for a different number of classes. The output layers don't match. To make the partial file by hand:

```powershell
build\install\bin\darknet.exe partial darknet\cfg\yolov4-tiny.cfg models\pretrained\yolov4-tiny.weights models\pretrained\yolov4-tiny.conv.29 29
```

### 4.5 Advanced options

Click **Advanced** at the top of the card to see more options. The page remembers your choice. Empty fields keep the model's own default.

| Option | What it does |
|---|---|
| **Width**, **Height** | A non-square network size (multiples of 32). Match your images' shape, for example 416 × 256 for wide video frames |
| **Batch** | Images per training step (64 is the Darknet default) |
| **Subdivisions** | Splits each batch into smaller pieces to save memory. Raise it (2, 4, 8) if training runs out of memory |
| **Learning rate** | The size of each learning step (model default 0.00261). Lower it if the loss becomes `nan` |
| **Validation images (%)** | The share of labeled images kept aside for mAP (default 10%). 0 measures mAP on the training images |
| **Start from pretrained weights** | Turn off to train from scratch |
| **Weights file** | Start from your own weights instead, for example the `_best.weights` of an earlier run with the same model and classes, after you added more images |
| **Augmentation** | Random changes to the training images so the model generalizes: **mosaic** (combines 4 images), **horizontal flip** (turn it off when left and right matter, for example for arrows or text), **hue**, **saturation**, **exposure** and **rotation** |

### 4.6 Use your own `.data` and `.cfg` files

For full control, in **Advanced** tick **Use my own .data and .cfg files** and enter their paths (or pick them with **Browse…**), plus an optional weights file. DarkStudio copies them into the run folder and doesn't change them, except that `backup` points to the run folder. Leave the paths empty to use the LEGO Gears demo files.

To write them by hand:

1. **Image lists.** List one image path per line in `train.txt` (about 90% of the images) and `valid.txt` (the rest). Don't put the same image in both. From PowerShell:

   ```powershell
   $dir    = "datasets\fruit"
   $images = Get-ChildItem $dir -Recurse -Include *.jpg,*.png | Sort-Object FullName | ForEach-Object FullName
   $valid  = $images | Where-Object { ($images.IndexOf($_) % 10) -eq 9 }   # every 10th image
   $train  = $images | Where-Object { $valid -notcontains $_ }
   Set-Content "$dir\train.txt" $train
   Set-Content "$dir\valid.txt" $valid
   ```

2. **`.data` file** (full paths are the safest):

   ```
   classes = 3
   train   = C:\dev\Python_dev\draknet\datasets\fruit\train.txt
   valid   = C:\dev\Python_dev\draknet\datasets\fruit\valid.txt
   names   = C:\dev\Python_dev\draknet\datasets\fruit\fruit.names
   backup  = C:\dev\Python_dev\draknet\datasets\fruit
   ```

3. **`.cfg` file.** Copy `darknet/cfg/yolov4-tiny.cfg` and change these lines:

   | Where | Set to | Example (3 classes) |
   |---|---|---|
   | `[net]` `batch`, `subdivisions` | `64`, `1` (more subdivisions if you run out of memory) | `batch=64` |
   | `[net]` `width`, `height` | Multiples of 32 | `416` × `416` |
   | `[net]` `max_batches` | The number of iterations | `max_batches=6000` |
   | `[net]` `steps` | 80% and 90% of `max_batches` | `steps=4800,5400` |
   | Each `[yolo]` section | `classes=` your class count | `classes=3` |
   | The `[convolutional]` section **just before** each `[yolo]` | `filters=(classes + 5) × 3` | `filters=24` |

## 5. Follow the run, evaluate it, find the weights

- **Chart**: the loss (thin line) and average loss (thick line) should go down and level off. The mAP points (right axis, %) should go up. If the average loss stops falling and mAP stays flat, more iterations won't help, but more or better labels will.
- **Stats**: iteration, loss, last mAP, seconds per iteration, time remaining.
- **Evaluate**: when a run finishes, choose an IoU (0.50 is the usual one, 0.75 is stricter) and press **Evaluate mAP** to compute mAP on the validation list with the best weights.
- **Run history**: every run stays in the list, even after a restart. Click one to see its chart and log.

Each run has its own folder: **`build/darkstudio/jobs/<run id>/`** (shown on the run's page). It holds the `.cfg`, `.data`, `.names` and image lists used, `log.txt`, and the weights Darknet saves:

| File | What it is |
|---|---|
| `<cfg name>_best.weights` | The weights with the highest mAP (only when mAP is calculated during training). **Use this one** |
| `<cfg name>_final.weights` | The weights at the end of training |
| `<cfg name>_last.weights`, `<cfg name>_1000.weights`, … | Checkpoints saved during training |

To use the model elsewhere, take the `_best.weights` together with the run's `.cfg` and your `.names` file. Darknet, DarkHelp, and the ONNX export in [intel-gpu.md](intel-gpu.md) use those same three files.

## 6. Logs, settings and troubleshooting

- **Logs & errors** shows the output of every job live. Errors are red and warnings are yellow, and you can filter to problems only or search. The sidebar badge counts failed jobs.
- **Settings** holds the tool paths, the default OpenVINO device and precision, the OpenVINO model cache (keep it on, so later runs start faster), the theme, and the language (English or Hebrew).

| Problem | What to check |
|---|---|
| A backend is red on **Devices** | The path in **Settings → Paths**. For SYCL, the oneAPI `bin` folder must be right (`C:\Program Files (x86)\Intel\oneAPI\2026.1\bin`) |
| "Another job is running" | Wait for it, or stop it on its page |
| Training doesn't start | The error under the card says why: no `.names` file, no labeled images, a class id in a label that isn't in the `.names` file, or missing COCO weights (turn off pretrained weights in **Advanced**, or download them) |
| Training fails right away | Open its log. With your own files, common causes are a wrong path in the `.data` file, `filters` not equal to `(classes + 5) × 3`, or a weights file from a different cfg |
| Out of memory | Raise `subdivisions` (2, 4, 8) or lower `width`/`height` |
| Loss becomes `nan` | Start from pretrained weights, check that the labels are valid (all values between 0 and 1), and lower the **Learning rate** in **Advanced** |
| mAP stays at 0 | Normal for the first few hundred iterations. If it stays at 0 longer: too few labeled images, or (with your own files) `valid.txt` is empty or the class counts in `.data`, `.cfg` and `.names` don't agree |
| No class names on Annotation | Add a `.names` file to the dataset folder ([4.1](#41-put-your-images-in-datasets)) |

For the API, the architecture and development mode, see [darkstudio-ui.md](darkstudio-ui.md).
