#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <locale>
#include <codecvt>
#include <sstream>
#include <algorithm>

const std::wstring inputFile = L"input.txt";
const std::wstring outputFile = L"output.txt";
const std::vector<std::wstring> keywords = { L"шарлотка", L"рецепт" };
const int threadCount = 4;

std::map<std::wstring, int> wordCount;
std::mutex countMutex;
std::vector<std::wstring> lines;

// Приведение строки к нижнему регистру
std::wstring toLower(std::wstring str) {
    std::transform(str.begin(), str.end(), str.begin(), towlower);
    return str;
}

// Очистка слова от знаков препинания
std::wstring cleanWord(const std::wstring& word) {
    std::wstring clean;
    for (wchar_t ch : word) {
        if (iswalpha(ch)) {
            clean += ch;
        }
    }
    return clean;
}

// Функция поиска ключевых слов в строках
DWORD WINAPI SearchKeywords(LPVOID param) {
    int threadNum = *(int*)param;

    int linesPerThread = lines.size() / threadCount;
    int start = threadNum * linesPerThread;
    int end = (threadNum == threadCount - 1) ? lines.size() : start + linesPerThread;

    std::map<std::wstring, int> localCount;

    for (int i = start; i < end; ++i) {
        std::wstring line = toLower(lines[i]);
        std::wstringstream ss(line);
        std::wstring word;

        while (ss >> word) {
            word = cleanWord(word);

            if (!word.empty()) {
                for (const std::wstring& keyword : keywords) {
                    if (word == keyword) {
                        localCount[keyword]++;
                    }
                }
            }
        }
    }

    std::lock_guard<std::mutex> guard(countMutex);
    for (const auto& pair : localCount) {
        wordCount[pair.first] += pair.second;
    }

    return 0;
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    std::wcout.imbue(std::locale(""));

    std::wifstream file(inputFile);
    file.imbue(std::locale(file.getloc(), new std::codecvt_utf8<wchar_t>));

    if (!file) {
        std::wcerr << L"Ошибка открытия файла!\n";
        return 1;
    }

    std::wstring line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    file.close();

    HANDLE threads[threadCount];
    int threadIds[threadCount];

    for (int i = 0; i < threadCount; ++i) {
        threadIds[i] = i;
        threads[i] = CreateThread(nullptr, 0, SearchKeywords, &threadIds[i], 0, nullptr);
        if (!threads[i]) {
            std::wcerr << L"Ошибка создания потока " << i << L"!\n";
            return 1;
        }
    }

    WaitForMultipleObjects(threadCount, threads, TRUE, INFINITE);

    for (int i = 0; i < threadCount; ++i) {
        CloseHandle(threads[i]);
    }

    std::wofstream outFile(outputFile);
    outFile.imbue(std::locale(outFile.getloc(), new std::codecvt_utf8<wchar_t>));

    for (const auto& pair : wordCount) {
        outFile << pair.first << L": " << pair.second << L"\n";
    }

    outFile.close();
    std::wcout << L"Анализ завершён. Результаты записаны в output.txt\n";

    system("pause");
    return 0;
}