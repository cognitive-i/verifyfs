################################################################################
# fuse driver executable
project(VerifyFS)
cmake_minimum_required(VERSION 2.8)
aux_source_directory("source" SRC_LIST)

INCLUDE(FindPkgConfig)
pkg_check_modules (FUSE REQUIRED fuse)
pkg_check_modules (OPENSSL REQUIRED openssl)
include_directories(${FUSE_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIRS})

add_definitions(${FUSE_CFLAGS} ${OPENSSL_CFLAGS})
link_directories(${FUSE_LIBRARY_DIRS} ${OPENSSL_LIBRARY_DIRS})


if(CMAKE_COMPILER_IS_GNUCXX)
  list(APPEND CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")
endif()

add_executable(${PROJECT_NAME} ${SRC_LIST})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(${PROJECT_NAME} ${FUSE_LIBRARIES} ${OPENSSL_LIBRARIES})


################################################################################
# unit tests
ADD_SUBDIRECTORY(gmock-1.7.0)
enable_testing()

set(TEST_SRC_LIST ${SRC_LIST})
list(REMOVE_ITEM TEST_SRC_LIST source/main.cpp)
list(APPEND TEST_SRC_LIST test/testFileVerifier.cpp)

include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR}/gtest/include source)
add_executable(testVerifier ${TEST_SRC_LIST})

target_link_libraries(testVerifier gtest gtest_main ${FUSE_LIBRARIES} ${OPENSSL_LIBRARIES})
set_property(TARGET testVerifier PROPERTY CXX_STANDARD 11)
set_property(TARGET testVerifier PROPERTY CXX_STANDARD_REQUIRED ON)
add_test(testVerifier testVerifier)
