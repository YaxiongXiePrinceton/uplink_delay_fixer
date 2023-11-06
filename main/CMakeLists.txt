
CMAKE_MINIMUM_REQUIRED (VERSION 3.10)

SET(PROJECT_NAME "delay_uplink_fix")
PROJECT("${PROJECT_NAME}")

add_subdirectory(packetIO)
add_subdirectory(dci_sink)

INCLUDE(GNUInstallDirs)
add_executable(main_uplink main.c dci_sink_client.c)

find_package(libconfig CONFIG REQUIRED)
set(CMAKE_PREFIX_PATH "/usr/local/lib/" ${CMAKE_PREFIX_PATH})
LINK_DIRECTORIES(
	"/usr/local/lib/"
)


find_package(Threads REQUIRED)

#find_package(dcisink)
target_link_libraries(main_uplink packetio dcisink config pthread)
