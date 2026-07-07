#include "GUI.h"
#include "CSVManager.h"
#include "imgui.h"
#include <cstring>
#include <fstream>
#include <algorithm>
#include <cmath>

static const char* kDepartments[] = {
    "Computer Science", "Engineering", "Business", "Mathematics",
    "Physics", "Biology", "Arts", "Medicine"
};
static const char* kGenders[] = { "Male", "Female", "Other" };
static const char* kStatuses[] = { "Active", "Inactive" };
static const char* kSortFields[] = { "Student ID", "Name", "GPA", "Department", "Year" };

static const ImVec4 kAccent      = ImVec4(0.30f, 0.55f, 0.95f, 1.0f);
static const ImVec4 kAccentGreen = ImVec4(0.25f, 0.70f, 0.45f, 1.0f);
static const ImVec4 kAccentRed   = ImVec4(0.85f, 0.30f, 0.30f, 1.0f);
static const ImVec4 kAccentGold  = ImVec4(0.85f, 0.65f, 0.15f, 1.0f);

// ---------------------------------------------------------------------
// StudentForm
// ---------------------------------------------------------------------
static void copyToBuf(char* buf, size_t bufSize, const std::string& s) {
    std::snprintf(buf, bufSize, "%s", s.c_str());
}

void StudentForm::loadFrom(const Student& s) {
    copyToBuf(studentId, sizeof(studentId), s.studentId);
    copyToBuf(firstName, sizeof(firstName), s.firstName);
    copyToBuf(lastName, sizeof(lastName), s.lastName);
    genderIdx = (s.gender == "Female") ? 1 : (s.gender == "Other") ? 2 : 0;
    copyToBuf(dob, sizeof(dob), s.dob);
    copyToBuf(phone, sizeof(phone), s.phone);
    copyToBuf(email, sizeof(email), s.email);
    copyToBuf(address, sizeof(address), s.address);
    copyToBuf(department, sizeof(department), s.department);
    copyToBuf(major, sizeof(major), s.major);
    year = s.year;
    gpa = static_cast<float>(s.gpa);
    copyToBuf(enrollmentDate, sizeof(enrollmentDate), s.enrollmentDate);
    statusIdx = (s.status == "Inactive") ? 1 : 0;
}

Student StudentForm::toStudent() const {
    Student s;
    s.studentId = studentId;
    s.firstName = firstName;
    s.lastName = lastName;
    s.gender = kGenders[genderIdx];
    s.dob = dob;
    s.phone = phone;
    s.email = email;
    s.address = address;
    s.department = department;
    s.major = major;
    s.year = year;
    s.gpa = gpa;
    s.enrollmentDate = enrollmentDate;
    s.status = kStatuses[statusIdx];
    s.recomputeAge();
    return s;
}

// ---------------------------------------------------------------------
// GUI lifecycle
// ---------------------------------------------------------------------
GUI::GUI(Database& db) : db_(db) {
    loadSettings();
    applyTheme();
}

void GUI::pushToast(const std::string& msg, ImVec4 color) {
    toasts_.push_back({ msg, color, 3.0f });
}

void GUI::refreshStudents() {
    if (searchBuffer_[0] != '\0') {
        students_ = db_.searchStudents(searchBuffer_);
    } else {
        filter_.useGpaRange = useGpaFilter_;
        filter_.gpaMin = gpaRange_[0];
        filter_.gpaMax = gpaRange_[1];
        students_ = db_.queryStudents(filter_, sortField_, sortOrder_);
    }
    dirty_ = false;
}

static const char* kSettingsPath = "database/settings.cfg";

void GUI::loadSettings() {
    std::ifstream in(kSettingsPath);
    if (!in.is_open()) return;
    std::string key;
    while (in >> key) {
        if (key == "theme") { std::string v; in >> v; darkTheme_ = (v == "dark"); }
        else if (key == "fontScale") { in >> fontScale_; }
        else { std::string skip; in >> skip; }
    }
}

void GUI::saveSettings() {
    std::ofstream out(kSettingsPath, std::ios::trunc);
    out << "theme " << (darkTheme_ ? "dark" : "light") << "\n";
    out << "fontScale " << fontScale_ << "\n";
}

