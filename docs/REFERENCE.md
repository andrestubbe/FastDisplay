# FastDisplay API Reference Manual

`FastDisplay` is the native Windows display telemetry and event monitoring substrate of the FastJava ecosystem.

---

## 1. Class: `fastdisplay.FastDisplay`

Primary engine for display monitoring and hardware queries.

### Lifecycle & Monitoring

- `public boolean startMonitoring()`  
  Starts a dedicated background Win32 message-only window thread intercepting `WM_DISPLAYCHANGE`, `WM_DPICHANGED`, and color management messages. Fires `DisplayListener.onInitialState(...)` immediately upon launch.
  
- `public void stopMonitoring()`  
  Stops the background message pump and cleans up native resources. Safe to call multiple times (idempotent).

- `public void setListener(DisplayListener listener)`  
  Registers a listener interface for real-time display resolution, DPI, orientation, and color profile change notifications.

### Hardware & Topology Queries

- `public MonitorInfo[] enumerateMonitors()`  
  Enumerates all currently active displays on the desktop grid, populating resolution, DPI scaling, refresh rates, orientation, DXGI HDR status, ICC color profiles, and raw EDID blocks.

- `public int[] getResolution()`  
  Returns primary display resolution as a 2-element array `[width, height]`.

- `public int getScale()`  
  Returns the primary display scaling percentage (e.g. `100`, `125`, `150`, `200`).

- `public Orientation getOrientation()`  
  Returns primary monitor orientation (`LANDSCAPE`, `PORTRAIT`, `LANDSCAPE_FLIPPED`, `PORTRAIT_FLIPPED`).

- `public boolean isHdrEnabled(int monitorIndex)`  
  Direct DXGI query testing whether high dynamic range (HDR) rendering is currently enabled for the given screen.

- `public String getColorProfileForMonitor(int monitorIndex)`  
  Queries Windows Color System (WCS) and returns the absolute path or identifier of the active `.icc` / `.icm` color profile.

- `public byte[] getEdidForMonitor(int monitorIndex)`  
  Extracts the raw 128 or 256-byte binary EDID block directly from hardware registry entries for deep device inspection.

- `public boolean setBrightness(int monitorIndex, int percent)`  
  Configures monitor hardware backlight brightness (0-100%) via DDC/CI (where supported by monitor and GPU driver).

---

## 2. Class: `fastdisplay.FastDesktop`

Integration with the Windows 10/11 Virtual Desktop Manager.

- `public DesktopInfo[] enumerateDesktops()`  
  Returns an array of all currently open virtual desktops with their GUIDs, names, and current active status.

- `public String getCurrentDesktopId()`  
  Returns the GUID string of the currently active virtual desktop.

- `public boolean switchDesktop(String desktopId)`  
  Programmatically switches the workspace view to the specified virtual desktop GUID.

- `public boolean moveWindowToDesktop(long hwnd, String desktopId)`  
  Moves an OS native window handle (`HWND`) to the target virtual desktop.

- `public void setListener(DesktopListener l)`  
  Subscribes to virtual desktop creation, deletion, switch, and window migration events.

---

## 3. Class: `fastdisplay.FastDisplayUtils`

High-speed, zero-allocation EDID binary parsers and diagnostic tools.

- `public static String parseManufacturer(byte[] edid)`  
  Decodes the 3-letter PNP vendor identification code (e.g. `DEL`, `SAM`, `LG`) from EDID offsets 8–9.

- `public static String parseModelName(byte[] edid)`  
  Extracts the monitor model descriptor ASCII string from the Detailed Timing / Descriptor blocks (offsets 54–125).

- `public static String parseSerialNumber(byte[] edid)`  
  Decodes the physical serial number string from the EDID descriptor blocks.

- `public static double parseSizeInInches(byte[] edid)`  
  Calculates the screen's physical diagonal size in inches from horizontal and vertical dimensions in centimeters.

- `public static int parseNativeWidth(byte[] edid)` / `parseNativeHeight(byte[] edid)`  
  Extracts the preferred native panel timing resolution directly from Detailed Timing Descriptor #1.

- `public static Map<String, Boolean> parseHdrCapabilities(byte[] edid)`  
  Parses CTA-861 extension blocks for static HDR metadata (SMPTE ST 2084 / PQ, Hybrid Log-Gamma HLG).

- `public static Map<String, Object> parseIccHeader(byte[] icc)`  
  Parses ICC color profile headers for signature, device class, color space, and version.

- `public static String formatMonitorReport(MonitorInfo m, byte[] edid, boolean dxgiHdr, String iccPath)`  
  Generates a cleanly formatted terminal report summarizing resolution, DPI scaling, EDID metadata, HDR status, and color profile.

---

**Part of the FastJava Ecosystem** — *Making the JVM faster.* 🚀