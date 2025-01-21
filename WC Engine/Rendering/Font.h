#pragma once

#undef INFINITE
#include <msdf-atlas-gen/msdfgen/msdfgen.h>
#include <msdf-atlas-gen/msdfgen/msdfgen-ext.h>
#include <msdf-atlas-gen/msdf-atlas-gen/msdf-atlas-gen.h>
#include <msdf-atlas-gen/msdf-atlas-gen/GlyphGeometry.h>
#define INFINITE 0xFFFFFFFF

#include <vector>
#include <glm/glm.hpp>
#include <wc/Texture.h>

namespace wc
{
    struct AssetManager;

    struct Font
    {
        void Load(const std::string filepath, AssetManager& assetManager);
        glm::vec2 CalculateTextSize(const std::string& text);

        float Kerning = 0.f;
        float LineSpacing = 0.f;
        uint32_t TextureID = 0;
        Texture Tex;
        msdf_atlas::FontGeometry FontGeometry;
    private:
        std::vector<msdf_atlas::GlyphGeometry> m_Glyphs;
    };
}
