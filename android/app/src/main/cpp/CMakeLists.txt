cmake_minimum_required(VERSION 3.22)

project(wolfenstein_android_test LANGUAGES C CXX)

add_subdirectory(SDL)

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
    "${CMAKE_CURRENT_SOURCE_DIR}/compile_audit"
)

target_compile_definitions(wolfenstein_engine_compile PRIVATE
    WOLFENSTEIN_ANDROID_COMPILE_AUDIT=1
)

target_link_libraries(wolfenstein_engine_compile PRIVATE
    SDL2
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
    compile_audit/OpenALCompileAudit.cpp
)

target_include_directories(wolfenstein_engine_audit PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/compile_audit"
)

target_link_libraries(wolfenstein_engine_audit PRIVATE
    SDL2
    GLESv1_CM
    z
)

add_library(main SHARED
    main.c
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
