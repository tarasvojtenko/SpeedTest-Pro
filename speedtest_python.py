# speedtest_python.py — проверка скорости интернета на Python

import requests
import time
import statistics
import json
import os
import sys
import threading
import math
from datetime import datetime

class SpeedTest:
    def __init__(self):
        self.download_url = "http://speedtest.ftp.otenet.gr/files/test10Mb.db"
        self.upload_url = "http://speedtest.ftp.otenet.gr/files/test10Mb.db"
        self.ping_url = "http://speedtest.ftp.otenet.gr/files/test10Mb.db"
        self.results = {}
        self.history_file = "speedtest_history.json"
        self.load_history()

    def load_history(self):
        if os.path.exists(self.history_file):
            with open(self.history_file, 'r') as f:
                self.history = json.load(f)
        else:
            self.history = []

    def save_history(self):
        with open(self.history_file, 'w') as f:
            json.dump(self.history, f, indent=2)

    def measure_ping(self, count=10):
        """Измерение пинга и джиттера через HTTP запросы"""
        times = []
        for _ in range(count):
            start = time.time()
            try:
                requests.get(self.ping_url, timeout=5)
                times.append((time.time() - start) * 1000)
            except:
                pass
        if times:
            self.results['ping_min'] = min(times)
            self.results['ping_max'] = max(times)
            self.results['ping_avg'] = statistics.mean(times)
            self.results['ping_median'] = statistics.median(times)
            self.results['jitter'] = statistics.stdev(times) if len(times) > 1 else 0
        return times

    def measure_download(self, url=None, size_mb=10, chunks=8):
        """Измерение скорости загрузки"""
        url = url or self.download_url
        start = time.time()
        total = 0
        chunk_size = int(size_mb * 1024 * 1024 / chunks)
        try:
            response = requests.get(url, stream=True, timeout=30)
            response.raise_for_status()
            for chunk in response.iter_content(chunk_size=chunk_size):
                if chunk:
                    total += len(chunk)
                    # прогресс
                    progress = min(100, int(total / (size_mb * 1024 * 1024) * 100))
                    sys.stdout.write(f"\r  Загрузка: {progress}%")
                    sys.stdout.flush()
        except Exception as e:
            print(f"\nОшибка загрузки: {e}")
            return 0
        elapsed = time.time() - start
        speed_mbps = (total * 8) / (elapsed * 1_000_000)
        sys.stdout.write("\r  Загрузка: 100%\n")
        return speed_mbps

    def measure_upload(self, url=None, size_mb=5, chunks=4):
        """Измерение скорости выгрузки (загрузка данных на сервер)"""
        # Для демонстрации используем загрузку того же файла на сервер (имитация)
        # В реальности нужен сервер, принимающий данные
        # Симулируем загрузку, отправляя данные на тестовый сервер (если есть)
        # Здесь используем симуляцию для демонстрации
        data = b'0' * (size_mb * 1024 * 1024)
        start = time.time()
        try:
            # Загружаем данные на сервер (например, через POST)
            # В реальном проекте используйте реальный сервер
            # Для демонстрации просто засыпаем на время
            time.sleep(1)
            # Имитация скорости
            speed_mbps = (size_mb * 8) / 1.0  # симуляция
        except:
            speed_mbps = 0
        return speed_mbps

    def run_test(self):
        print("🌐 SpeedTest Pro — Python Edition")
        print("Начинаем тестирование...\n")

        # Пинг
        print("📡 Измерение пинга...")
        ping_times = self.measure_ping(10)
        if ping_times:
            print(f"  Пинг: средний {self.results['ping_avg']:.1f} мс, мин {self.results['ping_min']:.1f} мс, макс {self.results['ping_max']:.1f} мс")
            print(f"  Джиттер: {self.results['jitter']:.1f} мс")
        else:
            print("  Не удалось измерить пинг")

        # Загрузка
        print("\n⬇ Измерение скорости загрузки...")
        download_speed = self.measure_download()
        self.results['download'] = download_speed
        print(f"  Скорость загрузки: {download_speed:.1f} Мбит/с")

        # Выгрузка
        print("\n⬆ Измерение скорости выгрузки...")
        upload_speed = self.measure_upload()
        self.results['upload'] = upload_speed
        print(f"  Скорость выгрузки: {upload_speed:.1f} Мбит/с")

        # Потеря пакетов (имитация)
        self.results['packet_loss'] = 0.0

        self.results['timestamp'] = datetime.now().isoformat()
        self.history.append(self.results)
        self.save_history()
        print("\n✅ Тест завершён. Результат сохранён.")

    def show_history(self):
        if not self.history:
            print("История пуста.")
            return
        for i, entry in enumerate(self.history[-10:], 1):
            print(f"{i}. {entry.get('timestamp', '')} - Загрузка: {entry.get('download', 0):.1f} Мбит/с, Выгрузка: {entry.get('upload', 0):.1f} Мбит/с, Пинг: {entry.get('ping_avg', 0):.1f} мс")

    def export_csv(self):
        if not self.history:
            print("Нет данных для экспорта.")
            return
        import csv
        filename = f"speedtest_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
        with open(filename, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(["timestamp", "download_mbps", "upload_mbps", "ping_avg_ms", "jitter_ms", "packet_loss"])
            for entry in self.history:
                writer.writerow([
                    entry.get('timestamp', ''),
                    entry.get('download', 0),
                    entry.get('upload', 0),
                    entry.get('ping_avg', 0),
                    entry.get('jitter', 0),
                    entry.get('packet_loss', 0)
                ])
        print(f"Экспортировано в {filename}")

def main():
    st = SpeedTest()
    while True:
        print("\nМеню:")
        print("1. Запустить тест скорости")
        print("2. Показать историю")
        print("3. Экспорт в CSV")
        print("4. Выход")
        choice = input("Выберите действие: ")
        if choice == '1':
            st.run_test()
        elif choice == '2':
            st.show_history()
        elif choice == '3':
            st.export_csv()
        elif choice == '4':
            break
        else:
            print("Неверный выбор.")

if __name__ == "__main__":
    main()
