CMAKE_MINIMUM_REQUIRED(VERSION 3.8.2)

PROJECT(LiveCapture LANGUAGES CXX)
SET(CMAKE_CXX_STANDARD 14)

# Option to set path for DeckLink SDK
if (NOT DEFINED DECKLINK_SDK_INCLUDE_DIR)
    SET(DECKLINK_SDK_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif ()

# ZMQ API
SET(ZeroMQ_DIR libs/ZMQ/libzmq-build)
find_package(cppzmq PATHS libs/ZMQ/cppzmq-build)


# OpenCV API
#set (OpenCV_DIR /home/imag2/StereoMatching/LiveCapture/libs/opencv-3.3.0/build)
find_package(OpenCV 3.3 EXACT PATHS libs/opencv-3.3.0/build NO_DEFAULT_PATH)
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

################## DECKLINK ONLY
ADD_EXECUTABLE(DeckLinkIterator
        src/DeckLinkIterator/main.cpp
        ${PLATFORM_SOURCES})

TARGET_INCLUDE_DIRECTORIES(DeckLinkIterator PRIVATE
        ${DECKLINK_SDK_INCLUDE_DIR}
        ${PLATFORM_INCLUDE_DIR}
        )

TARGET_LINK_LIBRARIES(DeckLinkIterator
        ${PLATFORM_LIBRARIES}
        ${OpenCV_LIBS}
        )

ADD_EXECUTABLE(ScheduledPlayback
        src/ScheduledPlayback/main.cpp
        ${PLATFORM_SOURCES}
        )

TARGET_INCLUDE_DIRECTORIES(ScheduledPlayback PRIVATE
        ${DECKLINK_SDK_INCLUDE_DIR}
        ${PLATFORM_INCLUDE_DIR}
        )

TARGET_LINK_LIBRARIES(ScheduledPlayback
        ${PLATFORM_LIBRARIES}
        ${OpenCV_LIBS}
        )

################## OPENCV ONLY

ADD_EXECUTABLE(OpenCVExample
        src/OpenCVExample/main.cpp
        ${PLATFORM_SOURCES}
        )

TARGET_INCLUDE_DIRECTORIES(OpenCVExample PRIVATE
        ${DECKLINK_SDK_INCLUDE_DIR}
        ${PLATFORM_INCLUDE_DIR}
        )

TARGET_LINK_LIBRARIES(OpenCVExample
        ${PLATFORM_LIBRARIES}
        ${OpenCV_LIBS}
        )

################## DECKLINK + OPENCV

SET(APP_HEADERS
        src/Decklink_opencv/com_ptr.h
        src/Decklink_opencv/com_utils.h
        src/Decklink_opencv/DeviceManager.h
        src/Decklink_opencv/LiveVideoWithOpenCV.h
        )

ADD_EXECUTABLE(LiveVideoWithOpenCV
        src/Decklink_opencv/LiveVideoWithOpenCV.cpp
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

################## ZMQ + OPENCV

ADD_EXECUTABLE(ZMQ_OpenCV
        src/ZMQ_OpenCV/client.cpp
        ${PLATFORM_SOURCES}
        )

TARGET_INCLUDE_DIRECTORIES(ZMQ_OpenCV PRIVATE
        ${DECKLINK_SDK_INCLUDE_DIR}
        ${PLATFORM_INCLUDE_DIR}
        )

TARGET_LINK_LIBRARIES(ZMQ_OpenCV
        cppzmq
        ${PLATFORM_LIBRARIES}
        ${OpenCV_LIBS}
        )

################## ZMQ + OpenCV + DeckLink

SET(APP_HEADERS
        src/Capture/Capture.h
        src/Capture/Config.cpp
        src/Capture/Config.h
        )

ADD_EXECUTABLE(Capture
        src/Capture/Capture.cpp
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

################# TestPattern (DeckLink)

SET(APP_HEADERS
        src/TestPattern/TestPattern.h
        src/TestPattern/Config.cpp
        src/TestPattern/Config.h
        src/TestPattern/VideoFrame3D.cpp
        src/TestPattern/VideoFrame3D.h
        )

ADD_EXECUTABLE(TestPattern
        src/TestPattern/TestPattern.cpp
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
