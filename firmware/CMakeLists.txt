# Firmware root CMakeLists.txt
# This file is included from the top-level CMakeLists.txt

# Add shared libraries once so every firmware image uses the same protocol.
add_subdirectory(TASD)
add_subdirectory(chirbot-common)

# Add chirbot-core
add_subdirectory(chirbot-core)

# Add modules
add_subdirectory(modules)
