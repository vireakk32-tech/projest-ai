#include "Database.h"
#include <filesystem>
#include <functional>
#include <sstream>
#include <cstring>

namespace fs = std::filesystem;

// NOTE on password storage: this is a student project, not a security
// product. We use a simple non-cryptographic hash (FNV-1a) purely so the
// password is not stored in plain text in the .db file. Do not reuse this
// scheme for anything real - a proper project would use bcrypt/argon2.
static std::string simpleHash(const std::string& input) {
    uint64_t hash = 14695981039346656037ULL; // FNV offset basis
    for (unsigned char c : input) {
        hash ^= c;
        hash *= 1099511628211ULL; // FNV prime
    }
    std::ostringstream ss;
    ss << std::hex << hash;
    return ss.str();
}

Database::Database() {}

Database::~Database() { close(); }

void Database::execOrThrow(const std::string& sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string msg = errMsg ? errMsg : "unknown sqlite error";
        sqlite3_free(errMsg);
        throw DatabaseException("SQL error: " + msg);
    }
}

void Database::open(const std::string& path) {
    path_ = path;
    fs::path p(path);
    if (p.has_parent_path() && !fs::exists(p.parent_path())) {
        fs::create_directories(p.parent_path());
    }
    int rc = sqlite3_open(path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::string msg = db_ ? sqlite3_errmsg(db_) : "cannot open database";
        throw DatabaseException("Failed to open database: " + msg);
    }
    execOrThrow("PRAGMA foreign_keys = ON;");
    createSchemaIfMissing();
    seedDefaultAdmin();
}

void Database::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

void Database::createSchemaIfMissing() {
    execOrThrow(R"SQL(
        CREATE TABLE IF NOT EXISTS students (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            student_id TEXT UNIQUE NOT NULL,
            first_name TEXT NOT NULL,
            last_name TEXT NOT NULL,
            gender TEXT,
            dob TEXT,
            age INTEGER,
            phone TEXT,
            email TEXT,
            address TEXT,
            department TEXT,
            major TEXT,
            year INTEGER,
            gpa REAL,
            enrollment_date TEXT,
            status TEXT
        );
    )SQL");

    execOrThrow("CREATE INDEX IF NOT EXISTS idx_students_dept ON students(department);");
    execOrThrow("CREATE INDEX IF NOT EXISTS idx_students_name ON students(last_name, first_name);");
    execOrThrow("CREATE INDEX IF NOT EXISTS idx_students_status ON students(status);");

    execOrThrow(R"SQL(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            role TEXT NOT NULL DEFAULT 'Admin'
        );
    )SQL");
}

void Database::seedDefaultAdmin() {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM users;", -1, &stmt, nullptr);
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    if (count == 0) {
        sqlite3_prepare_v2(db_,
            "INSERT INTO users (username, password_hash, role) VALUES (?, ?, ?);",
            -1, &stmt, nullptr);
        std::string hash = simpleHash("admin123");
        sqlite3_bind_text(stmt, 1, "admin", -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, "Admin", -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            throw DatabaseException("Failed to seed default admin account");
        }
        sqlite3_finalize(stmt);
    }
}

static void bindStudentFields(sqlite3_stmt* stmt, const Student& s, int startIdx = 1) {
    int i = startIdx;
    sqlite3_bind_text(stmt, i++, s.studentId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, s.firstName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, s.lastName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, s.gender.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, s.dob.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, i++, s.age);
    sqlite3_bind_text(stmt, i++, s.phone.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, s.email.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, s.address.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, s.department.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, s.major.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, i++, s.year);
    sqlite3_bind_double(stmt, i++, s.gpa);
    sqlite3_bind_text(stmt, i++, s.enrollmentDate.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, i++, s.status.c_str(), -1, SQLITE_TRANSIENT);
}

void Database::addStudent(const Student& s) {
    const char* sql =
        "INSERT INTO students (student_id, first_name, last_name, gender, dob, age, phone, "
        "email, address, department, major, year, gpa, enrollment_date, status) "
        "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw DatabaseException(sqlite3_errmsg(db_));
    bindStudentFields(stmt, s);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) throw DatabaseException(std::string("Failed to add student: ") + sqlite3_errmsg(db_));
}

void Database::updateStudent(int rowId, const Student& s) {
    const char* sql =
        "UPDATE students SET student_id=?, first_name=?, last_name=?, gender=?, dob=?, age=?, "
        "phone=?, email=?, address=?, department=?, major=?, year=?, gpa=?, enrollment_date=?, "
        "status=? WHERE id=?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw DatabaseException(sqlite3_errmsg(db_));
    bindStudentFields(stmt, s);
    sqlite3_bind_int(stmt, 16, rowId);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) throw DatabaseException(std::string("Failed to update student: ") + sqlite3_errmsg(db_));
}

void Database::deleteStudent(int rowId) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, "DELETE FROM students WHERE id=?;", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, rowId);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) throw DatabaseException(std::string("Failed to delete student: ") + sqlite3_errmsg(db_));
}

Student Database::rowToStudent(sqlite3_stmt* stmt) const {
    Student s;
    s.id = sqlite3_column_int(stmt, 0);
    auto text = [&](int col) -> std::string {
        const unsigned char* t = sqlite3_column_text(stmt, col);
        return t ? reinterpret_cast<const char*>(t) : "";
    };
    s.studentId = text(1);
    s.firstName = text(2);
    s.lastName = text(3);
    s.gender = text(4);
    s.dob = text(5);
    s.age = sqlite3_column_int(stmt, 6);
    s.phone = text(7);
    s.email = text(8);
    s.address = text(9);
    s.department = text(10);
    s.major = text(11);
    s.year = sqlite3_column_int(stmt, 12);
    s.gpa = sqlite3_column_double(stmt, 13);
    s.enrollmentDate = text(14);
    s.status = text(15);
    return s;
}

