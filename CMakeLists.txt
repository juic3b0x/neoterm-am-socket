cmake_minimum_required(VERSION 3.10)
project(neoterm-am-socket)

if (CMAKE_INSTALL_PREFIX STREQUAL "/usr/local")
    set(CMAKE_INSTALL_PREFIX $ENV{PREFIX})
    message(STATUS "prefix set from env: " ${CMAKE_INSTALL_PREFIX})
endif()

set(NEOTERM_PREFIX ${CMAKE_INSTALL_PREFIX})

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/neoterm-am.h.in
    neoterm-am.h @ONLY
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/neoterm-am.sh.in
  neoterm-am @ONLY
)

add_executable(neoterm-am-socket neoterm-am.cpp)
target_include_directories(neoterm-am-socket PUBLIC ${CMAKE_BINARY_DIR})


install(FILES ${CMAKE_BINARY_DIR}/neoterm-am TYPE BIN PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
install(TARGETS neoterm-am-socket DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
