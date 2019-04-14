// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <unordered_map>

#include <glad/glad.h>

#include "common/assert.h"
#include "common/common_types.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/rasterizer_cache.h"
#include "video_core/renderer_opengl/gl_resource_manager.h"

namespace OpenGL {

namespace GLShader {
class GlobalMemoryEntry;
}

class RasterizerOpenGL;
class CachedGlobalRegion;
using GlobalRegion = std::shared_ptr<CachedGlobalRegion>;

class CachedGlobalRegion final : public RasterizerCacheObject {
public:
    explicit CachedGlobalRegion(VAddr cpu_addr, u8* host_ptr, u32 size, u32 max_size);
    ~CachedGlobalRegion();

    VAddr GetCpuAddr() const override {
        return cpu_addr;
    }

    std::size_t GetSizeInBytes() const override {
        return size;
    }

    /// Gets the GL program handle for the buffer
    GLuint GetBufferHandle() const {
        return buffer.handle;
    }

    /// Reloads the global region from guest memory
    void Reload(u32 size_);

    void Flush() override;

private:
    VAddr cpu_addr{};
    u8* host_ptr{};
    u32 size{};
    u32 max_size{};

    OGLBuffer buffer;
};

class GlobalRegionCacheOpenGL final : public RasterizerCache<GlobalRegion> {
public:
    explicit GlobalRegionCacheOpenGL(RasterizerOpenGL& rasterizer);

    /// Gets the current specified shader stage program
    GlobalRegion GetGlobalRegion(const GLShader::GlobalMemoryEntry& descriptor,
                                 Tegra::Engines::Maxwell3D::Regs::ShaderStage stage);

private:
    GlobalRegion TryGetReservedGlobalRegion(CacheAddr addr, u32 size) const;
    GlobalRegion GetUncachedGlobalRegion(GPUVAddr addr, u8* host_ptr, u32 size);
    void ReserveGlobalRegion(GlobalRegion region);

    std::unordered_map<CacheAddr, GlobalRegion> reserve;
    u32 max_ssbo_size{};
};

} // namespace OpenGL
