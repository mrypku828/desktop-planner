cmake_minimum_required(VERSION 3.21)
project(DesktopPlanner VERSION 1.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Sql)
qt_standard_project_setup()

add_library(planner_core STATIC
    src/domain/Models.h
    src/storage/Database.h src/storage/Database.cpp
    src/services/PlannerService.h src/services/PlannerService.cpp
    src/platform/AppSettings.h src/platform/AppSettings.cpp
)
target_include_directories(planner_core PUBLIC src)
target_link_libraries(planner_core PUBLIC Qt6::Core Qt6::Sql)
add_library(planner_ui STATIC
    resources/planner.qrc
    src/ui/AppStyle.h
    src/ui/Theme.h src/ui/Theme.cpp
    src/ui/CalendarPage.h src/ui/CalendarPage.cpp
    src/ui/MainWindow.h src/ui/MainWindow.cpp
    src/ui/TodoDialog.h src/ui/TodoDialog.cpp
    src/ui/TodoPage.h src/ui/TodoPage.cpp
    src/ui/NotesPage.h src/ui/NotesPage.cpp
    src/ui/DesktopWidget.h src/ui/DesktopWidget.cpp
    src/services/ReminderService.h src/services/ReminderService.cpp
)
target_link_libraries(planner_ui PUBLIC planner_core Qt6::Widgets)
qt_add_executable(DesktopPlanner
    src/main.cpp
)
target_link_libraries(DesktopPlanner PRIVATE planner_ui)
set_target_properties(DesktopPlanner PROPERTIES WIN32_EXECUTABLE TRUE)

foreach(target planner_core planner_ui DesktopPlanner)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()
include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(planner_tests tests/PlannerTests.cpp)
    target_link_libraries(planner_tests PRIVATE planner_ui Qt6::Test)
    add_test(NAME planner_tests COMMAND planner_tests -platform offscreen -o test-results.txt,txt)
    set_tests_properties(planner_tests PROPERTIES TIMEOUT 60)
endif()