void GUI::applyTheme() {
    if (darkTheme_) ImGui::StyleColorsDark();
    else ImGui::StyleColorsLight();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;
    style.WindowPadding = ImVec2(12, 12);
    style.FramePadding = ImVec2(8, 5);
    style.ItemSpacing = ImVec2(10, 8);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Button] = kAccent;
    colors[ImGuiCol_ButtonHovered] = ImVec4(kAccent.x + 0.08f, kAccent.y + 0.08f, kAccent.z + 0.08f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(kAccent.x - 0.05f, kAccent.y - 0.05f, kAccent.z - 0.05f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.45f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.65f);
    colors[ImGuiCol_HeaderActive] = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.85f);
}

// ---------------------------------------------------------------------
// Login screen
// ---------------------------------------------------------------------
void GUI::renderLoginScreen() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                             ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(380, 280), ImGuiCond_Always);
    ImGui::Begin("Login", nullptr,
                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

    ImGui::TextColored(kAccent, "Student Database Management System");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Username");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##username", loginUser_, sizeof(loginUser_));

    ImGui::Text("Password");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##password", loginPass_, sizeof(loginPass_), ImGuiInputTextFlags_Password);

    ImGui::Spacing();
    if (ImGui::Button("Login", ImVec2(-1, 36)) ||
        (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))) {
        std::string role;
        if (db_.verifyLogin(loginUser_, loginPass_, role)) {
            loggedIn_ = true;
            currentUser_ = loginUser_;
            currentRole_ = role;
            loginError_.clear();
            dirty_ = true;
            pushToast("Welcome back, " + currentUser_ + "!", kAccentGreen);
        } else {
            loginError_ = "Invalid username or password";
        }
    }

    if (!loginError_.empty()) {
        ImGui::Spacing();
        ImGui::TextColored(kAccentRed, "%s", loginError_.c_str());
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Default account: admin / admin123");
    ImGui::End();
}

