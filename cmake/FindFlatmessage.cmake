# Finds flatmessage
#
# exports:
#
# flatmessage_FOUND
# flatmessage_INCLUDE_DIRS
# flatmessage_LIBRARIES
#
# requires:
#
# flatmessage_ROOT_DIR
# flatmessage_LIB_DIR
#

find_path(flatmessage_INCLUDE_DIR
    NAMES compiler.hpp
    PATHS ${flatmessage_ROOT_DIR}
    PATH_SUFFIXES
        src/include/flatmessage
)

find_library(flatmessage_LIBRARY
    NAMES flatmessage
    PATHS ${flatmessage_LIB_DIR}
)

if(flatmessage_INCLUDE_DIR AND flatmessage_LIBRARY)
    set(flatmessage_FOUND yes)
    set(flatmessage_INCLUDE_DIRS ${flatmessage_INCLUDE_DIR}/..)
    set(flatmessage_LIBRARIES ${flatmessage_LIBRARY})
else()
    set(flatmessage_FOUND no)
    set(flatmessage_INCLUDE_DIRS)
    set(flatmessage_LIBRARIES)
endif()
