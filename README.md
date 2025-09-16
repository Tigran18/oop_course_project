# OOP Course Project: Multi-PPTX CLI Slideshow

This project is a **C++ command-line application** that simulates a slideshow for multiple presentations defined in text files. Each "presentation" file contains shapes positioned on slides, separated by `---`. The application demonstrates object-oriented programming principles, including classes for shapes, slides, slideshows, tokenizing input, and command parsing.

It supports loading multiple files, navigating between slides and presentations, and displaying shape information via console output. Built with C++11+ features, it uses standard libraries for file I/O, string manipulation, and containers.

---

## **Project Structure**

```
oop_course_project/
│
├─ .vscode/                   # VSCode configuration files (optional)
├─ build/                     # Build output folder
├─ include/
│   ├─ Shape.hpp              # Shape class declaration
│   ├─ Slide.hpp              # Slide class declaration
│   ├─ SlideShow.hpp          # SlideShow class declaration
│   ├─ Tokenizer.hpp          # Tokenizer class declaration
│   └─ CommandParser.hpp      # CommandParser class declaration
├─ src/
│   ├─ Shape.cpp              # Shape implementation
│   ├─ Slide.cpp              # Slide implementation
│   ├─ SlideShow.cpp          # SlideShow implementation
│   ├─ Tokenizer.cpp          # Tokenizer implementation
│   ├─ CommandParser.cpp      # CommandParser implementation
│   └─ main.cpp               # Main CLI program
├─ CMakeLists.txt             # Build configuration (assumed for CMake build)
└─ README.md                  # Project description
```

---

## **Features**

- **Load Multiple Presentations**: Accepts multiple text files as command-line arguments, parsing them into slides with shapes.
- **Slide Navigation**:
  - `next`: Move to the next slide in the current presentation.
  - `prev`: Move to the previous slide in the current presentation.
  - `show`: Display the current slide's shapes.
  - `goto <n>`: Jump to slide number `n` (1-based) in the current presentation.
  - `goto <filename> <n>`: Jump to slide number `n` in the specified presentation (supports filenames with or without `./` prefix).
- **Presentation Navigation**:
  - `nextfile`: Switch to the next loaded presentation and show its current slide.
  - `prevfile`: Switch to the previous loaded presentation and show its current slide.
- **Help and Exit**:
  - `help`: Display a list of available commands.
  - `exit`: Exit the program.
- **Input File Format**: Text files with lines like `ShapeName, x, y` for shapes on a slide, separated by `---` for new slides. Example:
  ```
  Circle, 10, 20
  Square, 30, 40
  ---
  Triangle, 50, 60
  Rectangle, 70, 80
  ```
- **Error Handling**: Graceful handling of invalid commands, slide numbers, filenames, and empty presentations.
- **Prompt**: Dynamic CLI prompt showing current presentation filename and slide position (e.g., `[.\pp1.txt, Slide 1/2] > `).

---

## **Usage**

### **Build**

Use CMake to build the project:

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

This generates `SlideShow.exe` (or `SlideShow` on Unix-like systems) in `build/Release`.

---

### **Run**

Run the executable with one or more text files as arguments:

```bash
.\Release\SlideShow.exe .\pp1.txt .\pp2.txt .\pp3.txt
```

- The program loads the files and enters an interactive CLI mode.
- Type commands at the prompt and press Enter.

### **Commands**


| Command              | Description |
|----------------------|-------------|
| `next`               | Move to the next slide in the current presentation. |
| `prev`               | Move to the previous slide in the current presentation. |
| `show`               | Display the current slide's shapes (e.g., "Drawing shape: Circle at (10, 20)"). |
| `nextfile`           | Switch to the next presentation and show its current slide. |
| `prevfile`           | Switch to the previous presentation and show its current slide. |
| `goto <filename> <n>`| Jump to slide number `n` (1-based) in the specified presentation (e.g., `goto pp1.txt 2` or `goto .\pp1.txt 2`). |
| `goto <n>`           | Jump to slide number `n` (1-based) in the current presentation. |
| `help`               | Show the list of available commands. |
| `exit`               | Exit the slideshow. |

> Notes:
> - Commands are case-insensitive.
> - Invalid commands or arguments show error messages (e.g., unknown command, invalid slide number).
> - If a filename is not found in `goto`, it lists available presentations for reference.

---

## **Development Notes**

- **Classes and Components**:
  - `Shape`: Represents a shape with name and position (x, y).
  - `Slide`: Contains a vector of shapes and methods to add/show them.
  - `SlideShow`: Manages slides for a single file, with navigation methods (next, prev, gotoSlide).
  - `Tokenizer`: Splits lines by delimiter (e.g., comma) for parsing shape data, with trimming.
  - `CommandParser`: Tokenizes and parses CLI input, validates commands, and normalizes paths.
- **Dependencies**: Standard C++ libraries (`<iostream>`, `<fstream>`, `<string>`, `<vector>`, `<map>`, `<algorithm>`, `<cctype>`, `<sstream>`).
- **Build System**: CMake for cross-platform compilation. Assumes source files in `src/` and headers in `include/`.
- **Error Logging**: Uses console output with prefixes like `[INFO]`, `[WARN]`, `[ERR]` for messages.
- **Path Normalization**: Filenames are normalized (lowercase, `/` separators, strip `./` prefix) for case-insensitive matching.

---

## **Future Improvements**

- **Real PPTX Support**: Integrate a library like OpenXML or libpptx to parse actual `.pptx` files.
- **Advanced Commands**: Add `search` for shapes, `edit` to modify slides interactively, or `save` to export changes.
- **GUI Integration**: Extend to a graphical interface using SFML, Qt, or SDL for visual rendering of shapes.
- **Shape Enhancements**: Add more properties (e.g., color, size) and rendering options.
- **Performance**: Optimize for large files with lazy loading of slides.
- **Testing**: Add unit tests using Google Test or Catch2 for classes like Tokenizer and CommandParser.

---

## **Author**

Tigran Davtyan  
Bachelor's student at NPUA, OOP course project
