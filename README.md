# SlideShow (CLI + Qt GUI) — OOP Course Project

A small presentation editor/viewer written in **C++20** with:

- **`SlideShowGUI`** — a Qt6 GUI where you can add slides, add shapes, drag them around, edit properties, and save/load `.pptx`.
- **`SlideShowCLI`** — a command-driven interface that exposes the same core logic.

The project is structured so that **core logic is independent from the UI**:

```
SlideShowCLI  ─┐
              ├──>  core (library)  <── PPTXSerializer (OpenXML + libzip)
SlideShowGUI  ─┘
```

---

## Features

### Editing
- Create slides
- Add **Text**, **Rectangle**, **Ellipse**, and **Image** shapes
- Drag shapes with the mouse (GUI)
- Edit shape properties (X/Y/W/H/Text) from the Properties panel
- Undo/Redo (GUI + CLI)

### PPTX support
- Save presentations to **`.pptx`** (OpenXML)
- Load `.pptx` created by this program
- Importing slides with images works (images are scaled using the PPTX geometry)

> Note: This is *not* a full PowerPoint implementation. It writes/reads a practical subset of the OpenXML format.

---

## Repository structure

```
.
├── CMakeLists.txt
├── include/                 # public headers (core API)
├── src/                     # core implementation
├── gui/                     # Qt GUI sources
├── qt_main.cpp              # GUI entry point
└── README.md
```

### Key components

- **Model objects**
  - `SlideShow` → `Slide` → `Shape`
  - `ShapeKind`: `Text`, `Rect`, `Ellipse`, `Image`

- **Controller**
  - Holds the loaded presentations
  - Manages current slideshow/slide index
  - Stores Undo/Redo snapshots

- **Command parser (CLI + GUI command bar)**
  - Converts text commands (e.g. `next`, `goto 3`, `save out.pptx`, `undo`) into actions on the controller/model

- **PPTXSerializer**
  - Writes OpenXML parts into a `.pptx` using **libzip**
  - Reads the same subset back

---

## Dependencies

### Ubuntu / Debian
```bash
sudo apt update
sudo apt install \
  build-essential cmake pkg-config \
  qt6-base-dev qt6-base-dev-tools qt6-tools-dev \
  libzip-dev
```

---

## Build

```bash
mkdir -p build
cd build
cmake ..
cmake --build . -j
```

Outputs:
- `build/SlideShowGUI`
- `build/SlideShowCLI`

---

## Run

### GUI
```bash
./build/SlideShowGUI
```

### CLI
```bash
./build/SlideShowCLI
```

---

## GUI workflow

1. **New Slide** → adds a slide
2. **Rectangle / Ellipse / Text / Image** → inserts a shape
3. Drag a shape with the mouse to move it
4. Select a shape → edit X/Y/W/H/Text in the Properties panel → **Apply**
5. Use the command bar (bottom) for commands like `save out.pptx`, `open file.pptx`, `undo`, `redo`, etc.

---

## CLI commands (common)

- `new` — create a new empty presentation
- `newslide` — add a slide
- `rect ...`, `ellipse ...`, `text ...`, `image ...` — add shapes
- `next`, `prev`, `goto N` — navigate slides
- `open file.pptx` — load pptx
- `save out.pptx` — save pptx
- `undo`, `redo`
- `help`

(Exact parsing is implemented in `CommandParser`.)

---

## Notes

### Why there is a `build-asan/` directory
`ASAN` usually means **AddressSanitizer** — a special build configuration that detects memory bugs (use-after-free, buffer overflows, etc.).

A typical ASan build looks like:
```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer"
cmake --build build-asan -j
```

You normally **do not commit** `build/` or `build-asan/` folders to git.

---

## License / Author

OOP course project.
