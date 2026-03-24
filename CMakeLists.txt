cmake_minimum_required(VERSION 3.21)

project(wordSnapV1 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

qt_add_executable(wordSnapV1 WIN32
    src/main.cpp
    src/app/AppController.h
    src/app/AppController.cpp
    src/app/AppTypes.h
    src/platform/win/GlobalHotkeyManager.h
    src/platform/win/GlobalHotkeyManager.cpp
    src/services/ScreenCaptureService.h
    src/services/ScreenCaptureService.cpp
    src/services/DictionaryService.h
    src/services/DictionaryService.cpp
    src/services/ImagePreprocessor.h
    src/services/ImagePreprocessor.cpp
    src/services/OcrService.h
    src/services/OcrService.cpp
    src/services/SettingsService.h
    src/services/SettingsService.cpp
    src/services/StarDictBackend.h
    src/services/StarDictBackend.cpp
    src/services/WordNormalizer.h
    src/services/WordNormalizer.cpp
    src/ui/CaptureOverlay.h
    src/ui/CaptureOverlay.cpp
    src/ui/ResultCardWidget.h
    src/ui/ResultCardWidget.cpp
    src/ui/TrayController.h
    src/ui/TrayController.cpp
)

target_include_directories(wordSnapV1 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(wordSnapV1 PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

if (WIN32)
    target_link_libraries(wordSnapV1 PRIVATE user32)
endif()
