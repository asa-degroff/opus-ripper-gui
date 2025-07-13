# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Initial project structure with Qt6/QML
- FLAC decoding using libFLAC++
- Opus encoding using libopus reference implementation
- Dark theme UI with modern design
- Directory picker for input/output selection
- Recursive FLAC file scanning
- Directory structure preservation
- Multi-threaded batch conversion
- Progress tracking with per-file status
- Configurable encoding settings (bitrate, complexity, VBR)
- Basic error handling

### Known Issues
- Output files use a simple format instead of proper Ogg Opus container
- Metadata copying is not yet fully implemented
- Album art preservation not implemented

## [0.1.0] - TBD
- Initial release