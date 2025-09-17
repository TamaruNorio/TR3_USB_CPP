cmake_minimum_required(VERSION 3.20)

project(TR3_USB_CPP LANGUAGES CXX)

# ---- C++17 ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- 出力先をプロジェクト直下の build/ に集約 ----
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

# MSVC の PDB も build/ に出す
set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_PDB_OUTPUT_DIRECTORY          ${CMAKE_SOURCE_DIR}/build)

# ---- 既定のビルドタイプ（未指定なら Debug）----
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ---- 定義（Unicode / Windows の min/max マクロ抑止）----
add_definitions(-DUNICODE -D_UNICODE)
if (WIN32)
  add_compile_definitions(NOMINMAX)
endif()

# ---- インクルード ----
include_directories(${CMAKE_SOURCE_DIR}/include)

# ---- 実行ファイル ----
add_executable(tr3_usb
  src/main.cpp
  src/serial_port.cpp
  src/tr3_protocol.cpp
)

target_include_directories(tr3_usb PRIVATE ${CMAKE_SOURCE_DIR}/include)

# ---- コンパイルオプション（主に MSVC）----
if (MSVC)
  # /utf-8 /W4 /EHsc を常に付与
  target_compile_options(tr3_usb PRIVATE /utf-8 /W4 /EHsc)

  # ランタイム: Debug=/MDd, Release=/MD
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

  # デバッグ情報: Debug は /Zi、Release は最適化 /O2
  target_compile_options(tr3_usb PRIVATE
    $<$<CONFIG:Debug>:/Zi>
    $<$<CONFIG:Release>:/O2>
  )
  # リンカ: PDB を build/ に、Debug は /DEBUG、Release は /INCREMENTAL:NO
  target_link_options(tr3_usb PRIVATE
    $<$<CONFIG:Debug>:/DEBUG>
    $<$<CONFIG:Release>:/INCREMENTAL:NO>
  )
else()
  # 他ツールチェイン（参考）: 警告多め
  target_compile_options(tr3_usb PRIVATE -Wall -Wextra -Wpedantic)
endif()

# メッセージ（ビルド後の実行方法）
message(STATUS "Run: \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tr3_usb\"  （起動後に COM / ボーレート / インベントリ試行回数を対話入力）")
