#include "Loader.h"

#include "resource/ResMng.h"
#include "resource/loader/GShaderLoader.h"
#include "resource/loader/MaterialLoader.h"
#include "resource/loader/SceneLoader.h"
#include "resource/loader/ShaderLoader.h"
#include "resource/loader/TextureLoader.h"
#include "resource/loader/glTFLoader.h"

#include <memory>

namespace Goonya {

void register_all_loaders() {
    resources.register_loader(std::make_shared<TextureLoader>());
    resources.register_loader(std::make_shared<ShaderLoader>());
    resources.register_loader(std::make_shared<MateriaLoader>());
    resources.register_loader(std::make_shared<SceneLoader>());
    resources.register_loader(std::make_shared<GlTFLoader>());
    resources.register_loader(std::make_shared<GShaderLoader>());
}

} // namespace Goonya