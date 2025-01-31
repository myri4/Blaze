#pragma once

#include <filesystem>
#include "../UI/Widgets.h"
#include "imgui/imgui.h"
#include <wc/Utils/ImGuiFileDialogue.h>
#include "imgui/imgui_internal.h"
#include <wc/Utils/YAML.h>

namespace wc
{
    namespace Project
    {
        std::string name = "";
        std::string rootPath = "";
        std::string firstScene = "";
        std::vector<std::string> savedProjectPaths; // for manager
        std::vector<std::string> savedScenes; // for manager

        inline void Create(const std::string& filepath, const std::string& pName)
        {
            name = pName;
            rootPath = filepath;
            savedProjectPaths.push_back(filepath);
            std::filesystem::create_directory(filepath + "/" + pName);
            std::filesystem::create_directory(filepath + "/" + pName + "/Scenes");
            std::filesystem::create_directory(filepath + "/" + pName + "/Assets");
            std::filesystem::create_directory(filepath + "/" + pName + "/Assets/Scripts");
            std::filesystem::create_directory(filepath + "/" + pName + "/Assets/Textures");
            std::filesystem::create_directory(filepath + "/" + pName + "/Assets/Audio");
            std::filesystem::create_directory(filepath + "/" + pName + "/Assets/Fonts");
            std::filesystem::create_directory(filepath + "/" + pName + "/Assets/Shaders");
            std::filesystem::create_directory(filepath + "/" + pName + "/Assets/Entities");

            WC_CORE_INFO("Created project: {0} at {1}", pName, filepath);
        }

        inline bool Open(const std::string& filepath)
        {
            bool exists = true;

            auto checkDirectory = [&exists](const std::string& path)
            {
                if (!std::filesystem::exists(path))
                {
                    WC_CORE_ERROR("{} does not exist.", path);
                    exists = false;
                }
            };

            checkDirectory(filepath);
            checkDirectory(filepath + "/Scenes");
            checkDirectory(filepath + "/Assets");
            checkDirectory(filepath + "/Assets/Scripts");
            checkDirectory(filepath + "/Assets/Textures");
            checkDirectory(filepath + "/Assets/Audio");
            checkDirectory(filepath + "/Assets/Fonts");
            checkDirectory(filepath + "/Assets/Shaders");
            checkDirectory(filepath + "/Assets/Entities");

            if (!exists) return false;

            name = filepath.substr(filepath.find_last_of("/\\") + 1);
            rootPath = filepath;


            return exists;
        }

        inline void Clear()
        {
            name = "";
            rootPath = "";
            firstScene = "";
        }

        inline bool Exists()
        {
            return !name.empty() && !rootPath.empty();
        }

    };
}