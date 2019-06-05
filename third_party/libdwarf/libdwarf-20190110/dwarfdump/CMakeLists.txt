include(configure.cmake)

add_library(esb_and_tsearchbal OBJECT esb.c dwarf_tsearchbal.c)

set_folder(esb_and_tsearchbal dwarfdump)

target_include_directories(esb_and_tsearchbal PRIVATE
    $<TARGET_PROPERTY:${dwarf-target},INTERFACE_INCLUDE_DIRECTORIES>)

msvc_posix(esb_and_tsearchbal)

set_source_group(SOURCES "Source Files" addrmap.c checkutil.c dwarfdump.c dwconf.c helpertree.c 
	glflags.c command_options.c compiler_info.c
	macrocheck.c print_abbrevs.c print_aranges.c print_debugfission.c print_die.c 
	print_dnames.c print_frames.c  print_gdbindex.c
	print_lines.c print_locs.c print_macro.c print_macros.c print_pubnames.c print_ranges.c print_reloc.c 
	print_str_offsets.c
	print_sections.c  print_section_groups.c print_static_funcs.c  print_static_vars.c print_strings.c print_types.c print_weaknames.c  
	sanitized.c section_bitmaps.c strstrnocase.c true_section_name.c uri.c dwgetopt.c makename.c naming.c common.c $<TARGET_OBJECTS:esb_and_tsearchbal>)
	
set_source_group(HEADERS "Header Files" checkutil.h common.h dwconf.h
  command_options.h compiler_info.h
  dwconf_using_functions.h esb_using_functions.h
  dwgetopt.h esb.h glflags.h globals.h macrocheck.h defined_types.h
  makename.h dwarf_tsearch.h print_frames.h section_bitmaps.h uri.h)

