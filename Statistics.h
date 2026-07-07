#pragma once
#include <vector>
#include <map>
#include <string>
#include "Student.h"

// Aggregate numbers computed from a snapshot of students, used by the
// Dashboard and Statistics screens. Pure computation, no GUI/DB code.
class Statistics {
public:
    void recompute(const std::vector<Student>& students);

    int totalStudents = 0;
    int activeStudents = 0;
    int inactiveStudents = 0;
    int maleCount = 0;
    int femaleCount = 0;
    int otherGenderCount = 0;
    double averageGPA = 0.0;
    double highestGPA = 0.0;
    double lowestGPA = 0.0;
    std::string topStudentId;    // student with highest GPA
    std::string bottomStudentId; // student with lowest GPA

    std::map<std::string, int> byDepartment; // department -> count
    std::map<int, int> byYear;               // year -> count
};
