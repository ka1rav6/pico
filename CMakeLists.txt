cmake_minimum_required(VERSION 3.14)
project(pico
  VERSION 0.1.0
  LANGUAGES C CXX ASM
)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedFile.cmake)

set(GENERATED_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/generated)

file(MAKE_DIRECTORY ${GENERATED_DIR})

set(ASSET_HEADER
    ${GENERATED_DIR}/assets.hpp)

file(WRITE ${ASSET_HEADER}
"#pragma once

namespace Pico::Assets {

")

embed_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/init/config.toml
    ${ASSET_HEADER}
    CONFIG_TOML)

embed_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/init/pico_gitignore.txt
    ${ASSET_HEADER}
    GITIGNORE)

file(APPEND ${ASSET_HEADER}

"} // namespace Pico::Assets
")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(third_party_utils/blake3/c)

add_executable(pico
  src/main.cpp
  src/cli/cli_entry.cpp
  src/utils/hasher.cpp
  src/scanner/scanner.cpp
  src/diagnostics/output.cpp
)

target_include_directories(pico PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}/generated
)

target_link_libraries(pico PRIVATE blake3)
