// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/apm/apm.h"

namespace Service {
namespace APM {

void InstallInterfaces(SM::ServiceManager& service_manager) {
    std::make_shared<APM>()->InstallAsService(service_manager);
}

APM::APM() : ServiceFramework("apm") {
    static const FunctionInfo functions[] = {
        {0x00000000, nullptr, "OpenSession"}, {0x00000001, nullptr, "GetPerformanceMode"},
    };
    RegisterHandlers(functions);
}

} // namespace APM
} // namespace Service
