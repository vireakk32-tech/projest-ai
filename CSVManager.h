#pragma once
#include <string>
#include <vector>
#include "Student.h"

// Handles reading/writing plain CSV files for bulk import/export.
// Column order matches Student::toCSVRow() / fromCSVRow().
class CSVManager {
public:
    // Throws std::runtime_error on file I/O failure.
    static void exportToFile(const std::string& path, const std::vector<Student>& students);

    // Returns parsed students; skips the header line if present. Malformed
    // lines are collected in `skippedLines` (1-based line numbers) instead of
    // throwing, so a partially-bad file still imports what it can.
    static std::vector<Student> importFromFile(const std::string& path,
                                                std::vector<int>& skippedLines);

    static const char* csvHeader() {
        return "student_id,first_name,last_name,gender,dob,age,phone,email,address,"
               "department,major,year,gpa,enrollment_date,status";
    }
};
