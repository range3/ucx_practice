set(THIS_TARGET_NAME init_ep)

add_executable(${THIS_TARGET_NAME}_srv)
target_sources(${THIS_TARGET_NAME}_srv PRIVATE server.cpp)
target_compile_features(${THIS_TARGET_NAME}_srv PUBLIC cxx_std_20)
set_target_properties(${THIS_TARGET_NAME}_srv PROPERTIES CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(Ucx REQUIRED IMPORTED_TARGET ucx)

target_link_libraries(
  ${THIS_TARGET_NAME}_srv
  PRIVATE Threads::Threads
          fmt::fmt
          cxxopts::cxxopts
          nlohmann_json::nlohmann_json
          spdlog::spdlog
          ucx_practice::ucx_practice_version
          stdc++fs
          PkgConfig::Ucx
)

install(TARGETS ${THIS_TARGET_NAME}_srv DESTINATION "${CMAKE_INSTALL_BINDIR}")

add_executable(${THIS_TARGET_NAME}_cli)
target_sources(${THIS_TARGET_NAME}_cli PRIVATE client.cpp)
target_compile_features(${THIS_TARGET_NAME}_srv PUBLIC cxx_std_20)
set_target_properties(${THIS_TARGET_NAME}_srv PROPERTIES CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(Ucx REQUIRED IMPORTED_TARGET ucx)

target_link_libraries(
  ${THIS_TARGET_NAME}_srv
  PRIVATE Threads::Threads
          fmt::fmt
          cxxopts::cxxopts
          nlohmann_json::nlohmann_json
          spdlog::spdlog
          ucx_practice::ucx_practice_version
          stdc++fs
          PkgConfig::Ucx
)

target_link_libraries(
  ${THIS_TARGET_NAME}_cli
  PRIVATE Threads::Threads
          fmt::fmt
          cxxopts::cxxopts
          nlohmann_json::nlohmann_json
          spdlog::spdlog
          ucx_practice::ucx_practice_version
          stdc++fs
          PkgConfig::Ucx
)

install(TARGETS ${THIS_TARGET_NAME}_cli DESTINATION "${CMAKE_INSTALL_BINDIR}")
