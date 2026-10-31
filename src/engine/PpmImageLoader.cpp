#include "engine/PpmImageLoader.hpp"

#include <charconv>
#include <cctype>
#include <fstream>
#include <limits>
#include <string>
#include <system_error>
#include <utility>

namespace originalengine {
namespace {

void setError(std::string* error, const std::string& message) {
    if (error != nullptr) *error = message;
}

bool nextToken(std::istream& input, std::string& token) {
    token.clear();
    char character = '\0';
    while (input.get(character)) {
        if (std::isspace(static_cast<unsigned char>(character))) continue;
        if (character == '#') {
            input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        token.push_back(character);
        break;
    }
    if (token.empty()) return false;
    while (input.get(character)) {
        if (std::isspace(static_cast<unsigned char>(character))) break;
        if (character == '#') {
            input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;
        }
        token.push_back(character);
    }
    return true;
}

bool parseUnsigned(const std::string& token, std::uint32_t& value) {
    if (token.empty()) return false;
    const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
    return result.ec == std::errc{} && result.ptr == token.data() + token.size();
}

} // namespace

bool PpmImageLoader::loadFromFile(
    const std::filesystem::path& path, RgbaImage& output, std::string* error) {
    std::ifstream file(path);
    if (!file) {
        setError(error, "Could not open PPM file: " + path.string());
        return false;
    }
    return loadFromStream(file, output, error);
}

bool PpmImageLoader::loadFromStream(
    std::istream& input, RgbaImage& output, std::string* error) {
    std::string token;
    if (!nextToken(input, token) || token != "P3") {
        setError(error, "Only ASCII P3 PPM images are supported");
        return false;
    }

    RgbaImage parsed;
    std::uint32_t maxValue = 0;
    if (!nextToken(input, token) || !parseUnsigned(token, parsed.width) ||
        !nextToken(input, token) || !parseUnsigned(token, parsed.height) ||
        !nextToken(input, token) || !parseUnsigned(token, maxValue) ||
        parsed.width == 0 || parsed.height == 0 || maxValue == 0 || maxValue > 65535U) {
        setError(error, "Invalid PPM dimensions or maximum channel value");
        return false;
    }

    const std::uint64_t pixelCount =
        static_cast<std::uint64_t>(parsed.width) * static_cast<std::uint64_t>(parsed.height);
    if (pixelCount > std::numeric_limits<std::uint32_t>::max() / 4U) {
        setError(error, "PPM image is too large for an SDL_GPU texture upload");
        return false;
    }
    parsed.pixels.resize(static_cast<std::size_t>(pixelCount) * 4U);

    for (std::size_t pixel = 0; pixel < static_cast<std::size_t>(pixelCount); ++pixel) {
        for (std::size_t channel = 0; channel < 3; ++channel) {
            std::uint32_t value = 0;
            if (!nextToken(input, token) || !parseUnsigned(token, value) || value > maxValue) {
                setError(error, "Invalid or missing PPM color channel");
                return false;
            }
            parsed.pixels[pixel * 4U + channel] = static_cast<std::uint8_t>(
                (static_cast<std::uint64_t>(value) * 255U + maxValue / 2U) / maxValue);
        }
        parsed.pixels[pixel * 4U + 3U] = 255U;
    }

    if (nextToken(input, token)) {
        setError(error, "PPM image contains unexpected data after the final pixel");
        return false;
    }
    if (input.bad()) {
        setError(error, "I/O error while reading PPM data");
        return false;
    }

    output = std::move(parsed);
    if (error != nullptr) error->clear();
    return true;
}

} // namespace originalengine
