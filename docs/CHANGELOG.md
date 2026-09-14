# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.1] - 2026-09-14

### Changed
- Standardized release versioning to `0.1.1` across all modules and build descriptors.
- Synchronized documentation, installation guides, and showcase media links.
- Overhauled README with comprehensive API reference, architecture benchmarks, and FastJava alignment.

### Fixed
- Corrected version discrepancy in POM and demo configurations.

## [0.1.0] - 2026-05-17

### Added
- Initial release of FastDisplay.
- Real-time native display monitoring via Win32 message-only window (`WM_DISPLAYCHANGE`, `WM_DPICHANGED`).
- Multi-monitor enumeration with resolution, DPI, orientation, and refresh rate.
- Hardware-level EDID parsing, DXGI HDR detection, and ICC color profile extraction.
- Virtual desktop querying and switching integration.
