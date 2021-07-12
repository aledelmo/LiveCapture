CMAKE_MINIMUM_REQUIRED(VERSION 3.8.2)

PROJECT(LiveCapture LANGUAGES CXX)
SET(CMAKE_CXX_STANDARD 14)

# Option to set path for DeckLink SDK
if (NOT DEFINED DECKLINK_SDK_INCLUDE_DIR)
    SET(DECKLINK_SDK_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/DeckLinkSDK12)
endif ()

# ZMQ API
SET(ZeroMQ_DIR libs/ZMQ/libzmq/build)
find_package(cppzmq PATHS libs/ZMQ/cppzmq/build)

# OpenCV API
#set (OpenCV_DIR /home/imag2/StereoMatching/LiveCapture/libs/opencv-3.3.0/build)
find_package(OpenCV 3.3 EXACT PATHS libs/opencv/build NO_DEFAULT_PATH)
include_directories(${OpenCV_INCLUDE_DIRS})

# DeckLink API
SET(PLATFORM_SOURCES
        ${DECKLINK_SDK_INCLUDE_DIR}/DeckLinkAPI.h
        ${DECKLINK_SDK_INCLUDE_DIR}/DeckLinkAPIDispatch.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/Linux/platform.h
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/Linux/platform.cpp
        )

SET(PLATFORM_INCLUDE_DIR libs/Linux)

LIST(APPEND PLATFORM_LIBRARIES dl pthread)

################## DeckLink+OpenCV Example

SET(APP_HEADERS
        src/examples/LiveVideoWithOpenCV/com_ptr.h
        src/examples/LiveVideoWithOpenCV/com_utils.h
        src/examples/LiveVideoWithOpenCV/DeviceManager.h
        src/examples/LiveVideoWithOpenCV/LiveVideoWithOpenCV.h
        )

ADD_EXECUTABLE(LiveVideoWithOpenCV
        src/examples/LiveVideoWithOpenCV/LiveVideoWithOpenCV.cpp
        ${APP_HEADERS}
        ${PLATFORM_SOURCES}
        )

TARGET_INCLUDE_DIRECTORIES(LiveVideoWithOpenCV PRIVATE
        ${DECKLINK_SDK_INCLUDE_DIR}
        ${PLATFORM_INCLUDE_DIR}
        )

TARGET_LINK_LIBRARIES(LiveVideoWithOpenCV
        ${PLATFORM_LIBRARIES}
        ${OpenCV_LIBS}
        )

################## DeckLink+OpenCV+ZMQ Video acquisition and forwarding

SET(APP_HEADERS
        src/VideoStreaming/Client.h
        src/VideoStreaming/Config.cpp
        src/VideoStreaming/Config.h
        )

ADD_EXECUTABLE(Capture
        src/VideoStreaming/Client.cpp
        ${APP_HEADERS}
        ${PLATFORM_SOURCES}
        )

TARGET_INCLUDE_DIRECTORIES(Capture PRIVATE
        ${DECKLINK_SDK_INCLUDE_DIR}
        ${PLATFORM_INCLUDE_DIR}
        )

TARGET_LINK_LIBRARIES(Capture
        cppzmq
        ${PLATFORM_LIBRARIES}
        ${OpenCV_LIBS}
        )

################# DeckLink test pattern generation

SET(APP_HEADERS
        src/GeneratePattern/TestPattern.h
        src/GeneratePattern/Config.cpp
        src/GeneratePattern/Config.h
        src/GeneratePattern/VideoFrame3D.cpp
        src/GeneratePattern/VideoFrame3D.h
        )

ADD_EXECUTABLE(TestPattern
        src/GeneratePattern/TestPattern.cpp
        ${APP_HEADERS}
        ${PLATFORM_SOURCES}
        )

TARGET_INCLUDE_DIRECTORIES(TestPattern PRIVATE
        ${DECKLINK_SDK_INCLUDE_DIR}
        ${PLATFORM_INCLUDE_DIR}
        )

TARGET_LINK_LIBRARIES(TestPattern
        cppzmq
        ${PLATFORM_LIBRARIES}
        ${OpenCV_LIBS}
        )
