// Включаем необходимые заголовки
#include <iostream>
#include <fstream>
#include <unordered_map> // Эта библиотека используется для создания неупорядоченной карты (хеш-таблицы), которая хранит реестр файлов, 
//сопоставляя пути файлов со временем их последнего изменения.
#include <filesystem> //Эта библиотека используется для работы с файловой системой, таких как итерация по файлам и директориям, проверка, 
//является ли путь директорией, и получение времени последнего изменения файла.
#include <chrono> // Эта библиотека используется для работы с временем и датами, таких как преобразование между разными представлениями 
//времени и расчет разницы времени.
#include <ctime>
#include <vector>
#include <sstream> //Эта библиотека используется для создания строковых потоков, которые используются для парсинга строк и извлечения информации из них.
#include <iomanip> //Эта библиотека используется для манипуляции вводом-выводом, таких как установка точности вывода значений.
#include <windows.h>

namespace fs = std::filesystem;
using namespace std;

// Функция для преобразования time_t в строку
string time_to_str(time_t time) {
    // Создаем буфер для хранения строки времени
    char buf[100] = { 0 };
    // Создаем структуру tm для хранения информации о времени
    struct tm timeinfo;
    // Преобразуем time_t в структуру tm
    localtime_s(&timeinfo, &time);
    // Форматируем строку времени с помощью strftime
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    // Возвращаем строку времени как string
    return string(buf);
}

// Функция для преобразования строки в time_t
time_t str_to_time(const string& time_str) {
    // Создаем структуру tm для хранения информации о времени
    struct tm timeinfo = { 0 };
    // Создаем stringstream для парсинга строки времени
    istringstream ss(time_str);
    // Парсим строку времени с помощью манипулятора get_time
    ss >> get_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
    // Преобразуем структуру tm в time_t
    return mktime(&timeinfo);
}

// Функция для загрузки реестра из файла
unordered_map<string, time_t> load_registry(const string& filename) {
    // Создаем пустой реестр
    unordered_map<string, time_t> registry;
    // Открываем файл для чтения
    ifstream infile(filename);
    // Читаем файл строка за строкой
    string line;
    while (getline(infile, line)) {
        // Создаем stringstream для парсинга строки
        stringstream ss(line);
        // Парсим строку в путь и строку времени
        string path;
        string time_str;
        if (getline(ss, path, ',') && getline(ss, time_str)) {
            // Преобразуем строку времени в time_t и добавляем в реестр
            registry[path] = str_to_time(time_str);
        }
    }
    // Возвращаем загруженный реестр
    return registry;
}

// Функция для сохранения реестра в файл
void save_registry(const string& filename, const unordered_map<string, time_t>& registry) {
    // Открываем файл для записи
    ofstream outfile(filename);
    // Итерируемся по реестру и записываем каждую запись в файл
    for (const auto& entry : registry) {
        outfile << entry.first << "," << time_to_str(entry.second) << endl;
    }
}

// Функция для сканирования директории и создания реестра файлов
unordered_map<string, time_t> scan_directory(const string& directory) {
    // Создаем пустой реестр
    unordered_map<string, time_t> file_info;
    // Итерируемся по файлам в директории с помощью рекурсивного итератора директорий
    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        // Проверяем, является ли запись регулярным файлом
        if (fs::is_regular_file(entry.path())) {
            // Получаем время последнего изменения файла
            auto ftime = last_write_time(entry.path());
            // Преобразуем время файла в time_t
            auto sctp = chrono::time_point_cast<chrono::system_clock::duration>(
                ftime - decltype(ftime)::clock::now() + chrono::system_clock::now()
            );
            time_t cftime = chrono::system_clock::to_time_t(sctp);
            // Добавляем файл в реестр
            file_info[entry.path().string()] = cftime;
        }
    }
    // Возвращаем реестр
    return file_info;
}

