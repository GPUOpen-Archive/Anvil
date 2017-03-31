add_library(OSDependent STATIC ossource.cpp ../osinclude.h)
set_property(TARGET OSDependent PROPERTY FOLDER glslang)