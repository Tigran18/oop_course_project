#pragma once
#include <string>
#include <iostream>

namespace utils {
    inline void renderColorPreview(const std::vector<unsigned char>& rgba,
                                unsigned w, unsigned h, unsigned maxWidth = 120)
    {
        if (w == 0 || h == 0 || rgba.empty())
            return;

        // Scale if image is too wide
        float scale = 1.0f;
        if (w > maxWidth)
            scale = float(maxWidth) / float(w);

        unsigned newW = unsigned(w * scale);
        unsigned newH = unsigned(h * scale);

        // We render two image rows into one console row
        unsigned outH = newH / 2;

        auto sample = [&](unsigned x, unsigned y, unsigned c) {
            unsigned sx = std::min(unsigned(x / scale), w - 1);
            unsigned sy = std::min(unsigned(y / scale), h - 1);
            return rgba[4 * (sy * w + sx) + c];
        };

        for (unsigned y = 0; y < outH; ++y) {
            for (unsigned x = 0; x < newW; ++x) {

                // upper pixel
                unsigned r1 = sample(x, y * 2, 0);
                unsigned g1 = sample(x, y * 2, 1);
                unsigned b1 = sample(x, y * 2, 2);

                // lower pixel
                unsigned r2 = sample(x, y * 2 + 1, 0);
                unsigned g2 = sample(x, y * 2 + 1, 1);
                unsigned b2 = sample(x, y * 2 + 1, 2);

                // ANSI full color:
                // background = upper pixel
                // foreground = lower pixel using the "lower half block" character ▄
                std::cout
                    << "\x1b[48;2;" << r1 << ";" << g1 << ";" << b1 << "m"
                    << "\x1b[38;2;" << r2 << ";" << g2 << ";" << b2 << "m"
                    << "▄";
            }
            std::cout << "\x1b[0m\n"; // reset ANSI + newline
        }
    }

    inline bool endsWith(const std::string& str, const std::string& suffix) {
        if (suffix.size() > str.size()) return false;
        return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    inline std::string asciiFromImage(const std::vector<unsigned char>& rgba,
                           unsigned w, unsigned h)
{
    static const char* gradient = " .:-=+*#%@";
    const int gradSize = 10;

    std::string out;
    out.reserve((w + 1) * h);

    for (unsigned y = 0; y < h; ++y) {
        for (unsigned x = 0; x < w; ++x) {
            unsigned idx = 4 * (y * w + x);
            unsigned r = rgba[idx + 0];
            unsigned g = rgba[idx + 1];
            unsigned b = rgba[idx + 2];

            unsigned lum = (r + g + b) / 3;
            int gi = (lum * (gradSize - 1)) / 255;
            out += gradient[gi];
        }
        out += '\n';
    }

    return out;
}

}