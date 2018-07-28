// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include "common/common_funcs.h"
#include "common/swap.h"
#include "core/file_sys/vfs.h"

namespace FileSys {

struct IVFCLevel {
    u64_le offset;
    u64_le size;
    u32_le block_size;
    u32_le reserved;
};
static_assert(sizeof(IVFCLevel) == 0x18, "IVFCLevel has incorrect size.");

struct IVFCHeader {
    u32_le magic;
    u32_le magic_number;
    INSERT_PADDING_BYTES(8);
    std::array<IVFCLevel, 6> levels;
    INSERT_PADDING_BYTES(64);
};
static_assert(sizeof(IVFCHeader) == 0xE0, "IVFCHeader has incorrect size.");

// Converts a RomFS binary blob to VFS Filesystem
// Returns nullptr on failure
VirtualDir ExtractRomFS(VirtualFile file);

} // namespace FileSys
