package fastdisplay;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

public final class FastDisplayUtils {

    private FastDisplayUtils() {}

    // ------------------------------------------------------------
    // 1) EDID PARSING
    // ------------------------------------------------------------

    public static String parseManufacturer(byte[] edid) {
        if (edid == null || edid.length < 10) return "Unknown";
        int raw = ((edid[8] & 0xFF) << 8) | (edid[9] & 0xFF);

        int c1 = (raw >> 10) & 0x1F;
        int c2 = (raw >> 5) & 0x1F;
        int c3 = raw & 0x1F;

        if (c1 < 1 || c1 > 26 || c2 < 1 || c2 > 26 || c3 < 1 || c3 > 26) {
            return "UNK";
        }

        return new String(new char[] {
            (char) ('A' + c1 - 1),
            (char) ('A' + c2 - 1),
            (char) ('A' + c3 - 1)
        });
    }

    public static String parseModelName(byte[] edid) {
        if (edid == null || edid.length < 126) return "Unknown";
        for (int i = 54; i < 126; i += 18) {
            if (edid[i] == 0x00 && edid[i+1] == 0x00 && edid[i+2] == 0x00 && edid[i+3] == (byte)0xFC) {
                byte[] block = Arrays.copyOfRange(edid, i + 5, i + 18);
                String s = new String(block, StandardCharsets.US_ASCII)
                        .replace("\n", "")
                        .replace("\r", "")
                        .replace("\0", " ")
                        .trim();
                return s.isEmpty() ? "Generic/Internal Display" : s;
            }
        }
        return "Generic/Internal Display";
    }

    public static String parseSerialNumber(byte[] edid) {
        if (edid == null || edid.length < 126) return "Unknown";
        for (int i = 54; i < 126; i += 18) {
            if (edid[i] == 0x00 && edid[i+1] == 0x00 && edid[i+2] == 0x00 && edid[i+3] == (byte)0xFF) {
                byte[] block = Arrays.copyOfRange(edid, i + 5, i + 18);
                return new String(block, StandardCharsets.US_ASCII).trim();
            }
        }
        return "Unknown";
    }

    public static double parseSizeInInches(byte[] edid) {
        if (edid == null || edid.length < 23) return 0;
        int w = edid[21] & 0xFF;
        int h = edid[22] & 0xFF;
        if (w == 0 || h == 0) return 0;
        double diagCm = Math.sqrt(w * w + h * h);
        return diagCm / 2.54;
    }

    // First Detailed Timing Descriptor at offset 54
    public static int parseNativeWidth(byte[] edid) {
        if (edid == null || edid.length < 72) return 0;
        int dtd = 54;
        int hActiveLsb = edid[dtd + 2] & 0xFF;
        int hActiveMsb = (edid[dtd + 4] & 0xF0) >> 4;
        return (hActiveMsb << 8) | hActiveLsb;
    }

    public static int parseNativeHeight(byte[] edid) {
        if (edid == null || edid.length < 72) return 0;
        int dtd = 54;
        int vActiveLsb = edid[dtd + 5] & 0xFF;
        int vActiveMsb = (edid[dtd + 7] & 0xF0) >> 4;
        return (vActiveMsb << 8) | vActiveLsb;
    }

    // ------------------------------------------------------------
    // 2) CTA-861 HDR CAPABILITIES
    // ------------------------------------------------------------

    public static Map<String, Boolean> parseHdrCapabilities(byte[] edid) {
        Map<String, Boolean> caps = new HashMap<>();

        if (edid == null || edid.length < 256) {
            caps.put("HDR", false);
            return caps;
        }

        byte[] ext = Arrays.copyOfRange(edid, 128, 256);
        if (ext[0] != 0x02) {
            caps.put("HDR", false);
            return caps;
        }

        int dtdStart = ext[2] & 0xFF;
        int pos = 4;

        while (pos < dtdStart) {
            int tag = (ext[pos] & 0xE0) >> 5;
            int len = ext[pos] & 0x1F;
            int blockStart = pos + 1;

            if (tag == 0x07) {
                int extTag = ext[blockStart] & 0xFF;
                if (extTag == 0x06) {
                    int eotf = ext[blockStart + 1] & 0xFF;
                    caps.put("HDR", true);
                    caps.put("PQ", (eotf & 0b00000100) != 0);
                    caps.put("HLG", (eotf & 0b00001000) != 0);
                }
            }
            pos += 1 + len;
        }

        return caps;
    }

