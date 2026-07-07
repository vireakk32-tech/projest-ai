cmake_minimum_required(VERSION 3.16)
project(StudentDB LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)

if(MSVC)
    add_compile_options(/utf-8)
endif()

# -----------------------------------------------------------------------
# GLFW (window/context/input). Uses FetchContent so the project builds
# cleanly on a fresh machine without any manual dependency install.
# -----------------------------------------------------------------------
include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

# -----------------------------------------------------------------------
# Dear ImGui - vendored as source (no CMake support upstream), so we build
# it as a small static library from external/imgui.
# -----------------------------------------------------------------------
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw)

if(WIN32)
    target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_CUSTOM=0)
endif()

# -----------------------------------------------------------------------
# SQLite3 - vendored amalgamation (external/sqlite3/sqlite3.c + .h), built
# as a small C static library. No system SQLite install required.
# -----------------------------------------------------------------------
add_library(sqlite3 STATIC ${CMAKE_SOURCE_DIR}/external/sqlite3/sqlite3.c)
target_include_directories(sqlite3 PUBLIC ${CMAKE_SOURCE_DIR}/external/sqlite3)
if(UNIX)
    target_link_libraries(sqlite3 PUBLIC ${CMAKE_DL_LIBS} pthread)
endif()

# -----------------------------------------------------------------------
# OpenGL
# -----------------------------------------------------------------------
find_package(OpenGL REQUIRED)

# -----------------------------------------------------------------------
# StudentDB application
# -----------------------------------------------------------------------
add_executable(StudentDB
    src/main.cpp
    src/Student.cpp
    src/Database.cpp
    src/GUI.cpp
    src/Validation.cpp
    src/Statistics.cpp
    src/CSVManager.cpp
)

target_include_directories(StudentDB PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(StudentDB PRIVATE
    imgui
    glfw
    sqlite3
    OpenGL::GL
)

# Make sure the database/ folder (and thus students.db) is created next to
# the binary's working directory when run from VS Code / CMake Tools.
add_custom_command(TARGET StudentDB POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/database
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:StudentDB>/database
)

set_target_properties(StudentDB PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)
