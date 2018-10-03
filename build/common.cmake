#   Copyright (C) 2017 The YaCo Authors
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

# common cmake functions

set(re_sep "[\\\\/]")

# remove spurious spaces from input property
# which prevent unecessary rebuilds
macro(flatten property)
    string(REPLACE "  " " " ${property} "${${property}}")
    string(REGEX REPLACE "(^ | $)" "" ${property} "${${property}}")
    set(${property} "${${property}}" CACHE STRING "" FORCE)
endmacro()

# set_flag <property> <regexp> <value>
# set flag on property or replace existing values matching input regexp
macro(set_flag property regexp value)
    if(${property} MATCHES ${regexp})
        string(REGEX REPLACE ${regexp} ${value} ${property} "${${property}}")
    else()
        set(${property} "${${property}} ${value}")
    endif()
    flatten(${property})
endmacro()

# add_flag <property> <value>
# add a flag to input property
macro(add_flag property value)
    set_flag(${property} ${value} ${value})
endmacro()

# group_files <group> <root> <files...>
# call source_group on files relatively to root
function(group_files group root)
    foreach(it ${ARGN})
        get_filename_component(dir ${it} PATH)
        file(RELATIVE_PATH relative ${root} ${dir})
        set(local ${group})
        if(NOT "${relative}" STREQUAL "")
            set(local "${group}/${relative}")
        endif()
        string(REGEX REPLACE "[\\\\\\/]+" "\\\\\\\\" local ${local})
        source_group("${local}" FILES ${it})
    endforeach()
endfunction()

# split_args <left> <delimiter> <right> <args...>
# split input arguments into left and right parts on delimiter token
function(split_args left delimiter right)
    set(left_)
    set(has_delimiter false)
    set(right_)
    foreach(it ${ARGN})
        if("${it}" STREQUAL ${delimiter})
            set(has_delimiter true)
        elseif(has_delimiter)
            list(APPEND right_ ${it})
        else()
            list(APPEND left_ ${it})
        endif()
    endforeach()
    set(${left} ${left_} PARENT_SCOPE)
    set(${right} ${right_} PARENT_SCOPE)
endfunction()

# has_item <output> <item> <args...>
# return whether input list has input item
function(has_item output item)
    set(${output} false PARENT_SCOPE)
    foreach(it ${ARGN})
        if("${it}" STREQUAL ${item})
            set(${output} true PARENT_SCOPE)
            return()
        endif()
    endforeach()
endfunction()

# filter_input <keep> <data> <patterns...>
# include/exclude items from data list matching input regexp patterns
function(filter_input keep data)
    set(_data)
    foreach(it ${${data}})
        set(_touch)
        foreach(pattern ${ARGN})
            if(${it} MATCHES ${pattern})
                set(_touch true)
            endif()
        endforeach()
        if((keep EQUAL 1 AND DEFINED _touch) OR (keep EQUAL 0 AND NOT DEFINED _touch))
            list(APPEND _data ${it})
        endif()
    endforeach()
    set(${data} ${_data} PARENT_SCOPE)
endfunction()

# filter_out <data> <patterns...>
# exclude items from data matching input regexp patterns
macro(filter_out data)
    filter_input(0 ${data} ${ARGN})
endmacro()

# filter_in <data> <patterns...>
# include items from data matching input regexp patterns
macro(filter_in data)
    filter_input(1 ${data} ${ARGN})
endmacro()

# get_files <output_files> <input_directories...>
function(get_files output)
    # parse input directories & options
    split_args(dirs "OPTIONS" options ${ARGN})
    set(_glob GLOB)
    has_item(has_recurse "recurse" ${options})
    if(has_recurse)
        set(_glob GLOB_RECURSE)
    endif()
    has_item(has_asm "asm" ${options})

    # glob all input directories
    set(files)
    foreach(it ${dirs})
        set(patterns
            "${it}/*.c"
            "${it}/*.cc"
            "${it}/*.cpp"
            "${it}/*.cxx"
            "${it}/*.fbs"
            "${it}/*.h"
            "${it}/*.hpp"
            "${it}/*.i"
            "${it}/*.in"
            "${it}/*.py"
            "${it}/*.rc"
        )
        if(has_asm)
            set(patterns ${patterns} "${it}/*.asm")
        endif()
        file(${_glob} files_ ${patterns})
        list(APPEND files ${files_})
        group_files(src "${it}" ${files_})
    endforeach()

    # set output variables
    set(${output} ${files} PARENT_SCOPE)
