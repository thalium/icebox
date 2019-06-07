include(configure.cmake)

set_source_group(SOURCES "Source Files" createirepformfrombinary.cc createirepfrombinary.cc
    dwarfgen.cc irepattrtodbg.cc ireptodbg.cc
    ../libdwarf/dwgetopt.c)
    
set_source_group(HEADERS "Header Files" createirepfrombinary.h general.h irepattrtodbg.h 
    irepdie.h irepform.h irepframe.h 
    irepline.h irepmacro.h ireppubnames.h irepresentation.h ireptodbg.h 
    strtabdata.h ../libdwarf/dwgetopt.h)

set_source_group(CONFIGURATION_FILES "Configuration Files" configure.cmake config.h.in.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
    
add_executable(dwarfgen ${SOURCES} ${HEADERS} ${CONFIGURATION_FILES})

set_folder(dwarfgen dwarfgen)

target_link_libraries(dwarfgen PRIVATE ${dwarf-target} ${dwfzlib})

set(SUFFIX $<$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>:64>)
set(LIBDIR lib${SUFFIX})
set(BINDIR bin${SUFFIX})

install(TARGETS dwarfgen DESTINATION
	RUNTIME DESTINATION ${BINDIR}		
	LIBRARY DESTINATION ${LIBDIR}
	ARCHIVE DESTINATION ${LIBDIR})

#install(FILES dwarfgen.conf DESTINATION lib)

install(FILES dwarfgen.1 DESTINATION share/man/man1)
