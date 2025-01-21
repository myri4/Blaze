#pragma once

#include <unordered_map>
#include <vector>

#include <../../vendor/wc/Texture.h>
#include <../../vendor/wc/Utils/Image.h>
#include "Font.h"

namespace wc
{
    struct AssetManager
    {
    private:
        std::unordered_map<std::string, uint32_t> m_TextureCache;

        std::vector<Texture> m_Textures;
    public:

        auto& GetTextures() { return m_Textures; }
        const auto& GetTextures() const { return m_Textures; }

        void Init()
        {
			Texture texture;
			uint32_t white = 0xFFFFFFFF;
			texture.Load(&white, 1, 1);
			m_Textures.push_back(texture);
        }

        void Free()
        {
			for (auto& texture : m_Textures)
                texture.Destroy();
        }


        uint32_t PushTexture(const Texture& texture, const std::string& name)
        {
            if (m_TextureCache.find(name) != m_TextureCache.end())
                return m_TextureCache[name];

            m_Textures.push_back(texture);

            m_TextureCache[name] = uint32_t(m_Textures.size() - 1);
            return uint32_t(m_Textures.size() - 1);
        }

        uint32_t LoadTexture(const std::string& file)
        {
            if (m_TextureCache.find(file) != m_TextureCache.end())
                return m_TextureCache[file];

            if (std::filesystem::exists(file))
            {
                Texture texture;
                texture.Load(file);

                return PushTexture(texture, file);
            }

            m_TextureCache[file] = 0;
            WC_CORE_ERROR("Cannot find file at location: {}", file);
            return 0;
        }

        uint32_t LoadTextureFromMemory(const Image& image, const std::string& name)
        {
            if (m_TextureCache.find(name) != m_TextureCache.end())
                return m_TextureCache[name];

            Texture texture;
            texture.Load(image.Data, image.Width, image.Height);

            return PushTexture(texture, name);
        }

		uint32_t AllocateTexture(const TextureSpecification& specification)
		{
			Texture texture;
			texture.Allocate(specification);
			m_Textures.push_back(texture);

			return uint32_t(m_Textures.size() - 1);
		}
    };
}
