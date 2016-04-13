# - Try to find the CFLite library
# Once done this will define
#
#  CFLITE_INCLUDE_DIR - The CFLite include directory
#  CFLITE_LIBRARIES - The libraries needed to use CFLite
#

#**************************************************************
#Extract the list of supported voices fron conf file
#
#**************************************************************
#locate default.lv
set(FLITE_SHARE /share/flite)
set(FLITE_IN /usr/include/flite)
find_path(CFLITE_CONF_FILE NAMES default.lv
  HINTS ${FLITE_SHARE}
 )
set(CFLITE_CONF_FILE_NAME ${CFLITE_CONF_FILE}/default.lv)
message("Flite configuration  flie -" ${CFLITE_CONF_FILE_NAME} )

#get value of voice
file(READ ${CFLITE_CONF_FILE_NAME} CONFFILE)
string(LENGTH "${CONFFILE}" LEN)
set(SUB_STR "VOXES =")
string(FIND "${CONFFILE}" ${SUB_STR} VOXES_START)
string(SUBSTRING "${CONFFILE}" ${VOXES_START} ${LEN} VOICE  )
string(REGEX MATCHALL "cmu_[^ \n]*" VOICES "${VOICE}")

message("Flite supported voices - " ${VOICES})
#populate flite include and shared directories
set (SHARE_DIR "${CMAKE_FIND_ROOT_PATH}${FLITE_SHARE}" )
set (FLITE_INC_DIR "${CMAKE_FIND_ROOT_PATH}${FLITE_IN}")

#**************************************************************
#Execute the script to create voice list .c file
#
#**************************************************************
find_program(FLITE_CONFIG_SCRIPT NAMES make_voice_list
          HINTS ${SHARE_DIR}
          )
message("Flite voicelist creation script - " ${FLITE_CONFIG_SCRIPT})
if(FLITE_CONFIG_SCRIPT)
    execute_process(
        COMMAND "${FLITE_CONFIG_SCRIPT}"  ${VOICES}
        WORKING_DIRECTORY ${FLITE_INC_DIR}
        RESULT_VARIABLE CONFIGSCRIPT_RESULT
        OUTPUT_VARIABLE PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

#**************************************************************
#Find FLite libraries
#**************************************************************

#flite detfault libraries
set(CFLITE_LIBS flite)
find_library(CFLITE_LIB NAMES flite)
find_library(CFLITE_CMULEX_LIB NAMES flite_cmulex)
find_library(CFLITE_USENGLISH NAMES flite_usenglish)
find_library(ASOUND_LIB NAMES asound)

#flite voices libraries identified using default.lv
foreach (_library ${VOICES})
    find_library(FLITE_LIBRARIES_${_library} flite_${_library})
    set(CFLITE_VOICES_LIBRARIES  ${CFLITE_VOICES_LIBRARIES}  ${FLITE_LIBRARIES_${_library}})
endforeach() 

set(CFLITE_LIBRARIES ${CFLITE_LIB} ${CFLITE_CMULEX_LIB}  
    ${CFLITE_USENGLISH} ${ASOUND_LIB} ${CFLITE_VOICES_LIBRARIES} )

#**************************************************************
#find include directory
#**************************************************************
find_path(CFLITE_INCLUDE_DIR NAMES flite.h 
      HINTS ${FLITE_IN}
    )

include(FindPackageHandleStandardArgs)
set(CFLITE_LIBRARIES ${CFLITE_LIBRARIES}  CACHE PATH "Path to Flite library" )
set(CFLITE_INCLUDE_DIR ${CFLITE_INCLUDE_DIR}  CACHE PATH "Path to Flite headers" )

FIND_PACKAGE_HANDLE_STANDARD_ARGS(CFLite DEFAULT_MSG CFLITE_LIBRARIES  CFLITE_INCLUDE_DIR)
mark_as_advanced(CFLITE_INCLUDE_DIR CFLITE_LIBRARIES )
