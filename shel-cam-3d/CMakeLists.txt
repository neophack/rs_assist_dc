cmake_minimum_required(VERSION 3.4)
project(shel_cam_3d)

set(CMAKE_CXX_STANDARD 11)
list(APPEND CMAKE_MODULE_PATH  "${CMAKE_SOURCE_DIR}/cmake")

find_package(OpenCV REQUIRED)
find_package(Glog REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(GFlags REQUIRED)
find_package(Boost COMPONENTS filesystem system REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS} )
include_directories(${EIGEN3_INCLUDE_DIR} )

add_executable(shel_cam_3d main.cpp)
target_link_libraries(shel_cam_3d ${OpenCV_LIBS} glog gflags boost_filesystem boost_system)
