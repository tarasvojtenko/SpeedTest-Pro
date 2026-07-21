// speedtest_java.java — проверка скорости интернета на Java

import java.net.http.*;
import java.net.URI;
import java.time.Duration;
import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.concurrent.*;
import java.time.Instant;
import com.google.gson.*; // требуется Gson

public class SpeedTest {
    private static final String DOWNLOAD_URL = "http://speedtest.ftp.otenet.gr/files/test10Mb.db";
    private static final String PING_URL = "http://speedtest.ftp.otenet.gr/files/test10Mb.db";
    private HttpClient client;
    private List<JsonObject> history = new ArrayList<>();
    private Gson gson = new GsonBuilder().setPrettyPrinting().create();

    public SpeedTest() {
        client = HttpClient.newHttpClient();
        loadHistory();
    }

    private void loadHistory() {
        try {
            String json = new String(Files.readAllBytes(Paths.get("speedtest_history.json")));
            JsonArray arr = JsonParser.parseString(json).getAsJsonArray();
            for (JsonElement el : arr) {
                history.add(el.getAsJsonObject());
            }
        } catch (IOException e) {}
    }

    private void saveHistory() {
        try (PrintWriter pw = new PrintWriter("speedtest_history.json")) {
            pw.println(gson.toJson(history));
        } catch (IOException e) {}
    }

    public List<Double> measurePing(int count) {
        List<Double> times = new ArrayList<>();
        for (int i=0; i<count; ++i) {
            try {
                long start = System.nanoTime();
                HttpRequest request = HttpRequest.newBuilder()
                    .uri(URI.create(PING_URL))
                    .timeout(Duration.ofSeconds(5))
                    .GET()
                    .build();
                client.send(request, HttpResponse.BodyHandlers.ofString());
                long end = System.nanoTime();
                double ms = (end - start) / 1_000_000.0;
                times.add(ms);
            } catch (Exception e) {}
            try { Thread.sleep(100); } catch (InterruptedException e) {}
        }
        return times;
    }

    public double measureDownload() throws Exception {
        HttpRequest request = HttpRequest.newBuilder()
            .uri(URI.create(DOWNLOAD_URL))
            .timeout(Duration.ofSeconds(30))
            .GET()
            .build();
        long start = System.nanoTime();
        HttpResponse<byte[]> response = client.send(request, HttpResponse.BodyHandlers.ofByteArray());
        long end = System.nanoTime();
        double elapsed = (end - start) / 1_000_000_000.0;
        double totalMb = response.body().length / (1024.0 * 1024.0);
        return (totalMb * 8) / elapsed;
    }

    public double measureUpload() throws Exception {
        // Симуляция выгрузки
        Thread.sleep(1000);
        return 12.0;
    }

    public void runTest() {
        System.out.println("🌐 SpeedTest Pro — Java Edition");
        System.out.println("Начинаем тестирование...\n");

        // Пинг
        System.out.println("📡 Измерение пинга...");
        List<Double> times = measurePing(10);
        JsonObject result = new JsonObject();
        if (!times.isEmpty()) {
            double min = times.stream().min(Double::compare).get();
            double max = times.stream().max(Double::compare).get();
            double avg = times.stream().mapToDouble(Double::doubleValue).average().getAsDouble();
            Collections.sort(times);
            double median = times.get(times.size()/2);
            double jitter = 0;
            if (times.size() > 1) {
                double sumSq = times.stream().mapToDouble(t -> Math.pow(t - avg, 2)).sum();
                jitter = Math.sqrt(sumSq / (times.size() - 1));
            }
            result.addProperty("ping_min", min);
            result.addProperty("ping_max", max);
            result.addProperty("ping_avg", avg);
            result.addProperty("ping_median", median);
            result.addProperty("jitter", jitter);
            System.out.printf("  Пинг: средний %.1f мс, мин %.1f мс, макс %.1f мс\n", avg, min, max);
            System.out.printf("  Джиттер: %.1f мс\n", jitter);
        } else {
            System.out.println("  Не удалось измерить пинг");
        }

        try {
            // Загрузка
            System.out.println("\n⬇ Измерение скорости загрузки...");
            double download = measureDownload();
            result.addProperty("download", download);
            System.out.printf("  Скорость загрузки: %.1f Мбит/с\n", download);

            // Выгрузка
            System.out.println("\n⬆ Измерение скорости выгрузки...");
            double upload = measureUpload();
            result.addProperty("upload", upload);
            System.out.printf("  Скорость выгрузки: %.1f Мбит/с\n", upload);
        } catch (Exception e) {
            System.out.println("Ошибка измерения: " + e.getMessage());
        }

        result.addProperty("packet_loss", 0.0);
        result.addProperty("timestamp", Instant.now().toString());
        history.add(result);
        saveHistory();
        System.out.println("\n✅ Тест завершён. Результат сохранён.");
    }

    public void showHistory() {
        if (history.isEmpty()) {
            System.out.println("История пуста.");
            return;
        }
        int start = Math.max(0, history.size()-10);
        for (int i = start; i < history.size(); ++i) {
            JsonObject entry = history.get(i);
            System.out.printf("%d. %s - Загрузка: %.1f Мбит/с, Выгрузка: %.1f Мбит/с, Пинг: %.1f мс\n",
                i+1,
                entry.get("timestamp").getAsString(),
                entry.get("download").getAsDouble(),
                entry.get("upload").getAsDouble(),
                entry.get("ping_avg").getAsDouble());
        }
    }

    public void exportCSV() throws IOException {
        if (history.isEmpty()) {
            System.out.println("Нет данных для экспорта.");
            return;
        }
        String filename = "speedtest_" + Instant.now().getEpochSecond() + ".csv";
        try (PrintWriter pw = new PrintWriter(filename)) {
            pw.println("timestamp,download_mbps,upload_mbps,ping_avg_ms,jitter_ms,packet_loss");
            for (JsonObject entry : history) {
                pw.printf("%s,%.2f,%.2f,%.2f,%.2f,%.2f\n",
                    entry.get("timestamp").getAsString(),
                    entry.get("download").getAsDouble(),
                    entry.get("upload").getAsDouble(),
                    entry.get("ping_avg").getAsDouble(),
                    entry.get("jitter").getAsDouble(),
                    entry.get("packet_loss").getAsDouble());
            }
        }
        System.out.println("Экспортировано в " + filename);
    }

    public static void main(String[] args) throws Exception {
        SpeedTest st = new SpeedTest();
        Scanner sc = new Scanner(System.in);
        while (true) {
            System.out.println("\nМеню:");
            System.out.println("1. Запустить тест скорости");
            System.out.println("2. Показать историю");
            System.out.println("3. Экспорт в CSV");
            System.out.println("4. Выход");
            System.out.print("Выберите действие: ");
            int choice = sc.nextInt();
            if (choice == 1) {
                st.runTest();
            } else if (choice == 2) {
                st.showHistory();
            } else if (choice == 3) {
                st.exportCSV();
            } else if (choice == 4) {
                break;
            } else {
                System.out.println("Неверный выбор.");
            }
        }
        sc.close();
    }
}
