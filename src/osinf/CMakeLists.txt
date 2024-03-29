﻿# CMakeList.txt: Scmm에 대한 CMake 프로젝트, 여기에 소스를 포함하고
# 프로젝트 특정 논리를 정의합니다.
#

# 헤더 파일
include_directories (${CMAKE_SOURCE_DIR}/include)

add_definitions (-D_SBCS -DSCMSLIB_DYNAMIC -DUSING_DECLSPEC -DPOSIX_SHAREDMEM -DPOSIX_SEMAPHORES -DCREATING_CMSLIB -DCPP_STD_CHRONO_EPOCH_TIME -D_WINSOCK_DEPRECATED_NO_WARNINGS)

# 이 프로젝트의 실행 파일에 소스를 추가합니다.
add_library (osinf SHARED
                        "Shm.cpp"
                        "Sem.cpp"
                        "Tmr.cpp"
                        "IniFile.cpp"
                        "InetFile.cpp"
                        "Tty.cpp")

add_executable (testosinf)
target_sources (testosinf PRIVATE "TestOsinf.cpp")
target_link_libraries (osinf PRIVATE utils)
target_link_libraries (testosinf PRIVATE utils osinf)


if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_property (TARGET osinf PROPERTY CXX_STANDARD 20)
    set_property (TARGET testosinf PROPERTY CXX_STANDARD 20)
    set_target_properties (osinf
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set_target_properties (testosinf
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif ()

if (${BOOST_FOUND})
    if ("${Boost_LIBRARIES}" STREQUAL "")
        message (STATUS "[ Boost attached only header files. ]") 
    else ()
        message (STATUS "[ Link boost libraries. ]")        
        target_link_libraries (osinf PUBLIC ${Boost_LIBRARIES})
        if (WIN32)
            message (STATUS "[ CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR} ]")        
            file (GLOB DEPENDENCY_FILES "${BOOST_BINARY_DIR}/boost*.dll")
            if (DEFINED DEPENDENCY_FILES)
                foreach (file ${DEPENDENCY_FILES})
                    add_custom_command (TARGET osinf POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${file} ${CMAKE_BINARY_DIR}/bin
                        COMMENT "Copied specified dependency file: ${file}")
                endforeach (file)
            endif ()
            add_custom_command (TARGET testosinf POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:testosinf> ${CMAKE_BINARY_DIR}/bin
                        COMMENT "Copied executable file")
        endif ()
    endif ()
endif ()

