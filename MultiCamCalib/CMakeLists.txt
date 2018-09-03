cmake_minimum_required(VERSION 3.4)
project(multicamera_calibration)

set(CMAKE_CXX_STANDARD 11)
list(APPEND CMAKE_MODULE_PATH  "${CMAKE_SOURCE_DIR}/cmake")

add_subdirectory(3rdparty)


find_package(OpenCV REQUIRED)
find_package(Glog REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(GFlags REQUIRED)
find_package(Yaml REQUIRED)
find_package(Boost
        COMPONENTS filesystem system
        REQUIRED)

file(GLOB CPP_SOURCES src/*.cc)


include_directories(${OpenCV_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/3rdparty)
include_directories(${EIGEN3_INCLUDE_DIR} )

add_executable(multicamera_calibration main.cpp ${CPP_SOURCES})
target_link_libraries(multicamera_calibration ${OpenCV_LIBS} glog gflags
        ${YAMLCPP_LIBRARIES} TagDetector ${Boost_LIBRARIES}
        )