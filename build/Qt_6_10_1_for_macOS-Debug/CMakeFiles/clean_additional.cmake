# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/SprintIDE_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/SprintIDE_autogen.dir/ParseCache.txt"
  "SprintIDE_autogen"
  )
endif()
