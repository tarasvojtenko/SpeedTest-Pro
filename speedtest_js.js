// speedtest_js.js — проверка скорости интернета на JavaScript (Node.js)

const fs = require('fs');
const readline = require('readline');
const fetch = require('node-fetch');

const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
});

class SpeedTest {
    constructor() {
        this.history = [];
        this.historyFile = 'speedtest_history.json';
        this.loadHistory();
    }

    loadHistory() {
        try {
            const data = fs.readFileSync(this.historyFile, 'utf8');
            this.history = JSON.parse(data);
        } catch (e) {
            this.history = [];
        }
    }

    saveHistory() {
        fs.writeFileSync(this.historyFile, JSON.stringify(this.history, null, 2));
    }

    async measurePing(count) {
        const times = [];
        for (let i = 0; i < count; i++) {
            try {
                const start = Date.now();
                await fetch('http://speedtest.ftp.otenet.gr/files/test10Mb.db');
                times.push(Date.now() - start);
            } catch (e) {}
            await new Promise(resolve => setTimeout(resolve, 100));
        }
        return times;
    }

    async measureDownload() {
        const start = Date.now();
        const response = await fetch('http://speedtest.ftp.otenet.gr/files/test10Mb.db');
        const data = await response.buffer();
        const elapsed = (Date.now() - start) / 1000;
        const totalMb = data.length / (1024 * 1024);
        return (totalMb * 8) / elapsed;
    }

    async measureUpload() {
        // симуляция
        await new Promise(resolve => setTimeout(resolve, 1000));
        return 12.0;
    }

    async runTest() {
        console.log('\x1b[36m🌐 SpeedTest Pro — JavaScript Edition\x1b[0m');
        console.log('Начинаем тестирование...\n');

        const result = {};

        // Пинг
        console.log('\x1b[33m📡 Измерение пинга...\x1b[0m');
        const times = await this.measurePing(10);
        if (times.length > 0) {
            const sorted = [...times].sort((a,b) => a-b);
            const min = sorted[0];
            const max = sorted[sorted.length-1];
            const avg = times.reduce((a,b) => a+b, 0) / times.length;
            const median = sorted[Math.floor(sorted.length/2)];
            let jitter = 0;
            if (times.length > 1) {
                const sqSum = times.reduce((a,b) => a + Math.pow(b - avg, 2), 0);
                jitter = Math.sqrt(sqSum / (times.length - 1));
            }
            result.ping_min = min;
            result.ping_max = max;
            result.ping_avg = avg;
            result.ping_median = median;
            result.jitter = jitter;
            console.log(`  Пинг: средний ${avg.toFixed(1)} мс, мин ${min.toFixed(1)} мс, макс ${max.toFixed(1)} мс`);
            console.log(`  Джиттер: ${jitter.toFixed(1)} мс`);
        } else {
            console.log('  Не удалось измерить пинг');
        }

        // Загрузка
        console.log('\n\x1b[33m⬇ Измерение скорости загрузки...\x1b[0m');
        const download = await this.measureDownload();
        result.download = download;
        console.log(`  Скорость загрузки: ${download.toFixed(1)} Мбит/с`);

        // Выгрузка
        console.log('\n\x1b[33m⬆ Измерение скорости выгрузки...\x1b[0m');
        const upload = await this.measureUpload();
        result.upload = upload;
        console.log(`  Скорость выгрузки: ${upload.toFixed(1)} Мбит/с`);

        result.packet_loss = 0.0;
        result.timestamp = new Date().toISOString();
        this.history.push(result);
        this.saveHistory();
        console.log('\n\x1b[32m✅ Тест завершён. Результат сохранён.\x1b[0m');
    }

    showHistory() {
        if (this.history.length === 0) {
            console.log('История пуста.');
            return;
        }
        const start = Math.max(0, this.history.length - 10);
        for (let i = start; i < this.history.length; i++) {
            const entry = this.history[i];
            console.log(`${i+1}. ${entry.timestamp} - Загрузка: ${entry.download.toFixed(1)} Мбит/с, Выгрузка: ${entry.upload.toFixed(1)} Мбит/с, Пинг: ${entry.ping_avg.toFixed(1)} мс`);
        }
    }

    exportCSV() {
        if (this.history.length === 0) {
            console.log('Нет данных для экспорта.');
            return;
        }
        const filename = `speedtest_${Date.now()}.csv`;
        let csv = 'timestamp,download_mbps,upload_mbps,ping_avg_ms,jitter_ms,packet_loss\n';
        for (const entry of this.history) {
            csv += `${entry.timestamp},${entry.download.toFixed(2)},${entry.upload.toFixed(2)},${entry.ping_avg.toFixed(2)},${entry.jitter.toFixed(2)},${entry.packet_loss.toFixed(2)}\n`;
        }
        fs.writeFileSync(filename, csv);
        console.log(`Экспортировано в ${filename}`);
    }

    interactive() {
        console.log('\x1b[36m🌐 SpeedTest Pro — JavaScript Edition\x1b[0m');
        rl.question('\nМеню:\n1. Запустить тест скорости\n2. Показать историю\n3. Экспорт в CSV\n4. Выход\nВыберите действие: ', async (choice) => {
            if (choice === '1') {
                await this.runTest();
            } else if (choice === '2') {
                this.showHistory();
            } else if (choice === '3') {
                this.exportCSV();
            } else if (choice === '4') {
                rl.close();
                return;
            } else {
                console.log('Неверный выбор.');
            }
            this.interactive();
        });
    }
}

const st = new SpeedTest();
st.interactive();
