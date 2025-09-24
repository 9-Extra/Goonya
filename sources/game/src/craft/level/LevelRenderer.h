#pragma once

#include "core/ThreadPool.h"
#include "core/intrusive_ptr.h"
#include "craft/core/core.h"
#include "craft/level/SectionCompiler.h"
#include "craft/level/chunk.h"
#include "function/renderer/RenderProxy/StaticMesh.h"
#include "function/renderer/Renderer.h"
#include "platform/graphics/Material.h"

#include <cassert>
#include <functional>
#include <future>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Craft {

/*
Minecraft世界Level被分为16 * height *
16大小的区块Chunk，而区块内有进一步分为16*16*16的区段LevelChunkSection进行存储和渲染。
对于需要渲染的区段，从硬盘到屏幕大概经过以下流程：接收区块包 -> 更新ClientLevel区块 -> 可见性剔除 ->
编译可见区段为SectionRenderDispatcher.RenderSection -> 渲染
接下来主要关注可见性剔除和区块编译两个步骤，这两步Minecraft使用了复杂的异步实现，而且非常混乱。

先说SectionRenderDispatcher.RenderSection，这里叫他“渲染区段”，此为Minecraft中一个可直接渲染的16x16x16区域，与区块的LevelChunkSection相对应
其直接存储了不同RenderType对应的VertexBuffer。在LevelRenderer.renderSectionLayer中会遍历visibleSections中
的所有RenderSection，取出其中指定RenderType的VertexBuffer，和起点的BlockPos，设置进unifrom然后drawcall。

这些渲染区段同时由LevelRenderer中的四个子对象持有和管理（神人Mojang）：
1. ObjectArrayList<SectionRenderDispatcher.RenderSection> visibleSections //
当前所有可见的RenderSection的集合，是剔除的结果和编译的起点
2. SectionOcclusionGraph sectionOcclusionGraph // 负责剔除，每帧根据相机位置和旋转同步更新visibleSections
3. ViewArea viewArea // 负责根据当前的渲染距离和相机所在区段创建所有的RenderSection，只是创建
4. LevelRenderer本身 // 每帧遍历visibleSections发起同步或异步编译
5. SectionRenderDispatcher sectionRenderDispatcher // 负责RenderSection的编译，包含编译的代码

下面先说剔除流程：
visibleSections是剔除结果保存的地方，它的相关写入操作只在两个地方
1. 每次renderLevel中setupRender时运行的sectionOcclusionGraph.update(flag, camera, frustum,
this.visibleSections)，可能是负责添加刚刚加载出来的区块
2.
在此之后，如果相机方向微小旋转，或者相机移动超过8x8x8的范围，则会进入this.applyFrustum(offsetFrustum(frustum))分支，清空重建整个visibleSections。（也就是说视角稍微动一动，就需要重新做剔除，很合理）
这个重建过程（sectionRenderDispatcher.addSectionsInFrustum()）其实就是取出sectionRenderDispatcher.currentGraph中已经准备好的renderSections与当前相机的frustum求交，
也就是说什么sectionRenderDispatcher.currentGraph中其实已经保存了所有可能被渲染的区段。
可见虽然visibleSections直接存储在LevelRenderer中，其实质是由sectionOcclusionGraph负责管理的（有时LevelRenderer会清空它）

追溯RenderSection的构建流程，可以发现只要不是发生资源重加载，渲染距离更改等额外触发LevelRenderer.allChanged()的事件，
所有的RenderSection只会在初始化中的LevelRenderer.allChanged()-> new ViewArea ->
ViewArea.createSections中被一次性全部构建
并存储在viewArea.sections数组中。之后每帧的渲染都复用这些RenderSection。在以下情况对RenderSection进行更新（清空并标记为脏）：
1. 在setupRender中，如果相机所在的section发生变化，则调用viewArea.repositionCamera(player_pos_x, player_pos_y)，其调用
更新viewArea.sections中的所有RenderSection的setOrigin以更新原点，同时会将其标记为脏，取消运行中的RebuildTask和ResortTransparencyTask（如果有），
设置compiled为UNCOMPILED（这个对象用于记录整个section在不同方向上的遮挡，用于剔除），更新包围盒
2. 方块更新后，最终会调用到viewArea.setDirty()对指定section进行重置，重置时仅仅设置脏标记

viewArea只负责初始化和更新RenderSection中的原点信息（即它们的位置）以及接收一些位置更新或者方块更新（可以通过viewArea快速找到一个位置对应的section），
但RenderSection中其他数据，包括buffer，各种构建任务都不由viewArea负责。这些数据由RenderSection自身负责管理。只需要外部调用RenderSection.compileSync()
或者RenderSection.rebuildSectionAsync()就可以自己完成编译和buffers上传。

这两个重建section的函数是在每帧LevelRenderer.renderLevel -> compileSections（在所有的renderSectionLayer之前）中进行的。
LevelRenderer会遍历自己的visibleSections，如果发现section为脏或者发生光照更新，则根据距离，是否由玩家修改等依据调用同步或异步版本进行编译任务。
需要注意的是在遍历之前会创建一个新的RenderRegionCache对象（每帧新建）。此对象用于保存当前帧所有区块信息的副本，需要传入section的重建函数中以使
其在编译过程中可以异步访问区块数据。当然复制所有区块是昂贵的，因此RenderRegionCache是惰性求值并在不同任务中共享的（异步版本在创建任务过程中求值），只有当前帧被需要区块信息才会真正复制。
一个section的编译需要其附近共9个区块的信息，打包成一个RenderChunkRegion对象用于在任务中访问。

在RenderChunkRegion的异步编译流程中，创建任务的过程仍然是同步的，先取消上一个执行的任务（如果有），准备任务数据，然后将任务对象（RenderSection.CompileTask）写入lastRebuildTask中。
然后开始调度此任务，将任务对象送入SectionRenderDispatcher.mailbox任务队列中执行。取消任务就是设置任务对象中的isCancelled原子量然后将sections本体重新标记为脏。
在执行任务时如果isCancelled为true则直接返回SectionTaskResult.CANCELLED

收到区块包逻辑
1.
逻辑端ServerLevel从硬盘读出区块，自身保存此区块的同时发包给客户端（单人游戏也是这样，同时存在ServelLevel和ClientLevel两个世界的副本，运行在两个线程并且发包同步）
2.
接下来的事情都发生在渲染线程，ClientPackListener收到ClientboundLevelChunkWithLightPacket包后调用level.chunkSource.replaceWithPacketData创建或者替换自身保存的区块数据
3. 光换完数据是不够的，还需要调用level.levelRenderer.onChunkLoaded(chunkPos)通知渲染器区块发生更新，需要重新编译
4. sectionOcclusionGraph添加邻居

Minecraft中LevelRender和ClientLevel运行在同一线程，SeverLevel运行在另一个线程。因此Minecraft中渲染区块时可以直接读区块数据，没有同步问题。
收发包的操作与渲染的操作只有单个线程。
*/

