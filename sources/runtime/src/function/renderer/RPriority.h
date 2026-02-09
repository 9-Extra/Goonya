#pragma once

#include <cstdint>

namespace Goonya {

enum RenderPriority : uint32_t { OPAQUE = 1000, SKYBOX = 2000, TRANSPARENT = 3000 };

}