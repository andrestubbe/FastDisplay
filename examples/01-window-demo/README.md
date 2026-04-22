# FastTheme Window Demo

Window styling example demonstrating FastTheme's black titlebar and transparency features.

## What it shows

- **Black titlebar** with white text (the FastJava standard)
- **80% window transparency** for seamless look
- Real-time system information display
- Terminal-style UI with Cascadia Mono font

## Run it

```bash
mvn compile exec:java
```

## Features Demonstrated

- `FastTheme.getWindowHandle()` - Extract native window handle
- `FastTheme.setTitleBarColor(0, 0, 0)` - Black titlebar
- `FastTheme.setTitleBarTextColor(255, 255, 255)` - White text
- `FastTheme.setWindowTransparency(hwnd, 204)` - 80% opacity
- `FastTheme.setTitleBarDarkMode(hwnd, true)` - Dark mode

## Screenshot

Window with black titlebar, white text, and terminal-style content area.
