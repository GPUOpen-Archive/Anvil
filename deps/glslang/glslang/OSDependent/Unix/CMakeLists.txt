add_library(OSDependent STATIC ossource.cpp ../osinclude.h)
set_property(TARGET OSDependent PROPERTY FOLDER glslang POSITION_INDEPENDENT_CODE ON)

# Disabled for Anvil:
#>>
#install(TARGETS OSDependent
#        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
#<<