#   Copyright 2017 KeycapEmu
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


# Generates the given fileName witht he flatmessage compiler
function(flatmessage_compile_file fileName)
    execute_process(
        COMMAND flatmessage_compiler -i ${CMAKE_CURRENT_SOURCE_DIR}/${fileName} -t ${PROJECT_SOURCE_DIR}/templates/hpp.template -e hpp -o ${CMAKE_CURRENT_BINARY_DIR}
        OUTPUT_VARIABLE PROCESSOR_OUTPUT
        ERROR_VARIABLE PROCESSOR_ERROR
        RESULT_VARIABLE PROCESSOR_RET
    )

    if(PROCESSOR_RET)
        message(FATAL_ERROR "Error calling code generator with \"\n" "-i ${CMAKE_CURRENT_SOURCE_DIR}/${fileName} -t ${PROJECT_SOURCE_DIR}/templates/hpp.template -e hpp -o ${CMAKE_CURRENT_BINARY_DIR}\"\n" ${PROCESSOR_RET})
    endif()
endfunction()