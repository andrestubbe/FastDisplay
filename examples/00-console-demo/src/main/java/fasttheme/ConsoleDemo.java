package fasttheme;

/**
 * ConsoleDemo - Display Monitoring Example using FastTheme v1.3.0 API.
 * 
 * <p>This demo showcases the display monitoring capabilities of FastTheme:
 * <ul>
 *   <li>Real-time resolution change detection</li>
 *   <li>DPI scaling change events</li>
 *   <li>Display orientation monitoring</li>
 *   <li>Windows theme (dark/light mode) detection</li>
 *   <li>Refresh rate information</li>
 * </ul>
 * 
 * <p><b>How it works:</b> FastTheme creates a hidden message-only window and
 * background thread that listens for Windows system events:
 * <ul>
 *   <li>WM_DISPLAYCHANGE - Resolution/DPI changes</li>
 *   <li>WM_SETTINGCHANGE - Theme changes</li>
 * </ul>
 * 
 * <p><b>Usage:</b> Run and try changing display settings or Windows theme.</p>
 * 
 * <p><b>Events received:</b></p>
 * <pre>
 * Initial State:
 *   Display: 1920x1080 @ 96DPI
 *   Refresh: 144Hz
 *   Theme: DARK
 * 
 * Resolution changed: 2560x1440 @ 96DPI
 * Theme changed: LIGHT MODE
 * </pre>
 * 
 * @see FastTheme#startMonitoring()
 * @see FastTheme.ThemeListener
 * @version 1.3.0
 */
public class ConsoleDemo {
    public static void main(String[] args) {
        // Clear console for clean output
        clearConsole();
        
        System.out.println("=== FastTheme Demo ===\n");
        
        FastTheme theme = new FastTheme();
        
        theme.setListener(new FastTheme.ThemeListener() {
            @Override
            public void onInitialState(int width, int height, int dpi, int refreshRate, 
                                       FastTheme.Orientation orientation, boolean isDarkTheme) {
                System.out.println("Initial State:");
                System.out.println("  Display: " + width + "x" + height + " @ " + dpi + "DPI");
                System.out.println("  Refresh: " + refreshRate + "Hz");
                System.out.println("  Theme: " + (isDarkTheme ? "DARK" : "LIGHT"));
                System.out.println();
            }
            
            @Override
            public void onResolutionChanged(int width, int height, int dpi, int refreshRate) {
                System.out.println("Resolution changed: " + width + "x" + height + " @ " + refreshRate + "Hz");
            }
            
            @Override
            public void onDPIChanged(int dpi, int scalePercent) {
                System.out.println("DPI/Scale changed: " + dpi + "DPI (" + scalePercent + "%)");
            }
            
            @Override
            public void onOrientationChanged(FastTheme.Orientation orientation) {
                System.out.println("Orientation changed: " + orientation);
            }
            
            @Override
            public void onThemeChanged(boolean isDarkTheme) {
                System.out.println("Theme changed: " + (isDarkTheme ? "DARK MODE" : "LIGHT MODE"));
            }
        });
        
        System.out.println("Starting monitoring... (Press Ctrl+C to stop)");
        theme.startMonitoring();
        
        // Keep alive
        try {
            Thread.sleep(Long.MAX_VALUE);
        } catch (InterruptedException e) {
            theme.stopMonitoring();
        }
    }
    
    private static void clearConsole() {
        try {
            if (System.getProperty("os.name").contains("Windows")) {
                new ProcessBuilder("cmd", "/c", "cls").inheritIO().start().waitFor();
            } else {
                System.out.print("\033[H\033[2J");
                System.out.flush();
            }
        } catch (Exception e) {
            // Ignore if clear fails
        }
    }
}