// Функция для проверки, является ли путь директорией
bool is_directory(const string& path) {
    // Используем std::filesystem для проверки, является ли путь директорией
    return fs::is_directory(path);
}

int main() {
    // Устанавливаем кодовую страницу консоли в 1251
    SetConsoleCP(1251);
    // Устанавливаем кодовую страницу вывода консоли в 1251
    SetConsoleOutputCP(1251);
    // Устанавливаем локаль на русский
    setlocale(LC_ALL, "ru");

    // Получаем директорию для сканирования от пользователя
    string directory_to_scan;
    while (true) {
        cout << "Введите путь до папки: ";
        getline(cin, directory_to_scan);

        // Проверяем, существует ли директория
        if (is_directory(directory_to_scan)) {
            break;
        }
        else {
            cout << "Введенная строка не является папкой. Пожалуйста, введите корректный путь.\n";
        }
    }

    // Определяем имя файла реестра
    const string registry_filename = "file_registry.txt";

    // Создаем пустой предыдущий реестр
    unordered_map<string, time_t> previous_registry;
    // Флаг, указывающий, является ли это первым запуском
    bool first_run = true;

    // Проверяем, существует ли файл реестра
    if (fs::exists(registry_filename)) {
        // Загружаем предыдущий реестр из файла
        previous_registry = load_registry(registry_filename);
        // Это не первый запуск
        first_run = false;
    }

    // Основной цикл
    while (true) {
        // Отображаем меню
        cout << "Меню:\n";
        cout << "1. Показать новые файлы\n";
        cout << "2. Показать обновленные файлы\n";
        cout << "3. Выход\n";
        cout << "Выберите действие: ";

        // Получаем выбор пользователя
        int choice;
        cin >> choice;

        // Сканируем директорию и создаем текущий реестр
        unordered_map<string, time_t> current_registry = scan_directory(directory_to_scan);

        // Создаем векторы для хранения новых и обновленных файлов
        vector<string> new_files;
        vector<string> updated_files;

        // Итерируемся по текущему реестру
        for (const auto& entry : current_registry) {
            // Ищем запись в предыдущем реестре
            auto it = previous_registry.find(entry.first);
            // Если запись не найдена, это новый файл
            if (it == previous_registry.end()) {
                new_files.push_back(entry.first);
            }
            // Если запись найдена и время отличается, это обновленный файл
            else if (it->second != entry.second) {
                updated_files.push_back(entry.first);
            }
        }

        // Обрабатываем выбор пользователя
        if (choice == 1) {
            // Отображаем новые файлы
            cout << "Новые файлы:\n";
            for (const auto& file : new_files) {
                cout << file << " (обнаружен в " << time_to_str(current_registry[file]) << ")\n";
            }
        }
        else if (choice == 2) {
            // Отображаем обновленные файлы
            cout << "Обновленные файлы:\n";
            for (const auto& file : updated_files) {
                cout << file << " (обновлен в " << time_to_str(current_registry[file]) << ")\n";
            }
        }

        else if (choice == 3) {
            // Отображаем другие файлы
            cout << "\nОстальные файлы:\n";
            for (const auto& entry : current_registry) {
                if (find(new_files.begin(), new_files.end(), entry.first) == new_files.end() &&
                    find(updated_files.begin(), updated_files.end(), entry.first) == updated_files.end()) {
                    cout << entry.first << " (последнее изменение: " << time_to_str(entry.second) << ")\n";
                }
            }
        }

        else if (choice == 4) {
            // Выходим из программы
            cout << "Выход из программы...\n";
            break;
        }
        else {
            // Неверный выбор
            cout << "Неверный выбор. Попробуйте снова.\n";
        }

        // Сохраняем текущий реестр в файл
        save_registry(registry_filename, current_registry);

        // Обновляем предыдущий реестр
        if (first_run) {
            previous_registry = current_registry;
            first_run = false;
        }
    }

    // Возвращаем 0, чтобы указать успешное выполнение
    return 0;
}