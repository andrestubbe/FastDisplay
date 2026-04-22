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
            public void onResolutionChanged(int width, int height, int dpi, int refreshRate) {
                int scalePercent = (dpi * 100) / 96;
                System.out.println("EVENT: Resolution changed to " + width + "x" + height + 
                                 " | Scale: " + scalePercent + "% (DPI: " + dpi + ")" +
                                 " | " + refreshRate + "Hz");
            }
            
            @Override
            public void onDPIChanged(int dpi, int scalePercent) {
                System.out.println("EVENT: DPI changed to " + dpi + " (" + scalePercent + "%)");
            }
            
            @Override
            public void onOrientationChanged(FastDisplay.Orientation orientation) {
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
    <groupId>io.github.andrestubbe</groupId>
    <artifactId>fastdisplay</artifactId>
    <version>v1.0.0</version>
</dependency>
<dependency>
    <groupId>io.github.andrestubbe</groupId>
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
    implementation 'io.github.andrestubbe:fastdisplay:v1.0.0'
    implementation 'io.github.andrestubbe:fastcore:v1.0.0'
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
- **Thread-safe** — Background thread calls listener on main thread

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
            public void onResolutionChanged(int width, int height, int dpi, int refreshRate) {
                int scalePercent = (dpi * 100) / 96;
                String orientation = (width > height) ? "LANDSCAPE" : "PORTRAIT";
                System.out.println("EVENT: Resolution changed to " + width + "x" + height + 
                                 " | Scale: " + scalePercent + "% (DPI: " + dpi + ")" +
                                 " | " + refreshRate + "Hz" +
                                 " | " + orientation);
            }
            
            @Override
            public void onDPIChanged(int dpi, int scalePercent) {
                System.out.println("EVENT: DPI changed to " + dpi + " (" + scalePercent + "%)");
            }
            
            @Override
            public void onOrientationChanged(FastDisplay.Orientation orientation) {
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
| `void onInitialState(int w, int h, int dpi, int refresh, Orientation o)` | Called once on startup |
| `void onResolutionChanged(int w, int h, int dpi, int refresh)` | Resolution or DPI changed |
| `void onDPIChanged(int dpi, int scalePercent)` | DPI scaling changed |
| `void onOrientationChanged(Orientation o)` | Display orientation changed |

### Orientation Enum

| Value | Description |
|-------|-------------|
| `LANDSCAPE` | Normal landscape (0°) |
| `PORTRAIT` | Portrait (90° clockwise) |
| `LANDSCAPE_FLIPPED` | Flipped landscape (180°) |
| `PORTRAIT_FLIPPED` | Flipped portrait (270°) |

**Native APIs Used:**
- `WM_DISPLAYCHANGE` — Resolution changes
- `WM_DPICHANGED` — DPI scaling changes
- `GetDpiForMonitor` — Per-monitor DPI detection
- `EnumDisplaySettings` — Resolution, refresh rate, orientation

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

- [ ] **Multi-Monitor Support** — Per-display settings detection
- [ ] **HDR Detection** — Detect HDR-capable displays
- [ ] **Color Profile** — Read display color profiles
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
