#include <iostream>
#include <fstream>
#include <unordered_map>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <vector>
#include <sstream>

namespace fs = std::filesystem;

// Функция для получения текущего времени в виде строки
std::string current_time_str() {
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char buf[100] = { 0 };
    struct tm timeinfo;
    localtime_s(&timeinfo, &now);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return std::string(buf);
}

// Функция для загрузки информации о файлах из файла реестра
std::unordered_map<std::string, std::time_t> load_registry(const std::string& filename) {
    std::unordered_map<std::string, std::time_t> registry;
    std::ifstream infile(filename);
    std::string line;
    while (std::getline(infile, line)) {
        std::stringstream ss(line);
        std::string path;
        std::string time_str;
        if (std::getline(ss, path, ',') && std::getline(ss, time_str)) {
            registry[path] = std::stoll(time_str);
        }
    }
    return registry;
}

// Функция для сохранения информации о файлах в файл реестра
void save_registry(const std::string& filename, const std::unordered_map<std::string, std::time_t>& registry) {
    std::ofstream outfile(filename);
    for (const auto& entry : registry) {
        outfile << entry.first << "," << entry.second << std::endl;
    }
}

// Функция для сканирования каталога и получения информации о файлах
std::unordered_map<std::string, std::time_t> scan_directory(const std::string& directory) {
    std::unordered_map<std::string, std::time_t> file_info;
    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        if (fs::is_regular_file(entry.path())) {
            auto ftime = fs::last_write_time(entry.path());
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - decltype(ftime)::clock::now()
                + std::chrono::system_clock::now());
            std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
            file_info[entry.path().string()] = cftime;
        }
    }
    return file_info;
}

int main() {
    setlocale(LC_ALL, "ru");
    const std::string directory_to_scan = R"(C:\Users\PC\Desktop\Тест)";
    const std::string registry_filename = "file_registry.txt";

    // Загрузка предыдущего состояния реестра
    std::unordered_map<std::string, std::time_t> previous_registry = load_registry(registry_filename);

    // Сканирование текущего состояния каталога
    std::unordered_map<std::string, std::time_t> current_registry = scan_directory(directory_to_scan);

    // Сравнение и формирование списка новых и обновленных файлов
    std::vector<std::string> new_files;
    std::vector<std::string> updated_files;

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
    std::cout << "New files:\n";
    for (const auto& file : new_files) {
        std::cout << file << " (detected at " << current_time_str() << ")\n";
    }

    std::cout << "\nUpdated files:\n";
    for (const auto& file : updated_files) {
        std::cout << file << " (detected at " << current_time_str() << ")\n";
    }

    // Сохранение текущего состояния в файл реестра
    save_registry(registry_filename, current_registry);

    return 0;
}
