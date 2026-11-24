# PrintStation

Panel de control de impresora simulado — C++17 / Qt 6 / QML, con arquitectura MVVM y multithread.

## Requisitos (macOS)

```bash
brew install qt cmake ninja
```

En VS Code instalá las extensiones recomendadas (`.vscode/extensions.json`).

## Build y ejecución

```bash
cmake --preset debug
cmake --build --preset debug
open build/debug/PrintStation.app   # o ./build/debug/PrintStation.app/Contents/MacOS/PrintStation para ver los logs
```

O desde VS Code: `CMake: Configure` → `CMake: Build` → `CMake: Debug/Launch`.

## Estructura

```
src/core/          Lógica pura C++ (sin QML): PrintEngine, estados
src/viewmodels/    ViewModels expuestos a QML (QML_ELEMENT)
qml/               Vistas
docs/guia/         Notas de arquitectura
```
