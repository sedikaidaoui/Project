cmake_minimum_required(VERSION 3.10.2)

project("terminalx")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compile options
add_compile_options(
    -Wno-error=format-security
    -fvisibility=hidden
    -ffunction-sections
    -fdata-sections
    -w
    -fno-rtti
    -fno-exceptions
    -fpermissive
    -fms-extensions
)

# Link options
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections -Wl,--strip-all")

# Import prebuilt libdobby
add_library(libdobby STATIC IMPORTED)
set_target_properties(libdobby PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/Tools/Dobby/libdobby.a"
)

# Import prebuilt Keystone
add_library(Keystone STATIC IMPORTED)
set_target_properties(Keystone PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/Tools/KittyMemory/Deps/Keystone/libs-android/${ANDROID_ABI}/libkeystone.a"
)

# Find Android libraries
find_library(log-lib log)
find_library(android-lib android)
find_library(glesv2-lib GLESv2)

# Target: TERMINALX_MENU
add_library(TERMINALX_MENU SHARED
    client.cpp
    Tools/SOCKET/client.cpp
    Hack/Namegun.cpp
)

target_include_directories(TERMINALX_MENU PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(TERMINALX_MENU PRIVATE
    ${log-lib}
    ${android-lib}
    ${glesv2-lib}
)

# Target: TERMINALX999
add_library(TERMINALX999 SHARED
    backend.cpp
    Hack/Namegun.cpp
    AutoUpdate/Il2Cpp.cpp
    AutoUpdate/xdl/xdl.c
    AutoUpdate/xdl/xdl_iterate.c
    AutoUpdate/xdl/xdl_linker.c
    AutoUpdate/xdl/xdl_util.c
    AutoUpdate/xdl/xdl_lzma.c
    Tools/KittyMemory/KittyArm64.cpp
    Tools/KittyMemory/KittyMemory.cpp
    Tools/KittyMemory/KittyScanner.cpp
    Tools/KittyMemory/KittyUtils.cpp
    Tools/KittyMemory/MemoryBackup.cpp
    Tools/KittyMemory/MemoryPatch.cpp
    Tools/SOCKET/server.cpp
    Tools/Substrate/hde64.c
    Tools/Substrate/SubstrateDebug.cpp
    Tools/Substrate/SubstrateHook.cpp
    Tools/Substrate/SubstratePosixMemory.cpp
)

target_include_directories(TERMINALX999 PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/AutoUpdate/xdl/include"
)

target_link_libraries(TERMINALX999 PRIVATE
    libdobby
    Keystone
    ${log-lib}
    ${android-lib}
    ${glesv2-lib}
)
