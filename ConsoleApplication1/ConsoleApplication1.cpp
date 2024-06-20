#include <iostream>
#include <fstream>
#include <unordered_map>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <vector>
#include <sstream>
#include <iomanip>

namespace fs = std::filesystem;

std::string time_to_str(std::time_t time) {
    char buf[100] = { 0 };
    struct tm timeinfo;
    localtime_s(&timeinfo, &time);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return std::string(buf);
}

std::time_t str_to_time(const std::string& time_str) {
    struct tm timeinfo = { 0 };
    std::istringstream ss(time_str);
    ss >> std::get_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
    return mktime(&timeinfo);
}

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

void save_registry(const std::string& filename, const std::unordered_map<std::string, std::time_t>& registry) {
    std::ofstream outfile(filename);
    for (const auto& entry : registry) {
        outfile << entry.first << "," << time_to_str(entry.second) << std::endl;
    }
}

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
    setlocale(LC_ALL, "ru");
    const std::string directory_to_scan = R"(C:\Users\PC\Desktop\Тест)";
    const std::string registry_filename = "file_registry.txt";

    std::unordered_map<std::string, std::time_t> previous_registry;
    bool first_run = true;

    if (fs::exists(registry_filename)) {
        previous_registry = load_registry(registry_filename);
        first_run = false;
    }

    while (true) {
        std::cout << "Меню:\n";
        std::cout << "1. Показать новые файлы\n";
        std::cout << "2. Показать обновленные файлы\n";
        std::cout << "3. Выход\n";
        std::cout << "Выберите действие: ";

        int choice;
        std::cin >> choice;

        std::unordered_map<std::string, std::time_t> current_registry = scan_directory(directory_to_scan);

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
            std::cout << "Новые файлы:\n";
            for (const auto& file : new_files) {
                std::cout << file << " (обнаружен в " << time_to_str(current_registry[file]) << ")\n";
            }
        }
        else if (choice == 2) {
            std::cout << "Обновленные файлы:\n";
            for (const auto& file : updated_files) {
                std::cout << file << " (обновлен в " << time_to_str(current_registry[file]) << ")\n";
            }
        }
        else if (choice == 3) {
            std::cout << "Выход из программы...\n";
            break;
        }
        else {
            std::cout << "Неверный выбор. Попробуйте снова.\n";
        }

        save_registry(registry_filename, current_registry);

        if (first_run) {
            previous_registry = current_registry;
            first_run = false;
        }
    }

    return 0;
}
