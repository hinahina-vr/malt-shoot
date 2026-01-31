cmake_minimum_required(VERSION 3.20)
project(MaltShoot VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Unicode対応
add_definitions(-DUNICODE -D_UNICODE)

# FetchContentでGoogle Testを取得
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
# Windowsでのランタイムライブラリ設定
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# テストを有効化
enable_testing()

# Source files (excluding main.cpp, for library)
set(GAME_SOURCES
    src/Game.cpp
    src/Graphics.cpp
    src/Input.cpp
    src/Player.cpp
    src/BulletManager.cpp
    src/Enemy.cpp
    src/EnemyManager.cpp
    src/Background3D.cpp
    src/ParticleSystem.cpp
    src/ItemManager.cpp
)

# Header files
set(GAME_HEADERS
    src/Game.h
    src/Graphics.h
    src/Input.h
    src/Player.h
    src/Bullet.h
    src/BulletManager.h
    src/Enemy.h
    src/EnemyManager.h
    src/Background3D.h
    src/ParticleSystem.h
    src/ItemManager.h
)

# ゲームロジックをライブラリとして作成（テスト用）
add_library(MaltShootLib STATIC ${GAME_SOURCES} ${GAME_HEADERS})
target_include_directories(MaltShootLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(MaltShootLib
    d3d11
    d3dcompiler
    dxgi
    dxguid
    winmm
)

# メイン実行ファイル
add_executable(${PROJECT_NAME} WIN32 src/main.cpp)
target_link_libraries(${PROJECT_NAME} MaltShootLib)

# テスト実行ファイル
add_executable(MaltShootTests
    tests/test_bullet_manager.cpp
    tests/test_main.cpp
)
target_link_libraries(MaltShootTests
    MaltShootLib
    GTest::gtest
    GTest::gtest_main
)

# CTestにテストを登録
include(GoogleTest)
gtest_discover_tests(MaltShootTests)

# シェーダーファイルをビルドディレクトリにコピー
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})

# アセットをビルドディレクトリにコピー
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
