cmake_minimum_required(VERSION 3.20)
project(ServiceRecoveryScheduler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enforce clean executable delivery targets
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

option(BUILD_TESTING "Build the library test suite" ON)

# --------------------------------------------------------------------
# 1. Dependency Discovery (GoogleTest configuration pathing)
# --------------------------------------------------------------------
set(PREBUILT_GTEST_DIR "C:/gRPC_Libs/googletest")
list(APPEND CMAKE_PREFIX_PATH "${PREBUILT_GTEST_DIR}" "${PREBUILT_GTEST_DIR}/lib/cmake/GTest")

if(BUILD_TESTING)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
endif()

# --------------------------------------------------------------------
# 2. Core Scheduling Engine Library Target
# --------------------------------------------------------------------
add_library(recovery_scheduler src/scheduler.cpp)
target_include_directories(recovery_scheduler PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# --------------------------------------------------------------------
# 3. Executable Application Targets
# --------------------------------------------------------------------
# A. Master Scheduler Daemon process (Directory Monitoring Host)
add_executable(scheduler_server apps/main.cpp)
target_link_libraries(scheduler_server PRIVATE recovery_scheduler)

# B. Standalone Monitored Service Application process (Command Flag Dropper)
add_executable(mock_service_client apps/service_main.cpp)
target_link_libraries(mock_service_client PRIVATE recovery_scheduler)

# --------------------------------------------------------------------
# 4. Automated Unit Testing Target
# --------------------------------------------------------------------
if(BUILD_TESTING)
    add_executable(scheduler_tests tests/scheduler_test.cpp)
    target_link_libraries(scheduler_tests PRIVATE recovery_scheduler GTest::gtest_main)
    
    include(GoogleTest)
    gtest_discover_tests(scheduler_tests)
endif()
