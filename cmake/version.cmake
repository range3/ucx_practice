string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPERCASE)
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWERCASE)
set(THIS_TARGET_NAME ${PROJECT_NAME_LOWERCASE}_version)

configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/version.h.in include/${PROJECT_NAME_LOWERCASE}/version.h @ONLY
)

add_library(${THIS_TARGET_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${THIS_TARGET_NAME} ALIAS ${THIS_TARGET_NAME})

set_target_properties(
  ${THIS_TARGET_NAME}
  PROPERTIES PUBLIC_HEADER ${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME_LOWERCASE}/version.h
)
target_include_directories(
  ${THIS_TARGET_NAME}
  INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
            "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>"
)
