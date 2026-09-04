cmake_minimum_required(VERSION 3.16)

project(BITMiniSemesterProjects VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)
add_subdirectory(project1-financial-calculator)
add_subdirectory(project2-file-transfer)
