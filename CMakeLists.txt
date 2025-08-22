cmake_minimum_required(VERSION 3.16)

project(WingCodeEdit LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets PrintSupport)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets PrintSupport)

add_subdirectory(syntax-highlighting)

option(BUILD_EXAMPLES OFF)

if(BUILD_EXAMPLES)
    add_subdirectory(WingCodeEditExample)
endif()

add_library(
    WingCodeEdit STATIC
    wingcodeedit.cpp
    wingcodeedit.h
    wingsyntaxhighlighter.h
    wingsyntaxhighlighter.cpp
    winglinemargin.h
    winglinemargin.cpp
    wingcompleter.h
    wingcompleter.cpp
    wingcodepopup.h
    wingcodepopup.cpp
    wingsymbolcenter.h
    wingsymbolcenter.cpp
    squiggleinformationmodel.h squiggleinformationmodel.cpp)

target_link_libraries(
    WingCodeEdit PUBLIC Qt${QT_VERSION_MAJOR}::Widgets
                        Qt${QT_VERSION_MAJOR}::PrintSupport KSyntaxHighlighting)

target_include_directories(WingCodeEdit PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_compile_definitions(WingCodeEdit PRIVATE WINGCODEEDIT_LIBRARY)
