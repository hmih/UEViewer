cmake_minimum_required(VERSION 3.28)
project(umodel C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#------------------------------------------------
# Platform detection
#------------------------------------------------
if(APPLE)
    set(UMODEL_PLATFORM "osx")
    # macOS build: RENDERING=1 for full declarations, but no viewer at runtime.
    # OpenGL is in the system framework; SDL2 from Homebrew provides the headers.
    # The rendering code compiles but we never call it — we only use data-only
    # functions (GetParams, AppendReferencedTextures) for material export.
    set(UMODEL_RENDERING ON)
    set(UMODEL_THREADING OFF)
elseif(WIN32)
    set(UMODEL_PLATFORM "win32")
    set(UMODEL_RENDERING ON)
    set(UMODEL_THREADING ON)
else()
    set(UMODEL_PLATFORM "linux")
    set(UMODEL_RENDERING ON)
    set(UMODEL_THREADING ON)
endif()

#------------------------------------------------
# Compiler flags
#------------------------------------------------
add_compile_options(
    -fno-strict-aliasing
    -fno-stack-protector
    -Wunused-value
    -Wno-invalid-offsetof
)

if(NOT APPLE)
    add_compile_options(-msse2)
endif()

#------------------------------------------------
# Defines
#------------------------------------------------
add_compile_definitions(DO_GUARD=1)

if(UMODEL_RENDERING)
    add_compile_definitions(RENDERING=1)
else()
    add_compile_definitions(RENDERING=0)
endif()

if(UMODEL_THREADING)
    add_compile_definitions(THREADING=1)
else()
    add_compile_definitions(THREADING=0)
endif()

if(WIN32)
    add_compile_definitions(HAS_UI=1)
endif()

#------------------------------------------------
# Third-party library sources (built from vendor tree)
#------------------------------------------------

# Compression libs (always built from source)
set(COMP_LIB_SOURCES
    libs/lzo/lzo_init.c
    libs/lzo/lzo1x_d2.c
    libs/mspack/lzxd.c
    libs/lz4/lz4.c
)

# UE4 libs (rijndael — always built)
set(UE4_LIB_SOURCES
    libs/rijndael/*.c
)
file(GLOB UE4_LIB_SOURCES libs/rijndael/*.c)

# Image libs (detex — always built; libpng from system on macOS/Linux)
set(IMG_LIB_SOURCES
    libs/detex/*.cpp
)
file(GLOB IMG_LIB_SOURCES CONFIGURE_DEPENDS libs/detex/*.cpp)

# NVTT libs (nvimage) — used for DDS texture decoding
file(GLOB NV_LIB_SOURCES CONFIGURE_DEPENDS libs/nvtt/nvimage/*.cpp)

# Mobile texture libs
file(GLOB MOBILE_LIB_SOURCES CONFIGURE_DEPENDS libs/PowerVR/*.cpp libs/astc/*.cpp)

#------------------------------------------------
# zlib + libpng (system on macOS/Linux, vendored on Windows)
#------------------------------------------------
find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)

#------------------------------------------------
# SIMDE (portable SSE intrinsics for non-x86)
#------------------------------------------------
if(NOT APPLE)
    # x86: use native xmmintrin.h — no SIMDE needed
    set(SIMDE_INCLUDE_DIR "")
else()
    # arm64: use SIMDE from Homebrew or system
    find_path(SIMDE_INCLUDE_DIR simde/x86/sse.h
        PATHS /opt/homebrew/include /usr/local/include
    )
    if(NOT SIMDE_INCLUDE_DIR)
        message(FATAL_ERROR "SIMDE not found. Install with: brew install simde")
    endif()
    message(STATUS "SIMDE found: ${SIMDE_INCLUDE_DIR}")
endif()

#------------------------------------------------
# SDL2 (needed for RENDERING=1 declarations, even if we don't use the viewer)
#------------------------------------------------
if(UMODEL_RENDERING)
    find_package(SDL2 REQUIRED)
endif()

#------------------------------------------------
# umodel binary
#------------------------------------------------

# Collect main source files (mirrors umodel.project sources(MAIN))
file(GLOB UMODEL_SOURCES
    CONFIGURE_DEPENDS
    Exporters/*.cpp
    Unreal/*.cpp
    Unreal/FileSystem/*.cpp
    Unreal/GameSpecific/*.cpp
    Unreal/Mesh/*.cpp
    Unreal/UnrealMaterial/*.cpp
    Unreal/UnrealMesh/*.cpp
    Unreal/UnrealPackage/*.cpp
    Unreal/Wrappers/*.cpp
    Viewers/*.cpp
    MeshInstance/*.cpp
    Core/*.cpp
    UmodelTool/*.cpp
)

if(WIN32)
    file(GLOB UMODEL_UI_SOURCES CONFIGURE_DEPENDS UI/*.cpp)
    list(APPEND UMODEL_SOURCES ${UMODEL_UI_SOURCES})
endif()

add_executable(umodel ${UMODEL_SOURCES}
    ${COMP_LIB_SOURCES}
    ${UE4_LIB_SOURCES}
    ${IMG_LIB_SOURCES}
    ${NV_LIB_SOURCES}
    ${MOBILE_LIB_SOURCES}
)

target_include_directories(umodel PRIVATE
    .
    Core
    Unreal
    UmodelTool
    UI
    libs
    libs/include
    libs/nvtt
    libs/PowerVR
    libs/detex
    libs/rijndael
    ${SIMDE_INCLUDE_DIR}
)

target_link_libraries(umodel PRIVATE
    ZLIB::ZLIB
    PNG::PNG
)

if(UMODEL_RENDERING)
    target_link_libraries(umodel PRIVATE SDL2::SDL2)
endif()

if(NOT WIN32)
    target_link_libraries(umodel PRIVATE m stdc++ dl pthread)
endif()

if(UMODEL_RENDERING)
    if(APPLE)
        target_link_libraries(umodel PRIVATE "-framework OpenGL")
    elseif(NOT WIN32)
        target_link_libraries(umodel PRIVATE GL)
    endif()
endif()

#------------------------------------------------
# extract tool (headless package extractor)
#------------------------------------------------
file(GLOB EXTRACT_SOURCES
    CONFIGURE_DEPENDS
    Tools/PackageExtract/Main.cpp
)
set(EXTRACT_CORE_SOURCES
    Unreal/UnCore.cpp
    Unreal/UnCoreCompression.cpp
    Unreal/UnCoreDecrypt.cpp
    Unreal/UnCoreSerialize.cpp
    Unreal/UnObject.cpp
    Unreal/UnObject4.cpp
)
file(GLOB EXTRACT_PACKAGE_SOURCES
    CONFIGURE_DEPENDS
    Unreal/UnrealPackage/UnPackage*.cpp
)
set(EXTRACT_EXTRA_SOURCES
    Unreal/GameDatabase.cpp
    Unreal/FileSystem/FileSystemUtils.cpp
    Unreal/FileSystem/GameFileSystem.cpp
    Unreal/FileSystem/GameFileSystemGears4.cpp
    Unreal/FileSystem/IOStoreFileSystem.cpp
    Unreal/FileSystem/UnArchivePak.cpp
    Unreal/TypeInfo.cpp
    Core/Core.cpp
    Core/CoreWin32.cpp
    Core/Memory.cpp
)

add_executable(extract
    ${EXTRACT_SOURCES}
    ${EXTRACT_CORE_SOURCES}
    ${EXTRACT_PACKAGE_SOURCES}
    ${EXTRACT_EXTRA_SOURCES}
    ${COMP_LIB_SOURCES}
    ${UE4_LIB_SOURCES}
)

target_include_directories(extract PRIVATE
    .
    Core
    Unreal
    libs
    libs/include
    ${SIMDE_INCLUDE_DIR}
)

target_link_libraries(extract PRIVATE
    ZLIB::ZLIB
    PNG::PNG
)

if(NOT WIN32)
    target_link_libraries(extract PRIVATE m stdc++ dl pthread)
endif()

#------------------------------------------------
# Install
#------------------------------------------------
install(TARGETS umodel extract)
