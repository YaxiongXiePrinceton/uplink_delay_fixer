################################################################################
### Selecting CMake minimum version

CMAKE_MINIMUM_REQUIRED (VERSION 3.10)
SET(PROJECT_NAME "delay_uplink_fix_project")
PROJECT("${PROJECT_NAME}")

add_subdirectory(packetIO)
add_subdirectory(dci_sink)
add_subdirectory(main)

