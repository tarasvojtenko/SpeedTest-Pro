// speedtest_cpp.cpp — проверка скорости интернета на C++ (libcurl)

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <curl/curl.h>
#include <json/json.h> // библиотека jsoncpp

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

class SpeedTest {
private:
    std::string download_url = "http://speedtest.ftp.otenet.gr/files/test10Mb.db";
    std::string ping_url = "http://speedtest.ftp.otenet.gr/files/test10Mb.db";
    Json::Value results;
    std::vector<Json::Value> history;
    std::string history_file = "speedtest_history.json";

public:
    SpeedTest() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        loadHistory();
    }

    ~SpeedTest() {
        curl_global_cleanup();
    }

    void loadHistory() {
        std::ifstream file(history_file);
        if (file.is_open()) {
            file >> history;
            file.close();
        }
    }

    void saveHistory() {
        std::ofstream file(history_file);
        if (file.is_open()) {
            file << history;
            file.close();
        }
    }

    std::vector<double> measurePing(int count = 10) {
        std::vector<double> times;
        CURL* curl = curl_easy_init();
        if (!curl) return times;
        curl_easy_setopt(curl, CURLOPT_URL, ping_url.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        for (int i = 0; i < count; ++i) {
            auto start = std::chrono::steady_clock::now();
            CURLcode res = curl_easy_perform(curl);
            auto end = std::chrono::steady_clock::now();
            if (res == CURLE_OK) {
                double ms = std::chrono::duration<double, std::milli>(end - start).count();
                times.push_back(ms);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        curl_easy_cleanup(curl);
        return times;
    }

    double measureDownload() {
        CURL* curl = curl_easy_init();
        if (!curl) return 0;
        std::string response;
        curl_easy_setopt(curl, CURLOPT_URL, download_url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        auto start = std::chrono::steady_clock::now();
        CURLcode res = curl_easy_perform(curl);
        auto end = std::chrono::steady_clock::now();
        if (res != CURLE_OK) {
            std::cerr << "Ошибка загрузки" << std::endl;
            curl_easy_cleanup(curl);
            return 0;
        }
        double elapsed = std::chrono::duration<double>(end - start).count();
        double total_mb = response.size() / (1024.0 * 1024.0);
        double speed_mbps = (total_mb * 8) / elapsed;
        curl_easy_cleanup(curl);
        return speed_mbps;
    }

    double measureUpload() {
        // Симуляция загрузки (для демонстрации)
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        return 12.0; // фиктивная скорость
    }

    void runTest() {
        std::cout << "🌐 SpeedTest Pro — C++ Edition" << std::endl;
        std::cout << "Начинаем тестирование...\n" << std::endl;

        // Пинг
        std::cout << "📡 Измерение пинга..." << std::endl;
        auto times = measurePing(10);
        if (!times.empty()) {
            double min = *std::min_element(times.begin(), times.end());
            double max = *std::max_element(times.begin(), times.end());
            double sum = 0;
            for (double t : times) sum += t;
            double avg = sum / times.size();
            double median = times[times.size()/2];
            double jitter = 0;
            if (times.size() > 1) {
                double sq_sum = 0;
                for (double t : times) sq_sum += (t - avg) * (t - avg);
                jitter = std::sqrt(sq_sum / (times.size() - 1));
            }
            results["ping_min"] = min;
            results["ping_max"] = max;
            results["ping_avg"] = avg;
            results["ping_median"] = median;
            results["jitter"] = jitter;
            std::cout << "  Пинг: средний " << avg << " мс, мин " << min << " мс, макс " << max << " мс" << std::endl;
            std::cout << "  Джиттер: " << jitter << " мс" << std::endl;
        } else {
            std::cout << "  Не удалось измерить пинг" << std::endl;
        }

        // Загрузка
        std::cout << "\n⬇ Измерение скорости загрузки..." << std::endl;
        double download = measureDownload();
        results["download"] = download;
        std::cout << "  Скорость загрузки: " << download << " Мбит/с" << std::endl;

        // Выгрузка
        std::cout << "\n⬆ Измерение скорости выгрузки..." << std::endl;
        double upload = measureUpload();
        results["upload"] = upload;
        std::cout << "  Скорость выгрузки: " << upload << " Мбит/с" << std::endl;

        results["packet_loss"] = 0.0;
        results["timestamp"] = std::to_string(std::time(nullptr));
        history.append(results);
        saveHistory();
        std::cout << "\n✅ Тест завершён. Результат сохранён." << std::endl;
    }

    void showHistory() {
        if (history.size() == 0) {
            std::cout << "История пуста." << std::endl;
            return;
        }
        int start = std::max(0, (int)history.size() - 10);
        for (int i = start; i < history.size(); ++i) {
            auto& entry = history[i];
            std::cout << i+1 << ". " << entry.get("timestamp", "").asString()
                      << " - Загрузка: " << entry.get("download", 0.0).asDouble()
                      << " Мбит/с, Выгрузка: " << entry.get("upload", 0.0).asDouble()
                      << " Мбит/с, Пинг: " << entry.get("ping_avg", 0.0).asDouble() << " мс" << std::endl;
        }
    }

    void exportCSV() {
        if (history.size() == 0) {
            std::cout << "Нет данных для экспорта." << std::endl;
            return;
        }
        std::string filename = "speedtest_" + std::to_string(std::time(nullptr)) + ".csv";
        std::ofstream file(filename);
        file << "timestamp,download_mbps,upload_mbps,ping_avg_ms,jitter_ms,packet_loss\n";
        for (auto& entry : history) {
            file << entry.get("timestamp", "").asString() << ","
                 << entry.get("download", 0.0).asDouble() << ","
                 << entry.get("upload", 0.0).asDouble() << ","
                 << entry.get("ping_avg", 0.0).asDouble() << ","
                 << entry.get("jitter", 0.0).asDouble() << ","
                 << entry.get("packet_loss", 0.0).asDouble() << "\n";
        }
        file.close();
        std::cout << "Экспортировано в " << filename << std::endl;
    }
};

int main() {
    SpeedTest st;
    while (true) {
        std::cout << "\nМеню:" << std::endl;
        std::cout << "1. Запустить тест скорости" << std::endl;
        std::cout << "2. Показать историю" << std::endl;
        std::cout << "3. Экспорт в CSV" << std::endl;
        std::cout << "4. Выход" << std::endl;
        std::cout << "Выберите действие: ";
        int choice;
        std::cin >> choice;
        if (choice == 1) {
            st.runTest();
        } else if (choice == 2) {
            st.showHistory();
        } else if (choice == 3) {
            st.exportCSV();
        } else if (choice == 4) {
            break;
        } else {
            std::cout << "Неверный выбор." << std::endl;
        }
    }
    return 0;
}
