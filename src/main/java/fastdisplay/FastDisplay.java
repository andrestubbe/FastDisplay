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

    static {
        FastCore.loadLibrary("fastdisplay");
    }

    /** Display orientation values returned by the native API. */
    public enum Orientation {
        LANDSCAPE,
        PORTRAIT,
        LANDSCAPE_FLIPPED,
        PORTRAIT_FLIPPED
    }

    /**
     * Immutable monitor information returned by {@link #enumerateMonitors()}.
     */
    public static final class MonitorInfo {
        public final int index;
        public final int width;
        public final int height;
        public final int dpi;
        public final Orientation orientation;
        public final int refreshRate;
        public final boolean hdrEnabled;
        public final String colorProfile;
        public final String manufacturer;
        public final String modelName;
        public final String serialNumber;

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
            return "Monitor " + index + ": " + width + "x" + height +
                   " @ " + refreshRate + "Hz, " + dpi + " DPI, " + orientation +
                   (hdrEnabled ? ", HDR" : "") +
                   (colorProfile != null && !colorProfile.isEmpty() ? ", ColorProfile=" + colorProfile : "") +
                   (manufacturer != null && !manufacturer.isEmpty() ? ", Manufacturer=" + manufacturer : "") +
                   (modelName != null && !modelName.isEmpty() ? ", Model=" + modelName : "");
        }
    }

    /**
     * Listener interface for receiving display change events.
     */
    public interface DisplayListener {
        void onInitialState(int width, int height, int dpi, int refreshRate, Orientation orientation);
        void onResolutionChanged(int width, int height, int dpi, int refreshRate);
        void onDPIChanged(int dpi, int scalePercent);
        void onOrientationChanged(Orientation orientation);
        void onWindowMonitorChanged(int oldMonitorIndex, int newMonitorIndex, int newDPI);
    }

    private DisplayListener listener;
    private long windowHandle = 0;

    /** Assign a listener for display events. */
    public void setListener(DisplayListener listener) {
        this.listener = listener;
    }

    /**
     * Set the HWND of the window to track for monitor transitions.
     * Must be called before {@link #startMonitoring()}.
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
            listener.onInitialState(w, h, dpi, refresh, Orientation.values()[orientationOrdinal]);
        }
    }

    private void notifyResolutionChanged(int w, int h, int dpi, int refresh) {
        if (listener != null) {
            listener.onResolutionChanged(w, h, dpi, refresh);
        }
    }

    private void notifyOrientationChanged(int orientationOrdinal) {
        if (listener != null) {
            listener.onOrientationChanged(Orientation.values()[orientationOrdinal]);
        }
    }

    private void notifyDPIChanged(int dpi, int scalePercent) {
        if (listener != null) {
            listener.onDPIChanged(dpi, scalePercent);
        }
    }

    private void notifyWindowMonitorChanged(int oldIndex, int newIndex, int newDPI) {
        if (listener != null) {
            listener.onWindowMonitorChanged(oldIndex, newIndex, newDPI);
        }
    }

    // -------------------------
    // Native API
    // -------------------------

    /** Start monitoring display changes. */
    public native boolean startMonitoring();

    /** Stop monitoring display changes. */
    public native void stopMonitoring();

    /** Pass window handle to native layer. */
    private native void setWindowHandleNative(long hwnd);

    /** Enumerate all monitors. */
    public native MonitorInfo[] enumerateMonitors();

    /** Get DPI for a specific monitor. */
    public native int getMonitorDPI(int monitorIndex);

    /** Get primary display resolution. */
    public native int[] getResolution();

    /** Get primary display scale factor (100, 125, 150...). */
    public native int getScale();

    /** Get primary display orientation. */
    public native Orientation getOrientation();
}
