# AmigaOS 3.x with GCC (C): hunk objects have no PIC/PIE, so
# POSITION_INDEPENDENT_CODE adds nothing. Copyright (c) 2026 AmiCradle Team.
set(CMAKE_C_COMPILE_OPTIONS_PIC "")
set(CMAKE_C_COMPILE_OPTIONS_PIE "")
set(CMAKE_C_LINK_OPTIONS_PIE "")
set(CMAKE_SHARED_LIBRARY_C_FLAGS "")
