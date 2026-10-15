package fastdisplay;

import java.time.LocalTime;
import java.time.format.DateTimeFormatter;

/**
 * Minimal Console Demo for Windows Theme & Accent Color Monitoring in FastDisplay.
 *
 * <p>Listens for OS-level theme switches (Dark / Light mode) and accent color updates
 * without requiring any dummy windows or polling loops.</p>
 */
public class ThemeListenerDemo {

    private static final DateTimeFormatter TIME_FMT = DateTimeFormatter.ofPattern("HH:mm:ss.SSS");

    public static void main(String[] args) {
        System.out.println("=================================================");
        System.out.println(" FastDisplay - Windows Theme Listener Demo");
        System.out.println("=================================================");
        System.out.println();

        FastDisplay display = new FastDisplay();

        // 1. Initial direct queries
        boolean appDark = display.isAppDarkMode();
        boolean systemDark = display.isSystemDarkMode();
        int accent = display.getAccentColor();

        System.out.println("Initial OS Theme State:");
        System.out.printf("  • Apps Dark Mode    : %s%n", appDark ? "DARK" : "LIGHT");
        System.out.printf("  • System Dark Mode  : %s%n", systemDark ? "DARK" : "LIGHT");
        System.out.printf("  • Accent Color      : #%08X (ARGB)%n", accent);
        System.out.println();
        System.out.println("Listening for Windows theme / accent color changes...");
        System.out.println("Try toggling Dark/Light mode in Windows Settings (Win + I -> Personalization -> Colors).");
        System.out.println("Press Ctrl+C to exit.");
        System.out.println("-------------------------------------------------");

        // 2. Register listener
        display.setListener(new FastDisplay.DisplayListener() {
            @Override
            public void onInitialState(int width, int height, int dpi, int refreshRate, FastDisplay.Orientation orientation) {
                // Not the focus of this demo
            }

            @Override
            public void onResolutionChanged(int monitorIndex, int width, int height, int dpi, int refreshRate) {
                // Not the focus of this demo
            }

            @Override
            public void onDPIChanged(int monitorIndex, int dpi, int scalePercent) {
                // Not the focus of this demo
            }

            @Override
            public void onOrientationChanged(int monitorIndex, FastDisplay.Orientation orientation) {
                // Not the focus of this demo
            }

            @Override
            public void onColorProfileChanged(int monitorIndex) {
                // Not the focus of this demo
            }

            @Override
            public void onThemeChanged(boolean isDark, int accentColor) {
                String timestamp = LocalTime.now().format(TIME_FMT);
                System.out.printf("[%s] [THEME EVENT] Mode: %-5s | Accent: #%08X%n",
                        timestamp,
                        (isDark ? "DARK" : "LIGHT"),
                        accentColor);
            }
        });

        // 3. Start monitoring
        boolean started = display.startMonitoring();
        if (!started) {
            System.err.println("Failed to start display/theme monitoring!");
            return;
        }

        // Keep main thread alive
        try {
            while (true) {
                Thread.sleep(10000);
            }
        } catch (InterruptedException e) {
            System.out.println("Exiting...");
        } finally {
            display.stopMonitoring();
        }
    }
}
