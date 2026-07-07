#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include "Student.h"
#include "sqlite3.h"

// Thrown for any SQLite failure so callers can show a friendly popup instead
// of crashing.
class DatabaseException : public std::runtime_error {
public:
    explicit DatabaseException(const std::string& msg) : std::runtime_error(msg) {}
};

// Simple user record for the login system.
struct UserAccount {
    std::string username;
    std::string passwordHash;
    std::string role; // "Admin" / "Teacher"
};

enum class SortField { StudentId, Name, GPA, Department, Year };
enum class SortOrder { Ascending, Descending };

// Optional filter criteria applied server-side (in SQL) for speed at 10k+ rows.
struct StudentFilter {
    std::string department;   // empty = any
    std::string major;        // empty = any
    std::string gender;       // empty = any
    std::string status;       // empty = any
    double gpaMin = 0.0;
    double gpaMax = 4.0;
    bool useGpaRange = false;
};

// Encapsulates every SQLite interaction. GUI/Statistics/CSVManager never
// touch sqlite3* directly - they go through this class.
class Database {
public:
    Database();
    ~Database();

    void open(const std::string& path); // creates file + schema if missing
    void close();

    // --- CRUD ---
    void addStudent(const Student& s);
    void updateStudent(int rowId, const Student& s);
    void deleteStudent(int rowId);
    Student getStudentByRowId(int rowId);

    // --- Queries ---
    std::vector<Student> getAllStudents();
    std::vector<Student> searchStudents(const std::string& query) const; // matches id/name/dept/major
    std::vector<Student> queryStudents(const StudentFilter& filter,
                                        SortField sortField,
                                        SortOrder order) const;
    bool studentIdExists(const std::string& studentId, int excludeRowId = -1) const;

    // --- Users / login ---
    bool verifyLogin(const std::string& username, const std::string& password, std::string& outRole);
    void changePassword(const std::string& username, const std::string& newPassword);

    // --- Backup / restore ---
    void backupTo(const std::string& destPath);
    void restoreFrom(const std::string& srcPath);

    std::string dbPath() const { return path_; }

private:
    sqlite3* db_ = nullptr;
    std::string path_;

    void execOrThrow(const std::string& sql);
    void createSchemaIfMissing();
    void seedDefaultAdmin();
    Student rowToStudent(sqlite3_stmt* stmt) const;
};
