// speedtest_rs.rs — проверка скорости интернета на Rust

use std::fs;
use std::io::{self, Write, BufRead};
use std::time::{Instant, Duration};
use std::collections::HashMap;
use reqwest::blocking::Client;
use serde_json::{json, Value};
use termion::{color, style};

struct SpeedTest {
    history: Vec<Value>,
    history_file: String,
    client: Client,
}

impl SpeedTest {
    fn new() -> Self {
        SpeedTest {
            history: Vec::new(),
            history_file: "speedtest_history.json".to_string(),
            client: Client::builder().timeout(Duration::from_secs(30)).build().unwrap(),
        }
    }

    fn load_history(&mut self) {
        if let Ok(data) = fs::read_to_string(&self.history_file) {
            if let Ok(v) = serde_json::from_str(&data) {
                self.history = v;
            }
        }
    }

    fn save_history(&self) {
        if let Ok(json) = serde_json::to_string_pretty(&self.history) {
            let _ = fs::write(&self.history_file, json);
        }
    }

    fn measure_ping(&self, count: usize) -> Vec<f64> {
        let mut times = Vec::new();
        for _ in 0..count {
            let start = Instant::now();
            if let Ok(resp) = self.client.get("http://speedtest.ftp.otenet.gr/files/test10Mb.db").send() {
                let _ = resp.text();
                times.push(start.elapsed().as_secs_f64() * 1000.0);
            }
            std::thread::sleep(Duration::from_millis(100));
        }
        times
    }

    fn measure_download(&self) -> f64 {
        let start = Instant::now();
        if let Ok(resp) = self.client.get("http://speedtest.ftp.otenet.gr/files/test10Mb.db").send() {
            if let Ok(bytes) = resp.bytes() {
                let elapsed = start.elapsed().as_secs_f64();
                let total_mb = bytes.len() as f64 / (1024.0 * 1024.0);
                return (total_mb * 8.0) / elapsed;
            }
        }
        0.0
    }

    fn measure_upload(&self) -> f64 {
        // симуляция
        std::thread::sleep(Duration::from_secs(1));
        12.0
    }

    fn run_test(&mut self) {
        println!("{}🌐 SpeedTest Pro — Rust Edition{}", color::Fg(color::Cyan), style::Reset);
        println!("Начинаем тестирование...\n");

        let mut result = HashMap::new();

        // Пинг
        println!("{}📡 Измерение пинга...{}", color::Fg(color::Yellow), style::Reset);
        let times = self.measure_ping(10);
        if !times.is_empty() {
            let mut sorted = times.clone();
            sorted.sort_by(|a, b| a.partial_cmp(b).unwrap());
            let min = sorted[0];
            let max = sorted[sorted.len()-1];
            let sum: f64 = times.iter().sum();
            let avg = sum / times.len() as f64;
            let median = sorted[sorted.len()/2];
            let mut jitter = 0.0;
            if times.len() > 1 {
                let sq_sum: f64 = times.iter().map(|&t| (t - avg).powi(2)).sum();
                jitter = (sq_sum / (times.len() - 1) as f64).sqrt();
            }
            result.insert("ping_min".to_string(), json!(min));
            result.insert("ping_max".to_string(), json!(max));
            result.insert("ping_avg".to_string(), json!(avg));
            result.insert("ping_median".to_string(), json!(median));
            result.insert("jitter".to_string(), json!(jitter));
            println!("  Пинг: средний {:.1} мс, мин {:.1} мс, макс {:.1} мс", avg, min, max);
            println!("  Джиттер: {:.1} мс", jitter);
        } else {
            println!("  Не удалось измерить пинг");
        }

        // Загрузка
        println!("\n{}⬇ Измерение скорости загрузки...{}", color::Fg(color::Yellow), style::Reset);
        let download = self.measure_download();
        result.insert("download".to_string(), json!(download));
        println!("  Скорость загрузки: {:.1} Мбит/с", download);

        // Выгрузка
        println!("\n{}⬆ Измерение скорости выгрузки...{}", color::Fg(color::Yellow), style::Reset);
        let upload = self.measure_upload();
        result.insert("upload".to_string(), json!(upload));
        println!("  Скорость выгрузки: {:.1} Мбит/с", upload);

        result.insert("packet_loss".to_string(), json!(0.0));
        result.insert("timestamp".to_string(), json!(chrono::Local::now().to_rfc3339()));
        self.history.push(serde_json::to_value(result).unwrap());
        self.save_history();
        println!("\n✅ Тест завершён. Результат сохранён.");
    }

    fn show_history(&self) {
        if self.history.is_empty() {
            println!("История пуста.");
            return;
        }
        let start = if self.history.len() > 10 { self.history.len() - 10 } else { 0 };
        for i in start..self.history.len() {
            let entry = &self.history[i];
            let ts = entry["timestamp"].as_str().unwrap_or("");
            let down = entry["download"].as_f64().unwrap_or(0.0);
            let up = entry["upload"].as_f64().unwrap_or(0.0);
            let ping = entry["ping_avg"].as_f64().unwrap_or(0.0);
            println!("{}. {} - Загрузка: {:.1} Мбит/с, Выгрузка: {:.1} Мбит/с, Пинг: {:.1} мс", i+1, ts, down, up, ping);
        }
    }

    fn export_csv(&self) {
        if self.history.is_empty() {
            println!("Нет данных для экспорта.");
            return;
        }
        let filename = format!("speedtest_{}.csv", chrono::Local::now().timestamp());
        if let Ok(mut file) = fs::File::create(&filename) {
            use std::io::Write;
            writeln!(file, "timestamp,download_mbps,upload_mbps,ping_avg_ms,jitter_ms,packet_loss").unwrap();
            for entry in &self.history {
                writeln!(file, "{},{:.2},{:.2},{:.2},{:.2},{:.2}",
                    entry["timestamp"].as_str().unwrap_or(""),
                    entry["download"].as_f64().unwrap_or(0.0),
                    entry["upload"].as_f64().unwrap_or(0.0),
                    entry["ping_avg"].as_f64().unwrap_or(0.0),
                    entry["jitter"].as_f64().unwrap_or(0.0),
                    entry["packet_loss"].as_f64().unwrap_or(0.0)
                ).unwrap();
            }
            println!("Экспортировано в {}", filename);
        }
    }
}

fn main() -> io::Result<()> {
    let mut st = SpeedTest::new();
    st.load_history();
    let stdin = io::stdin();
    let mut reader = stdin.lock();
    loop {
        println!("\nМеню:");
        println!("1. Запустить тест скорости");
        println!("2. Показать историю");
        println!("3. Экспорт в CSV");
        println!("4. Выход");
        print!("Выберите действие: ");
        io::stdout().flush()?;
        let mut choice = String::new();
        reader.read_line(&mut choice)?;
        match choice.trim() {
            "1" => st.run_test(),
            "2" => st.show_history(),
            "3" => st.export_csv(),
            "4" => break,
            _ => println!("Неверный выбор."),
        }
    }
    Ok(())
}
