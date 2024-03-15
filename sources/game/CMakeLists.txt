set(TARGET_NAME GoonyaDemo)

add_executable(${TARGET_NAME})

file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/*.cpp src/*.h)
target_sources(${TARGET_NAME} PRIVATE ${sources})

target_link_libraries(${TARGET_NAME} PUBLIC GoonyaRuntime)

target_compile_definitions(${TARGET_NAME} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)