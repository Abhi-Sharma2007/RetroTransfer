
cmake_minimum_required(VERSION 3.10)

project(RetroTransfer VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# ------------------------------------------
# CORE BACKEND SOURCES
# ------------------------------------------

set(RETRO_CORE_SOURCES
    src/scanner.cpp
    src/fingerprint.c
    src/huffman_codec.c
    src/compression_engine.c
    src/compression_engine.cpp
    src/reconstruction_engine.cpp
    src/fixed_chunker.cpp
    src/cdc_chunker.cpp
    src/adaptive_policy.cpp
    src/chunk_engine.cpp
)

# ------------------------------------------
# TERMINAL APPLICATION
# ------------------------------------------

add_executable(RetroTransfer
    src/main.cpp
    ${RETRO_CORE_SOURCES}
)

# ------------------------------------------
# TRANSFER SETUP CLI
# ------------------------------------------

add_executable(RetroTransferSetupCLI
    examples/transfer_setup_cli.cpp
)

add_dependencies(RetroTransferSetupCLI RetroTransfer)

# ------------------------------------------
# TEST APPLICATION
# ------------------------------------------

enable_testing()

add_executable(test_chunking
    tests/test_chunking.cpp
    ${RETRO_CORE_SOURCES}
)

add_test(NAME ChunkingEngineTests COMMAND test_chunking)

# Preserve static linking for CLI and test targets.
if(MINGW)
    set_property(TARGET RetroTransfer APPEND_STRING
        PROPERTY LINK_FLAGS " -static")

    set_property(TARGET RetroTransferSetupCLI APPEND_STRING
        PROPERTY LINK_FLAGS " -static")

    set_property(TARGET test_chunking APPEND_STRING
        PROPERTY LINK_FLAGS " -static")
endif()

# ------------------------------------------
# QT 6 GUI APPLICATION
# ------------------------------------------

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(RetroTransferGUI
    gui/main_gui.cpp
    gui/mainwindow.cpp
    gui/mainwindow.h
    gui/tcp_transfer.cpp
    gui/tcp_transfer.h
    ${RETRO_CORE_SOURCES}
)

target_link_libraries(RetroTransferGUI
    PRIVATE Qt6::Widgets Qt6::Network
)

add_dependencies(RetroTransferGUI RetroTransfer)

add_executable(test_tcp_transfer
    tests/test_tcp_transfer.cpp
    gui/tcp_transfer.cpp
    gui/tcp_transfer.h
    ${RETRO_CORE_SOURCES}
)

target_include_directories(test_tcp_transfer PRIVATE gui)
target_link_libraries(test_tcp_transfer
    PRIVATE Qt6::Network
)

add_test(NAME TcpTransferIntegrationTests COMMAND test_tcp_transfer)