    // ------------------------------------------------------------
    // 3) ICC PROFILE HEADER PARSING (extended)
    // ------------------------------------------------------------

    public static Map<String, Object> parseIccHeader(byte[] icc) {
        Map<String, Object> info = new HashMap<>();
        if (icc == null || icc.length < 128) return info;

        String signature   = new String(Arrays.copyOfRange(icc, 36, 40), StandardCharsets.US_ASCII);
        String deviceClass = new String(Arrays.copyOfRange(icc, 12, 16), StandardCharsets.US_ASCII);
        String colorSpace  = new String(Arrays.copyOfRange(icc, 16, 20), StandardCharsets.US_ASCII);
        String pcs         = new String(Arrays.copyOfRange(icc, 20, 24), StandardCharsets.US_ASCII);

        int major = icc[8] & 0xFF;
        int minor = (icc[9] & 0xF0) >> 4;
        int bug   = icc[9] & 0x0F;
        String version = major + "." + minor + "." + bug;

        info.put("signature", signature);
        info.put("deviceClass", deviceClass);
        info.put("colorSpace", colorSpace);
        info.put("pcs", pcs);
        info.put("version", version);

        return info;
    }

    // ------------------------------------------------------------
    // 4) JSON EXPORTER
    // ------------------------------------------------------------

    public static String toJson(Map<String, ?> map) {
        StringBuilder sb = new StringBuilder("{");
        boolean first = true;
        for (var e : map.entrySet()) {
            if (!first) sb.append(",");
            first = false;
            sb.append("\"").append(e.getKey()).append("\":");

            Object v = e.getValue();
            if (v instanceof Number || v instanceof Boolean) {
                sb.append(v.toString());
            } else {
                sb.append("\"").append(v).append("\"");
            }
        }
        sb.append("}");
        return sb.toString();
    }

    // ------------------------------------------------------------
    // 5) DEBUG HELPERS
    // ------------------------------------------------------------

    public static String hexDump(byte[] data) {
        if (data == null) return "null";
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < data.length; i++) {
            if (i % 16 == 0) sb.append(String.format("%n%04X: ", i));
            sb.append(String.format("%02X ", data[i]));
        }
        return sb.toString().trim();
    }

    // ------------------------------------------------------------
    // 6) Pretty printer for monitor report
    // ------------------------------------------------------------

    public static String formatMonitorReport(FastDisplay.MonitorInfo m,
                                             byte[] edid,
                                             boolean dxgiHdr,
                                             String iccPath) {
        String ln = System.lineSeparator();
        StringBuilder sb = new StringBuilder();

        sb.append("MONITOR ").append(m.index).append(ln);
        sb.append("────────────────────────────────────────────────────────").append(ln);
        sb.append(String.format("Resolution      : %d × %d @ %d Hz%n", m.width, m.height, m.refreshRate));
        int scalePercent = (m.dpi * 100) / 96;
        sb.append(String.format("DPI / Scaling   : %d (%d%%)%n", m.dpi, scalePercent));
        sb.append(String.format("Orientation     : %s%n", m.orientation));

        if (edid != null) {
            String manu   = parseManufacturer(edid);
            String model  = parseModelName(edid);
            String serial = parseSerialNumber(edid);
            double sizeIn = parseSizeInInches(edid);
            Map<String, Boolean> hdr = parseHdrCapabilities(edid);

            sb.append(String.format("Manufacturer    : %s%n", manu));
            sb.append(String.format("Model Name      : %s%n", model));
            sb.append(String.format("Serial Number   : %s%n", serial));
            if (sizeIn > 0) {
                sb.append(String.format("Diagonal Size   : %.2f\"%n", sizeIn));
            }
            sb.append(String.format("HDR (EDID)      : %s%n", hdr.getOrDefault("HDR", false)));
        } else {
            sb.append("EDID            : not available").append(ln);
        }

        sb.append(String.format("DXGI HDR        : %s%n", dxgiHdr));

        if (iccPath != null && !iccPath.isEmpty()) {
            String file = new java.io.File(iccPath).getName();
            sb.append(String.format("ICC Profile     : %s%n", file));
        } else {
            sb.append("ICC Profile     : None").append(ln);
        }

        return sb.toString();
    }
}
