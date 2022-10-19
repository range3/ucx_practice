set(THIS_TARGET_NAME init)

file(
  GLOB_RECURSE
  ${THIS_TARGET_NAME}_SRC
  CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

add_executable(${THIS_TARGET_NAME})
target_sources(${THIS_TARGET_NAME} PRIVATE ${${THIS_TARGET_NAME}_SRC})

target_compile_features(${THIS_TARGET_NAME} PUBLIC cxx_std_20)
set_target_properties(${THIS_TARGET_NAME} PROPERTIES CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(Ucx REQUIRED IMPORTED_TARGET ucx)

target_link_libraries(
  ${THIS_TARGET_NAME}
  PRIVATE Threads::Threads
          fmt::fmt
          cxxopts::cxxopts
          nlohmann_json::nlohmann_json
          spdlog::spdlog
          ucx_practice::ucx_practice_version
          stdc++fs
          PkgConfig::Ucx
)

install(TARGETS ${THIS_TARGET_NAME} DESTINATION "${CMAKE_INSTALL_BINDIR}")
