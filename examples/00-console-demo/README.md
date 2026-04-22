# FastTheme Console Demo

Console monitoring example demonstrating FastTheme's display and theme detection capabilities.

## What it shows

- Real-time display monitoring (resolution, DPI, refresh rate)
- Windows theme detection (Dark/Light mode)
- Console-based event notifications

## Run it

```bash
mvn compile exec:java
```

## Expected Output

```
=== FastTheme Demo ===

Starting monitoring... (Press Ctrl+C to stop)
Initial State:
  Display: 2880x1920 @ 192DPI
  Refresh: 120Hz
  Theme: DARK
```

Try changing your display settings or Windows theme to see real-time events!
