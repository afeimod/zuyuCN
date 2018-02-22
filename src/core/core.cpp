// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include <utility>
#include "common/logging/log.h"
#ifdef ARCHITECTURE_x86_64
#include "core/arm/dynarmic/arm_dynarmic.h"
#endif
#include "core/arm/unicorn/arm_unicorn.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/gdbstub/gdbstub.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/thread.h"
#include "core/hle/service/service.h"
#include "core/hw/hw.h"
#include "core/loader/loader.h"
#include "core/memory_setup.h"
#include "core/settings.h"
#include "video_core/video_core.h"

namespace Core {

/*static*/ System System::s_instance;

System::ResultStatus System::RunLoop(int tight_loop) {
    status = ResultStatus::Success;
    if (!cpu_core) {
        return ResultStatus::ErrorNotInitialized;
    }

    if (GDBStub::IsServerEnabled()) {
        GDBStub::HandlePacket();

        // If the loop is halted and we want to step, use a tiny (1) number of instructions to
        // execute. Otherwise, get out of the loop function.
        if (GDBStub::GetCpuHaltFlag()) {
            if (GDBStub::GetCpuStepFlag()) {
                GDBStub::SetCpuStepFlag(false);
                tight_loop = 1;
            } else {
                return ResultStatus::Success;
            }
        }
    }

    // If we don't have a currently active thread then don't execute instructions,
    // instead advance to the next event and try to yield to the next thread
    if (Kernel::GetCurrentThread() == nullptr) {
        LOG_TRACE(Core_ARM, "Idling");
        CoreTiming::Idle();
        CoreTiming::Advance();
        PrepareReschedule();
    } else {
        CoreTiming::Advance();
        cpu_core->Run(tight_loop);
    }

    HW::Update();
    Reschedule();

    return status;
}

System::ResultStatus System::SingleStep() {
    return RunLoop(1);
}

System::ResultStatus System::Load(EmuWindow* emu_window, const std::string& filepath) {
    app_loader = Loader::GetLoader(filepath);

    if (!app_loader) {
        LOG_CRITICAL(Core, "Failed to obtain loader for %s!", filepath.c_str());
        return ResultStatus::ErrorGetLoader;
    }
    std::pair<boost::optional<u32>, Loader::ResultStatus> system_mode =
        app_loader->LoadKernelSystemMode();

    if (system_mode.second != Loader::ResultStatus::Success) {
        LOG_CRITICAL(Core, "Failed to determine system mode (Error %i)!",
                     static_cast<int>(system_mode.second));

        switch (system_mode.second) {
        case Loader::ResultStatus::ErrorEncrypted:
            return ResultStatus::ErrorLoader_ErrorEncrypted;
        case Loader::ResultStatus::ErrorInvalidFormat:
            return ResultStatus::ErrorLoader_ErrorInvalidFormat;
        default:
            return ResultStatus::ErrorSystemMode;
        }
    }

    ResultStatus init_result{Init(emu_window, system_mode.first.get())};
    if (init_result != ResultStatus::Success) {
        LOG_CRITICAL(Core, "Failed to initialize system (Error %i)!", init_result);
        System::Shutdown();
        return init_result;
    }

    const Loader::ResultStatus load_result{app_loader->Load(Kernel::g_current_process)};
    if (Loader::ResultStatus::Success != load_result) {
        LOG_CRITICAL(Core, "Failed to load ROM (Error %i)!", load_result);
        System::Shutdown();

        switch (load_result) {
        case Loader::ResultStatus::ErrorEncrypted:
            return ResultStatus::ErrorLoader_ErrorEncrypted;
        case Loader::ResultStatus::ErrorInvalidFormat:
            return ResultStatus::ErrorLoader_ErrorInvalidFormat;
        default:
            return ResultStatus::ErrorLoader;
        }
    }
    status = ResultStatus::Success;
    return status;
}

void System::PrepareReschedule() {
    cpu_core->PrepareReschedule();
    reschedule_pending = true;
}

PerfStats::Results System::GetAndResetPerfStats() {
    return perf_stats.GetAndResetStats(CoreTiming::GetGlobalTimeUs());
}

void System::Reschedule() {
    if (!reschedule_pending) {
        return;
    }

    reschedule_pending = false;
    Core::System::GetInstance().Scheduler().Reschedule();
}

System::ResultStatus System::Init(EmuWindow* emu_window, u32 system_mode) {
    LOG_DEBUG(HW_Memory, "initialized OK");

    CoreTiming::Init();

    switch (Settings::values.cpu_core) {
    case Settings::CpuCore::Unicorn:
        cpu_core = std::make_shared<ARM_Unicorn>();
        break;
    case Settings::CpuCore::Dynarmic:
    default:
#ifdef ARCHITECTURE_x86_64
        cpu_core = std::make_shared<ARM_Dynarmic>();
#else
        cpu_core = std::make_shared<ARM_Unicorn>();
        LOG_WARNING(Core, "CPU JIT requested, but Dynarmic not available");
#endif
        break;
    }

    gpu_core = std::make_unique<Tegra::GPU>();

    telemetry_session = std::make_unique<Core::TelemetrySession>();

    HW::Init();
    Kernel::Init(system_mode);
    scheduler = std::make_unique<Kernel::Scheduler>(cpu_core.get());
    Service::Init();
    GDBStub::Init();

    if (!VideoCore::Init(emu_window)) {
        return ResultStatus::ErrorVideoCore;
    }

    LOG_DEBUG(Core, "Initialized OK");

    // Reset counters and set time origin to current frame
    GetAndResetPerfStats();
    perf_stats.BeginSystemFrame();

    return ResultStatus::Success;
}

void System::Shutdown() {
    // Log last frame performance stats
    auto perf_results = GetAndResetPerfStats();
    Telemetry().AddField(Telemetry::FieldType::Performance, "Shutdown_EmulationSpeed",
                         perf_results.emulation_speed * 100.0);
    Telemetry().AddField(Telemetry::FieldType::Performance, "Shutdown_Framerate",
                         perf_results.game_fps);
    Telemetry().AddField(Telemetry::FieldType::Performance, "Shutdown_Frametime",
                         perf_results.frametime * 1000.0);

    // Shutdown emulation session
    VideoCore::Shutdown();
    GDBStub::Shutdown();
    Service::Shutdown();
    scheduler = nullptr;
    Kernel::Shutdown();
    HW::Shutdown();
    telemetry_session = nullptr;
    gpu_core = nullptr;
    cpu_core = nullptr;
    CoreTiming::Shutdown();

    app_loader = nullptr;

    LOG_DEBUG(Core, "Shutdown OK");
}

} // namespace Core
