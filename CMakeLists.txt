cmake_minimum_required(VERSION 3.20)
project(SlideShow LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# libzip via pkg-config (Linux-safe)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBZIP REQUIRED libzip)

# =========================
# Core library (logic)
# =========================
add_library(core
    src/CommandParser.cpp
    src/Commands.cpp
    src/Controller.cpp
    src/ICommand.cpp
    src/PPTXSerializer.cpp
    src/Shape.cpp
    src/Slide.cpp
    src/SlideShow.cpp
    src/Token.cpp
    src/Tokenizer.cpp
    src/lodepng.cpp
)

target_include_directories(core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${LIBZIP_INCLUDE_DIRS}
)

target_link_libraries(core PUBLIC
    ${LIBZIP_LIBRARIES}
)

# =========================
# CLI executable
# =========================
add_executable(SlideShowCLI
    src/main.cpp
)
target_link_libraries(SlideShowCLI PRIVATE core)

# =========================
# GUI executable
# =========================
add_executable(SlideShowGUI
    qt_main.cpp

    gui/MainWindow.cpp
    gui/CanvasView.cpp
    gui/SlideList.cpp
    gui/SettingsDialog.cpp
    gui/QtLogStream.cpp

    # Add headers too (helps AUTOMOC detect Q_OBJECT reliably)
    gui/MainWindow.hpp
    gui/CanvasView.hpp
    gui/SlideList.hpp
    gui/SettingsDialog.hpp
    gui/QtLogStream.hpp
)

target_include_directories(SlideShowGUI PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/gui
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(SlideShowGUI PRIVATE
    core
    Qt6::Widgets
)
