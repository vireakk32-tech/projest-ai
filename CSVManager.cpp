#include "CSVManager.h"
#include <fstream>
#include <stdexcept>

void CSVManager::exportToFile(const std::string& path, const std::vector<Student>& students) {
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out.is_open()) throw std::runtime_error("Could not open file for export: " + path);
    out << csvHeader() << "\n";
    for (const auto& s : students) out << s.toCSVRow() << "\n";
}

std::vector<Student> CSVManager::importFromFile(const std::string& path, std::vector<int>& skippedLines) {
    std::ifstream in(path);
    if (!in.is_open()) throw std::runtime_error("Could not open file for import: " + path);

    std::vector<Student> out;
    std::string line;
    int lineNo = 0;
    bool first = true;
    while (std::getline(in, line)) {
        lineNo++;
        if (line.empty()) continue;
        if (first) {
            first = false;
            // Skip header if it looks like our header (starts with "student_id")
            if (line.rfind("student_id", 0) == 0) continue;
        }
        Student s = Student::fromCSVRow(line);
        if (s.studentId.empty() || s.firstName.empty()) {
            skippedLines.push_back(lineNo);
            continue;
        }
        s.recomputeAge();
        out.push_back(s);
    }
    return out;
}
