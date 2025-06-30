cmake_minimum_required(VERSION 3.15)
project(ant)

message(STATUS "system processor: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "C++ compiler id: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ compiler version is: ${CMAKE_CXX_COMPILER_VERSION}")

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
        set(CMAKE_CXX_STANDARD 11)
else()
        set(CMAKE_CXX_STANDARD 17)
        add_definitions(-DPROGRESS)
endif()
set(CXX_STANDARD_REQUIRED ON)

# Include files
include_directories(src)

# Google Test Configures
if (WIN32)
        add_subdirectory(lib/googletest)
        include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
else()
        add_definitions(-DLINUX)
endif()
include_directories(include)

# 外部依赖库 #
set(CMAKE_PREFIX_PATH "/home/dev/vcpkg/installed/linaro-hf-dynamic")
if (${CMAKE_CXX_STANDARD} LESS 17)
        find_package(ghc_filesystem CONFIG REQUIRED)
        set(UTILS_LIBS ${UTILS_LIBS} ghcFilesystem::ghc_filesystem)
endif()

## Progress Bar Component
add_subdirectory(lib/progress_bar)
include_directories(lib/progress_bar/include)

set(ProtocolFiles
        src/protocol/Frame.cpp
        src/protocol/RequestPayload.cpp
        src/protocol/DataPayload.cpp
        src/protocol/RecvResponse.cpp
        src/protocol/VecStream.cpp)


# Application targets
add_executable(client
        src/UdpSocket.cpp
        src/AntClient.cpp
        src/ReadBuffer.cpp
        ${ProtocolFiles}
        src/main/client.cpp)
add_executable(server
        src/UdpSocket.cpp
        src/AntServer.cpp
        src/WriteBuffer.cpp
        ${ProtocolFiles}
        src/main/server.cpp)

if (WIN32)

target_link_libraries(client ws2_32)
target_link_libraries(server ws2_32)

# Unit test targets
add_executable(UdpSocketTest
       test/UdpSocketTest.cpp
       src/UdpSocket.cpp)

add_executable(VecStreamTest
	    test/VecStreamTest.cpp
	    ${ProtocolFiles})

add_executable(PayloadTest
	    test/PayloadTest.cpp
	    ${ProtocolFiles})

add_executable(DataFrameTest
	    test/FrameTest.cpp
	    ${ProtocolFiles})

add_executable(ReadBufferTest
	    src/ReadBuffer.cpp
	    src/WriteBuffer.cpp
	    test/BufferTest.cpp)

target_link_libraries(UdpSocketTest gtest gtest_main ws2_32)
target_link_libraries(VecStreamTest gtest gtest_main)
target_link_libraries(PayloadTest gtest gtest_main)
target_link_libraries(DataFrameTest gtest gtest_main)
target_link_libraries(ReadBufferTest gtest gtest_main)

else()

target_link_libraries(server ${UTILS_LIBS})
target_link_libraries(client ${UTILS_LIBS})

endif()

