#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <imgui.h>
#include <locale>
#include <runtime/Goonya.h>
#include <sys/types.h>

#include "core/RefCount.h"
#include "core/ThreadUtils.h"
#include "core/clock/GameClock.h"
#include "core/eventbus/eventbus.h"
#include "core/events.h"
#include "core/input/input.h"
#include "core/log/Log.h"
#include "core/path_formatter.h"
#include "function/renderer/Renderer.h"
#include "function/world/World.h"
#include "platform/display/display.h"
#include "platform/graphics/Graphics.h"
#include "platform/graphics/opengl/GLMesh.h"
#include "platform/graphics/opengl/GLTexture.h"
#include "platform/image/image.h"
#include "platform/imgui/imgui_module.h"
#include "resource/HardcodeAssets.h"
#include "resource/ResMng.h"
#include "resource/Resource.h"

namespace Goonya {

static uint32_t calculate_fps(GameClock::ClockType::duration delta) {
    using Clock = std::chrono::steady_clock;
    static Clock::duration average_frame_time = Clock::duration::zero();
    const float smoothing_factor = 0.05f;

    average_frame_time =
        Clock::duration((Clock::duration::rep)std::lerp(average_frame_time.count(), delta.count(), smoothing_factor));

    return static_cast<uint32_t>(std::chrono::seconds(1) / average_frame_time);
}

static void init_buildin_resource() {
    Ref<ResourcePack> buildin = create_ref<ResourcePack>();
    resources.put_resource("buildin", buildin);
    { // 部分硬编码的mesh
        std::span<const std::byte> plane_vertex_span = std::as_bytes(std::span(Assets::plane_vertices));
        Ref<GLMesh> plane = create_ref<GLMesh>(Assets::plane_vertices_vertex_layout);
        plane->set_vertices(0, plane_vertex_span);
        plane->set_indices(Assets::plane_indices);
        plane->submeshes.emplace_back(SubMesh{.start_index = 0,
                                              .index_count = (uint32_t)Assets::plane_indices.size(),
                                              .base_vertex_offset = 0,
                                              .topology = Topology::TRIANGLE,
                                              .aabb = Assets::plane_aabb});
        buildin->contents.emplace("plane", plane);
    }

    { // 添加天空盒的mesh，因为格式不一样所以单独处理
        std::span<const std::byte> skybox_cube_vertex_span = std::as_bytes(std::span(Assets::skybox_cube_vertices));
        Ref<GLMesh> skybox_cube = create_ref<GLMesh>(Assets::skybox_cube_vertex_layout);
        skybox_cube->set_vertices(0, skybox_cube_vertex_span);
        skybox_cube->set_indices(Assets::skybox_cube_indices);
        skybox_cube->submeshes.emplace_back(SubMesh{.start_index = 0,
                                                    .index_count = (uint32_t)Assets::skybox_cube_indices.size(),
                                                    .base_vertex_offset = 0,
                                                    .topology = Topology::TRIANGLE,
                                                    .aabb = Assets::skybox_cube_aabb});
        buildin->contents.emplace("skybox_cube", skybox_cube);
    }

    const uint32_t default_texture_size = 16;
    stb::Image image = stb::Image::create_empty(default_texture_size, default_texture_size, 3, false);
    struct Color {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };
    {
        Ref<GLTexture> white = create_ref<GLTexture>(TextureCreateDesc{
            .type = TextureType::TEXTURE_2D,
            .format = TextureStorageFormat::RGB_f16,
            .shape = {default_texture_size, default_texture_size, 0},
        });
        for (size_t i = 0; i < image.get_size_byte(); i++) {
            ((uint8_t *)image.get_data())[i] = 255;
        }
        white->import_image(image);
        buildin->contents.emplace("white", white);
    }
    {
        Ref<GLTexture> black = create_ref<GLTexture>(TextureCreateDesc{
            .type = TextureType::TEXTURE_2D,
            .format = TextureStorageFormat::RGB_f16,
            .shape = {default_texture_size, default_texture_size, 0},
        });
        for (size_t i = 0; i < image.get_size_byte(); i++) {
            ((uint8_t *)image.get_data())[i] = 0;
        }
        black->import_image(image);
        buildin->contents.emplace("black", black);
    }
    {
        Ref<GLTexture> normal = create_ref<GLTexture>(TextureCreateDesc{
            .type = TextureType::TEXTURE_2D,
            .format = TextureStorageFormat::RGB_f16,
            .shape = {default_texture_size, default_texture_size, 0},
        });
        for (size_t i = 0; i < default_texture_size; i++) {
            for (size_t j = 0; j < default_texture_size; j++) {
                Color &color_ref = ((Color *)image.get_data())[i * default_texture_size + j];
                color_ref = {127, 127, 255}; // (0.5, 0.5, 1)
            }
        }
        normal->import_image(image);
        buildin->contents.emplace("normal", normal);
    }
    {
        Ref<GLTexture> missing = create_ref<GLTexture>(TextureCreateDesc{
            .type = TextureType::TEXTURE_2D,
            .format = TextureStorageFormat::RGB_f16,
            .shape = {default_texture_size, default_texture_size, 0},
        });
        for (size_t i = 0; i < default_texture_size; i++) {
            for (size_t j = 0; j < default_texture_size; j++) {
                Color &color_ref = ((Color *)image.get_data())[i * default_texture_size + j];
                if ((i < default_texture_size / 2) != (j < default_texture_size / 2)) {
                    color_ref = {0, 0, 0}; // black
                } else {
                    color_ref = {128, 0, 128}; // purple
                }
            }
        }
        missing->import_image(image);
        buildin->contents.emplace("missing_texture", missing);
    }
}

void init_engine() {
    std::locale::global(std::locale("")); // 使用系统指定的本地化
    set_current_thread_name("game");

    LOG_INFO("Running on pwd: {}", std::filesystem::current_path());

    EventBus::initalize();
    Input::initalize();
    Display::initialize(1080, 720);

    current_thread_type = ThreadType::RENDER; // 目前先不拆分线程，资源加载的问题没有解决
    GL.initialize();

    resources.init(u8"../assets/");
    init_buildin_resource();

    renderer.init();

    ImguiMng::init();

    EventBus::subscribe_event<Events::PostTick>(0, [](Events::PostTick &e) {
        uint32_t fps = calculate_fps(GAME_CLOCK.delta());
        Display::set_title(std::format("Goonya - FPS: {}", fps));
        return false;
    });

    EventBus::subscribe_event<Events::SysWindowClose>(0, [](Events::SysWindowClose &e) {
        EventBus::dispatch_event(Events::EngineStop{});
        return false;
    });
}

static void logic_tick() {
    ImGui::ShowDemoWindow();
    // auto [x, y] = Input::get_mouse_pos();
    // LOG_DEBUG("鼠标位置: {}, {}", x, y);
    for (auto &&w : World::world_list) {
        w.tick();
    }
}

static void fixed_tick() {
    // auto [x, y] = Input::get_mouse_pos();
    // LOG_DEBUG("鼠标位置: {}, {}", x, y);
    for (auto &&w : World::world_list) {
        w.fixed_tick();
    }
}

static void render_frame() {

    renderer.render();

    ImGui::EndFrame();
    ImGui::Render();
    ImguiMng::render();
}

void main_loop() {
    bool should_continue = true;
    EventBus::ListenerID id =
        EventBus::subscribe_event<Events::EngineStop>(10, [&should_continue](Events::EngineStop &e) {
            should_continue = false;
            return true;
        });

    GAME_CLOCK.start();

    while (should_continue) {
        GAME_CLOCK.update();

        ImguiMng::new_frame();
        ImGui::NewFrame();

        Display::poll_events();
        while (GAME_CLOCK.advance_fixed_tick()) {
            fixed_tick();
        }

        logic_tick();
        render_frame();

        EventBus::dispatch_event(Events::PostTick{});
        Display::swap();
    }

    EventBus::remove_listener<Events::SysWindowClose>(id);
}

void drop_engine() {
    // 需要保证此函数在没有初始化或者初始化到一半的情况下也能正常工作
    LOG_WARN("退出");

    World::world_list.clear();
    renderer.clear();
    resources.clear(); // 在设备drop之前清理资源
    GL.drop();
    ImguiMng::drop();

    Display::drop();
    core_logger->flush();
}
} // namespace Goonya
