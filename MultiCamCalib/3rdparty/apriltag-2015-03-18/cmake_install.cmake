# Install script for directory: /Users/wangxiao05/codes/rs_assist_dc/MultiCamCalib/3rdparty/apriltag-2015-03-18

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/XP/lib_x86_64" TYPE SHARED_LIBRARY FILES "/Users/wangxiao05/codes/rs_assist_dc/MultiCamCalib/3rdparty/apriltag-2015-03-18/libTagDetector.dylib")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/XP/lib_x86_64/libTagDetector.dylib" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/XP/lib_x86_64/libTagDetector.dylib")
    execute_process(COMMAND "/usr/bin/install_name_tool"
      -id "libTagDetector.dylib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/XP/lib_x86_64/libTagDetector.dylib")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/anaconda2/lib"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/XP/lib_x86_64/libTagDetector.dylib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Library/Developer/CommandLineTools/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/XP/lib_x86_64/libTagDetector.dylib")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/XP/include/3rdparty/TagDetector" TYPE DIRECTORY FILES "/Users/wangxiao05/codes/rs_assist_dc/MultiCamCalib/3rdparty/apriltag-2015-03-18/include")
endif()

