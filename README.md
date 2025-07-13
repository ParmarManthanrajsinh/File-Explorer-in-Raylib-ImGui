# File Explorer

A modern file explorer application built with C++ using Raylib and Dear ImGui frameworks.

## Features

- Clean and intuitive graphical user interface
- File system navigation with directory browsing
- File size display with automatic unit conversion (B, KB, MB, GB, TB)
- Directory/file differentiation with clear indicators
- Resizable window support
- Menu bar with File, Edit, and Help options
- Side explorer panel with hierarchical navigation

## ScreenShots

![screenshot](assets/screenshot.png)

## Dependencies

- [raylib](https://www.raylib.com/) - A simple and easy-to-use game development library
- [Dear ImGui](https://github.com/ocornut/imgui) - Immediate mode GUI library
- [rlImGui](https://github.com/raylib-extras/rlImGui) - ImGui backend for Raylib

## Building

1. Make sure you have CMake installed
2. Clone this repository
3. Build the project:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Usage

After building, run the executable to launch the File Explorer. You can:

- Use the File menu to open new directories
- Navigate through directories using the side panel
- Click on directories to enter them
- Use ".." to go up one directory level
- View file sizes in human-readable format

## Contributing

Feel free to open issues and pull requests to help improve this project.