package fasttheme;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.image.BufferedImage;
import javax.swing.text.*;

/**
 * WindowDemo - Window Styling Example using FastTheme v1.3.0 API.
 * 
 * <p>This demo showcases the generic window styling capabilities of FastTheme:
 * <ul>
 *   <li>Custom titlebar colors matching content</li>
 *   <li>Window transparency (80% opacity)</li>
 *   <li>Native dark mode integration</li>
 *   <li>Real-time system information (DPI, resolution, refresh rate)</li>
 *   <li>Custom window icon generation</li>
 * </ul>
 * 
 * <p><b>Key Concept:</b> FastTheme extracts the native Windows HWND from any Swing
 * window and applies DWM (Desktop Window Manager) attributes directly.</p>
 * 
 * <p><b>Usage:</b></p>
 * <pre>
 * // 1. Make window visible first (required for HWND extraction)
 * frame.setVisible(true);
 * 
 * // 2. Get native handle
 * long hwnd = FastTheme.getWindowHandle(frame);
 * 
 * // 3. Apply styling
 * FastTheme.setTitleBarColor(hwnd, 12, 12, 12);  // Dark gray
 * FastTheme.setWindowTransparency(hwnd, 204);   // 80% opacity
 * </pre>
 * 
 * @see FastTheme#getWindowHandle(java.awt.Component)
 * @see FastTheme#setTitleBarColor(long, int, int, int)
 * @see FastTheme#setWindowTransparency(long, int)
 * @version 1.3.0
 */
public class WindowDemo extends JFrame {

    private JTextArea logArea;
    private StyleContext styleContext;
    private StyledDocument styledDoc;

    // Real detection values (populated via JNI)
    private int systemDPI = 96;
    private String systemTheme = "Unknown";
    private int systemRefresh = 60;
    private String systemResolution = "Unknown";
    
    // Monitoring
    private FastTheme themeMonitor;

    public WindowDemo() {
        // No decorations from Swing - we use native
        setTitle("FastTheme Window Demo");
        setSize(960, 500);
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setLocationRelativeTo(null);
        
        // Setup monitoring
        setupMonitoring();

        // Create terminal-like text area
        logArea = new JTextArea();
        // Windows 11 Terminal font: Cascadia Mono (or Consolas fallback)
        Font terminalFont = loadTerminalFont();
        logArea.setFont(terminalFont);
        logArea.setBackground(new Color(0, 0, 0, 0)); // Transparent
        logArea.setForeground(Color.WHITE);
        logArea.setCaretColor(Color.WHITE);
        logArea.setLineWrap(true);
        logArea.setWrapStyleWord(true);
        logArea.setEditable(false);
        logArea.setFocusable(false); // Prevent text selection
        logArea.setBorder(BorderFactory.createEmptyBorder(20, 20, 20, 20));

        // Dark scroll pane
        JScrollPane scrollPane = new JScrollPane(logArea);
        scrollPane.setBorder(null);
        scrollPane.getViewport().setOpaque(false);
        scrollPane.setOpaque(false);

        // Main panel with terminal-like background
        JPanel panel = new JPanel(new BorderLayout());
        panel.setBackground(new Color(12, 12, 12)); // Almost black
        panel.add(scrollPane, BorderLayout.CENTER);

        setContentPane(panel);

        // Set custom icon (white circle with black dot)
        setIconImage(createWindowIcon());

        // Log initial info
        log("FastTheme Terminal v1.3");
        log("======================");
        log("");
        log("OS Detection:");
        log("  Resolution: " + systemResolution);
        log("  DPI Scale: " + systemDPI + " (" + (systemDPI * 100 / 96) + "%)");
        log("  Theme: " + systemTheme);
        log("  Refresh: " + systemRefresh + "Hz");
        log("");
        log("Window Styling Applied:");
        log("  Titlebar: RGB(12,12,12) [seamless]");
        log("  Text: RGB(255,255,255) [white]");
        log("  Transparency: 80%");
        log("  Mode: Dark");
        log("");
        log("Ready. JNI + DWM APIs. Pure native.");
    }

    private void log(String msg) {
        logArea.append(msg + "\n");
        logArea.setCaretPosition(logArea.getDocument().getLength());
    }

