cmake_minimum_required(VERSION 3.20)
project(ServiceRecoveryScheduler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force clear build structures for output execution binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#---------------------------------------------------------------------
# 1. Pre-built Path Registration (Explicitly targeted)
#---------------------------------------------------------------------
set(PREBUILT_PROTOBUF_DIR  "C:/gRPC_Libs/protobuf")
set(PREBUILT_GRPC_DIR      "C:/gRPC_Libs/grpc")
set(PREBUILT_GOOGLETEST_DIR "C:/gRPC_Libs/googletest") # Added GoogleTest path

# Instruct CMake where to find the package config profiles for all dependencies
list(APPEND CMAKE_PREFIX_PATH 
    "${PREBUILT_PROTOBUF_DIR}"
    "${PREBUILT_PROTOBUF_DIR}/lib/cmake/protobuf"
    "${PREBUILT_GRPC_DIR}"
    "${PREBUILT_GRPC_DIR}/lib/cmake/grpc"
    "${PREBUILT_GOOGLETEST_DIR}"
    "${PREBUILT_GOOGLETEST_DIR}/lib/cmake/GTest"
)

#---------------------------------------------------------------------
# 2. Dependency Discovery
#---------------------------------------------------------------------
# 1. ALWAYS Find Protobuf first so its targets exist in CMake's memory
find_package(Protobuf REQUIRED)

# 2. Explicitly point gRPC to where your pre-built Protobuf CMake configs live
set(Protobuf_DIR "${PREBUILT_PROTOBUF_DIR}/lib/cmake/protobuf")
set(protobuf_DIR "${PREBUILT_PROTOBUF_DIR}/lib/cmake/protobuf")

# 3. Now find gRPC
find_package(gRPC CONFIG REQUIRED)

# 4. Find pre-built GoogleTest natively
option(BUILD_TESTING "Build the library test suite" ON)
if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
endif()


#---------------------------------------------------------------------
# 3. Automated Protobuf / gRPC Generation Pipeline
#---------------------------------------------------------------------
set(PROTO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(PROTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

set(PROTO_FILE "${PROTO_SRC_DIR}/scheduler.proto")
set(GEN_PB_SRC   "${PROTO_GEN_DIR}/scheduler.pb.cc")
set(GEN_PB_HDR   "${PROTO_GEN_DIR}/scheduler.pb.h")
set(GEN_GRPC_SRC "${PROTO_GEN_DIR}/scheduler.grpc.pb.cc")
set(GEN_GRPC_HDR "${PROTO_GEN_DIR}/scheduler.grpc.pb.h")

# Map compiler executables from their specific folders
find_program(PROTOC_EXECUTABLE protoc HINTS "${PREBUILT_PROTOBUF_DIR}/bin" REQUIRED)
find_program(gRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin HINTS "${PREBUILT_GRPC_DIR}/bin" REQUIRED)

add_custom_command(
    OUTPUT "${GEN_PB_SRC}" "${GEN_PB_HDR}" "${GEN_GRPC_SRC}" "${GEN_GRPC_HDR}"
    COMMAND ${PROTOC_EXECUTABLE}
    ARGS --cpp_out=${PROTO_GEN_DIR}
         --grpc_out=${PROTO_GEN_DIR}
         --plugin=protoc-gen-grpc=${gRPC_CPP_PLUGIN_EXECUTABLE}
         -I ${PROTO_SRC_DIR}
         ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Compiling Protocol Buffer and gRPC contract structures using explicit prebuilt tools"
    VERBATIM
)

set_source_files_properties(
    "${GEN_PB_SRC}" "${GEN_PB_HDR}" "${GEN_GRPC_SRC}" "${GEN_GRPC_HDR}"
    PROPERTIES GENERATED TRUE
)

# 1. Create the Generated sources wrapper target
add_library(proto_generated_objs OBJECT "${GEN_PB_SRC}" "${GEN_GRPC_SRC}")

# 2. STRIP all automatic include structures inherited from CMake packages
set_target_properties(proto_generated_objs PROPERTIES 
    INCLUDE_DIRECTORIES ""
    INTERFACE_INCLUDE_DIRECTORIES ""
)

# 3. MANUALLY feed the strict include path priority queue.
# We place Protobuf first and completely omit global package macros.
target_include_directories(proto_generated_objs PRIVATE 
    "${PREBUILT_PROTOBUF_DIR}/include"
    "${PROTO_GEN_DIR}"
    "${PREBUILT_GRPC_DIR}/include"
)

# 4. CRITICAL: Link against RAW libraries (.lib files) instead of the gRPC package macros.
# This prevents the compiler from pulling in the polluted include directories.
find_library(LIB_PROTOBUF NAMES libprotobuf HINTS "${PREBUILT_PROTOBUF_DIR}/lib" REQUIRED)
find_library(LIB_GRPC     NAMES grpc++      HINTS "${PREBUILT_GRPC_DIR}/lib"     REQUIRED)

target_link_libraries(proto_generated_objs PRIVATE 
    ${LIB_PROTOBUF}
    ${LIB_GRPC}
)

if(WIN32)
    target_compile_definitions(proto_generated_objs PRIVATE -D_WIN32_WINNT=0x0601)
endif()


#---------------------------------------------------------------------
# 4. Core Engine Library Target
#---------------------------------------------------------------------
add_library(recovery_scheduler src/scheduler.cpp)
target_include_directories(recovery_scheduler PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

#---------------------------------------------------------------------
# 5. Executable Application Processes
#---------------------------------------------------------------------
# A. Master Scheduler Daemon process
add_executable(scheduler_server apps/main.cpp)
target_link_libraries(scheduler_server PRIVATE recovery_scheduler proto_generated_objs)

# B. Monitored Service Client App process
add_executable(mock_service_client apps/service_main.cpp)
target_link_libraries(mock_service_client PRIVATE proto_generated_objs)

#---------------------------------------------------------------------
# 6. Automated Testing Target (Using Pre-built GTest)
#---------------------------------------------------------------------
if(BUILD_TESTING)
    add_executable(scheduler_tests tests/scheduler_test.cpp)
    # GTest::gtest_main automatically contains main() entry points
    target_link_libraries(scheduler_tests PRIVATE recovery_scheduler GTest::gtest_main)
    
    include(GoogleTest)
    gtest_discover_tests(scheduler_tests)
endif()
