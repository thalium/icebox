set_source_group(SOURCES "Source Files" dwarf_abbrev.c 
dwarf_alloc.c dwarf_arange.c 
dwarf_debuglink.c dwarf_die_deliv.c 
dwarf_dnames.c dwarf_dsc.c dwarf_elf_access.c 
dwarf_elf_load_headers.c 
dwarf_elfread.c 
dwarf_elf_rel_detector.c 
dwarf_error.c 
dwarf_form.c dwarf_frame.c dwarf_frame2.c dwarf_funcs.c 
dwarf_gdbindex.c dwarf_global.c dwarf_groups.c 
dwarf_harmless.c dwarf_generic_init.c dwarf_init_finish.c 
dwarf_leb.c 
dwarf_line.c dwarf_loc.c 
dwarf_machoread.c dwarf_macro.c dwarf_macro5.c
dwarf_names.c
dwarf_object_read_common.c dwarf_object_detector.c
dwarf_original_elf_init.c 
dwarf_peread.c 
dwarf_pubtypes.c dwarf_query.c dwarf_ranges.c 
dwarfstring.h dwarfstring.c
dwarf_stringsection.c
dwarf_tied.c 
dwarf_str_offsets.c
dwarf_tsearchhash.c dwarf_types.c dwarf_util.c 
dwarf_vars.c dwarf_weaks.c dwarf_xu_index.c dwarf_print_lines.c 
malloc_check.c pro_alloc.c pro_arange.c 
pro_die.c pro_dnames.c pro_encode_nm.c pro_error.c pro_expr.c 
pro_finish.c pro_forms.c pro_funcs.c pro_frame.c 
pro_init.c pro_line.c pro_reloc.c pro_reloc_stream.c 
pro_reloc_symbolic.c pro_pubnames.c pro_section.c 
pro_types.c pro_vars.c pro_macinfo.c pro_weaks.c)

set_source_group(HEADERS "Header Files" dwarf.h dwarf_abbrev.h
dwarf_alloc.h dwarf_arange.h dwarf_base_types.h 
dwarf_debuglink.h dwarf_die_deliv.h 
dwarf_dnames.h dwarf_dsc.h 
dwarf_elf_access.h dwarf_elf_defines.h dwarf_elfread.h 
dwarf_elf_rel_detector.h 
dwarf_elf_reloc_386.h dwarf_elf_reloc_aarch64.h 
dwarf_elf_reloc_arm.h dwarf_elf_reloc_mips.h 
dwarf_elf_reloc_ppc64.h dwarf_elf_reloc_ppc.h 
dwarf_elf_reloc_sparc.h 
dwarf_elf_reloc_x86_64.h 
dwarf_elfstructs.h 
dwarf_error.h dwarf_frame.h 
dwarf_funcs.h dwarf_gdbindex.h dwarf_global.h dwarf_harmless.h 
dwarf_incl.h dwarf_line.h dwarf_loc.h 
dwarf_machoread.h dwarf_macro.h dwarf_macro5.h 
dwarf_names.h
dwarf_object_detector.h dwarf_opaque.h 
dwarf_pe_descr.h dwarf_peread.h
dwarf_reading.h dwarf_reloc_arm.h dwarf_reloc_mips.h 
dwarf_reloc_ppc.h dwarf_reloc_ppc64.h dwarf_reloc_x86_64.h 
dwarf_tied_decls.h 
dwarf_tsearch.h 
dwarf_str_offsets.h
dwarf_types.h dwarf_util.h dwarf_vars.h dwarf_weaks.h 
dwarf_xu_index.h libdwarfdefs.h 
dwarf_macho_loader.h malloc_check.h 
memcpy_swap.h
pro_alloc.h pro_arange.h pro_die.h 
pro_encode_nm.h pro_error.h pro_expr.h pro_frame.h 
pro_incl.h pro_line.h 
pro_log_extra_flag_strings.c
pro_macinfo.h pro_opaque.h 
pro_reloc.h pro_reloc_stream.h pro_reloc_symbolic.h 
pro_section.h pro_types.h pro_util.h)

set_source_group(CONFIGURATION_FILES "Configuration Files" 
    ${CMAKE_SOURCE_DIR}/config.h.in.cmake 
    ${CMAKE_BINARY_DIR}/config.h)


	
list(LENGTH DWARF_TARGETS targetCount)
math(EXPR targetCount "${targetCount} - 1")
foreach(i RANGE ${targetCount})
	list(GET DWARF_TARGETS ${i} target)
	list(GET DWARF_TYPES ${i} type)
	add_library(${target} ${type} ${SOURCES} ${HEADERS}
            ${GENNAMES_OUTPUT} ${CONFIGURATION_FILES})

	set_folder(${target} libdwarf)
	target_include_directories(${target} PUBLIC 
            ${LIBELF_INCLUDE_DIRS})
        target_compile_options(${target} PRIVATE ${DW_FWALL})
	msvc_posix(${target})

	target_link_libraries(${target} PUBLIC ${LIBELF_LIBRARIES})
	
	set_target_properties(${target} PROPERTIES OUTPUT_NAME dwarf)

	set(SUFFIX $<$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>:64>)
	set(LIBDIR lib${SUFFIX})
	set(BINDIR bin${SUFFIX})

	install(TARGETS ${target}
		RUNTIME DESTINATION ${BINDIR}		
		LIBRARY DESTINATION ${LIBDIR}
		ARCHIVE DESTINATION ${LIBDIR})
endforeach()

if(UNIX AND BUILD_SHARED)
	target_link_libraries(dwarf-shared PUBLIC z)
endif()
		
if (DO_TESTING)
    set_source_group(TESTLEB "Source Files" dwarf_leb_test.c 
        dwarf_leb.c pro_encode_nm.c )
    add_executable(selfleb ${TESTLEB})
    target_compile_options(selfleb PRIVATE ${DW_FWALL})
    add_test(NAME selfleb COMMAND selfleb)
endif()
if (DO_TESTING)
    set_source_group(TESTTIED "Source Files" 
        dwarf_tied_test.c dwarf_tied.c dwarf_tsearchhash.c )
    add_executable(selftied ${TESTTIED})
    target_compile_options(selftied PRIVATE ${DW_FWALL})
    add_test(NAME selftied COMMAND selftied)

    set_source_group(TESTSTRING "Source Files" 
        test_dwarfstring.c dwarfstring.c dwarfstring.h)
    add_executable(teststring ${TESTSTRING})
    target_compile_options(selftied PRIVATE ${DW_FWALL})
    add_test(NAME teststring COMMAND teststring)
endif()