set_source_group(CONFIGURATION_FILES "Configuration Files" configure.cmake config.h.in.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
	
set_source_group(TAG_GENERATED "Generated Files" tmp-tt-table.h tmp-ta-table.h tmp-ta-ext-table.h tmp-tt-ext-table.h)

add_executable(dwarfdump ${SOURCES} ${HEADERS} ${TAG_GENERATED} ${CONFIGURATION_FILES})

set_folder(dwarfdump dwarfdump)

target_compile_options(dwarfdump PRIVATE ${dwfwall})

target_compile_definitions(dwarfdump PRIVATE "CONFPREFIX=${CMAKE_INSTALL_PREFIX}/lib")

target_link_libraries(dwarfdump PRIVATE ${dwarf-target} ${dwfzlib}) 

# We need this as naming.o has external references we cannot have
# in the tree builds.
configure_file(naming.c trivial_naming.c COPYONLY)
set_source_files_properties(trivial_naming.c PROPERTIES COMPILE_DEFINITIONS TRIVIAL_NAMING)

set(TAG_TREE_SOURCES tag_tree.c tag_common.c common.c makename.c trivial_naming.c dwgetopt.c dwarf_tsearchbal.c sanitized.c esb.c)

add_executable(tag_tree_build ${TAG_TREE_SOURCES})

set_folder(tag_tree_build dwarfdump/generators)

target_link_libraries(tag_tree_build PRIVATE ${dwarf-target})

set(TAG_ATTR_SOURCES tag_attr.c tag_common.c common.c makename.c trivial_naming.c dwgetopt.c dwarf_tsearchbal.c sanitized.c esb.c)
 
add_executable(tag_attr_build ${TAG_ATTR_SOURCES}) 

set_folder(tag_attr_build dwarfdump/generators)

target_link_libraries(tag_attr_build PRIVATE ${dwarf-target})

# Plain GNU C dash E does not work on a .list, 
# so copy to a .c name to run
# the following four table creations.
set(TAG_LIST_FILES tag_tree.list tag_attr.list tag_attr_ext.list tag_tree_ext.list)
set(TAG_SWITCH -s -s -e -e)
set(TAG_TOOL tag_tree_build tag_attr_build tag_attr_build tag_tree_build)
foreach(i RANGE 0 3)
    list(GET TAG_GENERATED ${i} generatedFile)
    list(GET TAG_LIST_FILES ${i} listFile)
    list(GET TAG_SWITCH ${i} tagSwitch)
    list(GET TAG_TOOL ${i} tagTool)
    
    math(EXPR j "${i} + 1")
    
    set(tmpSouce tmp-t${j})
	set(tmpDest tmp-tag-build${j}.tmp)
    
    configure_file(${listFile} ${tmpSouce}.c COPYONLY)
    
    add_custom_command(OUTPUT ${generatedFile}
		COMMAND ${CMAKE_C_COMPILER} -E -I${CMAKE_CURRENT_SOURCE_DIR}/../libdwarf -DCONFPREFIX=${CMAKE_INSTALL_PREFIX}/lib ${tmpSouce}.c > ${tmpDest}
        COMMAND ${tagTool} ${tagSwitch} -i ${tmpDest} -o ${generatedFile}
        DEPENDS ${tagTool} ${listFile})
endforeach()
	
set_source_group(TESTESB_SOURCES "Source Files" esb.c testesb.c)

add_executable(testesb ${TESTESB_SOURCES})

set_folder(testesb dwarfdump/tests)

target_include_directories(testesb PRIVATE
    $<TARGET_PROPERTY:${dwarf-target},INTERFACE_INCLUDE_DIRECTORIES>)

add_test(NAME test COMMAND testesb)

set_source_group(GETOPTEST_SOURCES "Source Files" getopttest.c dwgetopt.c)

add_executable(getopttest ${GETOPTEST_SOURCES})

set_folder(getopttest dwarfdump/tests)

add_test(NAME getopttest COMMAND getopttest)

add_executable(getopttestnat ${GETOPTEST_SOURCES})

set_folder(getopttestnat dwarfdump/tests)

if(UNIX)
	target_compile_definitions(getopttestnat PRIVATE GETOPT_FROM_SYSTEM)
endif()

foreach(i 1 2 3 5 6 7 8)
	add_test(NAME getopttestnat${i} COMMAND getopttestnat -c ${i})
endforeach()
	
set_source_group(SELFHELPERTREE_SOURCES "Source Files" helpertree.c)

add_executable(selfhelpertree ${SELFHELPERTREE_SOURCES} $<TARGET_OBJECTS:esb_and_tsearchbal>)

set_folder(selfhelpertree dwarfdump/tests)

target_compile_definitions(selfhelpertree PRIVATE SELFTEST)

target_include_directories(selfhelpertree PRIVATE
    $<TARGET_PROPERTY:${dwarf-target},INTERFACE_INCLUDE_DIRECTORIES>)

add_test(NAME selfhelpertree COMMAND selfhelpertree)

#doesn't compile	
#set_source_group(SELFMC_SOURCES "Source Files" macrocheck.c $<TARGET_OBJECTS:esb_and_tsearchbal>)
#
#add_executable(selfmc ${SELFMC_SOURCES})
#
# set_folder(selfmc dwarfdump/tests)
#
#target_compile_definitions(selfmc PRIVATE SELFTEST)
#
#target_include_directories(selfmc PRIVATE
#    $<TARGET_PROPERTY:${dwarf-target},INTERFACE_INCLUDE_DIRECTORIES>)
#
#add_test(NAME selfmc COMMAND selfmc)
	
#set_source_group(SELFESB_SOURCES "Source Files" esb.c)
#
#add_executable(selfesb ${SELFESB_SOURCES})
#
#set_folder(selfesb dwarfdump/tests)
#
#target_compile_definitions(selfesb PRIVATE SELFTEST)

#target_include_directories(selfesb PRIVATE
#    $<TARGET_PROPERTY:${dwarf-target},INTERFACE_INCLUDE_DIRECTORIES>)

#add_test(NAME selfesb COMMAND selfesb)

if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
	set(SUFFIX 64)
endif()
set(LIBDIR lib${SUFFIX})
set(BINDIR bin${SUFFIX})

install(TARGETS dwarfdump DESTINATION
	RUNTIME DESTINATION ${BINDIR}		
	LIBRARY DESTINATION ${LIBDIR}
	ARCHIVE DESTINATION ${LIBDIR})

install(FILES dwarfdump.conf DESTINATION ${LIBDIR})

install(FILES dwarfdump.1 DESTINATION share/man/man1)
