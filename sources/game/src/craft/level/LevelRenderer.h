#pragma once

#include "core/ThreadPool.h"
#include "core/intrusive_ptr.h"
#include "craft/core/LockQueue.h"
#include "craft/level/chunk.h"
#include "platform/graphics/Mesh.h"

#include <future>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Craft{

class LevelRenderer{
private:
    struct ChunkRendererData{
        intrusive_ptr<Goonya::Graphics::Mesh> complied_chunk_mesh; 
    };
    std::unordered_map<Ref<Chunk>, ChunkRendererData> rendering_chunk;
    std::vector<std::future<void>> compile_tasks;
    LockQueue<Ref<Chunk>> chunk_to_remove;

public:
    void register_chunk(Ref<Chunk> chunk) noexcept {
        compile_tasks.emplace_back(Goonya::THREAD_POOL.enqueue([]{
            
        }));
    }

    void unregister_chunk(Ref<Chunk> chunk) noexcept {
        chunk_to_remove.push_back(std::move(chunk));
    }

    void notify_chunk_update(Ref<Chunk> chunk) noexcept{


    }

    void render_frame(){


    }

private:
    ChunkRendererData complie(Ref<Chunk> chunk){
        return {};
    }
};

}