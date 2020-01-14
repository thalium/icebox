
set_source_group(SOURCES "Source Files" addrmap.c 
        checkutil.c dwarfdump.c dwconf.c helpertree.c 
	glflags.c command_options.c compiler_info.c
        dwarf_names.c
	macrocheck.c print_abbrevs.c print_aranges.c
        print_debugfission.c print_die.c 
	print_dnames.c print_frames.c  print_gdbindex.c
	print_lines.c print_locs.c print_macro.c 
        print_macros.c print_pubnames.c print_ranges.c print_reloc.c 
	print_str_offsets.c
	print_sections.c  print_section_groups.c print_static_funcs.c
        print_static_vars.c print_strings.c print_types.c 
        print_weaknames.c  
	sanitized.c section_bitmaps.c strstrnocase.c 
        true_section_name.c uri.c dwgetopt.c makename.c 
        naming.c common.c esb.c dwarf_tsearchbal.c)
	
set_source_group(HEADERS "Header Files" 
  addrmap.h checkutil.h common.h dwconf.h
  command_options.h compiler_info.h
  dwarf_names.h
  dwconf_using_functions.h esb_using_functions.h
  dwarfdump-ta-ext-table.h dwarfdump-ta-table.h 
  dwarfdump-tt-ext-table.h dwarfdump-tt-table.h
  dwgetopt.h esb.h glflags.h globals.h macrocheck.h defined_types.h
  makename.h dwarf_tsearch.h print_frames.h 
  print_reloc.h print_reloc_decls.h section_bitmaps.h uri.h)

set_source_group(CONFIGURATION_FILES "Configuration Files"
  ${CMAKE_SOURCE_DIR}/config.h.in.cmake
  ${CMAKE_BINARY_DIR}/config.h)
	
add_executable(dwarfdump ${SOURCES} ${HEADERS} ${CONFIGURATION_FILES})

set_folder(dwarfdump dwarfdump)


target_compile_definitions(dwarfdump PRIVATE 
    "CONFPREFIX=${CMAKE_INSTALL_PREFIX}/lib")
target_compile_options(dwarfdump PRIVATE ${DW_FWALL})

target_link_libraries(dwarfdump PRIVATE ${dwarf-target} ${DW_FZLIB}) 

# Plain GNU C dash E does not work on a .list, 
# so copy to a .c name to run
# the following four table creations.
set(SELFTEST "-DSELFTEST")
	
if (DO_TESTING) 
    set_source_group(GETOPTEST_SOURCES "Source Files" 
      getopttest.c dwgetopt.c)
    add_executable(selfgetopttest ${GETOPTEST_SOURCES})
    target_compile_options(selfgetopttest PRIVATE ${DW_FWALL})
    add_test(NAME selfgetopttest COMMAND selfgetopttest)
endif()    
    
if (DO_TESTING) 
    set_source_group(TESTESB_SOURCES "Source Files" esb.c testesb.c
       dwarf_tsearchbal.c)
    add_executable(selftestesb ${TESTESB_SOURCES})
    target_compile_options(selftestesb PRIVATE ${SELFTEST} )
    target_compile_options(selftestesb PRIVATE ${DW_FWALL})
    add_test(NAME selftestesb COMMAND selftestesb)
endif()

if (DO_TESTING) 
    set_source_group(TESTESB_SOURCES "Source Files" getopttest.c
        dwgetopt.c)
    add_executable(selfgetopttestnat ${GETOPTEST_SOURCES})
    target_compile_options(selfgetopttestnat PRIVATE ${DW_FWALL})
    if(UNIX)
	    target_compile_definitions(selfgetopttestnat 
            PRIVATE GETOPT_FROM_SYSTEM)
    endif()
    foreach(i 1 2 3 5 6 7 8 9 10)
	    add_test(NAME selfgetopttestnat${i} 
                COMMAND selfgetopttestnat -c ${i})
    endforeach()
endif()

if (DO_TESTING) 
    set_source_group(MAKENAME_SOURCES "Source Files"
       makename_test.c esb.c dwarf_tsearchbal.c makename.c)
    add_executable(selfmakename ${MAKENAME_SOURCES})
    target_compile_options(selfmakename PRIVATE ${DW_FWALL})
    target_compile_options(selfmakename PRIVATE 
        "-I${CMAKE_SOURCE_DIR}/libdwarf")
    target_compile_options(selfmakename PRIVATE 
        "-I${CMAKE_BINARY_DIR}/libdwarf")
    add_test(NAME selfmakename COMMAND selfmakename)
endif()


