cmake_minimum_required(VERSION 3.22)

project(wolfenstein_android_test LANGUAGES C)

add_subdirectory(SDL)

add_library(main SHARED
    main.c
)

target_link_libraries(main PRIVATE
    SDL2
    SDL2main
    GLESv1_CM
)