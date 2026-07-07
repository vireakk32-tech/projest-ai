#pragma once
#include <string>

// Represents a single student record. Pure data + small helpers (encapsulated
// via private fields and accessors) - no DB or GUI logic lives here.
class Student {
public:
    Student() = default;

    // --- fields (kept public-ish via getters/setters for encapsulation) ---
    int id = 0;                    // internal DB row id (PRIMARY KEY)
    std::string studentId;         // externally-visible unique Student ID
    std::string firstName;
    std::string lastName;
    std::string gender;             // "Male" / "Female" / "Other"
    std::string dob;                // ISO date "YYYY-MM-DD"
    int age = 0;
    std::string phone;
    std::string email;
    std::string address;
    std::string department;
    std::string major;
    int year = 1;                   // 1-5
    double gpa = 0.0;                // 0.0 - 4.0
    std::string enrollmentDate;      // ISO date
    std::string status = "Active";   // "Active" / "Inactive"

    std::string fullName() const { return firstName + " " + lastName; }

    // Recompute age from dob (YYYY-MM-DD) relative to today's date.
    void recomputeAge();

    // Serialize / deserialize a single CSV line (comma separated, quoted if needed).
    std::string toCSVRow() const;
    static Student fromCSVRow(const std::string& row);
};
