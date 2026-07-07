#pragma once
#include <string>
#include <vector>
#include <array>
#include <map>
#include "imgui.h"
#include "Database.h"
#include "Statistics.h"
#include "Validation.h"

enum class Screen { Dashboard, Students, Search, Stats, Settings, About };

struct Toast {
    std::string message;
    ImVec4 color;
    float timeLeft; // seconds
};

// Editable in-memory form used by the Add/Edit popup. Uses fixed char
// buffers because ImGui::InputText needs raw buffers.
struct StudentForm {
    char studentId[32] = "";
    char firstName[64] = "";
    char lastName[64] = "";
    int genderIdx = 0;   // 0 Male, 1 Female, 2 Other
    char dob[16] = "";
    char phone[32] = "";
    char email[64] = "";
    char address[128] = "";
    char department[64] = "";
    char major[64] = "";
    int year = 1;
    float gpa = 0.0f;
    char enrollmentDate[16] = "";
    int statusIdx = 0; // 0 Active, 1 Inactive

    void loadFrom(const Student& s);
    Student toStudent() const;
};

// Owns all UI state and renders every frame. This is the only class that
// touches ImGui widget calls directly.
class GUI {
public:
    explicit GUI(Database& db);

    void render(); // call once per frame from main.cpp

private:
    Database& db_;
    Statistics stats_;

    // session / auth
    bool loggedIn_ = false;
    std::string currentUser_;
    std::string currentRole_;
    char loginUser_[64] = "";
    char loginPass_[64] = "";
    std::string loginError_;

    Screen screen_ = Screen::Dashboard;

    // data cache
    std::vector<Student> students_;
    bool dirty_ = true;
    bool statsDirty_ = true; // dashboard/statistics recompute only when data changed

    // search / sort / filter
    char searchBuffer_[128] = "";
    SortField sortField_ = SortField::StudentId;
    SortOrder sortOrder_ = SortOrder::Ascending;
    StudentFilter filter_;
    int filterDeptIdx_ = 0, filterMajorIdx_ = 0, filterGenderIdx_ = 0, filterStatusIdx_ = 0;
    float gpaRange_[2] = {0.0f, 4.0f};
    bool useGpaFilter_ = false;

    // popups
    bool showAddEdit_ = false;
    bool isEditMode_ = false;
    int editingRowId_ = -1;
    StudentForm form_;
    std::string formError_;

    bool showDeleteConfirm_ = false;
    int deleteRowId_ = -1;
    std::string deleteLabel_;

    bool showViewPopup_ = false;
    Student viewStudent_;

    // csv / backup path buffers
    char csvPath_[256] = "database/export.csv";
    char backupPath_[256] = "database/backup.db";

    // settings
    bool darkTheme_ = true;
    float fontScale_ = 1.0f;
    char newPassword_[64] = "";
    char newPasswordConfirm_[64] = "";

    std::vector<Toast> toasts_;

    // --- lifecycle helpers ---
    void refreshStudents();
    void loadSettings();
    void saveSettings();
    void applyTheme();
    void pushToast(const std::string& msg, ImVec4 color);

    // --- screens ---
    void renderLoginScreen();
    void renderSidebar();
    void renderDashboard();
    void renderStudentManagement();
    void renderSearchScreen();
    void renderStatisticsScreen();
    void renderSettingsScreen();
    void renderAboutScreen();

    // --- shared widgets ---
    void renderStudentTable(std::vector<Student>& list, bool showActions);
    void renderAddEditPopup();
    void renderDeleteConfirmPopup();
    void renderViewPopup();
    void renderToasts();
    void renderStatCard(const char* label, const std::string& value, ImVec4 accent);
    void drawBarChart(const std::map<std::string, int>& data, const char* title);
    void drawPieChart(const std::map<std::string, int>& data, const char* title);
};
