cmake_minimum_required (VERSION 3.5)
set(REQUIRED_CPP_VERSION 17)
set(CMAKE_CXX_STANDARD ${REQUIRED_CPP_VERSION})

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()
message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

set(TARGET_NAME Test)
project(${TARGET_NAME} CXX)

set(sources 
  "${PROJECT_SOURCE_DIR}/src/main.cpp"
)
add_executable(${TARGET_NAME} ${sources} ${headers})
include_directories(src)

set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD ${REQUIRED_CPP_VERSION})
if (WIN32 OR WIN64)
  set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${TARGET_NAME})
  target_link_libraries(${TARGET_NAME} ${Boost_LIBRARIES})
elseif(UNIX)
  # target_link_libraries(${TARGET_NAME} c++ c++abi)
  target_link_libraries(${TARGET_NAME} c++)
endif()
