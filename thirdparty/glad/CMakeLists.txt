add_library(glad STATIC)

target_include_directories(glad PUBLIC include)
target_sources(glad PRIVATE src/glad.c)
target_link_libraries(glad INTERFACE opengl32 gdi32)