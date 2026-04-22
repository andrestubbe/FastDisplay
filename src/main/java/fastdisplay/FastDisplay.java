package fastdisplay;

import fastcore.FastCore;

/**
 * FastDisplay - Native Windows Display Monitoring API for Java.
 * 
 * <p>Provides real-time monitoring of display changes (resolution, DPI, refresh rate,
 * orientation) via JNI.</p>
 * 
 * <p><b>Display Monitoring Example:</b></p>
 * <pre>
 * FastDisplay display = new FastDisplay();
 * display.setListener(new FastDisplay.DisplayListener() {
 *     &#64;Override
 *     public void onInitialState(int width, int height, int dpi, int refreshRate,
 *                                Orientation orientation) {
 *         System.out.println("Display: " + width + "x" + height + 
 *                          " | " + dpi + "DPI | " + refreshRate + "Hz");
 *     }
 *     
 *     &#64;Override
 *     public void onResolutionChanged(int w, int h, int dpi, int refresh) {
 *         System.out.println("Resolution changed: " + w + "x" + h);
 *     }
 *     
 *     &#64;Override
 *     public void onDPIChanged(int dpi, int scalePercent) {
 *         System.out.println("DPI changed: " + dpi + " (" + scalePercent + "%)");
 *     }
 *     
 *     &#64;Override
 *     public void onOrientationChanged(Orientation o) {
 *         System.out.println("Orientation: " + o);
 *     }
 * });
 * display.startMonitoring();
 * </pre>
 * 
 * <p><b>Thread Safety:</b> Display monitoring creates a background thread that calls listener
 * methods on the main Java event thread.</p>
 * 
 * <p><b>Platform Support:</b> Windows 10/11 only (uses native Win32 APIs).</p>
 * 
 * @author Andre Stubbe
 * @version 1.0.0
 * @since 1.0.0
 * @see <a href="https://github.com/andrestubbe/FastDisplay">GitHub Repository</a>
 * @see <a href="https://github.com/andrestubbe/FastTheme">FastTheme (Window Styling)</a>
 */
public class FastDisplay {
    static {
        FastCore.loadLibrary("fastdisplay");
    }

    /**
     * Display orientation values returned by the display monitoring API.
     */
    public enum Orientation {
        LANDSCAPE,
        PORTRAIT,
        LANDSCAPE_FLIPPED,
        PORTRAIT_FLIPPED
    }

    /**
     * Listener interface for receiving display change events.
     */
    public interface DisplayListener {
        void onResolutionChanged(int width, int height, int dpi, int refreshRate);
        void onDPIChanged(int dpi, int scalePercent);
        void onOrientationChanged(Orientation orientation);
        void onInitialState(int width, int height, int dpi, int refreshRate, Orientation orientation);
    }

    private DisplayListener listener;

    public void setListener(DisplayListener listener) {
        this.listener = listener;
    }

    private void notifyResolutionChanged(int width, int height, int dpi, int orientationOrdinal, int refreshRate) {
        if (listener != null) {
            listener.onResolutionChanged(width, height, dpi, refreshRate);
            listener.onOrientationChanged(Orientation.values()[orientationOrdinal]);
        }
    }

    private void notifyDPIChanged(int dpi, int scalePercent) {
        if (listener != null) {
            listener.onDPIChanged(dpi, scalePercent);
        }
    }

    private void notifyInitialState(int width, int height, int dpi, int orientationOrdinal, int refreshRate) {
        if (listener != null) {
            listener.onInitialState(width, height, dpi, refreshRate, Orientation.values()[orientationOrdinal]);
        }
    }

    public native boolean startMonitoring();
    public native void stopMonitoring();
}