/**
 * @brief 一个可以渲染的区块，除了对应的mesh，还管理它的编译任务
 *
 */
class RenderSection : public std::enable_shared_from_this<RenderSection> {
public:
    ChunkPos chunk_pos;
    Ref<Chunk> origin_chunk;

    std::shared_ptr<ComplieTask> complie_task;
    bool is_dirty = true;

    Goonya::Graphics::MeshRenderProxy *mesh_proxy = nullptr;

    explicit RenderSection(Ref<Chunk> chunk) : chunk_pos(chunk->chunk_pos), origin_chunk(chunk) {
        assert(origin_chunk);
    }

    ~RenderSection() {
        if (complie_task) {
            complie_task->cancel();
        }
        if (mesh_proxy) {
            Goonya::Graphics::renderer.remove_mesh_proxy(mesh_proxy);
        }
    }

    void complie_async(
        RenderRegionCache &region_cache,
        std::move_only_function<void(std::shared_ptr<RenderSection> &, ComplieTask::ComplieResult &&)> delegate) {
        assert(is_dirty);
        if (complie_task) {
            complie_task->cancel();
        }

        complie_task = std::make_shared<ComplieTask>(this->weak_from_this(), region_cache.create_region(chunk_pos));
        complie_task->task_blocker =
            Goonya::THREAD_POOL.enqueue([task = this->complie_task, delegate = std::move(delegate)] mutable {
                // 在LevelRenderer销毁前一定提前结束所有任务，所以queue一定可用
                task->do_complie(std::move(delegate));
            });

        is_dirty = false;
    }
};

class LevelRenderer {
public:
    intrusive_ptr<Goonya::Graphics::Material> terrain_material;

private:
    std::unordered_map<ChunkPos, std::shared_ptr<RenderSection>> render_chunks; // 当前帧所有可能渲染的区块

    std::vector<RenderSection *> visible_chunk; // 当前帧可见的区块
public:
    LevelRenderer();
    ~LevelRenderer() {
        render_chunks.clear(); // 提前销毁render_chunks，保证所有ComplieTask已结束
    }

    void register_chunk(const Ref<Chunk> &chunk) noexcept {
        ChunkPos pos = chunk->chunk_pos;
        auto section = std::make_unique<RenderSection>(chunk);

        render_chunks.emplace(pos, std::move(section));
    }

    void unregister_chunk(const Ref<Chunk> &chunk) noexcept { render_chunks.erase(chunk->chunk_pos); }

    RenderSection *get_section(ChunkPos pos) const noexcept {
        auto iter = render_chunks.find(pos);
        return iter != render_chunks.end() ? iter->second.get() : nullptr;
    }

    /**
     * @brief 检查一个位置区块的所有6个相邻区块是否处于可渲染状态
     */
    bool has_all_neighbors(ChunkPos pos) const noexcept {
        bool all = render_chunks.contains(ChunkPos{pos + ChunkPos{0, 0, 1}}) &&
                   render_chunks.contains(ChunkPos{pos + ChunkPos{0, 0, -1}}) &&
                   render_chunks.contains(ChunkPos{pos + ChunkPos{0, 1, 0}}) &&
                   render_chunks.contains(ChunkPos{pos + ChunkPos{0, -1, 0}}) &&
                   render_chunks.contains(ChunkPos{pos + ChunkPos{1, 0, 0}}) &&
                   render_chunks.contains(ChunkPos{pos + ChunkPos{-1, 0, 0}});
        return all;
    }

    void notify_chunk_update(const Ref<Chunk> &chunk) noexcept {}

    void render_frame();

private:
    void do_cull();
};

} // namespace Craft