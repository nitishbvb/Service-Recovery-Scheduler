cmake_minimum_required(VERSION 3.20)
project(ServiceRecoveryScheduler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force clear build structures for output execution binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#---------------------------------------------------------------------
# 1. Dependency Discovery
#---------------------------------------------------------------------

find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf REQUIRED)

# Try finding GoogleTest for unit testing verification loops
option(BUILD_TESTING "Build the library test suite" ON)
if(BUILD_TESTING)
    find_package(GTest CONFIG)
    if(NOT GTest_FOUND)
        # Fallback to fetching it dynamically if not installed on the system
        include(FetchContent)
        FetchContent_Declare(
            googletest
            URL https://github.com
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
endif()

#---------------------------------------------------------------------
# 2. Automated Protobuf / gRPC Generation Pipeline
#---------------------------------------------------------------------
set(PROTO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(PROTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

set(PROTO_FILE "${PROTO_SRC_DIR}/scheduler.proto")
set(GEN_PB_SRC   "${PROTO_GEN_DIR}/scheduler.pb.cc")
set(GEN_PB_HDR   "${PROTO_GEN_DIR}/scheduler.pb.h")
set(GEN_GRPC_SRC "${PROTO_GEN_DIR}/scheduler.grpc.pb.cc")
set(GEN_GRPC_HDR "${PROTO_GEN_DIR}/scheduler.grpc.pb.h")

# Find the location of the native plugin executables
find_program(gRPC_CPP_PLUGIN_EXECUTABLE name grpc_cpp_plugin)

add_custom_command(
    OUTPUT "${GEN_PB_SRC}" "${GEN_PB_HDR}" "${GEN_GRPC_SRC}" "${GEN_GRPC_HDR}"
    COMMAND protobuf::protoc
    ARGS --cpp_out=${PROTO_GEN_DIR}
         --grpc_out=${PROTO_GEN_DIR}
         --plugin=protoc-gen-grpc=${gRPC_CPP_PLUGIN_EXECUTABLE}
         -I ${PROTO_SRC_DIR}
         ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Compiling Protocol Buffer and gRPC contract structures across platform boundaries"
    VERBATIM
)

# Generated sources wrapper target
add_library(proto_generated_objs OBJECT ${GEN_PB_SRC} ${GEN_GRPC_SRC})
target_include_directories(proto_generated_objs PUBLIC ${PROTO_GEN_DIR})
target_link_libraries(proto_generated_objs PUBLIC gRPC::grpc++ protobuf::libprotobuf)

#---------------------------------------------------------------------
# 3. Core Engine Library Target
#---------------------------------------------------------------------
add_library(recovery_scheduler src/scheduler.cpp)
target_include_directories(recovery_scheduler PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

#---------------------------------------------------------------------
# 4. Executable Application Processes
#---------------------------------------------------------------------
# A. Master Scheduler Daemon process
add_executable(scheduler_server apps/server_main.cpp)
target_link_libraries(scheduler_server PRIVATE recovery_scheduler proto_generated_objs)

# B. Monitored Service Client App process
add_executable(mock_service_client apps/service_main.cpp)
target_link_libraries(mock_service_client PRIVATE proto_generated_objs)

#---------------------------------------------------------------------
# 5. Automated Testing Target
#---------------------------------------------------------------------
if(BUILD_TESTING)
    add_executable(scheduler_tests tests/scheduler_test.cpp)
    target_link_libraries(scheduler_tests PRIVATE recovery_scheduler GTest::gtest_main)
    
    include(GoogleTest)
    gtest_discover_tests(scheduler_tests)
endif()
