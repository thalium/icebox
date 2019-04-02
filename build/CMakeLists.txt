cmake_minimum_required(VERSION 3.4)
project(icebox)

set(CMAKE_CXX_STANDARD 17)

include(common.cmake)
get_filename_component(root_dir     ${CMAKE_CURRENT_SOURCE_DIR}/.. ABSOLUTE)
get_filename_component(bin_d_dir    ${root_dir}/bin/fdp_d_${ARCH}  ABSOLUTE)
get_filename_component(bin_dir      ${root_dir}/bin/fdp_${ARCH}    ABSOLUTE)

if(NOT MSVC)
    add_compile_options(
        -Wno-unused-local-typedefs
    )
endif()

if(${USE_STATIC_ANALYZER})
    message("-- clang static analyzer: enabled")
    find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy clang-tidy-7)
    if(NOT CLANG_TIDY_EXECUTABLE)
        message(FATAL_ERROR "missing clang-tidy")
    endif()
    set(enable_checks "-*")
    foreach(check IN ITEMS
        "clang-analyzer-*"
        "bugprone-*"
        "modernize-*"
        "misc-*"
        "performance-*"
        "portability-*"
        "readability-*"
    )
        set(enable_checks "${enable_checks},${check}")
    endforeach()
    set(CLANG_TIDY_ARGS "${CLANG_TIDY_EXECUTABLE}"
        "--warnings-as-errors=*"
        "--checks=\"${enable_checks}\""
    )
endif()

# fdp
function(add_fdp target options)
    add_target(${target} libs "${root_dir}/src/FDP" OPTIONS fmt recurse warnings ${options})
    set_target_output_directory(${target} "")
    target_include_directories(${target} PUBLIC "${root_dir}/src/FDP/include")
    target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    if(NOT WIN32)
        find_package(Threads)
        target_link_libraries(${target} PUBLIC Threads::Threads)
    endif()
endfunction()
add_fdp(fdp_static "")
add_fdp(fdp shared)

# fix fmt dependencies
if(NOT "$ENV{DISABLE_CLANG_FORMAT}" STREQUAL "true")
    add_dependencies(fdp_static_fmt fdp_fmt)
    add_dependencies(fdp fdp_static_fmt)
endif()

# test_fdp
add_target(test_fdp tests "${root_dir}/src/TestFDP" OPTIONS executable warnings)
set_target_output_directory(test_fdp "")
target_link_libraries(test_fdp PRIVATE
    fdp_static
)
target_include_directories(test_fdp PRIVATE
    "${root_dir}/src/Winbagility"
)
if(NOT WIN32)
target_link_libraries(test_fdp PRIVATE
    rt
)
endif()

# fmtlib
set(fmt_dir "${root_dir}/third_party/fmt")
add_target(fmtlib third_party "${fmt_dir}/src" "${fmt_dir}/include" OPTIONS recurse external)
target_include_directories(fmtlib PUBLIC
    "${fmt_dir}/include"
)

# loguru
set(loguru_dir "${root_dir}/third_party/loguru-2.0.0")
add_target(loguru third_party "${loguru_dir}" OPTIONS external)
target_compile_definitions(loguru PRIVATE
    _CRT_SECURE_NO_WARNINGS
)
target_include_directories(loguru PUBLIC
    "${loguru_dir}"
)

# pdbparser
set(retdec_dir "${root_dir}/third_party/retdec-3.2")
add_target(pdbparser third_party "${retdec_dir}/src/pdbparser" "${retdec_dir}/include/retdec/pdbparser" OPTIONS external)
target_include_directories(pdbparser PUBLIC
    "${retdec_dir}/include"
)

# tracer code generator
set(icebox_dir "${root_dir}/src/icebox/icebox")
set(tracer_dir "${icebox_dir}/tracer")
function(add_tracer namespace input_file)
    find_package(PythonInterp 3.6 QUIET REQUIRED)
    get_filename_component(directory ${input_file} DIRECTORY)
    get_filename_component(filename ${input_file} NAME_WE)
    add_custom_command(
        COMMAND ${PYTHON_EXECUTABLE}
            "${tracer_dir}/codegen.py" -n ${namespace} -i "${input_file}" ${ARGV2}
        COMMENT "generating ${filename}.gen.cpp"
        OUTPUT
            "${directory}/${filename}.gen.hpp"
            "${directory}/${filename}.gen.cpp"
        DEPENDS
            "${tracer_dir}/codegen.py"
            ${input_file}
    )
endfunction()
add_tracer("nt"     "${tracer_dir}/syscalls.json")
add_tracer("wow64"  "${tracer_dir}/syscalls32.json" "-w")
add_tracer("nt"     "${tracer_dir}/heaps.json")

# icebox
add_target(icebox libs ${icebox_dir} OPTIONS fmt recurse tidy)
target_include_directories(icebox
    PUBLIC
    "${root_dir}/src/icebox"
    PRIVATE
    "${icebox_dir}"
    "${root_dir}/third_party/expected-lite/include"
    "${root_dir}/third_party/nlohmann_json/include"
)
target_compile_definitions(icebox PRIVATE
    _CRT_SECURE_NO_WARNINGS
    _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
)
target_link_libraries(icebox PRIVATE
    loguru
    pdbparser
    PUBLIC
    fdp_static
    fmtlib
)
if(NOT WIN32)
    target_link_libraries(icebox PRIVATE
        rt
        dl
    )
    target_link_libraries(icebox PUBLIC stdc++fs)
endif()
set_property(TARGET icebox PROPERTY CXX_STANDARD 17)

# icebox_cmd
add_target(icebox_cmd apps "${root_dir}/src/icebox/icebox_cmd" OPTIONS executable fmt tidy)
set_target_output_directory(icebox_cmd "")
target_link_libraries(icebox_cmd PRIVATE icebox)

# samples/vm_resume
foreach(sample IN ITEMS
    vm_resume
    nt_writefile
)
    add_target(${sample} samples "${root_dir}/src/icebox/samples/${sample}" OPTIONS executable fmt tidy)
    set_target_output_directory(${sample} "")
    target_compile_definitions(${sample} PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(${sample} PRIVATE icebox)
endforeach()

if(WIN32)
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

    # hbp
    add_target(hbp libs "${root_dir}/src/HDBPExtension" OPTIONS shared warnings)
    set_target_output_directory(hbp "")
    target_link_libraries(hbp PRIVATE
        fdp
    )
endif()
