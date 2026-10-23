# Agrupar archivos por tipo
file(GLOB_RECURSE HEADER_FILES
    "*.hpp"
    "*.h"
)

file(GLOB_RECURSE SOURCE_FILES
    "*.cpp"
    "*.c"
)

file(GLOB_RECURSE RESOURCES_FILES
    "*.rc"
    "*.ico"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${HEADER_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${SOURCE_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Resources Files" FILES ${RESOURCES_FILES})

# Crear el ejecutable
add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES} ${RESOURCES_FILES})

# Enlace de bibliotecas
target_link_libraries(${PROJECT_NAME} SDL2::SDL2 ZLIB::ZLIB OpenGL::GL ${HASH_LIBRARY_TGT_NAME})

# Enlace de OpenAL según el sistema operativo
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(${PROJECT_NAME} OpenAL::OpenAL ${OPENGL_gl_LIBRARY})
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(${PROJECT_NAME} OpenGL::GL ${OPENAL_LIBRARY})
    # Verifica si realmente necesitas una compilación estática
    target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)
endif()

# UTF-8 engine paths, native UTF-16 Windows filesystem access.
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} shell32 ole32 uuid)
endif()
