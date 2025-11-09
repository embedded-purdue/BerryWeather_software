<<<<<<< HEAD
# Install script for directory: C:/Users/dprak/esp/v5.5.1/esp-idf/components/esp_system/port
=======
# Install script for directory: /Users/stephenschafer/esp/v5.5.1/esp-idf/components/esp_system/port
>>>>>>> 6df9ad4c410838caba840c617aa98031ef5bf9ed

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/esp32_skeleton")
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
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
<<<<<<< HEAD
  set(CMAKE_OBJDUMP "C:/Users/dprak/.espressif/tools/xtensa-esp-elf/esp-14.2.0_20241119/xtensa-esp-elf/bin/xtensa-esp32-elf-objdump.exe")
=======
  set(CMAKE_OBJDUMP "/Users/stephenschafer/.espressif/tools/xtensa-esp-elf/esp-14.2.0_20241119/xtensa-esp-elf/bin/xtensa-esp32-elf-objdump")
>>>>>>> 6df9ad4c410838caba840c617aa98031ef5bf9ed
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
<<<<<<< HEAD
  include("C:/Users/dprak/esp/projects/BerryWeather/esp32_skeleton/build/esp-idf/esp_system/port/soc/esp32/cmake_install.cmake")
endif()

=======
  include("/Users/stephenschafer/Dev/esp_berryweather/GitHub/BerryWeather/esp32_skeleton/build/esp-idf/esp_system/port/soc/esp32/cmake_install.cmake")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/stephenschafer/Dev/esp_berryweather/GitHub/BerryWeather/esp32_skeleton/build/esp-idf/esp_system/port/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
>>>>>>> 6df9ad4c410838caba840c617aa98031ef5bf9ed
