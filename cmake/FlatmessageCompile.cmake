#   Copyright 2018 KeycapEmu
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.


# Generates the given fileName with the flatmessage compiler
function(flatmessage_compile_file fileName template fileExtension)
    execute_process(
        COMMAND flatmessage_compiler -i ${CMAKE_CURRENT_SOURCE_DIR}/${fileName} -t ${template} -e ${fileExtension} -o ${CMAKE_CURRENT_BINARY_DIR}/..
        OUTPUT_VARIABLE PROCESSOR_OUTPUT
        ERROR_VARIABLE PROCESSOR_ERROR
        RESULT_VARIABLE PROCESSOR_RET
    )

    if(PROCESSOR_RET)
        message(FATAL_ERROR "Error calling code generator with \"\n" "-i ${CMAKE_CURRENT_SOURCE_DIR}/${fileName} -t ${template} -e hpp -o ${CMAKE_CURRENT_BINARY_DIR}/..\"\n" ${PROCESSOR_RET} " - " ${PROCESSOR_ERROR})
    endif()
endfunction()

function(flatmessage_compile_files)
    set(oneValueArgs TEMPLATE EXTENSION)
    cmake_parse_arguments(PARSED_ARGS "" "${oneValueArgs}" "FILES" ${ARGN})
    foreach(file ${PARSED_ARGS_FILES})
        message("[flatmessage]: Compiling ${file} to " ${CMAKE_CURRENT_BINARY_DIR}/..)
        flatmessage_compile_file(${file} ${PARSED_ARGS_TEMPLATE} ${PARSED_ARGS_EXTENSION})
    endforeach()
endfunction()