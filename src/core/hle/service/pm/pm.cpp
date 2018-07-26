// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/service.h"

namespace Service::PM {

class BootMode final : public ServiceFramework<BootMode> {
public:
    explicit BootMode() : ServiceFramework{"pm:bm"} {
        static const FunctionInfo functions[] = {
            {0, nullptr, "GetBootMode"},
            {1, nullptr, "SetMaintenanceBoot"},
        };
        RegisterHandlers(functions);
    }
};

class DebugMonitor final : public ServiceFramework<DebugMonitor> {
public:
    explicit DebugMonitor() : ServiceFramework{"pm:dmnt"} {
        static const FunctionInfo functions[] = {
            {0, nullptr, "IsDebugMode"},
            {1, nullptr, "GetDebugProcesses"},
            {2, nullptr, "StartDebugProcess"},
            {3, nullptr, "GetTitlePid"},
            {4, nullptr, "EnableDebugForTitleId"},
            {5, nullptr, "GetApplicationPid"},
            {6, nullptr, "EnableDebugForApplication"},
        };
        RegisterHandlers(functions);
    }
};

class Info final : public ServiceFramework<Info> {
public:
    explicit Info() : ServiceFramework{"pm:info"} {
        static const FunctionInfo functions[] = {
            {0, nullptr, "GetTitleId"},
        };
        RegisterHandlers(functions);
    }
};

class Shell final : public ServiceFramework<Shell> {
public:
    explicit Shell() : ServiceFramework{"pm:shell"} {
        static const FunctionInfo functions[] = {
            {0, nullptr, "LaunchProcess"},
            {1, nullptr, "TerminateProcessByPid"},
            {2, nullptr, "TerminateProcessByTitleId"},
            {3, nullptr, "GetProcessEventWaiter"},
            {4, nullptr, "GetProcessEventType"},
            {5, nullptr, "NotifyBootFinished"},
            {6, nullptr, "GetApplicationPid"},
            {7, nullptr, "BoostSystemMemoryResourceLimit"},
        };
        RegisterHandlers(functions);
    }
};

void InstallInterfaces(SM::ServiceManager& sm) {
    std::make_shared<BootMode>()->InstallAsService(sm);
    std::make_shared<DebugMonitor>()->InstallAsService(sm);
    std::make_shared<Info>()->InstallAsService(sm);
    std::make_shared<Shell>()->InstallAsService(sm);
}

} // namespace Service::PM
