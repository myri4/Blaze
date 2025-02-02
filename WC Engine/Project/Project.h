#pragma once

#include <filesystem>
#include "../UI/Widgets.h"
#include "imgui/imgui.h"
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

        inline void SaveSavedProjects()
        {
            YAML::Node data;
            data["savedProjectPaths"] = savedProjectPaths;

            YAMLUtils::SaveFile(std::filesystem::current_path().string() + "/SavedProjects.yaml", data);
        }

        inline void LoadSavedProjects()
        {
            std::string filepath = std::filesystem::current_path().string() + "/SavedProjects.yaml";
            if (!std::filesystem::exists(filepath))
            {
                WC_CORE_ERROR("{} does not exist.", filepath);
                return;
            }

            YAML::Node data = YAML::LoadFile(filepath);
            for (auto each : data["savedProjectPaths"])
            {
                if (std::filesystem::exists(each.as<std::string>()))
                {
                    savedProjectPaths.push_back(each.as<std::string>());
                }
                else
                {
                    WC_CORE_WARN("Project path does not exist: {0} ->> Deleting", each.as<std::string>());
                    std::erase(savedProjectPaths, each.as<std::string>());
                    SaveSavedProjects();
                }
            }
        }

        inline void SaveListProj()
        {
            // add if doesnt exist
            if (std::find(savedProjectPaths.begin(), savedProjectPaths.end(), rootPath) == savedProjectPaths.end())
            {
                savedProjectPaths.push_back(rootPath + "/" + name + ".blz");
                SaveSavedProjects();
            }
        }

        inline void RemoveListProj()
        {
            std::erase(savedProjectPaths, rootPath + "/" + name + ".blz");
            SaveSavedProjects();
        }

        //name without extension
        inline bool ExistListProj(const std::string &pName)
        {
            for (auto& each : savedProjectPaths)
            {
                if (std::filesystem::path(each).filename().string() == pName + ".blz")
                {
                    return true;
                }
            }
            return false;
        }

        inline void Create(const std::string& filepath, const std::string& pName)
        {
            if (ExistListProj(pName))
            {
                WC_CORE_WARN("Project with this name already exists: {0}", pName);
                return;
            }

            if (std::filesystem::path(filepath).extension().string() == ".blz")
            {
                WC_CORE_WARN("Project path cannot have .blz extension: {0}", filepath);
                return;
            }

            name = pName;
            rootPath = filepath;
            SaveListProj();
            std::filesystem::create_directory(filepath + "/" + pName + ".blz");
            std::filesystem::create_directory(filepath + "/" + pName + ".blz" + "/Scenes");
            std::filesystem::create_directory(filepath + "/" + pName + ".blz" + "/Assets");
            std::filesystem::create_directory(filepath + "/" + pName + ".blz" + "/Assets/Scripts");
            std::filesystem::create_directory(filepath + "/" + pName + ".blz" + "/Assets/Textures");
            std::filesystem::create_directory(filepath + "/" + pName + ".blz" + "/Assets/Audio");
            std::filesystem::create_directory(filepath + "/" + pName + ".blz" + "/Assets/Fonts");
            std::filesystem::create_directory(filepath + "/" + pName + ".blz" + "/Assets/Shaders");
            std::filesystem::create_directory(filepath + "/" + pName + ".blz" + "/Assets/Entities");

            WC_CORE_INFO("Created project: {0} at {1}", pName, filepath);
        }

        inline bool Load(const std::string& filepath)
        {
            if(!std::filesystem::exists(filepath))
            {
                WC_CORE_ERROR("{} does not exist.", filepath);
                return false;
            }
            else if(!std::filesystem::is_directory(filepath))
            {
                WC_CORE_ERROR("{} is not a directory.", filepath);
                return false;
            }
            else if (filepath.find(".blz") == std::string::npos)
            {
                WC_CORE_ERROR("{} is not a .BLproj file.", filepath);
                return false;
            }
            else
            {
                //Everything is fine - open project
                name = std::filesystem::path(filepath).stem().string();
                rootPath = filepath;
                SaveListProj();
                WC_CORE_INFO("Opened project: {0} at {1}", name, filepath);
            }


            return true;
        }

        inline void Delete(const std::string& filepath)
        {
            if (std::filesystem::exists(filepath))
            {
                std::filesystem::remove_all(filepath);
                std::erase(savedProjectPaths, filepath);
                WC_CORE_INFO("Deleted project: {0}", filepath);
            }
            else
            {
                WC_CORE_WARN("Project path does not exist: {0}", filepath);
            }
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