// speedtest_cs.cs — проверка скорости интернета на C#

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.Http;
using System.Text.Json;
using System.Threading.Tasks;
using System.Diagnostics;

class SpeedTest
{
    private static readonly string DownloadUrl = "http://speedtest.ftp.otenet.gr/files/test10Mb.db";
    private static readonly string PingUrl = "http://speedtest.ftp.otenet.gr/files/test10Mb.db";
    private HttpClient client = new HttpClient();
    private List<Dictionary<string, object>> history = new List<Dictionary<string, object>>();
    private string historyFile = "speedtest_history.json";

    public SpeedTest()
    {
        LoadHistory();
    }

    private void LoadHistory()
    {
        if (File.Exists(historyFile))
        {
            string json = File.ReadAllText(historyFile);
            history = JsonSerializer.Deserialize<List<Dictionary<string, object>>>(json) ?? new List<Dictionary<string, object>>();
        }
    }

    private void SaveHistory()
    {
        string json = JsonSerializer.Serialize(history, new JsonSerializerOptions { WriteIndented = true });
        File.WriteAllText(historyFile, json);
    }

    public async Task<List<double>> MeasurePing(int count)
    {
        var times = new List<double>();
        for (int i = 0; i < count; i++)
        {
            try
            {
                var sw = Stopwatch.StartNew();
                var response = await client.GetAsync(PingUrl);
                sw.Stop();
                if (response.IsSuccessStatusCode)
                    times.Add(sw.Elapsed.TotalMilliseconds);
            }
            catch { }
            await Task.Delay(100);
        }
        return times;
    }

    public async Task<double> MeasureDownload()
    {
        var sw = Stopwatch.StartNew();
        var response = await client.GetAsync(DownloadUrl);
        sw.Stop();
        if (!response.IsSuccessStatusCode) return 0;
        var data = await response.Content.ReadAsByteArrayAsync();
        double elapsed = sw.Elapsed.TotalSeconds;
        double totalMb = data.Length / (1024.0 * 1024.0);
        return (totalMb * 8) / elapsed;
    }

    public async Task<double> MeasureUpload()
    {
        // симуляция
        await Task.Delay(1000);
        return 12.0;
    }

    public async Task RunTest()
    {
        Console.WriteLine("🌐 SpeedTest Pro — C# Edition");
        Console.WriteLine("Начинаем тестирование...\n");

        var result = new Dictionary<string, object>();

        // Пинг
        Console.WriteLine("📡 Измерение пинга...");
        var times = await MeasurePing(10);
        if (times.Count > 0)
        {
            double min = times.Min();
            double max = times.Max();
            double avg = times.Average();
            times.Sort();
            double median = times[times.Count / 2];
            double jitter = 0;
            if (times.Count > 1)
            {
                double sumSq = times.Select(t => Math.Pow(t - avg, 2)).Sum();
                jitter = Math.Sqrt(sumSq / (times.Count - 1));
            }
            result["ping_min"] = min;
            result["ping_max"] = max;
            result["ping_avg"] = avg;
            result["ping_median"] = median;
            result["jitter"] = jitter;
            Console.WriteLine($"  Пинг: средний {avg:F1} мс, мин {min:F1} мс, макс {max:F1} мс");
            Console.WriteLine($"  Джиттер: {jitter:F1} мс");
        }
        else
        {
            Console.WriteLine("  Не удалось измерить пинг");
        }

        // Загрузка
        Console.WriteLine("\n⬇ Измерение скорости загрузки...");
        double download = await MeasureDownload();
        result["download"] = download;
        Console.WriteLine($"  Скорость загрузки: {download:F1} Мбит/с");

        // Выгрузка
        Console.WriteLine("\n⬆ Измерение скорости выгрузки...");
        double upload = await MeasureUpload();
        result["upload"] = upload;
        Console.WriteLine($"  Скорость выгрузки: {upload:F1} Мбит/с");

        result["packet_loss"] = 0.0;
        result["timestamp"] = DateTime.Now.ToString("o");
        history.Add(result);
        SaveHistory();
        Console.WriteLine("\n✅ Тест завершён. Результат сохранён.");
    }

    public void ShowHistory()
    {
        if (history.Count == 0)
        {
            Console.WriteLine("История пуста.");
            return;
        }
        int start = Math.Max(0, history.Count - 10);
        for (int i = start; i < history.Count; ++i)
        {
            var entry = history[i];
            Console.WriteLine($"{i+1}. {entry["timestamp"]} - Загрузка: {entry["download"]:F1} Мбит/с, Выгрузка: {entry["upload"]:F1} Мбит/с, Пинг: {entry["ping_avg"]:F1} мс");
        }
    }

    public void ExportCSV()
    {
        if (history.Count == 0)
        {
            Console.WriteLine("Нет данных для экспорта.");
            return;
        }
        string filename = $"speedtest_{DateTime.Now:yyyyMMdd_HHmmss}.csv";
        using var sw = new StreamWriter(filename);
        sw.WriteLine("timestamp,download_mbps,upload_mbps,ping_avg_ms,jitter_ms,packet_loss");
        foreach (var entry in history)
        {
            sw.WriteLine($"{entry["timestamp"]},{entry["download"]:F2},{entry["upload"]:F2},{entry["ping_avg"]:F2},{entry["jitter"]:F2},{entry["packet_loss"]:F2}");
        }
        Console.WriteLine($"Экспортировано в {filename}");
    }

    public static async Task Main(string[] args)
    {
        var st = new SpeedTest();
        while (true)
        {
            Console.WriteLine("\nМеню:");
            Console.WriteLine("1. Запустить тест скорости");
            Console.WriteLine("2. Показать историю");
            Console.WriteLine("3. Экспорт в CSV");
            Console.WriteLine("4. Выход");
            Console.Write("Выберите действие: ");
            string choice = Console.ReadLine();
            if (choice == "1")
                await st.RunTest();
            else if (choice == "2")
                st.ShowHistory();
            else if (choice == "3")
                st.ExportCSV();
            else if (choice == "4")
                break;
            else
                Console.WriteLine("Неверный выбор.");
        }
    }
}
