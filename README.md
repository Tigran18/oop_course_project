Sure! Based on your project structure and functionality (CLI multi-PPTX slideshow with slide navigation), here’s a clear and professional `README.md` you can use:

```markdown
# OOP Course Project: Multi-PPTX CLI Slideshow

This project is a **C++ command-line application** that allows you to open and navigate through multiple simulated PPTX presentations.  
It is written in C++17 and demonstrates object-oriented programming concepts, file handling, and CLI interface design.

---

## **Project Structure**

```

oop\_course\_project/
│
├─ .vscode/                   # VSCode configuration files
├─ build/                     # Build output folder
├─ include/
│   ├─ presentation.hpp       # SlideShow struct declaration
│   ├─ slide.hpp              # Optional Slide struct
│   ├─ shape.hpp              # Optional Shape struct
│   └─ Tokenizer.hpp          # Tokenizer class
├─ src/
│   ├─ main.cpp               # CLI program
│   └─ Tokenizer.cpp          # Tokenizer implementation
├─ CMakeLists.txt             # Build configuration
└─ README.md                  # Project description

````

---

## **Features**

- Open multiple PPTX files (simulated as text slides for now)  
- Navigate slides:
  - `next` → go to next slide
  - `prev` → go to previous slide
  - `show` → show current slide  
- Navigate presentations:
  - `nextfile` → go to next presentation
  - `prevfile` → go to previous presentation  
- Exit program with `exit` command  
- Handles cases when no presentations are loaded

---

## **Usage**

### **Build**

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
````

This generates `SlideShows.exe` in `build/Release`.

---

### **Run**

```bash
# Open one or more presentations
./SlideShows.exe slides1.pptx slides2.pptx
```

### **Commands**
________________________________________________
| Command    | Description                     |
| ---------- | ------------------------------- |
| `next`     | Move to next slide              |
| ---------------------------------------------|
| `prev`     | Move to previous slide          |
|----------------------------------------------|
| `show`     | Show current slide              |
|----------------------------------------------|
| `nextfile` | Switch to next presentation     |
|----------------------------------------------|
| `prevfile` | Switch to previous presentation |
|----------------------------------------------|
| `exit`     | Exit the program                |
‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾

> Note: Slides are currently simulated with placeholder text: `"filename - Slide N"`.

---

## **Development Notes**

* The `SlideShow` struct is declared in `include/presentation.hpp`.
* `main.cpp` handles CLI logic and uses `SlideShow` objects.
* `Tokenizer` is prepared for parsing slide content (future feature).
* The project uses **CMake** for cross-platform building.

---

## **Future Improvements**

* Support actual `.pptx` file parsing
* Add slide content and shapes (`slide.hpp` + `shape.hpp`)
* Implement text formatting and images
* GUI support using Qt or similar library

---

## **Author**

Tigran Davtyan
Bachelor's student at NPUA, OOP course project
