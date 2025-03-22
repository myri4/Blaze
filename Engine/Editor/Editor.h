#pragma once

#include <fstream>

#include <glm/gtc/type_ptr.hpp>

#include "../Utils/List.h"
#include "../Utils/FileDialogs.h"

#include "../Rendering/Renderer2D.h"

#include "EditorScene.h"

#include "../Globals.h"

#include "../Sound/SoundEngine.h"

using namespace Editor;
namespace gui = ImGui;

#define PROJECT_SETTINGS_EXTENSION ".blzproj"
#define PROJECT_USER_SETTINGS_EXTENSION ".blzprojuser"

struct EditorInstance
{
	// Debug stats
	float m_DebugTimer = 0.f;

	uint32_t m_PrevMaxFPS = 0;
	uint32_t m_MaxFPS = 0;

	uint32_t m_PrevMinFPS = 0;
	uint32_t m_MinFPS = 0;

	uint32_t m_FrameCount = 0;
	uint32_t m_FrameCounter = 0;
	uint32_t m_PrevFrameCounter = 0;

	glm::vec2 WindowPos;
	glm::vec2 RenderSize;

	glm::vec2 ViewPortSize = glm::vec2(1.f);

	RenderData m_RenderData[FRAME_OVERLAP];
	Renderer2D m_Renderer;

	b2DebugDraw m_PhysicsDebugDraw;

    // Window Buttons
	Texture t_Close;
	Texture t_Minimize;
    Texture t_Maximize;
	Texture t_Collapse;

    // Assets
	Texture t_FolderOpen;
	Texture t_FolderClosed;
	Texture t_File;

    // Scene Editor Buttons
	Texture t_Play;
	Texture t_Simulate;
	Texture t_Stop;

    // Entities
    Texture t_Eye;
    Texture t_EyeClosed;

    // Console
    Texture t_Debug;
    Texture t_Info;
    Texture t_Warning;
    Texture t_Error;
    Texture t_Critical;


	Audio::SoundContext context;


	bool allowInput = true;

	bool showEditor = true;
	bool showSceneProperties = true;
	bool showEntities = true;
	bool showProperties = true;
	bool showConsole = true;
	bool showAssets = true;
	bool showDebugStats = false;
	bool showStyleEditor = false;

	EditorScene m_Scene;

	void Create();

	void Resize(glm::vec2 size);

	void Destroy();

	void Input();

	void RenderEntity(flecs::entity entt, glm::mat4& transform);

	void Render();

	void Update();

	void UI_Editor();

	void UI_SceneProperties();

	void EntityRightClickMenu(const flecs::entity& entity);

	void EntityReorderSeparator(const flecs::entity& entity);

	void DisplayEntity(const flecs::entity& entity);

	void UI_Entities();

	template<typename T, typename UIFunc>
	void EditComponent(const std::string& name, UIFunc uiFunc);

	void UI_Properties();

	void UI_Console();

	void UI_Assets();

	void UI_DebugStats();

	void UI_StyleEditor(ImGuiStyle* ref = nullptr);

	void WindowButtons();

	void UI();

	// [PROJECT MANAGING]
	std::string ProjectName;
	std::string ProjectRootPath;
	std::string ProjectFirstScene;

	std::string texturePath;
	std::string fontPath;
	std::string soundPath;

	std::string scenesPath;
	std::string scriptsPath;
	std::string entitiesPath;

	std::vector<std::string> savedProjectPaths; // @NOTE: This could just be std::set
	std::vector<std::string> savedProjectScenes;

	bool AutoCleanUpProjectOnExit = true; // If this flag is set, when exiting the application all materials and entities that are not used in any scenes will be deleted.

	void AddSceneToList(const std::string& path);

	void RemoveSceneFromList(const std::string& filepath);

	bool SceneExistInList(const std::string& path);

	void AddProjectToList(const std::string& path);

	void RemoveProjectFromList(const std::string& filepath);

	bool ProjectExistInList(const std::string& pName);

	void ResetProject();

	bool IsProject(const std::string& filepath);
	std::string GetProjectSettingsPath();
	std::string GetProjectUserSettingsPath();

	void SaveProjectData();

	void SaveProject();

	bool LoadProject(const std::string& filepath);

	void CreateProject(const std::string& filepath, const std::string& pName);

	void DeleteProject(const std::string& filepath);

	void RenameProject(const std::string& newName);

	bool ProjectExists();
	
	// [SETTINGS MANAGING]

	float ZoomSpeed = 1.f;

	void SaveSettings();
	
	void LoadSettings();
};
