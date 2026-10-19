cmake_minimum_required(VERSION 3.22)

project(wolfenstein_android_test LANGUAGES C CXX)

add_subdirectory(SDL)

set(LIBTYPE STATIC CACHE STRING "Build OpenAL Soft as a static library" FORCE)
set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ALSOFT_TESTS OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL OFF CACHE BOOL "" FORCE)
set(ALSOFT_UPDATE_BUILD_VERSION OFF CACHE BOOL "" FORCE)
set(ALSOFT_ENABLE_MODULES OFF CACHE BOOL "" FORCE)
set(ALSOFT_DLOPEN OFF CACHE BOOL "" FORCE)
set(ALSOFT_EAX OFF CACHE BOOL "" FORCE)
set(ALSOFT_EMBED_HRTF_DATA OFF CACHE BOOL "" FORCE)
set(ALSOFT_BACKEND_OBOE OFF CACHE BOOL "" FORCE)
set(ALSOFT_BACKEND_OPENSL ON CACHE BOOL "" FORCE)
set(ALSOFT_REQUIRE_OPENSL ON CACHE BOOL "" FORCE)
set(ALSOFT_BACKEND_WAVE OFF CACHE BOOL "" FORCE)
add_subdirectory(openal-soft EXCLUDE_FROM_ALL)

set(WOLFENSTEIN_REPOSITORY_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../..")

file(GLOB WOLFENSTEIN_ENGINE_SOURCES CONFIGURE_DEPENDS
    "${WOLFENSTEIN_REPOSITORY_ROOT}/src/*.c"
    "${WOLFENSTEIN_REPOSITORY_ROOT}/src/*.cpp"
)

add_library(wolfenstein_engine_compile OBJECT
    ${WOLFENSTEIN_ENGINE_SOURCES}
)

target_include_directories(wolfenstein_engine_compile PRIVATE
    "${WOLFENSTEIN_REPOSITORY_ROOT}/src"
    "${WOLFENSTEIN_REPOSITORY_ROOT}/third_party_libs/hash-library/hash-library"
)

target_link_libraries(wolfenstein_engine_compile PRIVATE
    SDL2
    OpenAL::OpenAL
)

add_library(wolfenstein_hash_compile OBJECT
    "${WOLFENSTEIN_REPOSITORY_ROOT}/third_party_libs/hash-library/hash-library/md5.cpp"
)

target_include_directories(wolfenstein_hash_compile PRIVATE
    "${WOLFENSTEIN_REPOSITORY_ROOT}/third_party_libs/hash-library/hash-library"
)

add_library(wolfenstein_engine_audit SHARED
    $<TARGET_OBJECTS:wolfenstein_engine_compile>
    $<TARGET_OBJECTS:wolfenstein_hash_compile>
    ResourceProbe.cpp
)

target_include_directories(wolfenstein_engine_audit PRIVATE
    "${WOLFENSTEIN_REPOSITORY_ROOT}/src"
)

target_link_libraries(wolfenstein_engine_audit PRIVATE
    SDL2
    OpenAL::OpenAL
    GLESv1_CM
    z
)

add_library(main SHARED
    main.c
)

add_library(audio_harness SHARED
    AudioHarness.c
)

target_link_libraries(audio_harness PRIVATE
    SDL2
    SDL2main
    OpenAL::OpenAL
    GLESv1_CM
    m
)

add_dependencies(main
    wolfenstein_engine_audit
)

target_link_libraries(main PRIVATE
    SDL2
    SDL2main
    GLESv1_CM
    wolfenstein_engine_audit
)
