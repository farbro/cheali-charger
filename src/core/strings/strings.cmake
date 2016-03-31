

set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})

set(CORE_SOURCE
    strings_standard.h  strings_common.h  strings.cpp
)

SET_SOURCE_FILES_PROPERTIES(${CURRENT_DIR}/strings.cpp PROPERTIES OBJECT_DEPENDS ${CURRENT_DIR}/strings_standard.h)

CHEALI_ADD("CORE_SOURCE_FILES" "${CORE_SOURCE}")



