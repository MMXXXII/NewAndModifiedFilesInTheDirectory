#include <iostream>
#include <fstream>
#include <unordered_map>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <vector>
#include <sstream>
#include <thread>

namespace fs = std::filesystem;

// Функция для получения текущего времени в виде строки
std::string time_to_str(std::time_t time) {
    char buf[100] = { 0 };
    struct tm timeinfo;
    localtime_s(&timeinfo, &time);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return std::string(buf);
}

// Функция для преобразования строки в std::time_t
std::time_t str_to_time(const std::string& time_str) {
    struct tm timeinfo = { 0 };
    std::istringstream ss(time_str);
    ss >> std::get_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
    return mktime(&timeinfo);
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
            registry[path] = str_to_time(time_str);
        }
    }
    return registry;
}

// Функция для сохранения информации о файлах в файл реестра
void save_registry(const std::string& filename, const std::unordered_map<std::string, std::time_t>& registry) {
    std::ofstream outfile(filename);
    for (const auto& entry : registry) {
        outfile << entry.first << "," << time_to_str(entry.second) << std::endl;
    }
}

// Функция для сканирования каталога и получения информации о файлах
std::unordered_map<std::string, std::time_t> scan_directory(const std::string& directory) {
    std::unordered_map<std::string, std::time_t> file_info;
    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        if (fs::is_regular_file(entry.path())) {
            auto ftime = fs::last_write_time(entry.path());
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - decltype(ftime)::clock::now() + std::chrono::system_clock::now()
            );
            std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
            file_info[entry.path().string()] = cftime;
        }
    }
    return file_info;
}

int main() {
    setlocale(LC_ALL, "ru"); // Устанавливаем русскую локаль для корректного вывода
    using namespace std; // Используем пространство имен std

    const std::string directory_to_scan = R"(C:\Users\PC\Desktop\Тест)";
    const std::string registry_filename = "file_registry.txt";

    while (true) {
        cout << "Меню:\n";
        cout << "1. Показать новые файлы\n";
        cout << "2. Показать обновленные файлы\n";
        cout << "3. Выход\n";
        cout << "Выберите действие: ";
        int choice;
        cin >> choice;

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

        if (choice == 1) {
            // Вывод информации о новых файлах
            cout << "Новые файлы:\n";
            for (const auto& file : new_files) {
                cout << file << " (обнаружен в " << time_to_str(current_registry[file]) << ")\n";
            }
        }
        else if (choice == 2) {
            // Вывод информации об обновленных файлах
            cout << "Обновленные файлы:\n";
            for (const auto& file : updated_files) {
                cout << file << " (обновлен в " << time_to_str(current_registry[file]) << ")\n";
            }
        }
        else if (choice == 3) {
            cout << "Выход из программы...\n";
            break;
        }
        else {
            cout << "Неверный выбор. Попробуйте снова.\n";
        }

        // Сохранение текущего состояния в файл реестра
        save_registry(registry_filename, current_registry);

        // Добавление задержки перед следующим циклом
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return 0;
}