// ---------------------------------------------------------------------
// Sidebar
// ---------------------------------------------------------------------
void GUI::renderSidebar() {
    ImGui::BeginChild("Sidebar", ImVec2(200, 0), true);

    ImGui::TextColored(kAccent, "StudentDB");
    ImGui::TextDisabled("%s (%s)", currentUser_.c_str(), currentRole_.c_str());
    ImGui::Separator();
    ImGui::Spacing();

    struct NavItem { const char* label; Screen screen; };
    static const NavItem items[] = {
        { "Dashboard", Screen::Dashboard },
        { "Student Management", Screen::Students },
        { "Search Student", Screen::Search },
        { "Statistics", Screen::Stats },
        { "Settings", Screen::Settings },
        { "About", Screen::About },
    };

    for (const auto& item : items) {
        bool selected = (screen_ == item.screen);
        if (ImGui::Selectable(item.label, selected, 0, ImVec2(0, 32))) {
            screen_ = item.screen;
            dirty_ = true;
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    if (ImGui::Button("Logout", ImVec2(-1, 30))) {
        loggedIn_ = false;
        std::memset(loginPass_, 0, sizeof(loginPass_));
    }

    ImGui::EndChild();
}

// ---------------------------------------------------------------------
// Stat card + charts
// ---------------------------------------------------------------------
void GUI::renderStatCard(const char* label, const std::string& value, ImVec4 accent) {
    ImGui::BeginGroup();
    ImGui::BeginChild(label, ImVec2(180, 80), true);
    ImGui::TextDisabled("%s", label);
    ImGui::TextColored(accent, "%s", value.c_str());
    ImGui::EndChild();
    ImGui::EndGroup();
}

void GUI::drawBarChart(const std::map<std::string, int>& data, const char* title) {
    ImGui::TextColored(kAccent, "%s", title);
    if (data.empty()) { ImGui::TextDisabled("No data"); return; }

    int maxCount = 1;
    for (auto& [k, v] : data) maxCount = std::max(maxCount, v);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();
    float chartH = 180.0f;
    float barW = 42.0f;
    float gap = 18.0f;
    float x = origin.x + 10;

    for (auto& [dept, count] : data) {
        float h = chartH * (static_cast<float>(count) / maxCount);
        ImVec2 topLeft(x, origin.y + (chartH - h));
        ImVec2 botRight(x + barW, origin.y + chartH);
        draw->AddRectFilled(topLeft, botRight, ImGui::ColorConvertFloat4ToU32(kAccent), 4.0f);

        char buf[16];
        snprintf(buf, sizeof(buf), "%d", count);
        ImVec2 textSize = ImGui::CalcTextSize(buf);
        draw->AddText(ImVec2(x + barW * 0.5f - textSize.x * 0.5f, topLeft.y - 18), IM_COL32_WHITE, buf);

        // department label (abbreviated), drawn rotated-free below the bar
        std::string label = dept.substr(0, 4);
        ImVec2 lblSize = ImGui::CalcTextSize(label.c_str());
        draw->AddText(ImVec2(x + barW * 0.5f - lblSize.x * 0.5f, origin.y + chartH + 4),
                       IM_COL32(200, 200, 200, 255), label.c_str());

        x += barW + gap;
    }

    ImGui::Dummy(ImVec2(x - origin.x, chartH + 24));
}

void GUI::drawPieChart(const std::map<std::string, int>& data, const char* title) {
    ImGui::TextColored(kAccent, "%s", title);
    if (data.empty()) { ImGui::TextDisabled("No data"); return; }

    int total = 0;
    for (auto& [k, v] : data) total += v;
    if (total == 0) { ImGui::TextDisabled("No data"); return; }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();
    float radius = 90.0f;
    ImVec2 center(origin.x + radius + 10, origin.y + radius + 10);

    static const ImU32 palette[] = {
        IM_COL32(66, 133, 244, 255), IM_COL32(219, 68, 55, 255),
        IM_COL32(244, 180, 0, 255),  IM_COL32(15, 157, 88, 255),
        IM_COL32(171, 71, 188, 255), IM_COL32(0, 172, 193, 255),
        IM_COL32(255, 112, 67, 255), IM_COL32(158, 157, 36, 255),
    };

    float startAngle = 0.0f;
    int colorIdx = 0;
    for (auto& [dept, count] : data) {
        float frac = static_cast<float>(count) / total;
        float endAngle = startAngle + frac * 2.0f * 3.14159265f;

        draw->PathArcTo(center, radius, startAngle, endAngle, 32);
        draw->PathLineTo(center);
        draw->PathFillConvex(palette[colorIdx % 8]);

        startAngle = endAngle;
        colorIdx++;
    }

    ImGui::Dummy(ImVec2(radius * 2 + 20, radius * 2 + 20));
    ImGui::SameLine();
    ImGui::BeginGroup();
    colorIdx = 0;
    for (auto& [dept, count] : data) {
        ImVec4 c = ImGui::ColorConvertU32ToFloat4(palette[colorIdx % 8]);
        ImGui::TextColored(c, "\xe2\x96\xa0"); // filled square glyph
        ImGui::SameLine();
        ImGui::Text("%s: %d", dept.c_str(), count);
        colorIdx++;
    }
    ImGui::EndGroup();
}

// ---------------------------------------------------------------------
// Dashboard
// ---------------------------------------------------------------------
void GUI::renderDashboard() {
    if (statsDirty_) {
        auto all = db_.getAllStudents();
        stats_.recompute(all);
        statsDirty_ = false;
    }

    ImGui::TextColored(kAccent, "Dashboard");
    ImGui::Separator();
    ImGui::Spacing();

    renderStatCard("Total Students", std::to_string(stats_.totalStudents), kAccent);
    ImGui::SameLine();
    renderStatCard("Active Students", std::to_string(stats_.activeStudents), kAccentGreen);
    ImGui::SameLine();
    char gpaBuf[16];
    snprintf(gpaBuf, sizeof(gpaBuf), "%.2f", stats_.averageGPA);
    renderStatCard("Average GPA", gpaBuf, kAccentGold);

    snprintf(gpaBuf, sizeof(gpaBuf), "%.2f", stats_.highestGPA);
    renderStatCard("Highest GPA", gpaBuf, kAccentGreen);
    ImGui::SameLine();
    snprintf(gpaBuf, sizeof(gpaBuf), "%.2f", stats_.lowestGPA);
    renderStatCard("Lowest GPA", gpaBuf, kAccentRed);
    ImGui::SameLine();
    renderStatCard("Inactive Students", std::to_string(stats_.inactiveStudents), kAccentRed);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Columns(2, nullptr, false);
    drawBarChart(stats_.byDepartment, "Students by Department (Bar)");
    ImGui::NextColumn();
    drawPieChart(stats_.byDepartment, "Students by Department (Pie)");
    ImGui::Columns(1);
}

// ---------------------------------------------------------------------
// Student table (shared by Student Management + Search screens)
// ---------------------------------------------------------------------
void GUI::renderStudentTable(std::vector<Student>& list, bool showActions) {
    ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                             ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable;
    int columns = showActions ? 9 : 8;
    if (ImGui::BeginTable("students_table", columns, flags, ImVec2(0, 420))) {
        ImGui::TableSetupColumn("Student ID");
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Department");
        ImGui::TableSetupColumn("Major");
        ImGui::TableSetupColumn("Year");
        ImGui::TableSetupColumn("GPA");
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("Email");
        if (showActions) ImGui::TableSetupColumn("Actions");
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(list.size()));
        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                Student& s = list[row];
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::PushID(s.id);
                ImGui::TextUnformatted(s.studentId.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(s.fullName().c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(s.department.c_str());
                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(s.major.c_str());
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%d", s.year);
                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%.2f", s.gpa);
                ImGui::TableSetColumnIndex(6);
                ImGui::TextColored(s.status == "Active" ? kAccentGreen : kAccentRed, "%s", s.status.c_str());
                ImGui::TableSetColumnIndex(7);
                ImGui::TextUnformatted(s.email.c_str());

                if (showActions) {
                    ImGui::TableSetColumnIndex(8);
                    if (ImGui::SmallButton("View")) { viewStudent_ = s; showViewPopup_ = true; }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Edit")) {
                        form_.loadFrom(s);
                        editingRowId_ = s.id;
                        isEditMode_ = true;
                        formError_.clear();
                        showAddEdit_ = true;
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Delete")) {
                        deleteRowId_ = s.id;
                        deleteLabel_ = s.fullName() + " (" + s.studentId + ")";
                        showDeleteConfirm_ = true;
                    }
                }
                ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }
}

// ---------------------------------------------------------------------
// Student Management screen
// ---------------------------------------------------------------------
void GUI::renderStudentManagement() {
    ImGui::TextColored(kAccent, "Student Management");
    ImGui::Separator();

    if (ImGui::Button("+ Add Student")) {
        form_ = StudentForm{};
        isEditMode_ = false;
        editingRowId_ = -1;
        formError_.clear();
        showAddEdit_ = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Refresh")) dirty_ = true;

    ImGui::Spacing();

    // --- Sort controls ---
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Sort by:");
    ImGui::SameLine();
    int sortIdx = static_cast<int>(sortField_);
    ImGui::SetNextItemWidth(160);
    if (ImGui::Combo("##sortfield", &sortIdx, kSortFields, IM_ARRAYSIZE(kSortFields))) {
        sortField_ = static_cast<SortField>(sortIdx);
        dirty_ = true;
    }
    ImGui::SameLine();
    bool asc = (sortOrder_ == SortOrder::Ascending);
    if (ImGui::RadioButton("Ascending", asc)) { sortOrder_ = SortOrder::Ascending; dirty_ = true; }
    ImGui::SameLine();
    if (ImGui::RadioButton("Descending", !asc)) { sortOrder_ = SortOrder::Descending; dirty_ = true; }

    // --- Filter controls ---
    ImGui::Spacing();
    ImGui::Text("Filters:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(160);
    static std::vector<std::string> deptOptions;
    {
        std::vector<const char*> opts = { "Any" };
        for (auto d : kDepartments) opts.push_back(d);
        if (ImGui::Combo("##fdept", &filterDeptIdx_, opts.data(), static_cast<int>(opts.size()))) {
            filter_.department = filterDeptIdx_ == 0 ? "" : kDepartments[filterDeptIdx_ - 1];
            dirty_ = true;
        }
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    {
        const char* opts[] = { "Any", "Male", "Female", "Other" };
        if (ImGui::Combo("##fgender", &filterGenderIdx_, opts, IM_ARRAYSIZE(opts))) {
            filter_.gender = filterGenderIdx_ == 0 ? "" : opts[filterGenderIdx_];
            dirty_ = true;
        }
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    {
        const char* opts[] = { "Any", "Active", "Inactive" };
        if (ImGui::Combo("##fstatus", &filterStatusIdx_, opts, IM_ARRAYSIZE(opts))) {
            filter_.status = filterStatusIdx_ == 0 ? "" : opts[filterStatusIdx_];
            dirty_ = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("GPA range", &useGpaFilter_)) dirty_ = true;
    ImGui::SameLine();
    ImGui::SetNextItemWidth(160);
    if (ImGui::DragFloatRange2("##gpaRange", &gpaRange_[0], &gpaRange_[1], 0.01f, 0.0f, 4.0f, "Min %.2f", "Max %.2f")) {
        dirty_ = true;
    }

    ImGui::Spacing();
    if (dirty_) refreshStudents();
    ImGui::Text("%zu student(s) found", students_.size());
    renderStudentTable(students_, true);

    renderAddEditPopup();
    renderDeleteConfirmPopup();
    renderViewPopup();
}

// ---------------------------------------------------------------------
// Search screen
// ---------------------------------------------------------------------
void GUI::renderSearchScreen() {
    ImGui::TextColored(kAccent, "Search Student");
    ImGui::Separator();
    ImGui::Text("Search by ID, Name, Department, Major, Year, GPA, or Status:");
    ImGui::SetNextItemWidth(400);
    if (ImGui::InputTextWithHint("##search", "Type to search instantly...", searchBuffer_, sizeof(searchBuffer_))) {
        dirty_ = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear")) { searchBuffer_[0] = '\0'; dirty_ = true; }

    ImGui::Spacing();
    if (dirty_) refreshStudents();
    ImGui::Text("%zu result(s)", students_.size());
    renderStudentTable(students_, true);

    renderAddEditPopup();
    renderDeleteConfirmPopup();
    renderViewPopup();
}

// ---------------------------------------------------------------------
// Statistics screen
// ---------------------------------------------------------------------
void GUI::renderStatisticsScreen() {
    if (statsDirty_) {
        auto all = db_.getAllStudents();
        stats_.recompute(all);
        statsDirty_ = false;
    }

    ImGui::TextColored(kAccent, "Statistics");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Total Students: %d", stats_.totalStudents);
    ImGui::Text("Active: %d   Inactive: %d", stats_.activeStudents, stats_.inactiveStudents);
    ImGui::Text("Male: %d   Female: %d   Other: %d", stats_.maleCount, stats_.femaleCount, stats_.otherGenderCount);
    if (stats_.totalStudents > 0) {
        float maleRatio = 100.0f * stats_.maleCount / stats_.totalStudents;
        float femaleRatio = 100.0f * stats_.femaleCount / stats_.totalStudents;
        ImGui::Text("Male/Female ratio: %.1f%% / %.1f%%", maleRatio, femaleRatio);
    }
    ImGui::Text("Average GPA: %.2f", stats_.averageGPA);
    ImGui::Text("Highest GPA: %.2f (%s)", stats_.highestGPA, stats_.topStudentId.c_str());
    ImGui::Text("Lowest GPA: %.2f (%s)", stats_.lowestGPA, stats_.bottomStudentId.c_str());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Department counts:");
    for (auto& [dept, count] : stats_.byDepartment) ImGui::BulletText("%s: %d", dept.c_str(), count);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Columns(2, nullptr, false);
    drawBarChart(stats_.byDepartment, "By Department (Bar)");
    ImGui::NextColumn();
    drawPieChart(stats_.byDepartment, "By Department (Pie)");
    ImGui::Columns(1);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(kAccent, "Import / Export / Backup");

    ImGui::SetNextItemWidth(320);
    ImGui::InputText("CSV path", csvPath_, sizeof(csvPath_));
    ImGui::SameLine();
    if (ImGui::Button("Export CSV")) {
        try {
            auto all = db_.getAllStudents();
            CSVManager::exportToFile(csvPath_, all);
            pushToast("Exported " + std::to_string(all.size()) + " students to CSV", kAccentGreen);
        } catch (const std::exception& e) {
            pushToast(std::string("Export failed: ") + e.what(), kAccentRed);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Import CSV")) {
        try {
            std::vector<int> skipped;
            auto imported = CSVManager::importFromFile(csvPath_, skipped);
            int added = 0, dupes = 0;
            for (auto& s : imported) {
                if (db_.studentIdExists(s.studentId)) { dupes++; continue; }
                db_.addStudent(s);
                added++;
            }
            dirty_ = true;
            statsDirty_ = true;
            pushToast("Imported " + std::to_string(added) + " students (" +
                          std::to_string(dupes) + " duplicate IDs skipped, " +
                          std::to_string(skipped.size()) + " malformed rows skipped)",
                      kAccentGreen);
        } catch (const std::exception& e) {
            pushToast(std::string("Import failed: ") + e.what(), kAccentRed);
        }
    }

    ImGui::SetNextItemWidth(320);
    ImGui::InputText("Backup path", backupPath_, sizeof(backupPath_));
    ImGui::SameLine();
    if (ImGui::Button("Backup DB")) {
        try {
            db_.backupTo(backupPath_);
            pushToast("Database backed up successfully", kAccentGreen);
        } catch (const std::exception& e) {
            pushToast(std::string("Backup failed: ") + e.what(), kAccentRed);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Restore DB")) {
        try {
            db_.restoreFrom(backupPath_);
            dirty_ = true;
            statsDirty_ = true;
            pushToast("Database restored successfully", kAccentGreen);
        } catch (const std::exception& e) {
            pushToast(std::string("Restore failed: ") + e.what(), kAccentRed);
        }
    }
}

// ---------------------------------------------------------------------
// Settings screen
// ---------------------------------------------------------------------
void GUI::renderSettingsScreen() {
    ImGui::TextColored(kAccent, "Settings");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Theme");
    bool changed = false;
    if (ImGui::RadioButton("Dark", darkTheme_)) { darkTheme_ = true; changed = true; }
    ImGui::SameLine();
    if (ImGui::RadioButton("Light", !darkTheme_)) { darkTheme_ = false; changed = true; }
    if (changed) { applyTheme(); saveSettings(); }

    ImGui::Spacing();
    ImGui::Text("Font Size");
    if (ImGui::SliderFloat("##fontscale", &fontScale_, 0.75f, 2.0f, "%.2fx")) {
        ImGui::GetIO().FontGlobalScale = fontScale_;
        saveSettings();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(kAccent, "Change Password");
    ImGui::SetNextItemWidth(240);
    ImGui::InputText("New password", newPassword_, sizeof(newPassword_), ImGuiInputTextFlags_Password);
    ImGui::SetNextItemWidth(240);
    ImGui::InputText("Confirm password", newPasswordConfirm_, sizeof(newPasswordConfirm_), ImGuiInputTextFlags_Password);
    if (ImGui::Button("Update Password")) {
        std::string p1 = newPassword_, p2 = newPasswordConfirm_;
        if (p1.empty()) {
            pushToast("Password cannot be empty", kAccentRed);
        } else if (p1 != p2) {
            pushToast("Passwords do not match", kAccentRed);
        } else {
            try {
                db_.changePassword(currentUser_, p1);
                pushToast("Password updated successfully", kAccentGreen);
                newPassword_[0] = newPasswordConfirm_[0] = '\0';
            } catch (const std::exception& e) {
                pushToast(std::string("Failed: ") + e.what(), kAccentRed);
            }
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(kAccent, "Database Backup");
    ImGui::SetNextItemWidth(320);
    ImGui::InputText("Path##settingsbackup", backupPath_, sizeof(backupPath_));
    ImGui::SameLine();
    if (ImGui::Button("Backup Now")) {
        try {
            db_.backupTo(backupPath_);
            pushToast("Backup created", kAccentGreen);
        } catch (const std::exception& e) {
            pushToast(std::string("Backup failed: ") + e.what(), kAccentRed);
        }
    }
}

// ---------------------------------------------------------------------
// About screen
// ---------------------------------------------------------------------
void GUI::renderAboutScreen() {
    ImGui::TextColored(kAccent, "About");
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Text("Project Name: Student Database Management System");
    ImGui::Text("Developer: Ka");
    ImGui::Text("Version: 1.0.0");
    ImGui::Text("University: (set your university name)");
    ImGui::Text("Course: (set your course name)");
    ImGui::Spacing();
    ImGui::TextWrapped("Built with C++17, Dear ImGui, GLFW, OpenGL 3, and SQLite3, "
                        "using CMake as the build system.");
}

// ---------------------------------------------------------------------
// Add / Edit popup
// ---------------------------------------------------------------------
void GUI::renderAddEditPopup() {
    if (showAddEdit_) ImGui::OpenPopup("Add / Edit Student");

    ImGui::SetNextWindowSize(ImVec2(480, 560), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Add / Edit Student", &showAddEdit_, ImGuiWindowFlags_NoResize)) {
        ImGui::Text(isEditMode_ ? "Editing student" : "New student");
        ImGui::Separator();

        ImGui::InputText("Student ID", form_.studentId, sizeof(form_.studentId));
        ImGui::InputText("First Name", form_.firstName, sizeof(form_.firstName));
        ImGui::InputText("Last Name", form_.lastName, sizeof(form_.lastName));
        ImGui::Combo("Gender", &form_.genderIdx, kGenders, IM_ARRAYSIZE(kGenders));
        ImGui::InputTextWithHint("Date of Birth", "YYYY-MM-DD", form_.dob, sizeof(form_.dob));
        ImGui::InputText("Phone", form_.phone, sizeof(form_.phone));
        ImGui::InputText("Email", form_.email, sizeof(form_.email));
        ImGui::InputText("Address", form_.address, sizeof(form_.address));

        ImGui::SetNextItemWidth(-1);
        {
            static std::vector<std::string> deptStorage;
            int idx = -1;
            for (int i = 0; i < IM_ARRAYSIZE(kDepartments); ++i)
                if (std::strcmp(form_.department, kDepartments[i]) == 0) idx = i;
            if (idx < 0) idx = 0;
            if (ImGui::Combo("Department", &idx, kDepartments, IM_ARRAYSIZE(kDepartments))) {
                copyToBuf(form_.department, sizeof(form_.department), kDepartments[idx]);
            } else if (form_.department[0] == '\0') {
                copyToBuf(form_.department, sizeof(form_.department), kDepartments[idx]);
            }
        }
        ImGui::InputText("Major", form_.major, sizeof(form_.major));
        ImGui::SliderInt("Year", &form_.year, 1, 6);
        ImGui::SliderFloat("GPA", &form_.gpa, 0.0f, 4.0f, "%.2f");
        ImGui::InputTextWithHint("Enrollment Date", "YYYY-MM-DD", form_.enrollmentDate, sizeof(form_.enrollmentDate));
        ImGui::Combo("Status", &form_.statusIdx, kStatuses, IM_ARRAYSIZE(kStatuses));

        if (!formError_.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(kAccentRed, "%s", formError_.c_str());
        }

        ImGui::Spacing();
        ImGui::Separator();
        if (ImGui::Button(isEditMode_ ? "Save Changes" : "Add Student", ImVec2(160, 34))) {
            Student s = form_.toStudent();
            ValidationResult vr = Validation::validateStudent(s, db_, isEditMode_ ? editingRowId_ : -1);
            if (!vr.ok) {
                formError_ = vr.joined();
            } else {
                try {
                    if (isEditMode_) db_.updateStudent(editingRowId_, s);
                    else db_.addStudent(s);
                    pushToast(isEditMode_ ? "Student updated successfully" : "Student added successfully", kAccentGreen);
                    showAddEdit_ = false;
                    dirty_ = true;
                    statsDirty_ = true;
                    formError_.clear();
                    ImGui::CloseCurrentPopup();
                } catch (const std::exception& e) {
                    formError_ = e.what();
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 34))) {
            showAddEdit_ = false;
            formError_.clear();
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

// ---------------------------------------------------------------------
// Delete confirmation popup
// ---------------------------------------------------------------------
void GUI::renderDeleteConfirmPopup() {
    if (showDeleteConfirm_) ImGui::OpenPopup("Confirm Delete");

    if (ImGui::BeginPopupModal("Confirm Delete", &showDeleteConfirm_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Delete student:");
        ImGui::TextColored(kAccentRed, "%s", deleteLabel_.c_str());
        ImGui::Text("This action cannot be undone.");
        ImGui::Spacing();
        if (ImGui::Button("Delete", ImVec2(120, 32))) {
            try {
                db_.deleteStudent(deleteRowId_);
                pushToast("Student deleted", kAccentGreen);
                dirty_ = true;
                statsDirty_ = true;
            } catch (const std::exception& e) {
                pushToast(std::string("Delete failed: ") + e.what(), kAccentRed);
            }
            showDeleteConfirm_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 32))) {
            showDeleteConfirm_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

// ---------------------------------------------------------------------
// View popup
// ---------------------------------------------------------------------
void GUI::renderViewPopup() {
    if (showViewPopup_) ImGui::OpenPopup("Student Details");

    ImGui::SetNextWindowSize(ImVec2(420, 460), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Student Details", &showViewPopup_)) {
        const Student& s = viewStudent_;
        ImGui::TextColored(kAccent, "%s", s.fullName().c_str());
        ImGui::Separator();
        ImGui::Text("Student ID: %s", s.studentId.c_str());
        ImGui::Text("Gender: %s", s.gender.c_str());
        ImGui::Text("Date of Birth: %s", s.dob.c_str());
        ImGui::Text("Age: %d", s.age);
        ImGui::Text("Phone: %s", s.phone.c_str());
        ImGui::Text("Email: %s", s.email.c_str());
        ImGui::Text("Address: %s", s.address.c_str());
        ImGui::Text("Department: %s", s.department.c_str());
        ImGui::Text("Major: %s", s.major.c_str());
        ImGui::Text("Year: %d", s.year);
        ImGui::Text("GPA: %.2f", s.gpa);
        ImGui::Text("Enrollment Date: %s", s.enrollmentDate.c_str());
        ImGui::TextColored(s.status == "Active" ? kAccentGreen : kAccentRed, "Status: %s", s.status.c_str());
        ImGui::Spacing();
        if (ImGui::Button("Close", ImVec2(-1, 32))) {
            showViewPopup_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

// ---------------------------------------------------------------------
// Toast notifications
// ---------------------------------------------------------------------
void GUI::renderToasts() {
    if (toasts_.empty()) return;
    ImGuiIO& io = ImGui::GetIO();
    float y = io.DisplaySize.y - 20;

    for (int i = static_cast<int>(toasts_.size()) - 1; i >= 0; --i) {
        Toast& t = toasts_[i];
        t.timeLeft -= io.DeltaTime;
        if (t.timeLeft <= 0) { toasts_.erase(toasts_.begin() + i); continue; }

        ImVec2 textSize = ImGui::CalcTextSize(t.message.c_str());
        float width = textSize.x + 32;
        y -= 46;
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - width - 20, y));
        ImGui::SetNextWindowBgAlpha(0.9f);
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                  ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;
        char id[32];
        snprintf(id, sizeof(id), "##toast%d", i);
        ImGui::Begin(id, nullptr, flags);
        ImGui::TextColored(t.color, "%s", t.message.c_str());
        ImGui::End();
    }
}

// ---------------------------------------------------------------------
// Main render dispatcher
// ---------------------------------------------------------------------
void GUI::render() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("StudentDBRoot", nullptr,
                  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                  ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                  ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar()) {
        ImGui::Text("Student Database Management System");
        ImGui::SameLine(io.DisplaySize.x - 220);
        ImGui::TextDisabled(loggedIn_ ? "Logged in" : "Not logged in");
        ImGui::EndMenuBar();
    }

    if (!loggedIn_) {
        ImGui::End();
        renderLoginScreen();
        renderToasts();
        return;
    }

    renderSidebar();
    ImGui::SameLine();
    ImGui::BeginChild("MainContent", ImVec2(0, 0), false);

    switch (screen_) {
        case Screen::Dashboard: renderDashboard(); break;
        case Screen::Students:  renderStudentManagement(); break;
        case Screen::Search:    renderSearchScreen(); break;
        case Screen::Stats:     renderStatisticsScreen(); break;
        case Screen::Settings:  renderSettingsScreen(); break;
        case Screen::About:     renderAboutScreen(); break;
    }

    ImGui::EndChild();
    ImGui::End();

    renderToasts();

    // status bar
    ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y - 24));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, 24));
    ImGui::Begin("##statusbar", nullptr,
                  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings);
    ImGui::Text("DB: %s   |   %zu record(s) loaded   |   User: %s (%s)",
                db_.dbPath().c_str(), students_.size(), currentUser_.c_str(), currentRole_.c_str());
    ImGui::End();
}
