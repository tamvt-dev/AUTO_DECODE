# Auto Decoder Pro Beta 2.0

Auto Decoder Pro Beta 2.0 is a Windows-focused Qt desktop application for decoding layered or ambiguous encoded text. It combines classic decoders, plugin-based transforms, scoring, retry logic, and a smart multi-stage pipeline to recover the most likely readable result.

## Status

This repository is currently centered on the Qt desktop build.

Current beta highlights:
- Qt Widgets desktop UI
- Dark and light themes with embedded QSS resources
- Smart pipeline for multi-layer decoding
- Built-in support for Base64, Hex, Binary, and Morse
- Plugin-based transforms such as ROT13, Caesar, Atbash, URL, XOR, and Scramble
- Inno Setup installer script for the Qt release build

## Smart Pipeline

Beta 2.0 introduces a staged decoding pipeline instead of a simple one-pass transform search.

Pipeline flow:

```text
Input
-> Fast Heuristic Engine
-> AI Strategy Planner
-> Multi-Pipeline Executor
-> Scoring Engine
-> Auto Retry + Mutation
-> Best Result
```

What that means in practice:
- The heuristic engine quickly detects structured signals such as binary, hex, base64, morse, and URL-like content.
- The strategy planner adjusts depth, beam width, and candidate priority before execution.
- The executor explores multiple decode paths in parallel.
- The scoring engine ranks candidates using readability plus confidence weighting.
- Retry and mutation logic tries normalized variants such as trimmed whitespace and normalized hex casing.
- The pipeline now prefers structured decoding layers before exploratory transforms.

## Project Layout

```text
auto_decoder/
├── core/          Core decoding logic, scoring, plugins, pipeline
├── Qt/            Qt desktop application
├── installer/     Inno Setup installer files
├── output/        Installer output folder
├── portable/      Portable distribution notes/assets
└── README.md
```

## Requirements

Windows build environment expected by this project:
- Qt 6 Widgets
- MinGW toolchain
- MSYS2 UCRT64 environment
- GLib 2.x

The `.pro` file is currently configured around the local MSYS2 UCRT64 layout.

## Build

### Qt Desktop App

From the `Qt` directory:

```bat
qmake auto_decoder_qt.pro
mingw32-make release
```

Expected output:

```text
Qt\release\auto_decoder_qt.exe
```

## Installer

The installer script for the Qt build is:

```text
installer\setup.iss
```

It is configured to package:
- `Qt\release\auto_decoder_qt.exe`
- Qt runtime DLLs from `Qt\release`
- Qt plugin folders such as `platforms`, `styles`, `imageformats`, and related runtime directories
- application icons from `Qt\icons`

Default installer output:

```text
output\AutoDecoderPro_Qt_Setup.exe
```

To build the installer, open `installer\setup.iss` in Inno Setup Compiler and compile it after the Qt release build is ready.

## Usage

Launch the Qt app, then use one of the tabs:
- `Decode`: direct decoding with optional format selection
- `Encode`: direct encoding
- `Pipeline`: layered decode search using the smart pipeline
- `Settings`: theme and behavior preferences
- `History`: recall previous operations

The pipeline view is the main Beta 2.0 feature. It shows:
- final score
- selected route
- best decoded result

## Notes For Beta 2.0

- This beta is optimized for nested and mixed encoded strings.
- Pipeline behavior is heuristic-driven and still evolving.
- Some routes may still require tuning for highly adversarial or intentionally misleading inputs.
- The current repository state is focused on the Qt application rather than a polished CLI release.

## License

This project includes an MIT license in the repository root:

```text
LICENSE
```
