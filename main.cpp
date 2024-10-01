#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>
#include <ctime>
#include <iomanip>

using namespace std;

// Структура для хранения информации о запросе
struct LogEntry {
    time_t timestamp;
    string request;
    int status;
};

// Функция для конвертации строки даты в time_t
time_t parseTimestamp(const string& dateStr) {
    tm tm = {};
    stringstream ss(dateStr);
    ss >> get_time(&tm, "%d/%b/%Y:%H:%M:%S");
    return mktime(&tm);
}

// Функция для парсинга строки лога в структуру LogEntry
LogEntry parseLogEntry(const string& line) {
    LogEntry entry;
    istringstream iss(line);
    string ip, dash1, dash2, dateStr, timeZone, request;
    int status;

    if (iss >> ip >> dash1 >> dash2 >> dateStr >> timeZone) {
        dateStr = dateStr.substr(1); // Убираем открывающую скобку [
        timeZone = timeZone.substr(0, timeZone.size() - 1); // Убираем закрывающую скобку ]

        entry.timestamp = parseTimestamp(dateStr);
        getline(iss, request, '"'); // Пропускаем до "
        getline(iss, request, '"'); // Читаем запрос
        iss >> status;

        entry.request = request;
        entry.status = status;
    }
    return entry;
}

// Функция для чтения и парсинга файла access.log
vector<LogEntry> parseLogFile(ifstream& file) {
    vector<LogEntry> entries;
    if (!file) {
        cerr << "Error: Unable to open file." << endl;
        return entries;
    }
    string line;
    while (getline(file, line)) {
        entries.push_back(parseLogEntry(line));
    }
    return entries;
}

// Функция для фильтрации запросов с кодом 5XX
vector<LogEntry> filter5xxRequests(const vector<LogEntry>& entries) {
    vector<LogEntry> filtered;
    for (const auto& entry : entries) {
        if (entry.status >= 500 && entry.status < 600) {
            filtered.push_back(entry);
        }
    }
    return filtered;
}

// Функция для нахождения самых частых 5XX запросов
map<string, int> findMostFrequent5xxRequests(const vector<LogEntry>& entries, int n) {
    map<string, int> frequency;
    for (const auto& entry : entries) {
        frequency[entry.request]++;
    }
    vector<pair<string, int>> freqVec(frequency.begin(), frequency.end());

    // Сортировка по убыванию частоты
    sort(freqVec.begin(), freqVec.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;  // Сортировка от большего к меньшему
    });

    map<string, int> topN;
    for (int i = 0; i < n && i < freqVec.size(); ++i) {
        topN[freqVec[i].first] = freqVec[i].second;
    }
    return topN;
}

// Функция для нахождения временного окна с максимальным количеством запросов
pair<time_t, time_t> findPeakRequestWindow(const vector<LogEntry>& entries, int windowSize) {
    if (entries.empty() || windowSize <= 0) {
        return {0, 0};
    }

    time_t maxStartTime = entries.front().timestamp;
    time_t maxEndTime = entries.front().timestamp;
    int maxCount = 0;
    for (size_t i = 0; i < entries.size(); ++i) {
        time_t windowStart = entries[i].timestamp;
        time_t windowEnd = windowStart + windowSize;
        int count = 0;
        // Считаем количество запросов в пределах этого окна
        for (size_t j = i; j < entries.size() && entries[j].timestamp <= windowEnd; ++j) {
            count++;
        }

        // Если найдено большее количество запросов, обновляем время
        if (count > maxCount) {
            maxCount = count;
            maxStartTime = windowStart;
            maxEndTime = windowEnd;
        }
    }

    return {maxStartTime, maxEndTime};
}

int main() {
    ifstream logFile("C:/Users/vahti/CLionProjects/untitled8/access_log_Jul95.txt");
    if (!logFile) {
        cerr << "Error: Unable to open file access_log_Jul95.txt" << endl;
        return 1;
    }

    vector<LogEntry> entries = parseLogFile(logFile);

    // Пример вызова функций
    for (const auto& entry : entries) {
        cout << "Timestamp: " << entry.timestamp << ", Request: " << entry.request << ", Status: " << entry.status << endl;
    }

    int topN = 10;  // Задайте нужное значение
    vector<LogEntry> filteredEntries = filter5xxRequests(entries);
    map<string, int> mostFrequent = findMostFrequent5xxRequests(filteredEntries, topN);

    cout << "Most frequent 5XX requests:" << endl;
    for (const auto& [request, count] : mostFrequent) {
        cout << request << ": " << count << endl;
    }

    int windowSize = 3600;  // Задайте нужное значение (например, 1 час)
    auto [startTime, endTime] = findPeakRequestWindow(entries, windowSize);

    cout << "Peak request window from " << put_time(localtime(&startTime), "%Y-%m-%d %H:%M:%S")
        << " to " << put_time(localtime(&endTime), "%Y-%m-%d %H:%M:%S") << endl;

    return 0;
}