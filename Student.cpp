#include "Student.h"
#include <ctime>
#include <sstream>
#include <vector>

void Student::recomputeAge() {
    if (dob.size() != 10) return; // expects YYYY-MM-DD
    int y = 0, m = 0, d = 0;
    if (sscanf(dob.c_str(), "%d-%d-%d", &y, &m, &d) != 3) return;

    time_t t = time(nullptr);
    tm* now = localtime(&t);
    int curY = now->tm_year + 1900;
    int curM = now->tm_mon + 1;
    int curD = now->tm_mday;

    int a = curY - y;
    if (curM < m || (curM == m && curD < d)) a--;
    if (a < 0) a = 0;
    age = a;
}

static std::string csvEscape(const std::string& field) {
    bool needsQuotes = field.find(',') != std::string::npos ||
                       field.find('"') != std::string::npos ||
                       field.find('\n') != std::string::npos;
    if (!needsQuotes) return field;
    std::string out = "\"";
    for (char c : field) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += "\"";
    return out;
}

std::string Student::toCSVRow() const {
    std::ostringstream ss;
    ss << csvEscape(studentId) << ","
       << csvEscape(firstName) << ","
       << csvEscape(lastName) << ","
       << csvEscape(gender) << ","
       << csvEscape(dob) << ","
       << age << ","
       << csvEscape(phone) << ","
       << csvEscape(email) << ","
       << csvEscape(address) << ","
       << csvEscape(department) << ","
       << csvEscape(major) << ","
       << year << ","
       << gpa << ","
       << csvEscape(enrollmentDate) << ","
       << csvEscape(status);
    return ss.str();
}

// Very small CSV parser sufficient for our own export format (handles quoted fields).
static std::vector<std::string> splitCSVLine(const std::string& line) {
    std::vector<std::string> out;
    std::string cur;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') { cur += '"'; ++i; }
                else inQuotes = false;
            } else cur += c;
        } else {
            if (c == '"') inQuotes = true;
            else if (c == ',') { out.push_back(cur); cur.clear(); }
            else cur += c;
        }
    }
    out.push_back(cur);
    return out;
}

Student Student::fromCSVRow(const std::string& row) {
    Student s;
    auto f = splitCSVLine(row);
    auto get = [&](size_t idx) -> std::string { return idx < f.size() ? f[idx] : ""; };
    s.studentId = get(0);
    s.firstName = get(1);
    s.lastName = get(2);
    s.gender = get(3);
    s.dob = get(4);
    try { s.age = get(5).empty() ? 0 : std::stoi(get(5)); } catch (...) { s.age = 0; }
    s.phone = get(6);
    s.email = get(7);
    s.address = get(8);
    s.department = get(9);
    s.major = get(10);
    try { s.year = get(11).empty() ? 1 : std::stoi(get(11)); } catch (...) { s.year = 1; }
    try { s.gpa = get(12).empty() ? 0.0 : std::stod(get(12)); } catch (...) { s.gpa = 0.0; }
    s.enrollmentDate = get(13);
    s.status = get(14).empty() ? "Active" : get(14);
    return s;
}
