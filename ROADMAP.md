# FastDisplay - Roadmap & TODO

## Was DEFINITIV zu FastDisplay gehört

### 1) Display Metrics (Hardware-Layer)
- **DPI / Scaling** - Per-Monitor DPI awareness (Windows 8.1+)
- **Resolution** - Screen width/height in pixels
- **Refresh Rate (Hz)** - Display frequency
- **Orientation** - Landscape/Portrait/Flipped

**Das ist der Kern - Hardware-nahe Display-Informationen.**

### 2) Multi-Monitor Support
- **Monitor Detection** - Welche Monitore sind angeschlossen?
- **Window-Monitor Mapping** - Auf welchem Monitor ist das Fenster?
- **Per-Monitor Metrics** - Jedes Display hat eigene DPI/Hz/Resolution
- **Monitor Hot-Plug Events** - Monitor angeschlossen/getrennt

**Das ist essentiell für moderne Multi-Monitor-Setups.**

### 3) Display Events (Real-Time)
- **WM_DISPLAYCHANGE** - Resolution/Refresh/Orientation Änderungen
- **WM_DPICHANGED** - DPI Scaling Änderungen (per Monitor)
- **Monitor Connect/Disconnect** - Hardware Events

**Apps müssen auf Display-Änderungen reagieren können.**

### 4) DisplayListener API
```java
FastDisplay display = new FastDisplay();
display.setListener(new DisplayListener() {
    @Override
    public void onDisplayChanged(int width, int height, int dpi, int hz) {}
    
    @Override
    public void onDPIChanged(int dpi, int scalePercent) {}
    
    @Override
    public void onMonitorChanged(Monitor monitor) {}
});
display.startMonitoring();
```

**High-Level API für Entwickler.**

### 5) Monitor Information
- **Monitor Name** - "DELL U2720Q"
- **Monitor Handle** - HMONITOR für native APIs
- **Work Area** - Desktop-Bereich (ohne Taskbar)
- **Primary/Secondary** - Hauptmonitor erkennen

**Nützlich für Fenster-Positionierung.**

---

## Optionale Erweiterungen

### 6) Advanced Display Features
- **HDR Detection** - HDR10, Dolby Vision
- **Color Profile (ICC)** - Farbraum-Informationen
- **Bit Depth** - 8-bit, 10-bit, 12-bit
- **G-Sync / FreeSync** - Adaptive Sync Status

**Für professionelle Bildbearbeitung/Gaming.**

### 7) Virtual Display Support
- **Simulierte Monitore** - Für Remote/Headless
- **Display Spoofing** - Testing ohne Hardware
- **Dummy Plug Detection** - Headless-GPU-Steuerung

**Entwickler- und Server-Szenarien.**

### 8) Display Configuration
- **Resolution ändern** - Programmatisch umstellen
- **DPI Scaling setzen** - System-Scaling beeinflussen
- **Primary Monitor wechseln** - Hauptdisplay setzen

**Advanced-Use-Case, nicht primary focus.**

---

## Was NICHT zu FastDisplay gehört

| Feature | Gehört zu | Warum |
|---------|-----------|-------|
| Titlebar Coloring | FastTheme | UI-Styling |
| Dark/Light Mode | FastTheme | Theme-Einstellung |
| Accent Color | FastTheme | System-Theme |
| Window Transparency | FastTheme | UI-Effekt |
| Mica/Acrylic | FastTheme | Material-Design |
| Window Buttons | FastTheme | Chrome-Styling |

**FastDisplay bleibt Hardware/Display-Layer.**
**FastTheme bleibt UI/Theme-Layer.**

---

## Architektur

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

┌─────────────────────────────────────────┐
│           FASTTHEME v2.0                 │
│      UI Theme & Window Chrome            │
├─────────────────────────────────────────┤
│                                         │
│  Theme Layer:                           │
│   • Dark/Light Mode                    │
│   • Accent Color                       │
│   • System Colors                      │
│                                         │
│  Chrome Layer:                          │
│   • Titlebar Coloring                  │
│   • Titlebar Text                      │
│   • Border Styling                     │
│   • Rounded Corners                    │
│                                         │
│  Material Layer:                        │
│   • Mica / Acrylic                     │
│   • Transparency                       │
│   • Blur Effects                       │
│                                         │
└─────────────────────────────────────────┘
```

---

## Aktuelle Aufgaben (v1.0 Initial)

- [x] Monitoring-Code aus FastTheme übernommen
- [x] DPI Change Detection (WM_DPICHANGED)
- [ ] Multi-Monitor Support
- [ ] Monitor-Handle API
- [ ] Window-to-Monitor Mapping
- [ ] Monitor Connect/Disconnect Events
- [ ] Per-Monitor DPI Awareness
- [ ] DisplayListener Interface
- [ ] README.md erstellen
- [ ] Examples erstellen (ConsoleDemo)
- [ ] Push zu GitHub
- [ ] JitPack Release v1.0

---

## Zusammenarbeit mit FastTheme

```java
// FastDisplay liefert Display-Info
FastDisplay display = new FastDisplay();
int dpi = display.getDPIForWindow(frame);

// FastTheme nutzt Display-Info für scaling
FastTheme.setTitleBarColor(hwnd, r, g, b);
// FastTheme reagiert auf Theme-Änderungen (nicht Display)
FastTheme.onThemeChanged(dark -> updateUI());
```

**Beide Module sind unabhängig, können aber zusammenarbeiten.**

---

## Design-Prinzipien

1. **Hardware-Nähe** - Direkte Windows-API-Nutzung
2. **Zero-Overhead** - Nur was nötig ist, kein Bloat
3. **Event-Driven** - Echtzeit-Updates via Listener
4. **Per-Monitor** - Erstklassige Multi-Monitor-Unterstützung
5. **Thread-Safe** - Alle APIs thread-sicher

---

## Ziel

**FastDisplay = Die Display-API für Java, die es bisher nicht gab.**

Präzise, hardware-nah, echtzeitfähig, multi-monitor-aware.

