cmake_minimum_required(VERSION 3.4)
project(icebox)
enable_testing()

set(CMAKE_CXX_STANDARD 17)

include(common.cmake)
get_filename_component(root_dir     ${CMAKE_CURRENT_SOURCE_DIR}/.. ABSOLUTE)
get_filename_component(bin_d_dir    ${root_dir}/bin/d_${ARCH}  ABSOLUTE)
get_filename_component(bin_dir      ${root_dir}/bin/${ARCH}    ABSOLUTE)

if(NOT MSVC)
    add_compile_options(
        -Wno-unused-local-typedefs
    )
endif()

if(${USE_STATIC_ANALYZER})
    message("-- clang static analyzer: enabled")
    find_program(CLANG_TIDY_EXECUTABLE NAMES $ENV{CLANG_TIDY} clang-tidy)
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
        # too extreme...
        "-misc-non-private-member-variables-in-classes"
        "-readability-magic-numbers"
        "-modernize-avoid-c-arrays"
        "-modernize-use-trailing-return-type"
        # style...
        "-readability-braces-around-statements"
        "-readability-implicit-bool-conversion"
        # buggy in clang-tidy-8
        "-readability-simplify-boolean-expr"
    )
        set(enable_checks "${enable_checks},${check}")
    endforeach()
    set(CLANG_TIDY_ARGS "${CLANG_TIDY_EXECUTABLE}"
        "--warnings-as-errors=*"
        "--checks=${enable_checks}"
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
target_compile_definitions(fdp PRIVATE FDP_EXPORT)

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
set(loguru_dir "${root_dir}/third_party/loguru")
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

# libdwarf
set(libdwarf_dir "${root_dir}/third_party/libdwarf-20191104/libdwarf")
set(zlib_dir "${root_dir}/third_party/virtualbox/src/libs/zlib-1.2.8")
get_files(libdwarf_files "${libdwarf_dir}" ${zlib_dir})
filter_out(libdwarf_files
    "_test[.]c"
    "test_.+[.]c"
)
make_target(libdwarf third_party ${libdwarf_files} OPTIONS external)
target_compile_definitions(libdwarf PRIVATE
    HAVE_ZLIB
)
if(NOT WIN32)
    target_compile_definitions(libdwarf PRIVATE
        HAVE_SYS_TYPES_H
        HAVE___UINT32_T_IN_SYS_TYPES_H
        HAVE___UINT64_T_IN_SYS_TYPES_H
    )
endif()
target_include_directories(libdwarf PUBLIC
    "${libdwarf_dir}"
    "${zlib_dir}"
)

# mbedtls
set(mbedtls_dir "${root_dir}/third_party/mbedtls-2.16.1")
add_target(mbedtls third_party "${mbedtls_dir}/library" OPTIONS external)
target_include_directories(mbedtls PUBLIC
    "${mbedtls_dir}/include"
)

# libco
set(libco_dir "${root_dir}/third_party/libco")
get_files(libco_files "${libco_dir}")
filter_out(libco_files
    "aarch64[.]c$"
    "amd64[.]c$"
    "arm[.]c$"
    "fiber[.]c$"
    "ppc[.]c$"
    "ppc64v2[.]c$"
    "sjlj[.]c$"
    "ucontext[.]c$"
    "x86[.]c$"
)
make_target(libco third_party "${libco_files}" OPTIONS external)
target_compile_definitions(libco PRIVATE LIBCO_MPROTECT)
target_include_directories(libco PUBLIC
    "${libco_dir}"
)

# tracer code generator
set(icebox_dir "${root_dir}/src/icebox/icebox")
set(tracer_dir "${icebox_dir}/tracer")
function(add_tracer namespace input_file)
    find_package(PythonInterp QUIET REQUIRED)
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
target_link_libraries(icebox
  PRIVATE
    libco
    libdwarf
    loguru
    mbedtls
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
    heapsan
    linux
    nt_writefile
    vm_resume
    wireshark
)
    add_target(${sample} samples "${root_dir}/src/icebox/samples/${sample}" OPTIONS executable fmt tidy)
    set_target_output_directory(${sample} "")
    target_compile_definitions(${sample} PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_include_directories(${sample} PUBLIC
        "${root_dir}/third_party/nlohmann_json/include"
    )
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

# Generate BUILD.gen.md from .gitlab-ci.yml
add_custom_target(md_doc ALL
    COMMAND
        ${PYTHON_EXECUTABLE}
        "${root_dir}/build/yml2md.py"
        -i "${root_dir}/.gitlab-ci.yml"
        -o "${root_dir}/doc/BUILD.gen.md"
    DEPENDS
        "${root_dir}/.gitlab-ci.yml"
    COMMENT "Generating BUILD.gen.md"
)

# gtest
set(gtest_dir "${root_dir}/third_party/googletest-1.8.1/googletest")
add_target(gtest third_party "${gtest_dir}/src" "${gtest_dir}/include" OPTIONS recursive external)
target_include_directories(gtest
    PUBLIC "${gtest_dir}/include"
    PRIVATE "${gtest_dir}"
)

# icebox_tests
add_target(icebox_tests tests "${root_dir}/src/icebox/tests" OPTIONS fmt tidy test)
target_link_libraries(icebox_tests PRIVATE
    gtest
    icebox
)
