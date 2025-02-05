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
        std::unordered_map<std::string, uint32_t> TextureCache;

        std::vector<Texture> Textures;
        bool TexturesUpdated = false; // This is set to true when a new texture is loaded. It's used to signal when we need to resize the descriptor set

        void Init()
        {
			Texture texture;
			uint32_t white = 0xFFFFFFFF;
			texture.Load(&white, 1, 1);
            PushTexture(texture);
        }

        void Free()
        {
			for (auto& texture : Textures)
                texture.Destroy();
        }

		uint32_t PushTexture(const Texture& texture)
		{
			Textures.push_back(texture);
            TexturesUpdated = true;

			return uint32_t(Textures.size() - 1);
		}

        uint32_t PushTexture(const Texture& texture, const std::string& name)
        {
            if (TextureCache.find(name) != TextureCache.end())
                return TextureCache[name];

            auto texID = PushTexture(texture);

            TextureCache[name] = texID;
            return texID;
        }

        uint32_t LoadTexture(const std::string& file)
        {
            if (TextureCache.find(file) != TextureCache.end())
                return TextureCache[file];

            if (std::filesystem::exists(file))
            {
                Texture texture;
                texture.Load(file);

                return PushTexture(texture, file);
            }

            TextureCache[file] = 0;
            WC_CORE_ERROR("Cannot find file at location: {}", file);
            return 0;
        }

        uint32_t LoadTextureFromMemory(const Image& image, const std::string& name)
        {
            if (TextureCache.find(name) != TextureCache.end())
                return TextureCache[name];

            Texture texture;
            texture.Load(image.Data, image.Width, image.Height);

            return PushTexture(texture, name);
        }

		uint32_t AllocateTexture(const TextureSpecification& specification)
		{
			Texture texture;
			texture.Allocate(specification);

			return PushTexture(texture);
		}
    };
}
