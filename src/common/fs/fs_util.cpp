// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>

#ifdef __APPLE__
#include "common/apple_compat/appleCompat.h"
#endif
#include "common/fs/fs_util.h"

namespace Common::FS {

std::u8string ToU8String(std::string_view utf8_string) {
    return std::u8string{utf8_string.begin(), utf8_string.end()};
}

std::u8string BufferToU8String(std::span<const u8> buffer) {
#ifdef __APPLE__
    return std::u8string{buffer.begin(), ranges::find(buffer, u8{0})};
#else
    return std::u8string{buffer.begin(), std::ranges::find(buffer, u8{0})};
#endif
}

std::u8string_view BufferToU8StringView(std::span<const u8> buffer) {
    return std::u8string_view{reinterpret_cast<const char8_t*>(buffer.data())};
}

std::string ToUTF8String(std::u8string_view u8_string) {
    return std::string{u8_string.begin(), u8_string.end()};
}

std::string BufferToUTF8String(std::span<const u8> buffer) {
#ifdef __APPLE__
    return std::string{buffer.begin(), ranges::find(buffer, u8{0})};
#else
    return std::string{buffer.begin(), std::ranges::find(buffer, u8{0})};
#endif
}

std::string_view BufferToUTF8StringView(std::span<const u8> buffer) {
    return std::string_view{reinterpret_cast<const char*>(buffer.data())};
}

std::string PathToUTF8String(const std::filesystem::path& path) {
    return ToUTF8String(path.u8string());
}

} // namespace Common::FS