    private Font loadTerminalFont() {
        // Try Windows 11 Terminal fonts in order
        String[] fontNames = {"Cascadia Mono", "Cascadia Code", "Consolas", "Lucida Console", "Monospaced"};
        for (String name : fontNames) {
            if (isFontAvailable(name)) {
                return new Font(name, Font.PLAIN, 14);
            }
        }
        return new Font("Monospaced", Font.PLAIN, 14);
    }

    private boolean isFontAvailable(String fontName) {
        GraphicsEnvironment ge = GraphicsEnvironment.getLocalGraphicsEnvironment();
        for (Font font : ge.getAllFonts()) {
            if (font.getName().equals(fontName)) {
                return true;
            }
        }
        return false;
    }

    private BufferedImage createWindowIcon() {
        int size = 32;
        int center = size / 2;
        int radius = 14;
        int dotRadius = 4;

        BufferedImage image = new BufferedImage(size, size, BufferedImage.TYPE_INT_ARGB);
        Graphics2D g2 = image.createGraphics();

        // Transparent background
        g2.setComposite(AlphaComposite.Clear);
        g2.fillRect(0, 0, size, size);
        g2.setComposite(AlphaComposite.SrcOver);

        // White circle
        g2.setColor(Color.WHITE);
        g2.fillOval(center - radius, center - radius, radius * 2, radius * 2);

        // Black dot in center
        g2.setColor(Color.BLACK);
        g2.fillOval(center - dotRadius, center - dotRadius, dotRadius * 2, dotRadius * 2);

        g2.dispose();
        return image;
    }

    private void detectSystemValues() {
        try {
            systemResolution = FastTheme.getSystemResolution();
            systemDPI = FastTheme.getSystemDPI();
            systemTheme = FastTheme.isSystemDarkMode() ? "Dark" : "Light";
            systemRefresh = FastTheme.getSystemRefreshRate();
        } catch (Exception e) {
            // Fallback values
            systemResolution = "Unknown";
            systemDPI = 96;
            systemTheme = "Unknown";
            systemRefresh = 60;
        }
    }

    @Override
    public void setVisible(boolean visible) {
        if (visible && !isVisible()) {
            super.setVisible(true);

            // IMMEDIATELY apply native styling using FastTheme API
            long hwnd = FastTheme.getWindowHandle(this);
            if (hwnd != 0) {
                // Detect real system values first
                detectSystemValues();

                // 80% transparency (204/255)
                FastTheme.setWindowTransparency(hwnd, 204);

                // Titlebar same color as content (RGB 12, 12, 12)
                FastTheme.setTitleBarColor(hwnd, 12, 12, 12);

                // White text
                FastTheme.setTitleBarTextColor(hwnd, 255, 255, 255);

                // Dark mode
                FastTheme.setTitleBarDarkMode(hwnd, true);

                // Set custom icon (white circle with black dot)
                boolean iconOk = false; // setWindowIcon not in FastTheme API yet

                // Update log with real values
                logArea.setText(""); // Clear
                log("FastTheme Terminal v1.3");
                log("======================");
                log("");
                log("OS Detection:");
                log("  Resolution: " + systemResolution);
                log("  DPI Scale: " + systemDPI + " (" + (systemDPI * 100 / 96) + "%)");
                log("  Theme: " + systemTheme);
                log("  Refresh: " + systemRefresh + "Hz");
                log("");
                log("Window Styling Applied:");
                log("  Titlebar: RGB(12,12,12) [seamless]");
                log("  Text: RGB(255,255,255) [white]");
                log("  Transparency: 80%");
                log("  Mode: Dark");
                log("");
                log("Ready. JNI + DWM APIs. Pure native.");
                log("");
                log("[JNI] Window handle: 0x" + Long.toHexString(hwnd));
                log("[JNI] Icon: " + (iconOk ? "white circle + black dot" : "FAILED"));
                log("[JNI] DWM attributes applied successfully");
                
                // Start monitoring after window is visible
                if (themeMonitor != null) {
                    themeMonitor.startMonitoring();
                    log("[MONITOR] Monitoring started");
                }
            } else {
                log("");
                log("[ERROR] Failed to get window handle");
            }
        } else {
            super.setVisible(visible);
        }
    }


    public static void main(String[] args) {
        // No Swing LAF - we want native
        try {
            UIManager.setLookAndFeel(UIManager.getCrossPlatformLookAndFeelClassName());
        } catch (Exception e) {
            e.printStackTrace();
        }

        SwingUtilities.invokeLater(() -> {
            new WindowDemo().setVisible(true);
        });
    }
}
