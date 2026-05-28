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
        char c1 = (char) ('A' + ((raw >> 10) & 0x1F) - 1);
        char c2 = (char) ('A' + ((raw >> 5) & 0x1F) - 1);
        char c3 = (char) ('A' + (raw & 0x1F) - 1);
        return "" + c1 + c2 + c3;
    }

    public static String parseModelName(byte[] edid) {
        if (edid == null || edid.length < 126) return "Unknown";
        for (int i = 54; i < 126; i += 18) {
            if (edid[i] == 0x00 && edid[i+1] == 0x00 && edid[i+2] == 0x00 && edid[i+3] == (byte)0xFC) {
                byte[] block = Arrays.copyOfRange(edid, i + 5, i + 18);
                String s = new String(block, StandardCharsets.US_ASCII).trim();
                return s.replace("\n", "").replace("\r", "");
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

    public static int parseNativeWidth(byte[] edid) {
        if (edid == null || edid.length < 59) return 0;
        return ((edid[56] & 0xF0) << 4) | (edid[58] & 0xFF);
    }

    public static int parseNativeHeight(byte[] edid) {
        if (edid == null || edid.length < 60) return 0;
        return ((edid[57] & 0xF0) << 4) | (edid[59] & 0xFF);
    }

    // ------------------------------------------------------------
    // 2) CTA-861 HDR CAPABILITIES
    // ------------------------------------------------------------

    public static Map<String, Boolean> parseHdrCapabilities(byte[] edid) {
        Map<String, Boolean> caps = new HashMap<>();

        // CTA-861 Extension Block at 0x80
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

            // Extended tag
            if (tag == 0x07) {
                int extTag = ext[blockStart] & 0xFF;

                // HDR Static Metadata Block
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
    // 3) ICC PROFILE HEADER PARSING
    // ------------------------------------------------------------

    public static Map<String, Object> parseIccHeader(byte[] icc) {
        Map<String, Object> info = new HashMap<>();
        if (icc == null || icc.length < 128) return info;

        String signature = new String(Arrays.copyOfRange(icc, 36, 40), StandardCharsets.US_ASCII);
        String deviceClass = new String(Arrays.copyOfRange(icc, 12, 16), StandardCharsets.US_ASCII);
        String colorSpace = new String(Arrays.copyOfRange(icc, 16, 20), StandardCharsets.US_ASCII);

        info.put("signature", signature);
        info.put("deviceClass", deviceClass);
        info.put("colorSpace", colorSpace);

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
}
