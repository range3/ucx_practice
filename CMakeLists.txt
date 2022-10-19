cmake_minimum_required(VERSION 3.14...3.22)

# ---- Project ----

# Note: update this to your new project's name and version
project(
  ucx_practice
  VERSION 0.8.0
  LANGUAGES CXX C
)

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

# ---- Set default build type ----
# Encourage user to specify a build type (e.g. Release, Debug, etc.), otherwise set it to Release.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE
      "RelWithDebInfo"
      CACHE STRING "Choose the type of build." FORCE
  )
  # Set the possible values of build type for cmake-gui
  set_property(
    CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo"
  )
endif()

# ---- Add dependencies via CPM ----
# see https://github.com/TheLartians/CPM.cmake for more info

include(cmake/CPM.cmake)

# PackageProject.cmake will be used to make our target installable
CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.8.0")

CPMAddPackage(
  NAME fmt
  GIT_TAG 8.1.1
  GITHUB_REPOSITORY fmtlib/fmt
  OPTIONS "FMT_INSTALL YES" # create an installable target
          EXCLUDE_FROM_ALL YES
)

CPMAddPackage(
  GITHUB_REPOSITORY jarro2783/cxxopts
  VERSION 3.0.0
  OPTIONS "CXXOPTS_BUILD_EXAMPLES NO" "CXXOPTS_BUILD_TESTS NO" "CXXOPTS_ENABLE_INSTALL YES"
          EXCLUDE_FROM_ALL YES
)

CPMAddPackage(
  NAME nlohmann_json
  VERSION 3.9.1
  GITHUB_REPOSITORY nlohmann/json
  OPTIONS "JSON_BuildTests OFF" EXCLUDE_FROM_ALL YES
)

CPMAddPackage(
  NAME spdlog
  VERSION 1.8.2
  GITHUB_REPOSITORY gabime/spdlog EXCLUDE_FROM_ALL YES
)

# ---- Other dependencies ----
# find_package(MPI REQUIRED)

include(cmake/version.cmake)
find_package(PkgConfig REQUIRED)
pkg_check_modules(Ucx REQUIRED IMPORTED_TARGET ucx)

# ---- Add source files ----

# Note: globbing sources is considered bad practice as CMake's generators may not detect new files
# automatically. Keep that in mind when changing files, or explicitly mention them here.
file(GLOB_RECURSE src_hpp CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/bin/*.hpp")
file(GLOB_RECURSE src_cpp CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/bin/*.cpp")
file(GLOB_RECURSE src_h CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/bin/*.h")
file(GLOB_RECURSE src_c CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/bin/*.c")

# ---- Create executable ----
add_executable(server src/bin/server.cpp)
set_target_properties(
  server
  PROPERTIES CXX_STANDARD 17
             CXX_STANDARD_REQUIRED ON
             OUTPUT_NAME "server"
)
target_compile_options(server PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")
target_link_libraries(
  server PRIVATE fmt::fmt cxxopts::cxxopts nlohmann_json::nlohmann_json spdlog::spdlog
                 ucx_practice::ucx_practice_version stdc++fs PkgConfig::Ucx
)
install(TARGETS server DESTINATION "${CMAKE_INSTALL_BINDIR}")

add_executable(client src/bin/client.cpp)
set_target_properties(
  client
  PROPERTIES CXX_STANDARD 17
             CXX_STANDARD_REQUIRED ON
             OUTPUT_NAME "client"
)
target_compile_options(client PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")
target_link_libraries(
  client PRIVATE fmt::fmt cxxopts::cxxopts nlohmann_json::nlohmann_json spdlog::spdlog
                 ucx_practice::ucx_practice_version stdc++fs PkgConfig::Ucx
)
install(TARGETS client DESTINATION "${CMAKE_INSTALL_BINDIR}")

add_subdirectory(src/bin/01_config)
add_subdirectory(src/bin/02_init_cxt_worker)