endfunction()

# setup configure files on input target
function(setup_configure target files_ includes_)
    set(in_input ${${files_}})
    filter_in(in_input ".+[.]in$")
    # configure every .in file
    foreach(it ${in_input})
        get_filename_component(outname ${it} NAME)
        string(REGEX REPLACE "[.]in$" "" outname ${outname})
        set(outfile "${CMAKE_CURRENT_BINARY_DIR}/${target}_/${outname}")
        configure_file(${it} ${outfile})
        list(APPEND ${files_} ${outfile})
        list(APPEND ${includes_} "${CMAKE_CURRENT_BINARY_DIR}/${target}_")
        source_group(autogen FILES ${outfile})
    endforeach()
    # set output
    set(${files_} ${${files_}} PARENT_SCOPE)
    set(${includes_} ${${includes_}} PARENT_SCOPE)
endfunction()

# setup flatbuffer generated headers
function(setup_flatbuffers target files_ includes_)
    set(fbs_input ${${files_}})
    filter_in(fbs_input ".+[.]fbs$")
    # configure every fbs file
    foreach(it ${fbs_input})
        get_filename_component(namewe ${it} NAME_WE)
        set(output "${CMAKE_CURRENT_BINARY_DIR}/${namewe}_generated.h")
        add_custom_command(OUTPUT ${output}
            COMMAND ${FLATBUFFERS_FLATC_EXECUTABLE}
            ARGS -c -p -o "${CMAKE_CURRENT_BINARY_DIR}/" ${it}
            COMMENT "${namewe}.fbs"
            DEPENDS "${it}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
        list(APPEND ${files_} ${output})
        list(APPEND ${includes_} ${CMAKE_CURRENT_BINARY_DIR})
        source_group(autogen FILES ${output})
    endforeach()
    # set output
    set(${files_} ${${files_}} PARENT_SCOPE)
    set(${includes_} ${${includes_}} PARENT_SCOPE)
endfunction()

# setup git_version.h & git_version.cpack on input target
function(setup_git target files_ includes_)
    find_package(Git REQUIRED)
    set(root ${CMAKE_CURRENT_BINARY_DIR}/${target}_)

    # generate input configure files
    file(WRITE ${root}/git_version.h.in "
        static const char GitVersion[] = \"@GIT_VERSION@\";
        #define GIT_VERSION() \"@GIT_VERSION@\"
    ")
    file(WRITE ${root}/git_version.cpack.in "
        set(CPACK_PACKAGE_VERSION     \"@GIT_VERSION@\")
        set(CPACK_PACKAGE_FILE_NAME   \"\${CPACK_PACKAGE_NAME}-\${CPACK_PACKAGE_VERSION}\")
        set(CPACK_PACKAGE_DESCRIPTION \"\${CPACK_PACKAGE_NAME} \${CPACK_PACKAGE_VERSION}\")
    ")

    # generate compile-time git version configuration
    file(WRITE ${root}/git_version.cmake "
        execute_process(COMMAND
            ${GIT_EXECUTABLE} describe --dirty --tags --long
            WORKING_DIRECTORY \"${root_dir}\"
            OUTPUT_VARIABLE GIT_VERSION
            RESULT_VARIABLE retcode
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(NOT \"$\{retcode}\" STREQUAL \"0\")
            message(FATAL_ERROR \"${GIT_EXECUTABLE} describe --dirty --tags --long: returned $\{retcode}\")
        endif()
        configure_file(\"\${SRC}\" \"\${DST}\" @ONLY)
    ")

    # add custom git version command
    add_custom_command(OUTPUT
        "${root}/git_version.dependency"
        COMMAND ${CMAKE_COMMAND}
            -D SRC=${root}/git_version.h.in
            -D DST=${root}/git_version.h
            -P ${root}/git_version.cmake
        COMMAND ${CMAKE_COMMAND}
            -D SRC=${root}/git_version.cpack.in
            -D DST=${root}/git_version.cpack
            -P ${root}/git_version.cmake
    )

    # write empty files to force inclusion
    if(NOT EXISTS ${root}/git_version.h)
        file(WRITE "${root}/git_version.h" "")
    endif()
    if(NOT EXISTS ${root}/git_version.cpack)
        file(WRITE "${root}/git_version.cpack" "")
    endif()

    # add all artifacts to target files & includes
    list(APPEND ${files_}
        ${root}/git_version.cpack
        ${root}/git_version.dependency
        ${root}/git_version.h
    )
    list(APPEND ${includes_} ${root})
    source_group(autogen FILES
        ${root}/git_version.cpack
        ${root}/git_version.cpack.in
        ${root}/git_version.dependency
        ${root}/git_version.h
        ${root}/git_version.h.in
    )

    # set output
    set(${files_}    ${${files_}}    PARENT_SCOPE)
    set(${includes_} ${${includes_}} PARENT_SCOPE)
endfunction()

# add input includes to target
function(add_target_includes target includes_)
    list(LENGTH ${includes_} size)
    if(NOT ${size} GREATER 0)
        return()
    endif()
    list(REMOVE_DUPLICATES ${includes_})
    list(SORT ${includes_})
    target_include_directories(${target} PRIVATE ${${includes_}})
endfunction()

# set up msvc-specific options on input target
function(setup_msvc target)
    # read options
    split_args(ignore "OPTIONS" options ${ARGN})
    has_item(is_executable "executable" ${options})
    has_item(is_test "test" ${options})
    has_item(is_shared "shared" ${options})
    has_item(has_warnings "warnings" ${options})
    has_item(is_external "external" ${options})
    has_item(is_static_runtime "static_runtime" ${options})

    target_compile_options(${target} PRIVATE "$<$<CONFIG:Debug>:/RTC1>")

    # add back standard libraries
    if(is_executable OR is_test OR is_shared)
        target_link_libraries(${target} PRIVATE ${STANDARD_LIBRARIES})
    endif()

    # add static runtime library link options
    set(mdd "/MDd")
    set(md  "/MD")
    if(is_static_runtime)
        set(mdd "/MTd")
        set(md  "/MT")
    endif()
    target_compile_options(${target} PRIVATE
        "$<$<CONFIG:Debug>:${mdd}>"
        "$<$<CONFIG:RelWithDebInfo>:${md}>"
        "$<$<CONFIG:Release>:${md}>"
    )

    # enable maximum warnings
    if(is_external)
        target_compile_options(${target}
            PRIVATE "/W0" # ignore warnings
        )
    elseif(has_warnings)
        target_compile_options(${target}
            PRIVATE "/W3" # default warnings
        )
    else()
        target_compile_options(${target}
            PRIVATE "/W4" # maximum warnings
            PRIVATE "/WX" # warnings as error
        )
    endif()
endfunction()

function(setup_gcc target)
    split_args(ignore "OPTIONS" options ${ARGN})
    has_item(has_warnings "warnings" ${options})
    has_item(is_external "external" ${options})
    if(is_external)
        target_compile_options(${target} PRIVATE "--no-warnings")
    elseif(has_warnings)
        target_compile_options(${target} PRIVATE "-Wall")
    else()
        target_compile_options(${target} PRIVATE
            "-Wall"
            "-Wextra"
            "-Werror"
            "-Wformat-security" # enabled by default on ubuntu so enable it for all
        )
    endif()
endfunction()

function(setup_target target)
    if(MSVC)
        setup_msvc(${target} OPTIONS ${ARGN})
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        setup_gcc(${target} OPTIONS ${ARGN})
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        setup_gcc(${target} OPTIONS ${ARGN})
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
        setup_gcc(${target} OPTIONS ${ARGN})
    else()
        message(FATAL_ERROR "unsupported compiler ${CMAKE_CXX_COMPILER_ID}")
    endif()
endfunction()

# auto-generate external cmake file containing all inputs
function(autogen_files target)
    set(files ${ARGN})
    list(REMOVE_DUPLICATES files)
    list(SORT files)
    set(data "# generated with cmake\nset(_${target}_files\n")
    foreach(it ${files})
        file(RELATIVE_PATH it ${CMAKE_CURRENT_SOURCE_DIR} ${it})
        set(data "${data}    \"${it}\"\n")
    endforeach()
    set(data "${data})")
    set(filename "${CMAKE_CURRENT_SOURCE_DIR}/files/${target}.${OS}.files.cmake")
    set(got)
    if(EXISTS "${filename}")
        file(READ "${filename}" got)
    endif()
    set(tmp_filename "${CMAKE_CURRENT_BINARY_DIR}/${target}.dir/build.list")
    file(WRITE "${tmp_filename}" "${data}")
    configure_file("${tmp_filename}" "${filename}" NEWLINE_STYLE LF)
    include(${filename})
    set(_${target}_files ${_${target}_files} PARENT_SCOPE)
endfunction()

# make_target <target> <group> <files...> [INCLUDES <includes...>] [OPTIONS <options...>]
# add a new target and apply options
# targets are static libraries by default
# available options are
# - executable      build an executable
# - test            build a test executable
# - shared          build a shared library
# - warnings        allow warnings
# - configure       auto-configure *.in files
# - static_runtime  statically link to libc runtime
# - git_version     auto-configure a git_version.h header
# - flatbuffers     auto-configure *.fbs flatbuffer specs
# - win32           build windows executable instead of console
# - external        disable warnings on external projects
function(make_target target group)
    message("-- Configuring ${group}/${target}")

    # get options
    split_args(inputs "OPTIONS" options ${ARGN})
    split_args(files "INCLUDES" includes ${inputs})
    has_item(is_executable "executable" ${options})
    has_item(is_test "test" ${options})
    has_item(is_shared "shared" ${options})
    has_item(has_configure "configure" ${options})
    has_item(has_fmt "fmt" ${options})
    has_item(has_flatbuffers "flatbuffers" ${options})
    has_item(has_git_version "git_version" ${options})
    has_item(is_win32 "win32" ${options})

    # sort files
    list(SORT files)

    # initialize additional include directory list
    set(includes)

    # add/remove configure files
    if(has_configure)
        setup_configure(${target} files includes)
    else()
        filter_out(files ".+[.]in$")
    endif()

    # add flatbuffer generated files
    if(has_flatbuffers)
        setup_flatbuffers(${target} files includes)
    else()
        filter_out(files ".+[.]fbs$")
    endif()

    # remove swig generated files
    filter_out(files ".+[.]i$")

    # add generated git version headers
    if(has_git_version)
        setup_git(${target} files includes)
    endif()

    # include auto-generated file list
    autogen_files(${target} ${files})
    set(files ${_${target}_files})

    # add the target
    if(is_executable OR is_test)
        set(suffix "")
        if(is_win32)
            set(suffix "WIN32")
        endif()
        add_executable(${target} ${suffix} ${files})
        if(is_test)
            add_test(NAME ${target} COMMAND ${target})
            #add_custom_command(TARGET ${target} POST_BUILD
            #    COMMAND $<TARGET_FILE:${target}>)
        endif()
    elseif(is_shared)
        add_library(${target} SHARED ${files})
    else()
        add_library(${target} STATIC ${files})
    endif()

    # setup compiler dependent options
    setup_target(${target} ${options})

    # add all additional include directories
    add_target_includes(${target} includes)

    # set directories for visual
    source_group(cmake REGULAR_EXPRESSION [.]rule$)
    source_group(cmake FILES CMakeLists.txt)
    set_target_properties(${target} PROPERTIES FOLDER ${group})
endfunction()

# add_target <target> <group> <directories...> [OPTIONS <options...>]
# auto generate target from directories and apply options
function(add_target target group)
    get_files(files ${ARGN})
    split_args(args "OPTIONS" options ${ARGN})
    make_target(${target} ${group} ${files} OPTIONS ${options})
endfunction()

function(split_swig_files itarget deps)
    set(itarget_)
    set(deps_)
    foreach(it ${ARGN})
        if(${it} MATCHES "[.]i$")
            list(APPEND itarget_ ${it})
        else()
            list(APPEND deps_ ${it})
        endif()
    endforeach()
    set(${itarget} ${itarget_} PARENT_SCOPE)
    set(${deps} ${deps_} PARENT_SCOPE)
endfunction()

function(add_swig_module target group)
    split_args(right_args "INCLUDES" includes ${ARGN})
    split_args(files "DEPS" deps ${right_args})
    split_swig_files(itarget others ${files})
    get_filename_component(outname ${itarget} NAME_WE)
    set(outi   "${CMAKE_CURRENT_BINARY_DIR}/${outname}.i")
    set(outpy  "${CMAKE_CURRENT_BINARY_DIR}/${outname}.py")
    set(outcpp "${CMAKE_CURRENT_BINARY_DIR}/${outname}PYTHON_wrap.cxx")
    set_property(SOURCE ${outi} PROPERTY CPLUSPLUS ON)
    source_group(autogen FILES ${outpy} ${outcpp} ${outi})
    # foreach input file create a custom command
    # which depend on every dependency and work-around
    # broken dependency handling from swig_add_module
    # we also create a dummy copy of input file &
    # ensure make clean doesn't clean original file
    add_custom_command(
        OUTPUT  "${outi}"
        COMMAND ${CMAKE_COMMAND} -E copy "${itarget}" "${outi}"
        DEPENDS ${itarget} ${deps}
        COMMENT "${outname}.i"
    )
    get_directory_property(backup_includes INCLUDE_DIRECTORIES)
    list(APPEND includes "${SWIG_DIR}/python" "${SWIG_DIR}")
    set_directory_properties(PROPERTIES INCLUDE_DIRECTORIES "${includes}")
    message("-- Configuring ${group}/_${target}")
    if(${CMAKE_VERSION} VERSION_LESS "3.8.0")
        swig_add_module(${target} python ${outi} ${others})
    else()
        swig_add_library(${target} LANGUAGE python SOURCES ${outi} ${others})
    endif()
    set_directory_properties(PROPERTIES INCLUDE_DIRECTORIES "${backup_includes}")
    set_target_properties(_${target} PROPERTIES FOLDER ${group})
endfunction()

# set_target_output_directory <target> <suffix>
# set target output directory at $bin(_d)?_dir/$suffix
function(set_target_output_directory target suffix)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${bin_d_dir}/${suffix}"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${bin_dir}/${suffix}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${bin_dir}/${suffix}"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG          "${bin_d_dir}/${suffix}"
        LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${bin_dir}/${suffix}"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE        "${bin_dir}/${suffix}"
    )
endfunction()

# exclude target from build
function(exclude_target target)
    set_target_properties(${target} PROPERTIES
        EXCLUDE_FROM_ALL true
        EXCLUDE_FROM_DEFAULT_BUILD true
    )
endfunction()

# set_target_output_name <target> <release_name> <debug_name>
function(set_target_output_name target release debug)
    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME_DEBUG           ${debug}
        OUTPUT_NAME_RELWITHDEBINFO  ${release}
        OUTPUT_NAME_RELEASE         ${release}
    )
endfunction()

# deploy <src> to $bin_dir/$dst in <target> post build event
function(deploy_to_bin target src dst)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${src}
            $<$<CONFIG:Debug>:${bin_d_dir}>$<$<CONFIG:RelWithDebInfo>:${bin_dir}>$<$<CONFIG:Release>:${bin_dir}>${dst}
        VERBATIM
    )
endfunction()

include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckVariableExists)
include(CheckTypeSize)

function(todef output input)
    string(TOUPPER ${input} output_)
    string(REGEX REPLACE "[^a-zA-Z0-9]" "_" output_ HAVE_${output_})
    set(${output} ${output_} PARENT_SCOPE)
endfunction()

function(parse_config input)
    set(re_fun "`([^']+)' function\.")
    set(re_cmake_fun "cmakedefine[ ]+HAVE_([A-Z_0-9]+)")
    set(re_inc "<([^>]+)> header file\.")
    set(re_cmake_inc "cmakedefine[ ]+HAVE_([A-Z_0-9]+)_H")
    set(re_type "<([^>]+)> declares ([A-Z_a-z0-9]+).")
    set(re_type_only " if you have the '([A-Za-z_0-9]+)' type.")
    set(re_sys_type " system has the type `([^']+)'.")

    # read input file
    file(READ ${input} contents)
    set(functions_)
    set(includes_)

    # match types with extra header
    string(REGEX MATCHALL "${re_type}" matchs ${contents})
    foreach(match ${matchs})
        string(REGEX REPLACE "${re_type}" "\\1" header ${match})
        string(REGEX REPLACE "${re_type}" "\\2" type ${match})
        todef(define ${type})
        set(CMAKE_EXTRA_INCLUDE_FILES ${header})
        check_type_size(${type} ${define})
    endforeach()

    # match types without headers
    string(REGEX MATCHALL "${re_type_only}" matchs ${contents})
    foreach(match ${matchs})
        string(REGEX REPLACE "${re_type_only}" "\\1" match ${match})
        todef(define ${match})
        check_type_size(${match} ${define})
    endforeach()

    # match system types
    string(REGEX MATCHALL "${re_sys_type}" matchs ${contents})
    foreach(match ${matchs})
        string(REGEX REPLACE "${re_sys_type}" "\\1" match ${match})
        todef(define ${match})
        check_type_size(${match} ${define})
    endforeach()

    # match functions
    string(REGEX MATCHALL "${re_fun}" matchs ${contents})
    foreach(match ${matchs})
        string(REGEX REPLACE "${re_fun}" "\\1" match ${match})
        list(APPEND functions_ ${match})
    endforeach()

    # match cmake functions
    string(REGEX MATCHALL "${re_cmake_fun}" matchs ${contents})
    foreach(match ${matchs})
		string(REGEX MATCHALL "${re_cmake_inc}" inc_match ${match})
		list(LENGTH inc_match size)
		if (size EQUAL 0)
		    string(REGEX REPLACE "${re_cmake_fun}" "\\1" match ${match})
			string(TOLOWER "${match}" match)
		    list(APPEND functions_ ${match})
		endif()
    endforeach()

    # match includes
    string(REGEX MATCHALL "${re_inc}" matchs ${contents})
    foreach(match ${matchs})
        string(REGEX REPLACE "${re_inc}" "\\1" match ${match})
        list(APPEND includes_ ${match})
    endforeach()

    # match cmake includes
    string(REGEX MATCHALL "${re_cmake_inc}" matchs ${contents})
    foreach(match ${matchs})
        string(REGEX REPLACE "${re_cmake_inc}" "\\1" match ${match})
        string(REPLACE "_" "/" match ${match})
		string(TOLOWER "${match}.h" match)
        list(APPEND includes_ ${match})
    endforeach()

    # check each function
    list(LENGTH functions_ size)
    if(size)
        list(REMOVE_DUPLICATES functions_)
        list(SORT functions_)
    endif()
    foreach(it ${functions_})
        todef(define_it ${it})
        check_function_exists("${it}" ${define_it})
    endforeach()

    # check each include
    list(LENGTH includes_ size)
    if(size)
        list(REMOVE_DUPLICATES includes_)
        list(SORT includes_)
    endif()
    foreach(it ${includes_})
        todef(define_it ${it})
        check_include_file("${it}" ${define_it})
    endforeach()
endfunction()

function(fixcfg output input)
    file(READ "${input}" output_h)
    string(REGEX REPLACE "\#undef (HAVE_[a-zA-Z0-9_]+)"
        "#cmakedefine01 \\1\n#if !\\1\n#undef \\1\n#endif" output_h ${output_h})
    foreach(it ${ARGN})
        set(output_h "${output_h} ${it}")
    endforeach()
    file(WRITE "${output}" "${output_h}")
endfunction()

function(cfg_file outputs input output)
    configure_file("${input}" "${output}")
    set(${outputs} ${${outputs}} "${input}" "${output}" PARENT_SCOPE)
    source_group(autogen FILES ${output})
endfunction()

# autoconfigure input config.h.in file
function(autoconfigure outputs includes target input)
    parse_config(${input})
    string(REPLACE "_cmake.h" ".h" input_fix ${input})
    get_filename_component(filename ${input_fix} NAME)
    get_filename_component(filename_we ${input_fix} NAME_WE)
    set(tgt_dir ${CMAKE_CURRENT_BINARY_DIR}/${target}_)
    set(output "${tgt_dir}/${filename}")
    set(output_we "${tgt_dir}/${filename_we}.h")
    fixcfg("${output}" "${input}" ${ARGN})
    cfg_file(outputs_ "${output}" "${output_we}")
    set(${outputs} ${${outputs}} ${outputs_} PARENT_SCOPE)
    set(${includes} ${tgt_dir} PARENT_SCOPE)
endfunction()

# set current arch flag
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH x64)
else()
    set(ARCH x86)
endif()
string(TOUPPER ARCH_${ARCH} arch_define)
add_definitions(-D${arch_define})
if(WIN32)
    set(OS nt)
elseif(APPLE)
    set(OS mac)
else()
    set(OS linux)
endif()
message("-- Configuring for ${OS}_${ARCH}")

# customize c & cpp compile flags
function(set_cx_flags config regexp value)
    # prepend config name with _ when not empty
    if(NOT "${config}" STREQUAL "")
        set(config "_${config}")
    endif()
    set_flag(CMAKE_C_FLAGS${config}   ${regexp} ${value})
    set_flag(CMAKE_CXX_FLAGS${config} ${regexp} ${value})
    # squash language independent flags too
    set_flag(CMAKE_C_FLAGS   ${regexp} ${value})
    set_flag(CMAKE_CXX_FLAGS ${regexp} ${value})
endfunction()

# call set_flag on all configurations & all kind of targets
function(set_flag_all property regexp output)
    foreach(it ${ARGN})
        foreach(item IN ITEMS
            EXE
            MODULE
            SHARED
        )
            set_flag(CMAKE_${item}_${property}_${it} ${regexp} ${output})
        endforeach()
    endforeach()
endfunction()

if(MSVC)
    # enable parallel compilation
    set_cx_flags("" "/MP" "/MP")
    # disable runtime checks (and add them back per target)
    set_cx_flags(DEBUG "/RTC1" " ")
    # disable broken incremental compilations on release
    set_flag_all(LINKER_FLAGS "/INCREMENTAL(:(YES|NO))?" "/INCREMENTAL:NO" RELEASE RELWITHDEBINFO)
    # remove unused symbols on release
    set_flag_all(LINKER_FLAGS "/OPT:REF" "/OPT:REF" RELEASE RELWITHDEBINFO)
    set_flag_all(LINKER_FLAGS "/OPT:ICF" "/OPT:ICF" RELEASE RELWITHDEBINFO)
    # disable manifests
    set_flag_all(LINKER_FLAGS "/MANIFESTUAC(:(YES|NO))?" " " RELEASE RELWITHDEBINFO)
    set_flag_all(LINKER_FLAGS "/MANIFEST(:(YES|NO))?" "/MANIFEST:NO" RELEASE RELWITHDEBINFO)
    # enable function-level linking and reduce binary size on release
    set_cx_flags(RELWITHDEBINFO "/Gy" "/Gy")
    set_cx_flags(RELEASE        "/Gy" "/Gy")
    # disable runtime link flags (and add them back per target)
    set_cx_flags(DEBUG          "/(MD|MT)d" " ")
    set_cx_flags(RELWITHDEBINFO "/(MD|MT)"  " ")
    set_cx_flags(RELEASE        "/(MD|MT)"  " ")
    # disable warnings (and add them back per target)
    set_cx_flags(DEBUG          "/W[0-9]" " ")
    set_cx_flags(RELWITHDEBINFO "/W[0-9]" " ")
    set_cx_flags(RELEASE        "/W[0-9]" " ")
    # save & disable default libraries
    if(NOT "${CMAKE_C_STANDARD_LIBRARIES}" STREQUAL "")
        set(STANDARD_LIBRARIES ${CMAKE_C_STANDARD_LIBRARIES})
        string(REPLACE " " ";" STANDARD_LIBRARIES ${STANDARD_LIBRARIES})
        set(libs_)
        foreach(it IN ITEMS ${STANDARD_LIBRARIES})
            string(REPLACE ".lib" "" it ${it})
            list(APPEND libs_ ${it})
        endforeach()
        set(STANDARD_LIBRARIES "${libs_}" CACHE STRING "" FORCE)
    endif()
    set(CMAKE_CXX_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)
    set(CMAKE_C_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)
    # check errors
    if(${CMAKE_C_FLAGS_DEBUG} MATCHES "/RTC1" OR
       ${CMAKE_C_FLAGS_DEBUG} MATCHES "(/MD|MT)")
        message(FATAL_ERROR "invalid CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG}")
    endif()
    if(NOT "${CMAKE_C_STANDARD_LIBRARIES}" STREQUAL ""
       OR  "${STANDARD_LIBRARIES}" STREQUAL "")
        message(FATAL_ERROR "invalid CMAKE_CXX_STANDARD_LIBRARIES ${CMAKE_C_STANDARD_LIBRARIES}/${STANDARD_LIBRARIES}")
    endif()
endif()

# enable visual studio folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "_cmake")

if("${OS}" STREQUAL "linux")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()
