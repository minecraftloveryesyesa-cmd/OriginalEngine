#pragma once

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

namespace originalengine {

struct RgbaImage {
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::vector<std::uint8_t> pixels;

    [[nodiscard]] bool empty() const noexcept {
        return width == 0 || height == 0 || pixels.empty();
    }
};

// Loads ASCII P3 Portable Pixmap images into tightly packed RGBA8 pixels.
class PpmImageLoader {
public:
    [[nodiscard]] static bool loadFromFile(
        const std::filesystem::path& path, RgbaImage& output, std::string* error = nullptr);
    [[nodiscard]] static bool loadFromStream(
        std::istream& input, RgbaImage& output, std::string* error = nullptr);
};

} // namespace originalengine
