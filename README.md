# FastDisplay — Native Windows Display Monitoring API for Java

**Lightweight native Windows display monitoring for Java applications.**

[![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![Java](https://img.shields.io/badge/Java-17+-blue.svg)](https://www.java.com)
[![Platform](https://img.shields.io/badge/Platform-Windows%2010+-lightgrey.svg)]()
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

FastDisplay provides **real-time Windows display monitoring** for Java applications: **DPI, Resolution, Refresh Rate, Orientation** — all native via JNI.

```java
// Quick Start — Display Monitoring with FastDisplay
import fastdisplay.FastDisplay;

public class DisplayMonitor {
    public static void main(String[] args) throws Exception {
        FastDisplay display = new FastDisplay();
        
        display.setListener(new FastDisplay.DisplayListener() {
            @Override
            public void onInitialState(int width, int height, int dpi, int refreshRate,
                                       FastDisplay.Orientation orientation) {
                int scalePercent = (dpi * 100) / 96;
                System.out.println("INIT: " + width + "x" + height + 
                                 " | Scale: " + scalePercent + "% (DPI: " + dpi + ")" +
                                 " | " + refreshRate + "Hz" +
                                 " | " + orientation);
            }
            
            @Override
            public void onResolutionChanged(int monitorIndex, int width, int height, int dpi, int refreshRate) {
                int scalePercent = (dpi * 100) / 96;
                System.out.println("EVENT: Resolution changed to " + width + "x" + height + 
                                 " | Scale: " + scalePercent + "% (DPI: " + dpi + ")" +
                                 " | " + refreshRate + "Hz");
            }
            
            @Override
            public void onDPIChanged(int monitorIndex, int dpi, int scalePercent) {
                System.out.println("EVENT: DPI changed to " + dpi + " (" + scalePercent + "%)");
            }
            
            @Override
            public void onOrientationChanged(int monitorIndex, FastDisplay.Orientation orientation) {
                System.out.println("EVENT: Orientation changed to " + orientation);
            }
        });

        if (display.startMonitoring()) {
            System.out.println("Monitoring started...");
            System.in.read(); // Press Enter to stop
            display.stopMonitoring();
        }
    }
}
```

FastDisplay is a **minimal, native, fast** library that provides:

- **📊 Display Monitoring** — Real-time resolution, DPI, orientation, refresh rate changes
- **⚡ Zero Dependencies** — Java 17+ and Windows only
- **🎯 Event-Driven** — Instant callbacks via Windows messages


---

## Table of Contents

- [Installation](#installation)
- [Key Features](#key-features)
- [Quick Start](#quick-start)
- [API Reference](#api-reference)
- [Use Cases](#use-cases)
- [Platform Support](#platform-support)
- [Roadmap](#roadmap)
- [License](#license)

---

## Installation

### Maven (JitPack)

```xml
<repositories>
    <repository>
        <id>jitpack.io</id>
        <url>https://jitpack.io</url>
    </repository>
</repositories>

<dependency>
    <groupId>com.github.andrestubbe</groupId>
    <artifactId>fastdisplay</artifactId>
    <version>v1.0.0</version>
</dependency>
<dependency>
    <groupId>com.github.andrestubbe</groupId>
    <artifactId>fastcore</artifactId>
    <version>v1.0.0</version>
</dependency>
```

> **Note:** FastCore handles native library loading from the JAR automatically. Both dependencies are required.

### Gradle (JitPack)

```groovy
repositories {
    maven { url 'https://jitpack.io' }
}

dependencies {
    implementation 'com.github.andrestubbe:fastdisplay:v1.0.0'
    implementation 'com.github.andrestubbe:fastcore:v1.0.0'
}
```

---

## Key Features

### 📊 Display Monitoring
- **Real-time detection** — Resolution, DPI, orientation, refresh rate
- **Instant callbacks** — `WM_DISPLAYCHANGE`, `WM_DPICHANGED` events
- **Complete metrics** — Scale %, DPI, orientation, refresh rate

### ⚡ Technical
- **Zero dependencies** — Java 17+ and Windows only
- **Lightweight** — Minimal CPU/memory overhead
- **MIT licensed** — Free for commercial use
- **Thread-safe** — Background thread with proper JNI thread management

---

## Quick Start

```java
import fastdisplay.FastDisplay;

public class Main {
    public static void main(String[] args) throws Exception {
        FastDisplay display = new FastDisplay();
        
        display.setListener(new FastDisplay.DisplayListener() {
            @Override
            public void onInitialState(int width, int height, int dpi, int refreshRate,
                                       FastDisplay.Orientation orientation) {
                int scalePercent = (dpi * 100) / 96;
                System.out.println("INIT: " + width + "x" + height + 
                                 " | Scale: " + scalePercent + "% (DPI: " + dpi + ")" +
                                 " | " + refreshRate + "Hz" +
                                 " | " + orientation);
            }
            
            @Override
            public void onResolutionChanged(int monitorIndex, int width, int height, int dpi, int refreshRate) {
                int scalePercent = (dpi * 100) / 96;
                System.out.println("EVENT: Resolution changed to " + width + "x" + height + 
                                 " | Scale: " + scalePercent + "% (DPI: " + dpi + ")" +
                                 " | " + refreshRate + "Hz");
            }
            
            @Override
            public void onDPIChanged(int monitorIndex, int dpi, int scalePercent) {
                System.out.println("EVENT: DPI changed to " + dpi + " (" + scalePercent + "%)");
            }
            
            @Override
            public void onOrientationChanged(int monitorIndex, FastDisplay.Orientation orientation) {
                System.out.println("EVENT: Orientation changed to " + orientation);
            }
        });

        if (display.startMonitoring()) {
            System.out.println("Monitoring started...");
            System.in.read(); // Press Enter to stop
            display.stopMonitoring();
        }
    }
}
```

---

## API Reference

### Display Monitoring

| Method | Description |
|--------|-------------|
| `void setListener(DisplayListener listener)` | Set the event listener for display changes |
| `boolean startMonitoring()` | Start monitoring (creates background thread) |
| `void stopMonitoring()` | Stop monitoring and release resources |

### DisplayListener Interface

| Method | Description |
|--------|-------------|
| `void onInitialState(int w, int h, int dpi, int refresh, Orientation o)` | Called once on startup with initial display state |
| `void onResolutionChanged(int monitorIndex, int w, int h, int dpi, int refresh)` | Resolution or refresh rate changed |
| `void onDPIChanged(int monitorIndex, int dpi, int scalePercent)` | DPI scaling changed |
| `void onOrientationChanged(int monitorIndex, Orientation o)` | Display orientation changed |

### Orientation Enum

| Value | Description |
|-------|-------------|
| `LANDSCAPE` | Normal landscape (0°) |
| `PORTRAIT` | Portrait (90° clockwise) |
| `LANDSCAPE_FLIPPED` | Flipped landscape (180°) |
| `PORTRAIT_FLIPPED` | Flipped portrait (270°) |

**Native APIs Used:**
- `WM_DISPLAYCHANGE` — Resolution changes
- `WM_DPICHANGED` — DPI scaling changes (when supported)
- `GetDpiForMonitor` — Per-monitor DPI detection
- `EnumDisplaySettings` — Resolution, refresh rate, orientation
- `EnumDisplayMonitors` — Monitor enumeration and detection
- DPI polling timer — Backup mechanism for DPI detection

### Architecture & Polling Strategy

FastDisplay uses a **hybrid event-driven architecture** to minimize polling overhead:

**Monitor Detection:**
- Monitors are detected using `EnumDisplayMonitors` which provides HMONITOR handles
- Each monitor's properties (resolution, DPI, refresh rate, orientation) are queried once during enumeration
- Monitor handles are cached for efficient lookups during event processing

**Event-Driven Primary Path:**
- Windows messages (`WM_DISPLAYCHANGE`, `WM_DPICHANGED`) trigger immediate callbacks
- No polling for resolution, refresh rate, or orientation changes
- Zero CPU overhead when display state is stable

**DPI Polling Fallback:**
- A lightweight 500ms polling timer runs only as a backup for DPI changes
- This compensates for situations where `WM_DPICHANGED` doesn't fire (some Windows configurations)
- Future versions aim to eliminate this polling entirely through improved event handling

The goal is **zero polling** where possible, with minimal fallback only for edge cases.

---

## Use Cases

- **Adaptive UI Applications** — Automatically adjust layouts when DPI or resolution changes
- **Multi-Monitor Apps** — Detect display configuration changes
- **Tablet/Convertible Apps** — Handle orientation switches (landscape ↔ portrait)
- **Gaming Overlays** — Match refresh rate for smooth rendering
- **System Monitoring Tools** — Track display configuration over time

---

## Platform Support

| Platform | Status | Notes |
|----------|--------|-------|
| Windows 10/11 | ✅ Supported | Full feature set via JNI |
| Linux | 🚧 Planned | X11/Wayland implementation |
| macOS | 🚧 Planned | macOS display API implementation |

---

## Roadmap

### v1.1 (Future)
- [ ] **Multi-Monitor Support** — Per-display settings detection
- [ ] **HDR Detection** — Detect HDR-capable displays
- [ ] **Color Profile** — Read display color profiles
- [ ] **Window Monitor Tracking** — Track which monitor a window is on

### v2.0 (Future)
- [ ] **Linux Support** — X11/Wayland display monitoring
- [ ] **macOS Support** — macOS display configuration monitoring

---

## License

MIT License — See [LICENSE](LICENSE) file for details.

---

## Related Projects

- [FastTheme](https://github.com/andrestubbe/FastTheme) — Window Styling (Titlebar Colors, Dark Mode)
- [FastCore](https://github.com/andrestubbe/FastCore) — Native Library Loader for Java

---

**Made with ⚡ by Andre Stubbe**

