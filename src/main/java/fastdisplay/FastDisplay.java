package fastdisplay;

import fastcore.FastCore;

/**
 * FastDisplay – Native Windows Display Monitoring API for Java.
 *
 * <p>Provides real-time monitoring of display changes (resolution, DPI,
 * refresh rate, orientation, and monitor transitions) via JNI.</p>
 *
 * <p>All callbacks occur on the monitoring thread. If UI-thread dispatching
 * is required (Swing/JavaFX), the application must forward events manually.</p>
 *
 * <p><b>Platform:</b> Windows 10/11 (Win32 API)</p>
 *
 * @author Andre
 * @version 1.1.0
 */
public class FastDisplay {
    /** Default constructor. */
    public FastDisplay() {}

    static {
        FastCore.loadLibrary("fastdisplay");
    }

    /** Display orientation values returned by the native API. */
    public enum Orientation {
        /** Standard landscape orientation. */
        LANDSCAPE,
        /** Portrait orientation (90° rotation). */
        PORTRAIT,
        /** Landscape flipped (180° rotation). */
        LANDSCAPE_FLIPPED,
        /** Portrait flipped (270° rotation). */
        PORTRAIT_FLIPPED
    }

    /**
     * Immutable monitor information returned by {@link #enumerateMonitors()}.
     */
    public static final class MonitorInfo {
        /** Monitor index (0-based). */
        public final int index;
        /** Display width in pixels. */
        public final int width;
        /** Display height in pixels. */
        public final int height;
        /** Dots per inch. */
        public final int dpi;
        /** Display orientation. */
        public final Orientation orientation;
        /** Refresh rate in Hz. */
        public final int refreshRate;
        /** Whether HDR is enabled. */
        public final boolean hdrEnabled;
        /** Color profile path. */
        public final String colorProfile;
        /** Manufacturer code. */
        public final String manufacturer;
        /** Model name. */
        public final String modelName;
        /** Serial number. */
        public final String serialNumber;

        /**
         * Constructs a MonitorInfo object.
         * @param index monitor index
         * @param width display width
         * @param height display height
         * @param dpi dots per inch
         * @param orientation display orientation
         * @param refreshRate refresh rate in Hz
         * @param hdrEnabled HDR support
         * @param colorProfile color profile path
         * @param manufacturer manufacturer code
         * @param modelName model name
         * @param serialNumber serial number
         */
        public MonitorInfo(int index, int width, int height, int dpi,
                           Orientation orientation, int refreshRate, boolean hdrEnabled, String colorProfile,
                           String manufacturer, String modelName, String serialNumber) {
            this.index = index;
            this.width = width;
            this.height = height;
            this.dpi = dpi;
            this.orientation = orientation;
            this.refreshRate = refreshRate;
            this.hdrEnabled = hdrEnabled;
            this.colorProfile = colorProfile;
            this.manufacturer = manufacturer;
            this.modelName = modelName;
            this.serialNumber = serialNumber;
        }

        @Override
        public String toString() {
            StringBuilder sb = new StringBuilder();
            int scalePercent = (dpi * 100) / 96;
            sb.append("MONITOR ").append(index).append("\n");
            sb.append("────────────────────────────────────────────────────────\n");
            sb.append(String.format("Resolution : %d × %d @ %d Hz\n", width, height, refreshRate));
            sb.append(String.format("DPI        : %d (%d%%)\n", dpi, scalePercent));
            sb.append("Orientation: ").append(orientation).append("\n");
            if (colorProfile != null && !colorProfile.isEmpty()) {
                sb.append("ColorProf. : ").append(colorProfile).append("\n");
            }
            if (manufacturer != null && !manufacturer.isEmpty()) {
                sb.append("Vendor     : ").append(manufacturer).append("\n");
            }
            if (modelName != null && !modelName.isEmpty()) {
                sb.append("Model      : ").append(modelName).append("\n");
            }
            return sb.toString();
        }
    }

    /**
     * Listener interface for receiving display change events.
     */
    public interface DisplayListener {
        /** Called with initial display state when monitoring starts. */
        void onInitialState(int width, int height, int dpi, int refreshRate, Orientation orientation);
        /** Called when display resolution changes. */
        void onResolutionChanged(int monitorIndex, int width, int height, int dpi, int refreshRate);
        /** Called when DPI scaling changes. */
        void onDPIChanged(int monitorIndex, int dpi, int scalePercent);
        /** Called when display orientation changes. */
        void onOrientationChanged(int monitorIndex, Orientation orientation);
        /** Called when tracked window moves to a different monitor. */
        void onWindowMonitorChanged(int oldMonitorIndex, int newMonitorIndex, int newDPI);
        /** Called when color profile changes. */
        void onColorProfileChanged(int monitorIndex, String colorProfile);
    }

