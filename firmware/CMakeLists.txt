# Firmware root CMakeLists.txt
# This file is included from the top-level CMakeLists.txt

function(chirbot_enable_strict_warnings target)
	if(MSVC)
		target_compile_options(${target} PRIVATE /W4 /WX)
	else()
		target_compile_options(${target} PRIVATE
			"$<$<COMPILE_LANGUAGE:C>:-Wall;-Wextra;-Werror>"
			"$<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Werror>"
		)
	endif()
endfunction()

# Add shared libraries once so every firmware image uses the same protocol.
add_subdirectory(TASD)
add_subdirectory(chirbot-common)

# Add chirbot-core
add_subdirectory(chirbot-core)

# Add modules
add_subdirectory(modules)
