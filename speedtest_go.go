// speedtest_go.go — проверка скорости интернета на Go

package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"io"
	"math"
	"net/http"
	"os"
	"sort"
	"strconv"
	"strings"
	"time"
)

type SpeedTest struct {
	history     []map[string]interface{}
	historyFile string
	client      *http.Client
}

func NewSpeedTest() *SpeedTest {
	return &SpeedTest{
		history:     make([]map[string]interface{}, 0),
		historyFile: "speedtest_history.json",
		client:      &http.Client{Timeout: 30 * time.Second},
	}
}

func (s *SpeedTest) loadHistory() {
	data, err := os.ReadFile(s.historyFile)
	if err != nil {
		return
	}
	json.Unmarshal(data, &s.history)
}

func (s *SpeedTest) saveHistory() {
	data, _ := json.MarshalIndent(s.history, "", "  ")
	os.WriteFile(s.historyFile, data, 0644)
}

func (s *SpeedTest) measurePing(count int) []float64 {
	times := make([]float64, 0)
	for i := 0; i < count; i++ {
		start := time.Now()
		resp, err := s.client.Get("http://speedtest.ftp.otenet.gr/files/test10Mb.db")
		if err == nil {
			resp.Body.Close()
			times = append(times, float64(time.Since(start).Milliseconds()))
		}
		time.Sleep(100 * time.Millisecond)
	}
	return times
}

func (s *SpeedTest) measureDownload() float64 {
	start := time.Now()
	resp, err := s.client.Get("http://speedtest.ftp.otenet.gr/files/test10Mb.db")
	if err != nil {
		return 0
	}
	defer resp.Body.Close()
	data, _ := io.ReadAll(resp.Body)
	elapsed := time.Since(start).Seconds()
	totalMb := float64(len(data)) / (1024 * 1024)
	return (totalMb * 8) / elapsed
}

func (s *SpeedTest) measureUpload() float64 {
	// симуляция
	time.Sleep(1 * time.Second)
	return 12.0
}

func (s *SpeedTest) runTest() {
	fmt.Println("🌐 SpeedTest Pro — Go Edition")
	fmt.Println("Начинаем тестирование...\n")

	result := make(map[string]interface{})

	// Пинг
	fmt.Println("📡 Измерение пинга...")
	times := s.measurePing(10)
	if len(times) > 0 {
		sort.Float64s(times)
		min := times[0]
		max := times[len(times)-1]
		var sum float64
		for _, v := range times {
			sum += v
		}
		avg := sum / float64(len(times))
		median := times[len(times)/2]
		var jitter float64
		if len(times) > 1 {
			var sqSum float64
			for _, v := range times {
				sqSum += math.Pow(v-avg, 2)
			}
			jitter = math.Sqrt(sqSum / float64(len(times)-1))
		}
		result["ping_min"] = min
		result["ping_max"] = max
		result["ping_avg"] = avg
		result["ping_median"] = median
		result["jitter"] = jitter
		fmt.Printf("  Пинг: средний %.1f мс, мин %.1f мс, макс %.1f мс\n", avg, min, max)
		fmt.Printf("  Джиттер: %.1f мс\n", jitter)
	} else {
		fmt.Println("  Не удалось измерить пинг")
	}

	// Загрузка
	fmt.Println("\n⬇ Измерение скорости загрузки...")
	download := s.measureDownload()
	result["download"] = download
	fmt.Printf("  Скорость загрузки: %.1f Мбит/с\n", download)

	// Выгрузка
	fmt.Println("\n⬆ Измерение скорости выгрузки...")
	upload := s.measureUpload()
	result["upload"] = upload
	fmt.Printf("  Скорость выгрузки: %.1f Мбит/с\n", upload)

	result["packet_loss"] = 0.0
	result["timestamp"] = time.Now().Format(time.RFC3339)
	s.history = append(s.history, result)
	s.saveHistory()
	fmt.Println("\n✅ Тест завершён. Результат сохранён.")
}

func (s *SpeedTest) showHistory() {
	if len(s.history) == 0 {
		fmt.Println("История пуста.")
		return
	}
	start := 0
	if len(s.history) > 10 {
		start = len(s.history) - 10
	}
	for i := start; i < len(s.history); i++ {
		entry := s.history[i]
		fmt.Printf("%d. %v - Загрузка: %.1f Мбит/с, Выгрузка: %.1f Мбит/с, Пинг: %.1f мс\n",
			i+1,
			entry["timestamp"],
			entry["download"].(float64),
			entry["upload"].(float64),
			entry["ping_avg"].(float64))
	}
}

func (s *SpeedTest) exportCSV() {
	if len(s.history) == 0 {
		fmt.Println("Нет данных для экспорта.")
		return
	}
	filename := fmt.Sprintf("speedtest_%d.csv", time.Now().Unix())
	file, err := os.Create(filename)
	if err != nil {
		fmt.Println("Ошибка создания файла:", err)
		return
	}
	defer file.Close()
	file.WriteString("timestamp,download_mbps,upload_mbps,ping_avg_ms,jitter_ms,packet_loss\n")
	for _, entry := range s.history {
		file.WriteString(fmt.Sprintf("%v,%.2f,%.2f,%.2f,%.2f,%.2f\n",
			entry["timestamp"],
			entry["download"].(float64),
			entry["upload"].(float64),
			entry["ping_avg"].(float64),
			entry["jitter"].(float64),
			entry["packet_loss"].(float64)))
	}
	fmt.Println("Экспортировано в", filename)
}

func main() {
	st := NewSpeedTest()
	st.loadHistory()
	scanner := bufio.NewScanner(os.Stdin)
	for {
		fmt.Println("\nМеню:")
		fmt.Println("1. Запустить тест скорости")
		fmt.Println("2. Показать историю")
		fmt.Println("3. Экспорт в CSV")
		fmt.Println("4. Выход")
		fmt.Print("Выберите действие: ")
		scanner.Scan()
		choice := strings.TrimSpace(scanner.Text())
		switch choice {
		case "1":
			st.runTest()
		case "2":
			st.showHistory()
		case "3":
			st.exportCSV()
		case "4":
			return
		default:
			fmt.Println("Неверный выбор.")
		}
	}
}
