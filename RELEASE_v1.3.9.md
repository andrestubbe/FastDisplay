# FastTheme v1.3.9 Release Notes

## 🎉 Release v1.3.9 - Complete Maven/JitPack Distribution

**Release Date:** April 21, 2026  
**Tag:** `v1.3.9`

---

## ✨ What's New

### Complete FatJar Distribution
- **Native DLL bundled** inside JAR (`src/main/resources/native/fasttheme.dll`)
- **Automatic extraction** at runtime to temp directory
- **No manual DLL setup required** - works out of the box via Maven

### Architecture
- **FastCore integration** for cross-platform native library loading
- Clean separation: FastTheme focuses on Windows styling, FastCore handles library loading
- Ready for future macOS/Linux support

### JNI Native Methods (All Working)
```java
// Display monitoring
public native boolean startMonitoring();
public native void stopMonitoring();

// Window styling (static methods)
public static native long getWindowHandle(Component c);
public static native boolean setWindowTransparency(long hwnd, int alpha);
public static native boolean setTitleBarColor(long hwnd, int r, int g, int b);
public static native boolean setTitleBarTextColor(long hwnd, int r, int g, int b);
public static native boolean setTitleBarDarkMode(long hwnd, boolean enabled);

// System detection (static methods)
public static native String getSystemResolution();
public static native int getSystemDPI();
public static native boolean isSystemDarkMode();
public static native int getSystemRefreshRate();
```

---

## 📦 Installation

### Maven (Recommended)

Add to your `pom.xml`:

```xml
<repositories>
    <repository>
        <id>jitpack.io</id>
        <url>https://jitpack.io</url>
    </repository>
</repositories>

<dependencies>
    <dependency>
        <groupId>io.github.andrestubbe</groupId>
        <artifactId>fasttheme</artifactId>
        <version>v1.3.9</version>
    </dependency>
    <dependency>
        <groupId>io.github.andrestubbe</groupId>
        <artifactId>fastcore</artifactId>
        <version>v1.0.0</version>
    </dependency>
</dependencies>
```

### Run Demos

```bash
# Console Demo - Display monitoring
cd examples/00-console-demo
mvn clean compile exec:java

# Window Demo - Dark mode window styling  
cd examples/01-window-demo
mvn clean compile exec:java
```

---

## 🔧 Technical Details

### JitPack Build
- **Build URL:** https://jitpack.io/io/github/andrestubbe/fasttheme/v1.3.9/
- **JAR Size:** ~84 KB (includes 148 KB DLL compressed)
- **Build Time:** ~2-3 minutes after tag push

### Native DLL
- **Size:** 148 KB
- **Functions:** 11 JNI methods
- **Windows APIs:** DWM (Desktop Window Manager), JAWT (Java AWT Native Interface)
- **Min Windows:** Windows 10 1809+ (full DWM support)

### File Structure in JAR
```
fasttheme-1.3.9.jar
├── fasttheme/
│   └── FastTheme.class          # Main class with FastCore integration
└── native/
    └── fasttheme.dll            # Native Windows library
```

---

## 🎯 Usage Example

```java
import fasttheme.FastTheme;

public class MyApp {
    public static void main(String[] args) {
        // Create themed window
        JFrame frame = new JFrame("My App");
        
        // Get native window handle
        long hwnd = FastTheme.getWindowHandle(frame);
        
        // Apply dark mode styling
        FastTheme.setTitleBarDarkMode(hwnd, true);
        FastTheme.setTitleBarColor(hwnd, 32, 32, 32);    // Dark gray
        FastTheme.setTitleBarTextColor(hwnd, 255, 255, 255);  // White text
        
        // Monitor display changes
        FastTheme theme = new FastTheme();
        theme.setListener(new FastTheme.ThemeListener() {
            @Override
            public void onDPIChanged(int dpi, int scalePercent) {
                System.out.println("DPI changed: " + dpi + " (" + scalePercent + "%)");
            }
            
            @Override
            public void onThemeChanged(boolean isDarkTheme) {
                System.out.println("System theme: " + (isDarkTheme ? "Dark" : "Light"));
            }
            
            // ... other callback methods
        });
        theme.startMonitoring();
    }
}
```

---

## 🐛 Fixed Issues

| Issue | Status |
|-------|--------|
| DLL not in JAR | ✅ Fixed - now in `src/main/resources/native/` |
| JNI signature mismatch | ✅ Fixed - static methods use `jclass` |
| FastCore dependency | ✅ Fixed - properly declared in pom.xml |
| Version mismatch | ✅ Fixed - pom.xml 1.3.9 matches tag v1.3.9 |
| UnsatisfiedLinkError | ✅ Fixed - all native methods implemented |

---

## 📚 Documentation

- **JitPack Guide:** `JITPACK_FATJAR_GUIDE.md`
- **Examples:** `examples/00-console-demo/` and `examples/01-window-demo/`
- **API Reference:** See `FastTheme.java` JavaDoc

---

## 🔮 Future Roadmap

- **v1.4.0:** macOS support via FastCore
- **v1.5.0:** Linux support
- **v1.6.0:** Additional window styling options (backdrop, corner radius)

---

## 🙏 Credits

- **FastCore:** Cross-platform native library loading
- **JitPack:** Maven repository for GitHub projects
- **Windows DWM API:** Modern Windows styling capabilities

---

**Full Changelog:** Compare with v1.3.2  
**Issues:** https://github.com/andrestubbe/FastTheme/issues