if (DO_TESTING) 
    set_source_group(HELPERTREE_SOURCES "Source Files"
       helpertree_test.c helpertree.c dwarf_tsearchbal.c)
    add_executable(selfhelpertree ${HELPERTREE_SOURCES})
    target_compile_options(selfhelpertree PRIVATE ${DW_FWALL})
    target_compile_options(selfhelpertree PRIVATE 
        "-I${CMAKE_SOURCE_DIR}/dwarfdump")
    target_compile_options(selfhelpertree PRIVATE 
        "-I${CMAKE_SOURCE_DIR}/libdwarf")
    target_compile_options(selfhelpertree PRIVATE 
        "-I${CMAKE_BINARY_DIR}/libdwarf")
    add_test(NAME selfhelpertree COMMAND selfhelpertree)
endif()

if (DO_TESTING) 
    set_source_group(SELFMC_SOURCES "Source Files" macrocheck.c
        esb.c dwarf_tsearchbal.c)
    add_executable(selfmc ${SELFMC_SOURCES} )
    target_compile_options(selfmc PRIVATE ${SELFTEST} )
    target_compile_options(selfmc PRIVATE ${DW_FWALL})
    target_compile_options(selfmc PRIVATE 
        "-I${CMAKE_SOURCE_DIR}/libdwarf")
    target_compile_options(selfmc PRIVATE 
        "-I${CMAKE_BINARY_DIR}/libdwarf")
    add_test(NAME selfmc COMMAND selfmc)
endif()

if (DO_TESTING) 
    set_source_group(SELFESB_SOURCES "Source Files"
        testesb.c esb.c)
    add_executable(selfesb ${SELFESB_SOURCES})
    target_compile_options(selfesb PRIVATE ${SELFTEST} )
    target_compile_options(selfesb PRIVATE ${DW_FWALL})
    target_compile_options(selfmakename PRIVATE 
        "-I${CMAKE_SOURCE_DIR}/libdwarf")
    target_compile_options(selfmakename PRIVATE 
        "-I${CMAKE_BINARY_DIR}/libdwarf")
    add_test(NAME selfesb COMMAND selfesb)
endif()

if (DO_TESTING) 
    set_source_group(SECTIONBITMAPS_SOURCES "Source Files" 
        section_bitmaps.c section_bitmaps_test.c )
    add_executable(selfsectionbitmaps ${SECTIONBITMAPS_SOURCES})
    target_compile_options(selfsectionbitmaps PRIVATE 
        ${SELFTEST} )
    target_compile_options(selfsectionbitmaps PRIVATE ${DW_FWALL})
    target_compile_options(selfsectionbitmaps PRIVATE 
        "-I${CMAKE_SOURCE_DIR}/libdwarf")
    target_compile_options(selfsectionbitmaps PRIVATE 
        "-I${CMAKE_BINARY_DIR}/libdwarf")
    add_test(NAME selfsectionbitmaps COMMAND selfsectionbitmaps)
endif()

if (DO_TESTING) 
    set_source_group(SELFPRINTRELOC_SOURCES "Source Files" 
        esb.c print_reloc_test.c)
    add_executable(selfprintreloc ${SELFPRINTRELOC_SOURCES})
    # We need access to a few relocation-data libdwarf headers
    target_compile_options(selfprintreloc PRIVATE ${SELFTEST} )
    target_compile_options(selfprintreloc PRIVATE ${DW_FWALL})
    target_compile_options(selfprintreloc PRIVATE 
        "-I${CMAKE_SOURCE_DIR}/libdwarf")
    target_compile_options(selfprintreloc PRIVATE 
        "-I${CMAKE_BINARY_DIR}/libdwarf")
    add_test(NAME selfprintreloc COMMAND selfprintreloc)
endif()
if (DO_TESTING) 
    set(peobj "${CMAKE_SOURCE_DIR}/dwarfdump/testobjLE32PE.exe") 
    set(pebase "${CMAKE_SOURCE_DIR}/dwarfdump/testobjLE32PE.base") 
    set(execpedd "${CMAKE_BINARY_DIR}/dwarfdump/dwarfdump")
    add_test(NAME selfdwarfdumppe COMMAND sh -c "${execpedd} ${peobj} |  head -500 >junk.testoutpe ; diff ${pebase} junk.testoutpe" )
endif()

if (DO_TESTING) 
    set(elfobj "${CMAKE_SOURCE_DIR}/dwarfdump/testuriLE64ELf.obj") 
    set(elfbase "${CMAKE_SOURCE_DIR}/dwarfdump/testuriLE64ELf.base")
    set(execelfdd "${CMAKE_BINARY_DIR}/dwarfdump/dwarfdump")
    add_test(NAME selfdwarfdumpelf COMMAND sh -c "${execelfdd} ${elfobj} | head -500 > junk.testoutelf ; diff ${elfbase} junk.testoutelf" )
endif()

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
