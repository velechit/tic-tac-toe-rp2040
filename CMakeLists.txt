cmake_minimum_required(VERSION 3.13)
set(PROJ_NAME ttt2040)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(${PROJ_NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPICO_USE_MALLOC_MUTEX=1")

# add_subdirectory(libs/littlefs)
# add_subdirectory(libs/interpreter)
# add_subdirectory(libs/sd_driver)

file( GLOB PROJ_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.c )
file( GLOB GAME_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/game/*.c )

add_executable(${PROJ_NAME} ${PROJ_SRCS} ${GAME_SRCS})

target_link_libraries(${PROJ_NAME} pico_stdlib hardware_spi hardware_pwm hardware_dma hardware_pio hardware_timer hardware_clocks)
#  littlefs-lib interpreter-lib sd_driver)

target_include_directories(${PROJ_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc ${CMAKE_CURRENT_SOURCE_DIR}/game)


pico_enable_stdio_usb(${PROJ_NAME} 1)
pico_enable_stdio_uart(${PROJ_NAME} 1)

pico_set_program_description(${PROJ_NAME} "Tic Tac Toe on RP2040")


pico_add_extra_outputs(${PROJ_NAME})


