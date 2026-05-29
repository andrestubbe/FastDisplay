import java.io.*;
import java.nio.file.*;

public class Extract {
    public static void main(String[] args) throws Exception {
        BufferedReader reader = new BufferedReader(new InputStreamReader(new FileInputStream("repo.jsonl"), "UTF-8"));
        String line;
        while ((line = reader.readLine()) != null) {
            if (line.contains("\"native/FastDisplay.cpp\"")) {
                int contentStart = line.indexOf("\"content\":\"") + 11;
                int contentEnd = line.lastIndexOf("\"}");
                if (contentEnd == -1) contentEnd = line.length() - 2;
                String content = line.substring(contentStart, contentEnd);
                content = content.replace("\\n", "\n").replace("\\r", "\r").replace("\\\"", "\"").replace("\\\\", "\\").replace("\\t", "\t");
                Files.write(Paths.get("old_FastDisplay.cpp"), content.getBytes("UTF-8"));
                break;
            }
        }
        reader.close();
    }
}
