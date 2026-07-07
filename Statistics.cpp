#include "Statistics.h"
#include <limits>

void Statistics::recompute(const std::vector<Student>& students) {
    totalStudents = static_cast<int>(students.size());
    activeStudents = inactiveStudents = 0;
    maleCount = femaleCount = otherGenderCount = 0;
    averageGPA = 0.0;
    highestGPA = 0.0;
    lowestGPA = std::numeric_limits<double>::max();
    topStudentId.clear();
    bottomStudentId.clear();
    byDepartment.clear();
    byYear.clear();

    if (students.empty()) {
        lowestGPA = 0.0;
        return;
    }

    double gpaSum = 0.0;
    for (const auto& s : students) {
        if (s.status == "Active") activeStudents++;
        else inactiveStudents++;

        if (s.gender == "Male") maleCount++;
        else if (s.gender == "Female") femaleCount++;
        else otherGenderCount++;

        gpaSum += s.gpa;
        if (s.gpa > highestGPA) { highestGPA = s.gpa; topStudentId = s.studentId; }
        if (s.gpa < lowestGPA) { lowestGPA = s.gpa; bottomStudentId = s.studentId; }

        byDepartment[s.department]++;
        byYear[s.year]++;
    }
    averageGPA = gpaSum / static_cast<double>(students.size());
}
