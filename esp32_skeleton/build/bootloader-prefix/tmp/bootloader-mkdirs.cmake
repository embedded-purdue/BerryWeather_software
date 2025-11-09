# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
<<<<<<< HEAD
if(NOT EXISTS "C:/Users/dprak/esp/v5.5.1/esp-idf/components/bootloader/subproject")
  file(MAKE_DIRECTORY "C:/Users/dprak/esp/v5.5.1/esp-idf/components/bootloader/subproject")
endif()
file(MAKE_DIRECTORY
  "C:/Users/dprak/esp/projects/BerryWeather/esp32_skeleton/build/bootloader"
  "C:/Users/dprak/esp/projects/BerryWeather/esp32_skeleton/build/bootloader-prefix"
  "C:/Users/dprak/esp/projects/BerryWeather/esp32_skeleton/build/bootloader-prefix/tmp"
  "C:/Users/dprak/esp/projects/BerryWeather/esp32_skeleton/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/dprak/esp/projects/BerryWeather/esp32_skeleton/build/bootloader-prefix/src"
  "C:/Users/dprak/esp/projects/BerryWeather/esp32_skeleton/build/bootloader-prefix/src/bootloader-stamp"
=======
if(NOT EXISTS "/Users/stephenschafer/esp/v5.5.1/esp-idf/components/bootloader/subproject")
  file(MAKE_DIRECTORY "/Users/stephenschafer/esp/v5.5.1/esp-idf/components/bootloader/subproject")
endif()
file(MAKE_DIRECTORY
  "/Users/stephenschafer/Dev/esp_berryweather/GitHub/BerryWeather/esp32_skeleton/build/bootloader"
  "/Users/stephenschafer/Dev/esp_berryweather/GitHub/BerryWeather/esp32_skeleton/build/bootloader-prefix"
  "/Users/stephenschafer/Dev/esp_berryweather/GitHub/BerryWeather/esp32_skeleton/build/bootloader-prefix/tmp"
  "/Users/stephenschafer/Dev/esp_berryweather/GitHub/BerryWeather/esp32_skeleton/build/bootloader-prefix/src/bootloader-stamp"
  "/Users/stephenschafer/Dev/esp_berryweather/GitHub/BerryWeather/esp32_skeleton/build/bootloader-prefix/src"
  "/Users/stephenschafer/Dev/esp_berryweather/GitHub/BerryWeather/esp32_skeleton/build/bootloader-prefix/src/bootloader-stamp"
>>>>>>> 6df9ad4c410838caba840c617aa98031ef5bf9ed
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
<<<<<<< HEAD
    file(MAKE_DIRECTORY "C:/Users/dprak/esp/projects/BerryWeather/esp32_skeleton/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/dprak/esp/projects/BerryWeather/esp32_skeleton/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
=======
    file(MAKE_DIRECTORY "/Users/stephenschafer/Dev/esp_berryweather/GitHub/BerryWeather/esp32_skeleton/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/stephenschafer/Dev/esp_berryweather/GitHub/BerryWeather/esp32_skeleton/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
>>>>>>> 6df9ad4c410838caba840c617aa98031ef5bf9ed
endif()
