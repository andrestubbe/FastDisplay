# FastDisplay - Roadmap

## Core Features (v1.0 - Released)

### 1) Display Metrics (Hardware Layer)
- [x] **DPI / Scaling** - Per-Monitor DPI awareness (Windows 8.1+)
- [x] **Resolution** - Screen width/height in pixels
- [x] **Refresh Rate (Hz)** - Display frequency
- [x] **Orientation** - Landscape/Portrait/Flipped

**This is the core - hardware-level display information.**

### 2) Display Events (Real-Time)
- [x] **WM_DISPLAYCHANGE** - Resolution/Refresh/Orientation changes
- [x] **WM_DPICHANGED** - DPI scaling changes (when supported)
- [x] **DPI Polling** - Backup mechanism for DPI detection

**Apps must be able to react to display changes.**

### 3) DisplayListener API
```java
FastDisplay display = new FastDisplay();
display.setListener(new DisplayListener() {
    @Override
    public void onInitialState(int width, int height, int dpi, int refreshRate, Orientation orientation) {}
    
    @Override
    public void onResolutionChanged(int monitorIndex, int width, int height, int dpi, int refreshRate) {}
    
    @Override
    public void onDPIChanged(int monitorIndex, int dpi, int scalePercent) {}
    
    @Override
    public void onOrientationChanged(int monitorIndex, Orientation orientation) {}
});
display.startMonitoring();
```

**High-level API for developers.**

---

## Future Enhancements (v1.1+)

### 4) Multi-Monitor Support
- [ ] **Monitor Detection** - Which monitors are connected?
- [ ] **Window-Monitor Mapping** - Which monitor is the window on?
- [ ] **Per-Monitor Metrics** - Each display has its own DPI/Hz/Resolution
- [ ] **Monitor Hot-Plug Events** - Monitor connected/disconnected

**Essential for modern multi-monitor setups.**

### 5) Monitor Information
- [ ] **Monitor Name** - "DELL U2720Q"
- [ ] **Monitor Handle** - HMONITOR for native APIs
- [ ] **Work Area** - Desktop area (excluding taskbar)
- [ ] **Primary/Secondary** - Detect primary monitor

**Useful for window positioning.**

### 6) Advanced Display Features
- [ ] **HDR Detection** - HDR10, Dolby Vision
- [ ] **Color Profile (ICC)** - Color space information
- [ ] **Bit Depth** - 8-bit, 10-bit, 12-bit
- [ ] **G-Sync / FreeSync** - Adaptive Sync status

**For professional image editing/gaming.**

---

## Architecture

```
┌─────────────────────────────────────────┐
│           FASTDISPLAY                   │
│      Display & Monitor System API        │
├─────────────────────────────────────────┤
│                                         │
│  Hardware Layer:                        │
│   • DPI / PPI                          │
│   • Resolution (px)                    │
│   • Refresh Rate (Hz)                  │
│   • Orientation                        │
│   • Bit Depth / HDR                    │
│                                         │
│  Monitor Layer:                         │
│   • Multi-Monitor Detection            │
│   • Window-to-Monitor Mapping          │
│   • Per-Monitor Metrics                │
│   • Hot-Plug Events                    │
│                                         │
│  Event Layer:                           │
│   • Display Changed                    │
│   • DPI Changed                        │
│   • Monitor Changed                    │
│   • Display Connected/Disconnected     │
│                                         │
└─────────────────────────────────────────┘
```

---

## Completed Tasks (v1.0)

- [x] Monitoring code from FastTheme
- [x] DPI change detection (WM_DPICHANGED + polling)
- [x] DisplayListener interface
- [x] README.md
- [x] Examples (ConsoleDemo)
- [x] Javadoc documentation
- [x] Doxygen comments
- [x] JitPack configuration
- [x] GitHub release

---

## Integration with FastTheme

```java
// FastDisplay provides display info
FastDisplay display = new FastDisplay();
int dpi = display.getDPIForWindow(frame);

// FastTheme uses display info for scaling
FastTheme.setTitleBarColor(hwnd, r, g, b);
// FastTheme reacts to theme changes (not display)
FastTheme.onThemeChanged(dark -> updateUI());
```

**Both modules are independent but can work together.**

---

## Design Principles

1. **Hardware-Native** - Direct Windows API usage
2. **Zero-Overhead** - Only what's needed, no bloat
3. **Event-Driven** - Real-time updates via listener
4. **Per-Monitor** - First-class multi-monitor support
5. **Thread-Safe** - All APIs are thread-safe

---

## Goal

**FastDisplay = The display API for Java that didn't exist before.**

Precise, hardware-native, real-time, multi-monitor-aware.
