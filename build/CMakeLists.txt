project(WINBAGILITY)

cmake_minimum_required(VERSION 3.4)

include_directories("../src/FDP/include")
include_directories("C:/Program Files (x86)/Microsoft Visual Studio 14.0/DIA SDK/include")

#FDP Static Library
add_library(FDP STATIC "../src/FDP/FDP.c")

#FDP Dynamic Library
#TODO condition...
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_library(FDP_x64 SHARED "../src/FDP/FDP.c")
else(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_library(FDP_x86 SHARED "../src/FDP/FDP.c")
endif(CMAKE_SIZEOF_VOID_P EQUAL 8)

#TestFDP
add_executable(TestFDP "../src/TestFDP/TestFDP.c")
target_link_libraries(TestFDP FDP)

#Include for Winbagility
include_directories("../src/Winbagility/")
include_directories("../src/Winbagility/STUB/")
#Winbagility
add_executable(winbagility
            "../src/Winbagility/winbagility.c"
            "../src/Winbagility/kd.c"
            "../src/Winbagility/dissectors.c"
            "../src/Winbagility/kdproxy.c"
            "../src/Winbagility/mmu.c"
            "../src/Winbagility/symbols.c"
            "../src/Winbagility/utils.c"
            "../src/Winbagility/kdserver.c"
            "../src/Winbagility/STUB/LIVEKADAY.c"
            "../src/Winbagility/STUB/GDB.c"
            "../src/Winbagility/STUB/CRASH.c")
target_link_libraries(winbagility dbghelp.lib)
target_link_libraries(winbagility ws2_32)
target_link_libraries(winbagility FDP)

#WinDbg Extension
add_library(hbp SHARED "../src/HDBPExtension/hbp.c")
target_link_libraries(hbp FDP)
