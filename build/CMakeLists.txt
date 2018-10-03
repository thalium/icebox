cmake_minimum_required(VERSION 3.4)
project(winbagility)

include(common.cmake)
get_filename_component(root_dir     ${CMAKE_CURRENT_SOURCE_DIR}/.. ABSOLUTE)
get_filename_component(bin_d_dir    ${root_dir}/bin/fdp_d_${ARCH}  ABSOLUTE)
get_filename_component(bin_dir      ${root_dir}/bin/fdp_${ARCH}    ABSOLUTE)

# fdp
add_target(fdp libs "${root_dir}/src/FDP" OPTIONS recurse shared warnings)
set_target_output_directory(fdp "")
target_include_directories(fdp PUBLIC "${root_dir}/src/FDP/include")

# winbagility
add_target(winbagility apps "${root_dir}/src/Winbagility" OPTIONS executable recurse warnings)
set_target_output_directory(winbagility "")
target_include_directories(winbagility PRIVATE
    "${root_dir}/src/Winbagility"
    "${root_dir}/src/Winbagility/STUB"
)
target_link_libraries(winbagility PRIVATE
    dbghelp
    fdp
    ws2_32
)

# test_fdp
add_target(test_fdp tests "${root_dir}/src/TestFDP" OPTIONS executable warnings)
set_target_output_directory(test_fdp "")
target_link_libraries(test_fdp PRIVATE
    fdp
)
target_include_directories(test_fdp PRIVATE
    "${root_dir}/src/Winbagility"
)

# hbp
add_target(hbp libs "${root_dir}/src/HDBPExtension" OPTIONS shared warnings)
set_target_output_directory(hbp "")
target_link_libraries(hbp PRIVATE
    fdp
)
