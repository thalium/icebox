# used to compile on MSVC upto 2013 where snprintf is not available
macro(msvc_posix target)
	if(MSVC AND ("${MSVC_VERSION}" LESS 1900))
		# under VS 2015
		target_compile_definitions(${target} PUBLIC 
			snprintf=_snprintf)
	endif()
endmacro()

# set target folder on IDE
macro(set_folder target folder)
	set_target_properties(${target} PROPERTIES FOLDER ${folder})
endmacro()

# set source groups on IDE
macro(set_source_group list_name group_name)
	set(${list_name} ${ARGN})
	source_group(${group_name} FILES ${ARGN})
endmacro()