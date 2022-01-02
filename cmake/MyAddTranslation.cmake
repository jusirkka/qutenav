# copied from qt 15.5 - qt 5.6 (used in sfos) version of qt5_add_translation does not support options
function(MY_ADD_TRANSLATION _qm_files)
  set(options)
  set(oneValueArgs)
  set(multiValueArgs OPTIONS)

  cmake_parse_arguments(_LRELEASE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  set(_lrelease_files ${_LRELEASE_UNPARSED_ARGUMENTS})

  foreach(_current_FILE ${_lrelease_files})
    get_filename_component(_abs_FILE ${_current_FILE} ABSOLUTE)
    get_filename_component(qm ${_abs_FILE} NAME)
    # everything before the last dot has to be considered the file name (including other dots)
    string(REGEX REPLACE "\\.[^.]*$" "" FILE_NAME ${qm})
    get_source_file_property(output_location ${_abs_FILE} OUTPUT_LOCATION)
    if(output_location)
      file(MAKE_DIRECTORY "${output_location}")
      set(qm "${output_location}/${FILE_NAME}.qm")
    else()
      set(qm "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.qm")
    endif()

    add_custom_command(OUTPUT ${qm}
      COMMAND ${Qt5_LRELEASE_EXECUTABLE}
      ARGS ${_LRELEASE_OPTIONS} ${_abs_FILE} -qm ${qm}
      DEPENDS ${_abs_FILE} VERBATIM
    )
    list(APPEND ${_qm_files} ${qm})
  endforeach()
  set(${_qm_files} ${${_qm_files}} PARENT_SCOPE)
endfunction()
