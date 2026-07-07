#include "Validation.h"
#include "Database.h"
#include <regex>

bool Validation::isValidEmail(const std::string& email) {
    static const std::regex pattern(R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)");
    return std::regex_match(email, pattern);
}

bool Validation::isValidPhone(const std::string& phone) {
    // Accepts digits, spaces, +, -, () with 7-15 digits total.
    static const std::regex allowedChars(R"(^[0-9+\-() ]+$)");
    if (!std::regex_match(phone, allowedChars)) return false;
    int digits = 0;
    for (char c : phone) if (isdigit(static_cast<unsigned char>(c))) digits++;
    return digits >= 7 && digits <= 15;
}

bool Validation::isValidGPA(double gpa) {
    return gpa >= 0.0 && gpa <= 4.0;
}

bool Validation::isValidDate(const std::string& date) {
    static const std::regex pattern(R"(^\d{4}-\d{2}-\d{2}$)");
    if (!std::regex_match(date, pattern)) return false;
    int y, m, d;
    sscanf(date.c_str(), "%d-%d-%d", &y, &m, &d);
    if (m < 1 || m > 12 || d < 1 || d > 31) return false;
    return true;
}

bool Validation::isNonEmpty(const std::string& s) {
    return !s.empty();
}

ValidationResult Validation::validateStudent(const Student& s, Database& db, int excludeRowId) {
    ValidationResult r;

    if (!isNonEmpty(s.studentId)) r.fail("Student ID is required");
    else if (db.studentIdExists(s.studentId, excludeRowId)) r.fail("Student ID already exists");

    if (!isNonEmpty(s.firstName)) r.fail("First name is required");
    if (!isNonEmpty(s.lastName)) r.fail("Last name is required");
    if (!isNonEmpty(s.gender)) r.fail("Gender is required");

    if (!isNonEmpty(s.dob)) r.fail("Date of birth is required");
    else if (!isValidDate(s.dob)) r.fail("Date of birth must be YYYY-MM-DD");

    if (!isNonEmpty(s.phone)) r.fail("Phone number is required");
    else if (!isValidPhone(s.phone)) r.fail("Phone number is invalid");

    if (!isNonEmpty(s.email)) r.fail("Email is required");
    else if (!isValidEmail(s.email)) r.fail("Email is invalid");

    if (!isNonEmpty(s.department)) r.fail("Department is required");
    if (!isNonEmpty(s.major)) r.fail("Major is required");

    if (s.year < 1 || s.year > 6) r.fail("Year must be between 1 and 6");

    if (!isValidGPA(s.gpa)) r.fail("GPA must be between 0.0 and 4.0");

    if (!isNonEmpty(s.enrollmentDate)) r.fail("Enrollment date is required");
    else if (!isValidDate(s.enrollmentDate)) r.fail("Enrollment date must be YYYY-MM-DD");

    if (s.status != "Active" && s.status != "Inactive") r.fail("Status must be Active or Inactive");

    return r;
}