Student Database::getStudentByRowId(int rowId) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, "SELECT * FROM students WHERE id=?;", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, rowId);
    Student s;
    if (sqlite3_step(stmt) == SQLITE_ROW) s = rowToStudent(stmt);
    else { sqlite3_finalize(stmt); throw DatabaseException("Student not found"); }
    sqlite3_finalize(stmt);
    return s;
}

std::vector<Student> Database::getAllStudents() {
    std::vector<Student> out;
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, "SELECT * FROM students ORDER BY id;", -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) out.push_back(rowToStudent(stmt));
    sqlite3_finalize(stmt);
    return out;
}

std::vector<Student> Database::searchStudents(const std::string& query) const {
    std::vector<Student> out;
    std::string like = "%" + query + "%";
    const char* sql =
        "SELECT * FROM students WHERE student_id LIKE ?1 OR first_name LIKE ?1 OR "
        "last_name LIKE ?1 OR department LIKE ?1 OR major LIKE ?1 OR status LIKE ?1 "
        "OR CAST(year AS TEXT) LIKE ?1 OR CAST(gpa AS TEXT) LIKE ?1 ORDER BY id;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, like.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(stmt) == SQLITE_ROW) out.push_back(rowToStudent(stmt));
    sqlite3_finalize(stmt);
    return out;
}

std::vector<Student> Database::queryStudents(const StudentFilter& f, SortField sortField, SortOrder order) const {
    std::vector<Student> out;
    std::ostringstream sql;
    sql << "SELECT * FROM students WHERE 1=1 ";
    if (!f.department.empty()) sql << "AND department = ? ";
    if (!f.major.empty()) sql << "AND major = ? ";
    if (!f.gender.empty()) sql << "AND gender = ? ";
    if (!f.status.empty()) sql << "AND status = ? ";
    if (f.useGpaRange) sql << "AND gpa BETWEEN ? AND ? ";

    switch (sortField) {
        case SortField::StudentId: sql << "ORDER BY student_id "; break;
        case SortField::Name: sql << "ORDER BY last_name, first_name "; break;
        case SortField::GPA: sql << "ORDER BY gpa "; break;
        case SortField::Department: sql << "ORDER BY department "; break;
        case SortField::Year: sql << "ORDER BY year "; break;
    }
    sql << (order == SortOrder::Ascending ? "ASC" : "DESC") << ";";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        throw DatabaseException(sqlite3_errmsg(db_));

    int idx = 1;
    if (!f.department.empty()) sqlite3_bind_text(stmt, idx++, f.department.c_str(), -1, SQLITE_TRANSIENT);
    if (!f.major.empty()) sqlite3_bind_text(stmt, idx++, f.major.c_str(), -1, SQLITE_TRANSIENT);
    if (!f.gender.empty()) sqlite3_bind_text(stmt, idx++, f.gender.c_str(), -1, SQLITE_TRANSIENT);
    if (!f.status.empty()) sqlite3_bind_text(stmt, idx++, f.status.c_str(), -1, SQLITE_TRANSIENT);
    if (f.useGpaRange) {
        sqlite3_bind_double(stmt, idx++, f.gpaMin);
        sqlite3_bind_double(stmt, idx++, f.gpaMax);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) out.push_back(rowToStudent(stmt));
    sqlite3_finalize(stmt);
    return out;
}

bool Database::studentIdExists(const std::string& studentId, int excludeRowId) const {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, "SELECT id FROM students WHERE student_id = ?;", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, studentId.c_str(), -1, SQLITE_TRANSIENT);
    bool exists = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        if (id != excludeRowId) { exists = true; break; }
    }
    sqlite3_finalize(stmt);
    return exists;
}

bool Database::verifyLogin(const std::string& username, const std::string& password, std::string& outRole) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, "SELECT password_hash, role FROM users WHERE username = ?;", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string storedHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (storedHash == simpleHash(password)) {
            ok = true;
            outRole = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        }
    }
    sqlite3_finalize(stmt);
    return ok;
}

void Database::changePassword(const std::string& username, const std::string& newPassword) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, "UPDATE users SET password_hash=? WHERE username=?;", -1, &stmt, nullptr);
    std::string hash = simpleHash(newPassword);
    sqlite3_bind_text(stmt, 1, hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) throw DatabaseException("Failed to change password");
}

void Database::backupTo(const std::string& destPath) {
    sqlite3* destDb;
    if (sqlite3_open(destPath.c_str(), &destDb) != SQLITE_OK) {
        throw DatabaseException("Cannot create backup file");
    }
    sqlite3_backup* backup = sqlite3_backup_init(destDb, "main", db_, "main");
    if (backup) {
        sqlite3_backup_step(backup, -1);
        sqlite3_backup_finish(backup);
    }
    int rc = sqlite3_errcode(destDb);
    sqlite3_close(destDb);
    if (rc != SQLITE_OK && rc != SQLITE_DONE) throw DatabaseException("Backup failed");
}

void Database::restoreFrom(const std::string& srcPath) {
    sqlite3* srcDb;
    if (sqlite3_open(srcPath.c_str(), &srcDb) != SQLITE_OK) {
        throw DatabaseException("Cannot open backup file");
    }
    sqlite3_backup* backup = sqlite3_backup_init(db_, "main", srcDb, "main");
    if (backup) {
        sqlite3_backup_step(backup, -1);
        sqlite3_backup_finish(backup);
    }
    sqlite3_close(srcDb);
}
