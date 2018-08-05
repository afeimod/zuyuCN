// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <boost/optional.hpp>
#include "common/assert.h"
#include "common/common_types.h"
#include "video_core/gpu.h"
#include "video_core/rasterizer_interface.h"

class EmuWindow;

namespace VideoCore {

class RendererBase : NonCopyable {
public:
    /// Used to reference a framebuffer
    enum kFramebuffer { kFramebuffer_VirtualXFB = 0, kFramebuffer_EFB, kFramebuffer_Texture };

    explicit RendererBase(EmuWindow& window);
    virtual ~RendererBase();

    /// Swap buffers (render frame)
    virtual void SwapBuffers(boost::optional<const Tegra::FramebufferConfig&> framebuffer) = 0;

    /// Initialize the renderer
    virtual bool Init() = 0;

    /// Shutdown the renderer
    virtual void ShutDown() = 0;

    /// Updates the framebuffer layout of the contained render window handle.
    void UpdateCurrentFramebufferLayout();

    // Getter/setter functions:
    // ------------------------

    f32 GetCurrentFPS() const {
        return m_current_fps;
    }

    int GetCurrentFrame() const {
        return m_current_frame;
    }

    RasterizerInterface& Rasterizer() {
        return *rasterizer;
    }

    const RasterizerInterface& Rasterizer() const {
        return *rasterizer;
    }

    void RefreshRasterizerSetting();

protected:
    EmuWindow& render_window; ///< Reference to the render window handle.
    std::unique_ptr<RasterizerInterface> rasterizer;
    f32 m_current_fps = 0.0f; ///< Current framerate, should be set by the renderer
    int m_current_frame = 0;  ///< Current frame, should be set by the renderer
};

} // namespace VideoCore
