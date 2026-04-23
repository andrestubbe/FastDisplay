package fastdisplay;

import fastcore.FastCore;

/**
 * FastDisplay – Native Windows Display Monitoring API for Java.
 *
 * <p>Provides real-time monitoring of display changes (resolution, DPI,
 * refresh rate, orientation) via JNI.</p>
 *
 * <p>All callbacks occur on the monitoring thread. If UI-thread dispatching
 * is required (Swing/JavaFX), the application must forward events manually.</p>
 *
 * <p><b>Platform:</b> Windows 10/11 (Win32 API)</p>
 *
 * <p><b>Usage Example:</b></p>
 * <pre>{@code
 * FastDisplay display = new FastDisplay();
 * display.setListener(new FastDisplay.DisplayListener() {
 *     @Override
 *     public void onInitialState(int width, int height, int dpi, int refreshRate, Orientation orientation) {
 *         System.out.println("Initial: " + width + "x" + height + " @ " + dpi + " DPI");
 *     }
 *     
 *     @Override
 *     public void onResolutionChanged(int monitorIndex, int width, int height, int dpi, int refreshRate) {
 *         System.out.println("Resolution changed: " + width + "x" + height);
 *     }
 *     
 *     @Override
 *     public void onDPIChanged(int monitorIndex, int dpi, int scalePercent) {
 *         System.out.println("DPI changed: " + dpi + " (" + scalePercent + "%)");
 *     }
 *     
 *     @Override
 *     public void onOrientationChanged(int monitorIndex, Orientation orientation) {
 *         System.out.println("Orientation changed: " + orientation);
 *     }
 * });
 * 
 * if (display.startMonitoring()) {
 *     // Monitor until stopped
 *     display.stopMonitoring();
 * }
 * }</pre>
 *
 * @author Andre Stubbe
 * @version 1.0.0
 */
public class FastDisplay {
    /** Default constructor. Initializes the FastDisplay instance. */
    public FastDisplay() {}

    static {
        FastCore.loadLibrary("fastdisplay");
    }

    /** Display orientation values returned by the native API.
     * 
     * <p>These values correspond to the Windows DM_DISPLAYORIENTATION constants:</p>
     * <ul>
     * <li>LANDSCAPE - 0° rotation (default)</li>
     * <li>PORTRAIT - 90° clockwise rotation</li>
     * <li>LANDSCAPE_FLIPPED - 180° rotation</li>
     * <li>PORTRAIT_FLIPPED - 270° clockwise rotation</li>
     * </ul>
     */
    public enum Orientation {
        /** Standard landscape orientation (0°). */
        LANDSCAPE,
        /** Portrait orientation (90° clockwise rotation). */
        PORTRAIT,
        /** Landscape flipped (180° rotation). */
        LANDSCAPE_FLIPPED,
        /** Portrait flipped (270° clockwise rotation). */
        PORTRAIT_FLIPPED
    }

