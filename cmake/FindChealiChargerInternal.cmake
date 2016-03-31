
MESSAGE(STATUS "setting up cheali-charger MACROS.")

MACRO(CHEALI_ADD DEST FILES)
    include_directories(${CMAKE_CURRENT_LIST_DIR})
    CHEALI_FIND("${DEST}" "${FILES}" "${CMAKE_CURRENT_LIST_DIR}")
#    message("CMAKE_CURRENT_LIST_DIR ${CMAKE_CURRENT_LIST_DIR}")
ENDMACRO(CHEALI_ADD)


MACRO(CHEALI_FIND DEST FILES PATHS)
    foreach (SOURCE ${FILES})
        set(THIS_FILE THIS_FILE-NOTFOUND)
        find_file(THIS_FILE ${SOURCE} PATHS ${PATHS} NO_DEFAULT_PATH)
#        message("append ${THIS_FILE}, ${SOURCE}")
        list (APPEND ${DEST} ${THIS_FILE})
    endforeach()
ENDMACRO(CHEALI_FIND)



MACRO(CHEALI_HARDWARE hard)
    set(hardware ${hard})
    message(STATUS "target: ${hardware}" )
ENDMACRO(CHEALI_HARDWARE)


MACRO(CHEALI_CPU CPU)
    SET(TARGET_CPU ${CPU})
    message(STATUS "cpu: ${TARGET_CPU}" )
    include(${CMAKE_SOURCE_DIR}/src/hardware/${TARGET_CPU}/cpu/cpu.cmake)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
ENDMACRO(CHEALI_CPU)

MACRO(CHEALI_GENERIC_CHARGER CHARGER)
    SET(TARGET_GENERIC_CHARGER ${CHARGER})
    message(STATUS "generic charger: ${TARGET_GENERIC_CHARGER}" )
    include(${CMAKE_SOURCE_DIR}/src/hardware/${TARGET_CPU}/generic/${TARGET_GENERIC_CHARGER}/generic.cmake)

    set(name "${CMAKE_PROJECT_NAME}-${hardware}")
#    hexfile short name
#    set(name "${hardware}")
    set(execName "${name}_${cheali-charger-version}-${cheali-charger-buildnumber}_${TARGET_CPU}")
    set(sizeName "${name}")

    include(${CMAKE_SOURCE_DIR}/src/core/core.cmake)

    SET(ALL_SOURCE_FILES
        ${SOURCE_FILES}
        ${GENERIC_SOURCE_FILES}
        ${CPU_SOURCE_FILES}
        ${CORE_SOURCE_FILES}
    )

ENDMACRO(CHEALI_GENERIC_CHARGER)


MACRO(CHEALI_GENERATE_MCS51_EXEC)

    set(I_DIRS "")
    get_property(inc_dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
    set_property(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} PROPERTY INCLUDE_DIRECTORIES)

    foreach (DIR ${inc_dirs})
        list (APPEND I_DIRS "-I")
        list (APPEND I_DIRS ${DIR})
    endforeach()

    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/out/")

    set(C_FILES "")
    foreach (SOURCE ${ALL_SOURCE_FILES})
#        message("checking: ${SOURCE}")
        get_filename_component(EXTEN ${SOURCE} EXT)
        get_filename_component(FNAME ${SOURCE} NAME_WE)
#        message("checking: ${EXTEN}")
        if("${EXTEN}" STREQUAL ".cpp")
#                string(LENGTH ${SOURCE} LEN)
#                MATH(EXPR LEN2 "${LEN} - 2")
#                string(SUBSTRING ${SOURCE} 0 ${LEN2} C_SOURCE)
                SET(C_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/out/${FNAME}.c")
        endif()
        if("${EXTEN}" STREQUAL ".h")
                SET(C_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/out/${FNAME}.h")
        endif()

        message("c file: ${C_SOURCE}")
        list (APPEND C_FILES ${C_SOURCE})
        add_custom_command(OUTPUT ${C_SOURCE}
#                        COMMAND clang-3.7 -fsyntax-only -x c++ ${SOURCE} ${I_DIRS}
                        COMMAND ${CMAKE_SOURCE_DIR}/utils/cxx_to_c/cxx_to_c.py ${SOURCE} ${C_SOURCE} ${I_DIRS}
                        DEPENDS ${SOURCE})

#        SET_SOURCE_FILES_PROPERTIES(${SOURCE}  PROPERTIES LANGUAGE C)
#        list (APPEND ${DEST} ${THIS_FILE})
    endforeach()

    add_library(${execName}_lib ${C_FILES})
    add_executable(${execName} main.c)
    target_link_libraries(${execName} ${execName}_lib)

#    set_property(DIRECTORY ${execName} PROPERTY INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR}/out/)
ENDMACRO(CHEALI_GENERATE_MCS51_EXEC)



MACRO(CHEALI_GENERATE_ARM_EXEC)
    add_executable(${execName} ${ALL_SOURCE_FILES})

    add_custom_command(
        TARGET ${execName}
        POST_BUILD
        COMMAND ${TOOLCHAIN}-objcopy -O ihex $<TARGET_FILE:${execName}> $<TARGET_FILE:${execName}>.hex)

    add_custom_command(
        TARGET ${execName}
        POST_BUILD
        COMMAND ${TOOLCHAIN}-objcopy -O binary $<TARGET_FILE:${execName}> $<TARGET_FILE:${execName}>.bin)

    add_custom_target(${execName}.size ALL
        COMMAND ${TOOLCHAIN}-size $<TARGET_FILE:${execName}>
        DEPENDS ${execName})

ENDMACRO(CHEALI_GENERATE_ARM_EXEC)



MACRO(CHEALI_GENERATE_AVR_EXEC)
    add_executable(${execName} ${ALL_SOURCE_FILES})

    add_custom_command(
        TARGET ${execName}
        POST_BUILD
        COMMAND avr-objcopy -R .eeprom -O ihex $<TARGET_FILE:${execName}> $<TARGET_FILE:${execName}>.hex)

    add_custom_command(
        TARGET ${execName}
        POST_BUILD
        COMMAND avr-objcopy -R .eeprom -O binary $<TARGET_FILE:${execName}> $<TARGET_FILE:${execName}>.bin)

    add_custom_command(
        TARGET ${execName}
        POST_BUILD
        COMMAND avr-objcopy -j .eeprom --no-change-warnings --change-section-lma .eeprom=0 -O ihex $<TARGET_FILE:${execName}> $<TARGET_FILE:${execName}>.eep)

    if(BASH)
        add_custom_target(${execName}.size ALL
            COMMAND ${CMAKE_SOURCE_DIR}/src/core/avr-size.sh $<TARGET_FILE:${execName}> ${sizeName}.size
            DEPENDS ${execName})
    endif(BASH)

ENDMACRO(CHEALI_GENERATE_AVR_EXEC)
