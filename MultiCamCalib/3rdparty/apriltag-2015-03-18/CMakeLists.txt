cmake_minimum_required (VERSION 2.8.11)
project(TagDetector)

file(GLOB HEADERS include/*.h)
file(GLOB C_SOURCES src/*.c)
file(GLOB CPP_SOURCES src/*.cpp)

find_package(OpenCV REQUIRED)

set(verbose 0)
if (${verbose})
  foreach(c_src ${C_SOURCES})
    message(STATUS "Add c source: ${c_src}")
  endforeach(c_src)

  foreach(cpp_src ${CPP_SOURCES})
    message(STATUS "Add cpp source: ${cpp_src}")
  endforeach(cpp_src)
endif()

add_library(${PROJECT_NAME} SHARED
 ${HEADERS}
 ${C_SOURCES}
 ${CPP_SOURCES}
)
 
target_include_directories(${PROJECT_NAME} PUBLIC
 ${CMAKE_CURRENT_SOURCE_DIR}/include
 ${USER_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
 ${OpenCV_LIBS}
)
install(TARGETS ${PROJECT_NAME}
  LIBRARY DESTINATION XP/lib_${CMAKE_SYSTEM_PROCESSOR}
  RUNTIME DESTINATION XP/lib_${CMAKE_SYSTEM_PROCESSOR}  # for cygwin
)
install(DIRECTORY include DESTINATION XP/include/3rdparty/TagDetector)

# post build for cygwin
# Copy the generated library (.dll) to /usr/local/bin
if (CYGWIN)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
      $<TARGET_FILE:${PROJECT_NAME}>
      /usr/local/bin/$<TARGET_FILE_NAME:${PROJECT_NAME}>
  )
endif()
