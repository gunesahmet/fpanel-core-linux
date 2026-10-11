cmake_minimum_required(VERSION 3.6.0)
project(fpanel-core)

set(dir ${CMAKE_CURRENT_SOURCE_DIR}/Compiled)
set(CMAKE_CXX_STANDARD 14)
# BOOST DEFINITIONS
set(Boost_USE_STATIC_LIBS        ON)  # only find static libs
set(Boost_USE_DEBUG_LIBS        OFF)  # ignore debug libs
set(Boost_USE_RELEASE_LIBS       ON)  # only find release libs
set(Boost_USE_MULTITHREADED      ON)  # only multithreaded
set(Boost_USE_STATIC_RUNTIME    OFF)

set(EXECUTABLE_OUTPUT_PATH ${dir})
set(CMAKE_CXX_FLAGS "-g -pthread -lcrypt -lpam -fpermissive -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -g2 -w -ggdb -O2 -Wall -ldl -pedantic -Wextra -fext-numeric-literals")

include_directories(./)
include_directories(/usr/local/include/)
include_directories(/usr/include/boost/)

find_package(Boost 1.67.0 REQUIRED COMPONENTS
        program_options
        filesystem
        thread
        regex
        date_time system)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable(fpanel-core main.cpp
            plog/Log.h
            Helpers/md5.cpp
            Ini/ini.c
            Database/MySQL.h
            Ini/inireader.cpp
            tsl/ordered_map.h
            Commands/Background.h
            Commands/LicenceCommand.h)
    target_link_libraries(fpanel-core
            /usr/lib/libcrypto++.a
            /usr/local/lib/libboost_system.a
            /usr/local/lib/libboost_program_options.a
            /usr/local/lib/libboost_filesystem.a
            /usr/local/lib/libboost_thread.a
            /usr/local/lib/libboost_regex.a
            /usr/local/lib/libboost_date_time.a)
endif()
