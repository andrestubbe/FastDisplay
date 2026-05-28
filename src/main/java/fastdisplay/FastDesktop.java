package fastdisplay;

import fastcore.FastCore;

public class FastDesktop {

    static {
        FastCore.loadLibrary("fastdisplay");
    }

    public static class DesktopInfo {
        public final String id;
        public final String name;
        public final boolean isCurrent;

        public DesktopInfo(String id, String name, boolean isCurrent) {
            this.id = id;
            this.name = name;
            this.isCurrent = isCurrent;
        }
    }

    public interface DesktopListener {
        void onDesktopChanged(String newDesktopId);
        void onDesktopCreated(String desktopId);
        void onDesktopDeleted(String desktopId);
        void onWindowMovedToDesktop(long hwnd, String desktopId);
    }

    public native DesktopInfo[] enumerateDesktops();
    public native String getCurrentDesktopId();
    public native boolean switchDesktop(String desktopId);
    public native boolean moveWindowToDesktop(long hwnd, String desktopId);
    public native void setListener(DesktopListener l);
}
