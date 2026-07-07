#pragma once
#include <string>
#include <vector>
#include "Student.h"

// Result of validating a student record: ok flag + human readable messages
// for every field that failed, so the GUI can show them all at once.
struct ValidationResult {
    bool ok = true;
    std::vector<std::string> errors;

    void fail(const std::string& msg) {
        ok = false;
        errors.push_back(msg);
    }

    std::string joined() const {
        std::string out;
        for (auto& e : errors) { out += "- " + e + "\n"; }
        return out;
    }
};

class Database; // fwd decl

// Stateless static validation helpers used both by the GUI (live field
// feedback) and before any Database write (defense in depth).
class Validation {
public:
    static bool isValidEmail(const std::string& email);
    static bool isValidPhone(const std::string& phone);
    static bool isValidGPA(double gpa);
    static bool isValidDate(const std::string& date); // YYYY-MM-DD
    static bool isNonEmpty(const std::string& s);

    // Full-record validation. `db` + `excludeRowId` are used to check
    // Student ID uniqueness (excludeRowId lets edits pass against themselves).
    static ValidationResult validateStudent(const Student& s, Database& db, int excludeRowId = -1);
};
