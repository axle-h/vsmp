cmake_minimum_required(VERSION 3.13)
project(vsmp)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_STANDARD 14)
configure_file (${PROJECT_SOURCE_DIR}/scripts/vsmp.service.in ${PROJECT_BINARY_DIR}/scripts/vsmp.service)
include(FetchContent)

# Find required libav libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
        libavformat
        libavcodec
        libavutil
        libswscale)

# Fetch json library from github
FetchContent_Declare(json
        GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
        GIT_TAG v3.9.1)
FetchContent_GetProperties(json)
if(NOT json_POPULATED)
    FetchContent_Populate(json)
    add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# Fetch date library from github
FetchContent_Declare(date_src
        GIT_REPOSITORY https://github.com/HowardHinnant/date.git
        GIT_TAG        v3.0.0)
FetchContent_GetProperties(date_src)
if(NOT date_src_POPULATED)
    FetchContent_Populate(date_src)
    add_subdirectory(${date_src_SOURCE_DIR} ${date_src_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# Glob all source files
file(GLOB_RECURSE SOURCE_FILES "${PROJECT_SOURCE_DIR}/src/*.h" "${PROJECT_SOURCE_DIR}/src/*.cpp")
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # Non-linux platforms cannot use SPI
    list(REMOVE_ITEM SOURCE_FILES "${CMAKE_SOURCE_DIR}/src/spi/Spi.cpp")
    list(REMOVE_ITEM SOURCE_FILES "${CMAKE_SOURCE_DIR}/src/epaper/EPaperDisplay.cpp")
endif()

add_executable(vsmp ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE
        PkgConfig::LIBAV
        nlohmann_json::nlohmann_json
        date::date)

### Install
install(TARGETS vsmp
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        DESTINATION bin)
install(FILES "${PROJECT_BINARY_DIR}/scripts/vsmp.service"
        PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
        DESTINATION /etc/systemd/system)

install(CODE "execute_process(COMMAND systemctl daemon-reload)")
install(CODE "execute_process(COMMAND systemctl restart vsmp)")
install(CODE "execute_process(COMMAND systemctl enable vsmp)")

