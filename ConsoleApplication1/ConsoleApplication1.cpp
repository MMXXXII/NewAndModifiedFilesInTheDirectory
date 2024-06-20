#include <iostream>
#include <fstream>
#include <unordered_map>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <vector>
#include <sstream>
#include <iomanip>

using namespace std;

namespace fs = filesystem;

string time_to_str(time_t time) {
    char buf[100] = { 0 };
    struct tm timeinfo;
    localtime_s(&timeinfo, &time);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return string(buf);
}

time_t str_to_time(const string& time_str) {
    struct tm timeinfo = { 0 };
    istringstream ss(time_str);
    ss >> get_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
    return mktime(&timeinfo);
}

unordered_map<string, time_t> load_registry(const string& filename) {
    unordered_map<string, time_t> registry;
    ifstream infile(filename);
    string line;
    while (getline(infile, line)) {
        stringstream ss(line);
        string path;
        string time_str;
        if (getline(ss, path, ',') && getline(ss, time_str)) {
            registry[path] = str_to_time(time_str);
        }
    }
    return registry;
}

void save_registry(const string& filename, const unordered_map<string, time_t>& registry) {
    ofstream outfile(filename);
    for (const auto& entry : registry) {
        outfile << entry.first << "," << time_to_str(entry.second) << endl;
    }
}

unordered_map<string, time_t> scan_directory(const string& directory) {
    unordered_map<string, time_t> file_info;
    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        if (fs::is_regular_file(entry.path())) {
            auto ftime = fs::last_write_time(entry.path());
            auto sctp = chrono::time_point_cast<chrono::system_clock::duration>(
                ftime - decltype(ftime)::clock::now() + chrono::system_clock::now()
            );
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

    unordered_map<string, time_t> previous_registry;
    bool first_run = true;

    if (fs::exists(registry_filename)) {
        previous_registry = load_registry(registry_filename);
        first_run = false;
    }

    while (true) {
        cout << "Меню:\n";
        cout << "1. Показать новые файлы\n";
        cout << "2. Показать обновленные файлы\n";
        cout << "3. Выход\n";
        cout << "Выберите действие: ";

        int choice;
        cin >> choice;

        unordered_map<string, time_t> current_registry = scan_directory(directory_to_scan);

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

        if (choice == 1) {
            cout << "Новые файлы:\n";
            for (const auto& file : new_files) {
                cout << file << " (обнаружен в " << time_to_str(current_registry[file]) << ")\n";
            }
        }
        else if (choice == 2) {
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

        save_registry(registry_filename, current_registry);

        if (first_run) {
            previous_registry = current_registry;
            first_run = false;
        }
    }

    return 0;
}
