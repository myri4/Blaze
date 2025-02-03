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
            WC_CORE_INFO("List Saving Successful -> Saved Projects: {0}", savedProjectPaths.size());

            YAMLUtils::SaveFile(std::filesystem::current_path().string() + "/SavedProjects.yaml", data);
        }

        inline void LoadSavedProjects()
        {
            std::string filepath = std::filesystem::current_path().string() + "/SavedProjects.yaml";
            if (!std::filesystem::exists(filepath))
            {
                WC_CORE_ERROR("List Loading Failed -> Save file at does not exist: {}", filepath);
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
                    WC_CORE_WARN("List Loading Failed -> Project path does not exist: {0} ->> Deleting", each.as<std::string>());
                    std::erase(savedProjectPaths, each.as<std::string>());
                    SaveSavedProjects();
                }
            }
        }

        inline void AddProjectToList()
        {
            // add if doesnt exist
            if (std::find(savedProjectPaths.begin(), savedProjectPaths.end(), rootPath) == savedProjectPaths.end())
            {
                savedProjectPaths.push_back(rootPath);
                SaveSavedProjects();
            }
        }

        inline void RemoveProjectFromList(const std::string &filepath)
        {
            std::erase(savedProjectPaths, filepath);
            SaveSavedProjects();
        }

        //name without extension
        inline bool ExistListProject(const std::string &pName)
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
            if (ExistListProject(pName))
            {
                WC_CORE_WARN("Create Failed -> Project with this name already exists: {0}", pName);
                return;
            }

            if (filepath.find(".blz") != std::string::npos)
            {
                WC_CORE_WARN("Create Failed -> Project path cannot have .blz extension: {0}", filepath);
                return;
            }

            name = pName;
            rootPath = filepath + "/" + pName + ".blz";
            AddProjectToList();
            std::filesystem::create_directory(filepath + "/" + pName + ".blz");
            std::filesystem::create_directory(filepath + "/" + pName + ".blz" + "/Scenes");
            std::filesystem::create_directory(filepath + "/" + pName + ".blz" + "/Assets");
            std::filesystem::create_directory(filepath + "/" + pName + ".blz" + "/Assets/Scripts");
            std::filesystem::create_directory(filepath + "/" + pName + ".blz" + "/Assets/Textures");
            std::filesystem::create_directory(filepath + "/" + pName + ".blz" + "/Assets/Audio");
            std::filesystem::create_directory(filepath + "/" + pName + ".blz" + "/Assets/Fonts");
            std::filesystem::create_directory(filepath + "/" + pName + ".blz" + "/Assets/Shaders");
            std::filesystem::create_directory(filepath + "/" + pName + ".blz" + "/Assets/Entities");

            WC_CORE_INFO("Create Successful -> Created project: {0} at {1}", pName, filepath);
        }

        inline bool Load(const std::string& filepath)
        {
            if(!std::filesystem::exists(filepath))
            {
                WC_CORE_ERROR("Load Failed -> {} does not exist.", filepath);
                return false;
            }
            if(!std::filesystem::is_directory(filepath))
            {
                WC_CORE_ERROR("Load Failed -> {} is not a directory.", filepath);
                return false;
            }
            if (std::filesystem::path(filepath).extension().string() != ".blz")
            {
                WC_CORE_ERROR("Load Failed -> {} is not a .blz file.", filepath);
                return false;
            }
            {
                //Everything is fine - open project
                name = std::filesystem::path(filepath).stem().string();
                rootPath = filepath;
                AddProjectToList();
                WC_CORE_INFO("Load Successful -> Opened project: {0} at {1}", name, filepath);
            }


            return true;
        }

        inline void Clear()
        {
            name = "";
            rootPath = "";
            firstScene = "";
        }

        inline void Delete(const std::string& filepath)
        {
            if (std::filesystem::exists(filepath))
            {
                if (std::filesystem::path(filepath).extension().string() == ".blz")
                {
                    RemoveProjectFromList(filepath);
                    Clear();
                    std::filesystem::remove_all(filepath);
                    WC_CORE_INFO("Delete Successful -> Deleted project: {0}", filepath);
                }
                else WC_CORE_WARN("Delete Failed -> Directory is not a .blz project: {0}", filepath);
            }
            else
            {
                WC_CORE_WARN("Delete Failed -> Project path does not exist: {0}", filepath);
            }
        }

        inline bool Exists()
        {
            return !name.empty() && !rootPath.empty();
        }

    };
}