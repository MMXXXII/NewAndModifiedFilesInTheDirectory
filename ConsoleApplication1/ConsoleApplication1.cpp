#include <iostream>
#include <fstream>
#include <unordered_map>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <vector>
#include <sstream>

namespace fs = std::filesystem;
using namespace std;

// Функция для получения текущего времени в виде строки
string current_time_str() {
    time_t now = chrono::system_clock::to_time_t(chrono::system_clock::now());
    char buf[100] = { 0 };
    struct tm timeinfo;
    localtime_s(&timeinfo, &now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return string(buf);
}

// Функция для загрузки информации о файлах из файла реестра
unordered_map<string, time_t> load_registry(const string& filename) {
    unordered_map<string, time_t> registry;
    ifstream infile(filename);
    string line;
    while (getline(infile, line)) {
        stringstream ss(line);
        string path;
        string time_str;
        if (getline(ss, path, ',') && getline(ss, time_str)) {
            registry[path] = stoll(time_str);
        }
    }
    return registry;
}

// Функция для сохранения информации о файлах в файл реестра
void save_registry(const string& filename, const unordered_map<string, time_t>& registry) {
    ofstream outfile(filename);
    for (const auto& entry : registry) {
        outfile << entry.first << "," << entry.second << endl;
    }
}

// Функция для сканирования каталога и получения информации о файлах
unordered_map<string, time_t> scan_directory(const string& directory) {
    unordered_map<string, time_t> file_info;
    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        if (fs::is_regular_file(entry.path())) {
            auto ftime = fs::last_write_time(entry.path());
            auto sctp = chrono::time_point_cast<chrono::system_clock::duration>(ftime - decltype(ftime)::clock::now() + chrono::system_clock::now());
            time_t cftime = chrono::system_clock::to_time_t(sctp);
            file_info[entry.path().string()] = cftime;
        }
    }
    return file_info;
}

int main() {
    setlocale(LC_ALL, "ru");
    const string directory_to_scan = R"(C:\Users\PC\Desktop\Тест)";
    const string registry_filename = "file_registry.txt";

    // Загрузка предыдущего состояния реестра
    unordered_map<string, time_t> previous_registry = load_registry(registry_filename);

    // Сканирование текущего состояния каталога
    unordered_map<string, time_t> current_registry = scan_directory(directory_to_scan);

    // Сравнение и формирование списка новых и обновленных файлов
    vector<string> new_files;
    vector<string> updated_files;

    for (const auto& entry : current_registry) {
        auto it = previous_registry.find(entry.first);
        if (it == previous_registry.end()) {
            new_files.push_back(entry.first);
        }
        else if (it->second != entry.second) {
            updated_files.push_back(entry.first);
        }
    }

    // Вывод информации о новых и обновленных файлах
    cout << "New files:\n";
    for (const auto& file : new_files) {
        cout << file << " (detected at " << current_time_str() << ")\n";
    }

    cout << "\nUpdated files:\n";
    for (const auto& file : updated_files) {
        cout << file << " (detected at " << current_time_str() << ")\n";
    }

    // Сохранение текущего состояния в файл реестра
    save_registry(registry_filename, current_registry);

    return 0;
}
