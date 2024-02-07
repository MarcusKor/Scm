# CMakeList.txt: Scmm에 대한 CMake 프로젝트, 여기에 소스를 포함하고
# 프로젝트 특정 논리를 정의합니다.
#

# 헤더 파일
include_directories (${CMAKE_SOURCE_DIR}/include)

add_definitions (-D_SBCS -DSCMSLIB_DYNAMIC -DUSING_DECLSPEC -DPOSIX_SHAREDMEM -DPOSIX_SEMAPHORES -DCREATING_CMSLIB -DCPP_STD_CHRONO_EPOCH_TIME -D_WINSOCK_DEPRECATED_NO_WARNINGS)

# 이 프로젝트의 실행 파일에 소스를 추가합니다.
add_library (core SHARED
                        "Buffer.cpp"
                        "Polynomial.cpp"
                        "Device.cpp"
                        "WorkQueue.cpp"
                        "StructuredMessage.cpp"
                        "TcpSession.cpp"
                        "TcpSessionContainer.cpp"
                        "TcpServer.cpp"
                        "TcpClient.cpp"
                        "TcpStructuredMessageSession.cpp"
                        "TcpStructuredMessageSessionContainer.cpp"
                        "TcpStructuredMessageServer.cpp"                        
                        "TcpStructuredMessageClient.cpp"
                        "UdpServer.cpp"
                        "UdpClient.cpp"
                        "SerialPort.cpp")

message (STATUS "[ FLATBUFFERS_FLATC_EXECUTABLE=${FLATBUFFERS_FLATC_EXECUTABLE} ]")

#set (FB_SCHEMA "${PROJECT_SOURCE_DIR}/schema/protocol.fbs")
#build_flatbuffers ("${FB_SCHEMA}" "${PROJECT_SOURCE_DIR}/schema" fbschemas "" "${PROJECT_SOURCE_DIR}/include" "" "")
#add_library (FlatbuffersTarget INTERFACE)
#target_include_directories (FlatbuffersTarget INTERFACE ${FLATBUFFERS_INCLUDE_DIR})
#add_dependencies (FlatbuffersTarget fbschemas)
add_executable (test_chatserver)
add_executable (test_chatclient)
target_include_directories (test_chatserver INTERFACE ${FLATBUFFERS_INCLUDE_DIR})
target_include_directories (test_chatclient INTERFACE ${FLATBUFFERS_INCLUDE_DIR})
target_sources (test_chatserver PRIVATE "TestStructuredMessageServer.cpp")
target_sources (test_chatclient PRIVATE "TestStructuredMessageClient.cpp")
target_link_libraries (core PRIVATE utils)
target_link_libraries (test_chatserver PRIVATE utils core)
target_link_libraries (test_chatclient PRIVATE utils core)

if (CMAKE_VERSION VERSION_GREATER 3.12)
    set_property (TARGET core PROPERTY CXX_STANDARD 20)
    set_property (TARGET test_chatserver PROPERTY CXX_STANDARD 20)
    set_property (TARGET test_chatclient PROPERTY CXX_STANDARD 20)
    set_target_properties (core
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set_target_properties (test_chatserver
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set_target_properties (test_chatclient
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
        target_link_libraries (core PUBLIC ${Boost_LIBRARIES})
        if (WIN32)
            message (STATUS "[ CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR} ]")        
            file (GLOB DEPENDENCY_FILES "${BOOST_BINARY_DIR}/boost*.dll")
            if (DEFINED DEPENDENCY_FILES)
                foreach (file ${DEPENDENCY_FILES})
                    add_custom_command (TARGET core POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${file} ${CMAKE_BINARY_DIR}/bin
                        COMMENT "Copied specified dependency file: ${file}")
                endforeach (file)
            endif ()
            add_custom_command (TARGET test_chatserver POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:test_chatserver> ${CMAKE_BINARY_DIR}/bin
                        COMMENT "Copied executable file")
            add_custom_command (TARGET test_chatclient POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:test_chatclient> ${CMAKE_BINARY_DIR}/bin
                        COMMENT "Copied executable file")
        endif ()
    endif ()
endif ()