    /**
     * Immutable monitor information returned by {@code enumerateMonitors()}.
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

        /**
         * Constructs a MonitorInfo object.
         * @param index monitor index
         * @param width display width
         * @param height display height
         * @param dpi dots per inch
         * @param orientation display orientation
         * @param refreshRate refresh rate in Hz
         */
        public MonitorInfo(int index, int width, int height, int dpi,
                           Orientation orientation, int refreshRate) {
            this.index = index;
            this.width = width;
            this.height = height;
            this.dpi = dpi;
            this.orientation = orientation;
            this.refreshRate = refreshRate;
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
            return sb.toString();
        }
    }

    /**
     * Listener interface for receiving display change events.
     * 
     * <p>All listener methods are called on the monitoring thread. If you need to update
     * UI components (Swing/JavaFX), you must dispatch the event to the UI thread manually.</p>
     * 
     * <p>Event filtering is applied to prevent duplicate events - the same value will not
     * be reported twice in succession.</p>
     */
    public interface DisplayListener {
        /** Called once with initial display state when monitoring starts.
         * 
         * @param width display width in pixels
         * @param height display height in pixels
         * @param dpi dots per inch (96 = 100% scale)
         * @param refreshRate refresh rate in Hz
         * @param orientation current display orientation
         */
        void onInitialState(int width, int height, int dpi, int refreshRate, Orientation orientation);
        
        /** Called when display resolution changes.
         * 
         * <p>This event fires when the actual pixel resolution or refresh rate changes.
         * DPI-only changes are reported via {@link #onDPIChanged}.</p>
         * 
         * @param monitorIndex monitor index (0-based)
         * @param width new display width in pixels
         * @param height new display height in pixels
         * @param dpi current DPI value
         * @param refreshRate new refresh rate in Hz
         */
        void onResolutionChanged(int monitorIndex, int width, int height, int dpi, int refreshRate);
        
        /** Called when DPI scaling changes.
         * 
         * <p>This event fires when the user changes the display scale in Windows settings
         * (e.g., from 100% to 150%). The DPI value changes, but the pixel resolution
         * remains the same.</p>
         * 
         * @param monitorIndex monitor index (0-based)
         * @param dpi new DPI value (96 = 100% scale)
         * @param scalePercent scale percentage (100, 125, 150, 175, 200, etc.)
         */
        void onDPIChanged(int monitorIndex, int dpi, int scalePercent);
        
        /** Called when display orientation changes.
         * 
         * <p>This event fires when the display is rotated (e.g., on a tablet or
         * when using a monitor with rotation support).</p>
         * 
         * @param monitorIndex monitor index (0-based)
         * @param orientation new display orientation
         */
        void onOrientationChanged(int monitorIndex, Orientation orientation);
    }

    // ============================
    // Instance Fields
    // ============================
    
    /** The registered listener for display events. */
    private DisplayListener listener;

    // ============================
    // Event Filtering
    // ============================
    
    /** Tracking variables to prevent duplicate events. */
    private int lastReportedWidth = 0;
    private int lastReportedHeight = 0;
    private int lastReportedDpi = 0;
    private int lastReportedRefreshRate = 0;
    private Orientation lastReportedOrientation = null;

    // ============================
    // Public API
    // ============================
    
    /** Assign a listener for display events.
     * 
     * @param listener the listener to set, or null to remove the current listener
     */
    public void setListener(DisplayListener listener) {
        this.listener = listener;
    }

    // ============================
    // Native Callback Handlers
    // ============================
    
    /** Called from native code with initial display state. */
    private void notifyInitialState(int w, int h, int dpi, int orientationOrdinal, int refresh) {
        if (listener != null) {
            lastReportedWidth = w;
            lastReportedHeight = h;
            lastReportedDpi = dpi;
            lastReportedRefreshRate = refresh;
            lastReportedOrientation = Orientation.values()[orientationOrdinal];
            listener.onInitialState(w, h, dpi, refresh, lastReportedOrientation);
        }
    }

    /** Called from native code when resolution changes. */
    private void notifyResolutionChanged(int monitorIndex, int w, int h, int dpi, int refresh) {
        if (listener != null) {
            if (w != lastReportedWidth || h != lastReportedHeight || refresh != lastReportedRefreshRate) {
                lastReportedWidth = w;
                lastReportedHeight = h;
                lastReportedRefreshRate = refresh;
                listener.onResolutionChanged(monitorIndex, w, h, dpi, refresh);
            }
        }
    }

    /** Called from native code when orientation changes. */
    private void notifyOrientationChanged(int monitorIndex, int orientationOrdinal) {
        if (listener != null) {
            Orientation newOrientation = Orientation.values()[orientationOrdinal];
            if (newOrientation != lastReportedOrientation) {
                lastReportedOrientation = newOrientation;
                listener.onOrientationChanged(monitorIndex, newOrientation);
            }
        }
    }

    /** Called from native code when DPI changes. */
    private void notifyDPIChanged(int monitorIndex, int dpi, int scalePercent) {
        if (listener != null) {
            if (dpi != lastReportedDpi) {
                lastReportedDpi = dpi;
                listener.onDPIChanged(monitorIndex, dpi, scalePercent);
            }
        }
    }

    // ============================
    // Native Method Declarations
    // ============================
    
    /** Start monitoring display changes.
     * 
     * <p>Creates a background thread that listens for Windows display change events.
     * The listener's {@link DisplayListener#onInitialState} method will be called
     * immediately with the current display state.</p>
     * 
     * @return true if monitoring started successfully, false if already monitoring or failed
     */
    public native boolean startMonitoring();

    /** Stop monitoring display changes.
     * 
     * <p>Stops the background monitoring thread and releases native resources.
     * This method is idempotent - calling it multiple times is safe.</p>
     */
    public native void stopMonitoring();
}
