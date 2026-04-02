cmake_minimum_required(VERSION 3.16)
project(atm_gui_qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Widgets Charts REQUIRED)

qt_standard_project_setup()

qt_add_executable(atm_gui_qt
    main.cpp
)

target_link_libraries(atm_gui_qt PRIVATE Qt6::Widgets Qt6::Charts)

if (MSVC)
    target_compile_options(atm_gui_qt PRIVATE /Zc:__cplusplus /utf-8)
endif()
