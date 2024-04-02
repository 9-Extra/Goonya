set(TARGET_NAME GoonyaDemo)

add_executable(${TARGET_NAME})

file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/*.cpp src/*.h)
target_sources(${TARGET_NAME} PRIVATE ${sources})

target_link_libraries(${TARGET_NAME} PUBLIC GoonyaRuntime)

target_compile_definitions(${TARGET_NAME} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:${TARGET_NAME}>
        $<TARGET_FILE_DIR:${TARGET_NAME}> 
    COMMAND_EXPAND_LISTS
)