set(TARGET_NAME GoonyaRuntime)

add_library(${TARGET_NAME} STATIC)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/*.cpp src/*.h)
target_sources(${TARGET_NAME} PRIVATE ${sources})

target_include_directories(${TARGET_NAME} PUBLIC include PRIVATE src)

target_compile_definitions(${TARGET_NAME} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)

find_package(spdlog REQUIRED)
find_package(Protobuf REQUIRED)
find_package(jsoncpp REQUIRED)
find_package(directx-headers CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
# 编译.proto为.cpp
# file(GLOB_RECURSE proto_sources CONFIGURE_DEPENDS proto/*.proto)
# PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${proto_sources})
# target_sources(${TARGET_NAME} PRIVATE ${PROTO_SRCS} PRIVATE ${PROTO_HDRS})
# target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})  # 包含proto生成文件

target_link_libraries(${TARGET_NAME} PRIVATE Microsoft::DirectX-Headers)
target_link_libraries(${TARGET_NAME} PUBLIC spdlog::spdlog PUBLIC JsonCpp::JsonCpp PUBLIC imgui::imgui PUBLIC protobuf::libprotobuf-lite)
target_link_libraries(${TARGET_NAME} PRIVATE DXGI.lib D3D12.lib)