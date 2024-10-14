cmake_minimum_required(VERSION 3.5)

project(Sensorium)

# name of application. replace 'app' with desired app name
set(APP_NAME app)

# path to main source file
add_executable(${APP_NAME}
  src/main.cpp
  src/FileWatcher.cpp
  src/ShaderManager.cpp
  src/OceanDataViewer.cpp
  src/VideoPlayer.cpp
  src/AudioPlayer.cpp
)

target_include_directories(${APP_NAME} PUBLIC src)

# add allolib as a subdirectory to the project
add_subdirectory(allolib)

if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/al_ext)
  message("Buiding extensions in al_ext")
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/al_ext)
  get_target_property(AL_EXT_LIBRARIES al_ext AL_EXT_LIBRARIES)
  target_link_libraries(${APP_NAME} PRIVATE ${AL_EXT_LIBRARIES})
endif()

# link allolib to project
target_link_libraries(${APP_NAME} PRIVATE alapp)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # On macOS if jack is present, RtMidi will link to jack but not provide the
    # /usr/local/lib location as link directory. This statement only serves the
    # purpose of working around that bug. Perhaps can be removed on future
    # updates of RtMidi
    link_directories("/usr/local/lib")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

# example line for find_package usage
# find_package(Qt5Core REQUIRED CONFIG PATHS "C:/Qt/5.12.0/msvc2017_64/lib" NO_DEFAULT_PATH)

# replace ${PATH_TO_INCLUDE_DIR} before including other libraries
# target_include_directories(${APP_NAME} PRIVATE ${PATH_TO_INCLUDE_DIR})

# replace ${PATH_TO_LIB_FILE} before linking other libraries
# target_link_libraries(${APP_NAME} PRIVATE ${PATH_TO_LIB_FILE})

# set(FFMPEG_ROOT "/alloshare/code/kenny/ffmpeg_install/" CACHE STRING "Path to FFMPEG lib and include directories")
# message("Using FFMPEG at ${FFMPEG_ROOT}")

# FFMPEG
# find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h PATHS ${FFMPEG_ROOT} PATH_SUFFIXES "include")
# find_library(AVCODEC_LIBRARY avcodec PATHS ${FFMPEG_ROOT} PATH_SUFFIXES "lib")

# find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h PATHS ${FFMPEG_ROOT} PATH_SUFFIXES "include")
# find_library(AVFORMAT_LIBRARY avformat PATHS ${FFMPEG_ROOT} PATH_SUFFIXES "lib")

# find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h PATHS ${FFMPEG_ROOT} PATH_SUFFIXES "include")
# find_library(AVUTIL_LIBRARY avutil PATHS ${FFMPEG_ROOT} PATH_SUFFIXES "lib")

# find_path(AVDEVICE_INCLUDE_DIR libavdevice/avdevice.h PATHS ${FFMPEG_ROOT} PATH_SUFFIXES "include")
# find_library(AVDEVICE_LIBRARY avdevice PATHS ${FFMPEG_ROOT} PATH_SUFFIXES "lib")

# find_path(SWSCALE_INCLUDE_DIR libswscale/swscale.h PATHS ${FFMPEG_ROOT} PATH_SUFFIXES "include")
# find_library(SWSCALE_LIBRARY swscale PATHS ${FFMPEG_ROOT} PATH_SUFFIXES "lib")

# find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h PATHS ${FFMPEG_ROOT} NO_DEFAULT_PATH)
# find_library(AVCODEC_LIBRARY avcodec PATHS ${FFMPEG_ROOT} PATH_SUFFIXES "libavcodec" NO_DEFAULT_PATH)

# find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h PATHS ${FFMPEG_ROOT} NO_DEFAULT_PATH)
# find_library(AVFORMAT_LIBRARY avformat PATHS ${FFMPEG_ROOT} PATH_SUFFIXES "libavformat" NO_DEFAULT_PATH)

# find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h PATHS ${FFMPEG_ROOT} NO_DEFAULT_PATH)
# find_library(AVUTIL_LIBRARY avutil PATHS ${FFMPEG_ROOT} PATH_SUFFIXES "libavutil" NO_DEFAULT_PATH)

# find_path(AVDEVICE_INCLUDE_DIR libavdevice/avdevice.h PATHS ${FFMPEG_ROOT} NO_DEFAULT_PATH)
# find_library(AVDEVICE_LIBRARY avdevice PATHS ${FFMPEG_ROOT} PATH_SUFFIXES "libavdevice" NO_DEFAULT_PATH)

# find_path(SWSCALE_INCLUDE_DIR libswscale/swscale.h PATHS ${FFMPEG_ROOT} NO_DEFAULT_PATH)
# find_library(SWSCALE_LIBRARY swscale PATHS ${FFMPEG_ROOT} PATH_SUFFIXES "libswscale" NO_DEFAULT_PATH)

# target_link_libraries(${APP_NAME} PRIVATE "${AVCODEC_LIBRARY}" "${AVFORMAT_LIBRARY}" "${AVUTIL_LIBRARY}" "${ADEVICE_LIBRARY}" "${SWSCALE_LIBRARY}")
# target_include_directories(${APP_NAME} PRIVATE ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR}  ${AVDEVICE_INCLUDE_DIR} ${SWSCALE_INCLUDE_PATH})


# binaries are put into the ./bin directory by default
set_target_properties(${APP_NAME} PROPERTIES
  CXX_STANDARD 14
  CXX_STANDARD_REQUIRED ON
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin
  RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_LIST_DIR}/bin
  RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_LIST_DIR}/bin
)
