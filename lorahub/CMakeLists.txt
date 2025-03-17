# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS ../components/liblorahub ../components/smtc_ral ../components/radio_drivers)

add_compile_definitions(LR11XX_DISABLE_WARNINGS)
add_compile_definitions(LR11XX_DISABLE_HIGH_ACP_WORKAROUND) # workaround not required for LR1121 fw >= 0103

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(lorahub)
