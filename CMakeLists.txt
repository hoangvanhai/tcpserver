project(tcpserver.service)
cmake_minimum_required(VERSION 2.8)

set (CMAKE_BUILD_TYPE Release)
set (SRC ${CMAKE_SOURCE_DIR})

if(WIN32)
    set (CMAKE_CXX_FLAGS            "${CMAKE_CXX_FLAGS} /EHsc")
    set (CMAKE_CXX_FLAGS_RELEASE    "${CMAKE_CXX_FLAGS} /MD /O2")
    set (CMAKE_CXX_FLAGS_DEBUG      "${CMAKE_CXX_FLAGS} /MDd")
    include_directories("C:/OpenSSL-Win32/include")
    link_directories("C:/OpenSSL-Win32/lib")
    link_directories("C:/OpenSSL-Win32/lib/VC")
elseif(UNIX)
    # -Wall : Hiển thị tất cả các warning
    # -std=c++11 : Sử dụng c++11
    # -fstack-protector-all : Dừng chương trình khi phát hiện vi phạm stack (buffer overflows)
    # -fPIE : Chương trình chạy tại vị trí được random trên memory (Position-independent executable)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -fstack-protector-all -fPIE -Wno-unused-function")
    # set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -pedantic -Wshadow -Wfloat-equal")
    # set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wconversion")
    # set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

    set (CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -Wlogical-op -rdynamic")
    set (CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=leak -fsanitize=address -fsanitize=undefined -fno-sanitize-recover")
    set (CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -D_FORTIFY_SOURCE=2")        

    # Định nghĩa chế độ DEBUG
    # -ggdb -g3 : Thêm thông tin debug, hỗ trợ gdb
    # định nghĩa __DEBUG__
    set (CMAKE_CXX_FLAGS_DEBUG "-g3 -ggdb")
    # Định nghĩa chế độ RELEASE
    # -O3 : Tối ưu mức 3
    # -Wno-strict-aliasing : Không hiển thị warning khi ép kiểu
    set (CMAKE_CXX_FLAGS_RELEASE "-O3")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif()

if (DEFINED FLAGS_CUSTOM)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAGS_CUSTOM}")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAGS_CUSTOM}")
endif()
if (DEFINED LINK_DIR_CUSTOM)
    link_directories(${LINK_DIR_CUSTOM})
endif()

include(../cpp_lib/cpp_lib.cmake)

file (GLOB_RECURSE SRC_APP_LIST ${SRC}/*.cpp)
list(APPEND SRC_LIST ${SRC_APP_LIST})
include_directories(.)
include_directories(board)
include_directories(configuration)
include_directories(network)

aux_source_directory(. SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})

if(WIN32)
    if(${CMAKE_BUILD_TYPE} MATCHES Release)
        target_link_libraries(${PROJECT_NAME} LINK_PRIVATE libssl32MD libcrypto32MD)
    else()
        target_link_libraries(${PROJECT_NAME} LINK_PRIVATE libssl32MDd libcrypto32MDd)
    endif()
elseif(UNIX)
    if(NOT ${BENCHMARK} STREQUAL "")
        target_link_libraries(${PROJECT_NAME} LINK_PRIVATE benchmark)
    endif()
    target_link_libraries(${PROJECT_NAME} LINK_PRIVATE pthread dl rt)
#    target_link_libraries(${PROJECT_NAME} LINK_PRIVATE ssl crypto)
endif()
target_link_libraries(${PROJECT_NAME} LINK_PRIVATE jsoncpp sqlite3)# DCTag)

add_custom_target(protocol
    COMMAND echo "build dependent server")
add_custom_target(dep DEPENDS protocol)


install(TARGETS ${PROJECT_NAME} DESTINATION .)