    private DisplayListener listener;
    private long windowHandle = 0;

    // Tracking variables to prevent duplicate events
    private int lastReportedWidth = 0;
    private int lastReportedHeight = 0;
    private int lastReportedDpi = 0;
    private int lastReportedRefreshRate = 0;
    private Orientation lastReportedOrientation = null;

    /** Assign a listener for display events.
     * @param listener the listener to set */
    public void setListener(DisplayListener listener) {
        this.listener = listener;
    }

    /**
     * Set the HWND of the window to track for monitor transitions.
     * Must be called before {@link #startMonitoring()}.
     * @param hwnd the window handle
     */
    public void setWindowHandle(long hwnd) {
        this.windowHandle = hwnd;
        setWindowHandleNative(hwnd);
    }

    // -------------------------
    // Native callbacks → Java
    // -------------------------

    private void notifyInitialState(int w, int h, int dpi, int orientationOrdinal, int refresh) {
        if (listener != null) {
            // Update tracking variables with initial state
            lastReportedWidth = w;
            lastReportedHeight = h;
            lastReportedDpi = dpi;
            lastReportedRefreshRate = refresh;
            lastReportedOrientation = Orientation.values()[orientationOrdinal];
            listener.onInitialState(w, h, dpi, refresh, lastReportedOrientation);
        }
    }

    private void notifyResolutionChanged(int monitorIndex, int w, int h, int dpi, int refresh) {
        if (listener != null) {
            // Only fire if resolution (width/height/refresh) actually changed, not DPI
            if (w != lastReportedWidth || h != lastReportedHeight || refresh != lastReportedRefreshRate) {
                lastReportedWidth = w;
                lastReportedHeight = h;
                lastReportedRefreshRate = refresh;
                listener.onResolutionChanged(monitorIndex, w, h, dpi, refresh);
            }
        }
    }

    private void notifyOrientationChanged(int monitorIndex, int orientationOrdinal) {
        if (listener != null) {
            Orientation newOrientation = Orientation.values()[orientationOrdinal];
            // Only fire if orientation actually changed
            if (newOrientation != lastReportedOrientation) {
                lastReportedOrientation = newOrientation;
                listener.onOrientationChanged(monitorIndex, newOrientation);
            }
        }
    }

    private void notifyDPIChanged(int monitorIndex, int dpi, int scalePercent) {
        if (listener != null) {
            // Only fire if DPI actually changed
            if (dpi != lastReportedDpi) {
                lastReportedDpi = dpi;
                listener.onDPIChanged(monitorIndex, dpi, scalePercent);
            }
        }
    }

    private void notifyWindowMonitorChanged(int oldIndex, int newIndex, int newDPI) {
        if (listener != null) {
            listener.onWindowMonitorChanged(oldIndex, newIndex, newDPI);
        }
    }

    private void notifyColorProfileChanged(int monitorIndex, String colorProfile) {
        if (listener != null) {
            listener.onColorProfileChanged(monitorIndex, colorProfile);
        }
    }

    // -------------------------
    // Native API
    // -------------------------

    /** Start monitoring display changes.
     * @return true if monitoring started successfully */
    public native boolean startMonitoring();

    /** Stop monitoring display changes. */
    public native void stopMonitoring();

    /** Pass window handle to native layer. */
    private native void setWindowHandleNative(long hwnd);

    /** Enumerate all monitors.
     * @return array of monitor information */
    public native MonitorInfo[] enumerateMonitors();

    /** Get DPI for a specific monitor.
     * @param monitorIndex the monitor index
     * @return DPI value, or -1 if invalid */
    public native int getMonitorDPI(int monitorIndex);

    /** Get primary display resolution.
     * @return array with [width, height] */
    public native int[] getResolution();

    /** Get primary display scale factor (100, 125, 150...).
     * @return scale percentage */
    public native int getScale();

    /** Get primary display orientation.
     * @return current orientation */
    public native Orientation getOrientation();

    /** Get the monitor index where the monitoring window is located.
     * @return monitor index, or -1 if not monitoring */
    public native int getCurrentMonitorIndex();
}
