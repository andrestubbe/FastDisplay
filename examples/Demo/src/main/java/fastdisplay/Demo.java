package fastdisplay;

/**
 * Demo - Display Monitoring Example using FastDisplay v1.0.0 API.
 *
 * <p>This demo showcases the display monitoring capabilities of FastDisplay:
 * <ul>
 *   <li>Real-time resolution change detection</li>
 *   <li>DPI scaling change events</li>
 *   <li>Display orientation monitoring</li>
 *   <li>Refresh rate information</li>
 * </ul>
 *
 * <p><b>How it works:</b> FastDisplay creates a hidden message-only window and
 * background thread that listens for Windows system events:
 * <ul>
 *   <li>WM_DISPLAYCHANGE - Resolution/DPI changes</li>
 *   <li>WM_DPICHANGED - DPI scaling changes</li>
 * </ul>
 *
 * <p><b>Usage:</b> Run and try changing display settings.</p>
 *
 * @see FastDisplay#startMonitoring()
 * @see FastDisplay.DisplayListener
 * @version 1.0.0
 */
public class Demo {
    public static void main(String[] args) {
        clearConsole();

        System.out.println(FastANSI.BOLD + FastANSI.FG_CYAN + "FastDisplay v0.2.0" + FastANSI.RESET);
        System.out.println("──────────────────────────────────────────────");
        System.out.println();

        FastDisplay display = new FastDisplay();

        FastDisplay.MonitorInfo[] monitors = display.enumerateMonitors();
        if (monitors != null && monitors.length > 0) {
            for (FastDisplay.MonitorInfo m : monitors) {
                System.out.println(FastANSI.BOLD + "MONITOR " + m.index + FastANSI.RESET);
                System.out.println("• " + m.width + "×" + m.height + " @ " + m.refreshRate + " Hz");
                System.out.println("• DPI " + m.dpi + " (" + m.scalePercent + "%)");
                System.out.println("• " + m.orientation);

                byte[] edid = display.getEdidForMonitor(m.index);
                if (edid != null) {
                    System.out.println("• " + FastDisplayUtils.parseManufacturer(edid) + " – " + FastDisplayUtils.parseModelName(edid));
                    System.out.println("• Size: " + String.format("%.2f\"", FastDisplayUtils.parseSizeInInches(edid)));
                    System.out.println("• HDR: " + FastDisplayUtils.parseHdrCapabilities(edid).getOrDefault("HDR", false));
                } else {
                    System.out.println("• EDID: " + FastANSI.FG_RED + "not available" + FastANSI.RESET);
                }

                System.out.println("• DXGI HDR: " + display.isHdrEnabled(m.index));
                
                String profile = display.getColorProfileForMonitor(m.index);
                if (profile != null && !profile.isEmpty()) {
                    String file = new java.io.File(profile).getName();
                    System.out.println("• ICC: " + file);
                } else {
                    System.out.println("• ICC: " + FastANSI.FG_RED + "None" + FastANSI.RESET);
                }
                System.out.println();
            }
        }

        System.out.println(FastANSI.BOLD + "VIRTUAL DESKTOPS" + FastANSI.RESET);
        try {
            FastDesktop desktop = new FastDesktop();
            String currentDesktopId = desktop.getCurrentDesktopId();
            System.out.println("• Current ID: " + currentDesktopId);
            System.out.println();
        } catch (Throwable t) {
            System.out.println("Virtual Desktops not supported or failed: " + t.getMessage());
        }
        System.out.println();
        System.out.println(FastANSI.BOLD + "EVENT LOG" + FastANSI.RESET);
        System.out.println("• —");
        System.out.println();

        display.setListener(new FastDisplay.DisplayListener() {
            @Override
            public void onInitialState(int width, int height, int dpi, int refreshRate,
                                       FastDisplay.Orientation orientation) {
                // Initial state already shown by enumerateMonitors() above
            }

            @Override
            public void onResolutionChanged(int monitorIndex, int width, int height, int dpi, int refreshRate) {
                printDisplayInfo(monitorIndex, width, height, dpi, refreshRate, null, "[EVENT]", "[RESOLUTION]");
            }

            @Override
            public void onDPIChanged(int monitorIndex, int dpi, int scalePercent) {
                printDisplayInfo(monitorIndex, 0, 0, dpi, 0, null, "[EVENT]", "[DPI]");
            }

            @Override
            public void onOrientationChanged(int monitorIndex, FastDisplay.Orientation orientation) {
                printDisplayInfo(monitorIndex, 0, 0, 0, 0, orientation, "[EVENT]", "[ORIENTATION]");
            }

            @Override
            public void onColorProfileChanged(int monitorIndex) {
                System.out.printf("%-21s monitor=%d -> ICC profile updated%n", "[EVENT] [COLORPROFILE]", monitorIndex);
            }
        });

        display.startMonitoring();

        // Keep alive
        try {
            Thread.sleep(Long.MAX_VALUE);
        } catch (InterruptedException e) {
            display.stopMonitoring();
        }
    }

    private static void printDisplayInfo(int monitor, int width, int height, int dpi, int refreshRate, FastDisplay.Orientation orientation, String prefix, String eventType) {
        int scalePercent = (dpi * 100) / 96;
        String orientationName = orientation != null ? orientation.name() : "-";
        String monitorStr = monitor >= 0 ? String.valueOf(monitor) : "-";

        String prefixFormatted = prefix;
        if (!eventType.isEmpty()) {
            prefixFormatted += " " + eventType;
        }

        // Format based on event type
        if ("[DPI]".equals(eventType)) {
            System.out.printf("%-21s monitor=%s → dpi=%-3d (%-3d%%)%n",
                prefixFormatted, monitorStr, dpi, scalePercent);
        } else if ("[ORIENTATION]".equals(eventType)) {
            System.out.printf("%-21s monitor=%s → %s%n",
                prefixFormatted, monitorStr, orientationName);
        } else if ("[RESOLUTION]".equals(eventType)) {
            System.out.printf("%-21s monitor=%s → %d×%d @ %d Hz, dpi=%-3d (%-3d%%)%n",
                prefixFormatted, monitorStr, width, height, refreshRate, dpi, scalePercent);
        } else {
            // FULL or default (PRESENT/CURRENT)
            System.out.printf("%-21s monitor=%s, res=%dx%d, dpi=%-3d, scale=%-3d%%, hz=%-3d, orientation=%s%n",
                prefixFormatted, monitorStr, width, height, dpi, scalePercent, refreshRate, orientationName);
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

