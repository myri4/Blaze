#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
//#include <Windows.h> // Needed for file attributes

namespace wc
{
	namespace UI
	{
	    const std::unordered_map<std::string, std::string> fileTypeExt = {{".*", "(All)"}, {".", "(Folder)"}, {".png", "(Image)"}, {".wav", "(Audio)"}};

        // FileDialog - A simple file dialog for opening files or selecting directories
	    // filters: [.* - All] [. - Folder] [.png - Image] [.wav - Audio]
        std::string FileDialog(const char* name, const std::string &filter = ".*", const bool selectDir = false)
        {
            bool showPopup = true;
            static std::filesystem::path currentPath = std::filesystem::current_path().root_path();
            static std::vector<std::filesystem::path> disks;
            static std::string selectedPath;
            std::string finalPath;
            static std::vector<std::filesystem::directory_entry> fileEntries;

            // Initialize disks (only once)
            if (disks.empty())
            {
                for (char drive = 'A'; drive <= 'Z'; ++drive)
                {
                    std::filesystem::path drive_path = std::string(1, drive) + ":\\";
                    std::error_code ec;
                    if (std::filesystem::exists(drive_path, ec))
                    {
                        disks.push_back(drive_path);
                    }
                }
            }

            if (ImGui::BeginPopupModal(name, &showPopup))
            {
                // Navigation controls
                ImGui::BeginDisabled(currentPath == currentPath.root_path());
                if (ImGui::ArrowButton("##back", ImGuiDir_Left))
                {
                    if (currentPath.has_parent_path())
                        currentPath = currentPath.parent_path();
                    else
                        currentPath = currentPath.root_path();
                    fileEntries.clear();
                }
                ImGui::EndDisabled();

                ImGui::SameLine();
                float comboWidth = std::max(ImGui::CalcTextSize(currentPath.string().c_str()).x + 50, 300.0f);
                ImGui::SetWindowSize(ImVec2(ImGui::GetItemRectSize().x + comboWidth + ImGui::CalcTextSize("Refresh").x + ImGui::GetStyle().FramePadding.x * 4 + ImGui::GetStyle().ItemSpacing.x * 3, 0));
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x - ImGui::CalcTextSize("Refresh").x - ImGui::GetStyle().FramePadding.x * 2);
                if (ImGui::BeginCombo("##disks", currentPath.string().c_str()))
                {
                    for (const auto& disk : disks)
                    {
                        if (ImGui::Selectable(disk.string().c_str()))
                        {
                            currentPath = disk;
                            fileEntries.clear();
                        }
                    }
                    ImGui::EndCombo();
                }

                // Refresh button
                ImGui::SameLine();
                if (ImGui::Button("Refresh"))
                {
                    fileEntries.clear();
                    std::error_code ec;
                    std::filesystem::directory_iterator(currentPath, ec); // Force refresh
                }

                // File list
                if (ImGui::BeginChild("##file_list", ImVec2(0,  300.0f), true))
                {
                    try
                    {
                        // Refresh directory contents if empty
                        if (fileEntries.empty())
                        {
                            std::error_code ec;
                            auto dir_iter = std::filesystem::directory_iterator(currentPath, ec);
                            if (ec)
                            {
                                ImGui::TextColored(ImVec4(1,0,0,1), "Error: %s", ec.message().c_str());
                            }
                            else
                            {
                                for (const auto& entry : dir_iter)
                                {
                                    try
                                    {
                                        //TODO - add for other systems

                                        // Skip hidden/system files using attributes
                                        #ifdef _WIN32
                                        DWORD attrs = GetFileAttributesW(entry.path().wstring().c_str());
                                        if (attrs & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))
                                            continue;
                                        #endif

                                        // Skip reserved system files
                                        if (entry.path().filename().string()[0] == '$')
                                            continue;

                                        fileEntries.push_back(entry);
                                    }
                                    catch (...)
                                    {
                                        // Skip problematic entries
                                        continue;
                                    }
                                }

                                // Sort directories first (case-insensitive)
                                std::sort(fileEntries.begin(), fileEntries.end(),
                                    [](const auto& a, const auto& b) {
                                        std::string a_name = a.path().filename().string();
                                        std::string b_name = b.path().filename().string();
                                        std::transform(a_name.begin(), a_name.end(), a_name.begin(), ::tolower);
                                        std::transform(b_name.begin(), b_name.end(), b_name.begin(), ::tolower);

                                        if (a.is_directory() == b.is_directory())
                                            return a_name < b_name;
                                        return a.is_directory() > b.is_directory();
                                    });
                            }
                        }

                        // Display entries
                        for (const auto& entry : fileEntries)
                        {
                            const bool isDirectory = entry.is_directory();
                            std::string filename = entry.path().filename().string();

                            // File type filtering (case-insensitive)
                            if (!isDirectory && !filter.empty() && filter != ".*")
                            {
                                std::string target_ext = filter;
                                std::transform(target_ext.begin(), target_ext.end(), target_ext.begin(), ::tolower);

                                std::string entry_ext = entry.path().extension().string();
                                std::transform(entry_ext.begin(), entry_ext.end(), entry_ext.begin(), ::tolower);

                                if (entry_ext != target_ext)
                                    continue;
                            }

                            // Display as tree node
                            ImGui::PushID(filename.c_str());

                            // Set tree node flags
                            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth |
                                                      ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                                      ImGuiTreeNodeFlags_OpenOnArrow;

                            if (!isDirectory) {
                                flags |= ImGuiTreeNodeFlags_Leaf;
                            }

                            if (selectedPath == entry.path().string())
                            {
                                flags |= ImGuiTreeNodeFlags_Selected;
                            }

                            // Display tree node (returns true if clicked, but doesn't actually open)
                            bool nodeClicked = ImGui::TreeNodeEx("##node", flags, "%s", filename.c_str());

                            // Handle single click
                            if (ImGui::IsItemClicked())
                            {
                                if (isDirectory)
                                {
                                    if (selectDir)
                                    {
                                        selectedPath = entry.path().string();
                                    }
                                    else
                                    {
                                        currentPath = entry.path();
                                        fileEntries.clear();
                                    }
                                }
                                else
                                {
                                    selectedPath = entry.path().string();
                                }
                            }

                            // Handle double-click
                            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                            {
                                if (isDirectory)
                                {
                                    currentPath = entry.path();
                                    fileEntries.clear();
                                }
                                else
                                {
                                    selectedPath = entry.path().string();
                                }
                            }

                            ImGui::PopID();
                        }
                    }
                    catch (const std::exception& e)
                    {
                        ImGui::TextColored(ImVec4(1,0,0,1), "Error: %s", e.what());
                    }
                }
                ImGui::EndChild();

                // Selected file path
                ImGui::SetNextItemWidth(std::max(ImGui::CalcTextSize(selectedPath.c_str()).x + ImGui::GetStyle().FramePadding.x * 2, 300.0f));
                ImGui::Text("Selected:"); ImGui::SameLine();
                auto selectedFileName = selectedPath.empty() ? "* None *" : std::filesystem::path(selectedPath).filename().string();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(filter.c_str()).x - ImGui::CalcTextSize(fileTypeExt.at(filter).c_str()).x - ImGui::GetStyle().FramePadding.x * 2 - ImGui::GetStyle().ItemSpacing.x);
                ImGui::InputText("##SelectedFile", &selectedFileName, ImGuiInputTextFlags_ReadOnly);

                 // Filter text
                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::CalcTextSize(filter.c_str()).x + ImGui::CalcTextSize(fileTypeExt.at(filter).c_str()).x + ImGui::GetStyle().FramePadding.x * 2);
                auto filterText = filter + " " + fileTypeExt.at(filter);
                ImGui::InputText("##Filter", &filterText, ImGuiInputTextFlags_ReadOnly);

                // Action buttons
                ImGui::BeginDisabled(selectedPath.empty());
                if (ImGui::Button("OK") || ImGui::IsKeyPressed(ImGuiKey_Enter) && !selectedPath.empty())
                {
                    finalPath = selectedPath;
                    selectedPath.clear();
                    currentPath = disks[0];
                    fileEntries.clear();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndDisabled();

                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - ImGui::CalcTextSize("Cancel").x - ImGui::GetStyle().FramePadding.x * 2);
                if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape))
                {
                    selectedPath.clear();
                    currentPath = disks[0];
                    fileEntries.clear();
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            return finalPath;
        }

	    void ApplyHue(ImGuiStyle& style, float hue)
	    {
	        for (int i = 0; i < ImGuiCol_COUNT; i++)
	        {
	            ImVec4& col = style.Colors[i];

	            float h, s, v;
	            ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, h, s, v);
	            h = hue;
	            ImGui::ColorConvertHSVtoRGB(h, s, v, col.x, col.y, col.z);
	        }
	    }

		ImVec2 ImConv(glm::vec2 v) { return ImVec2(v.x, v.y); }

		ImGuiStyle SoDark(float hue)
		{
			ImGuiStyle style;
			ImVec4* colors = style.Colors;
			colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
			colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
			colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
			colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
			colors[ImGuiCol_Border] = ImVec4(0.33f, 0.33f, 0.33f, 0.59f);
			colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
			colors[ImGuiCol_FrameBg] = ImVec4(0.25f, 0.25f, 0.25f, 0.54f);
			colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
			colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
			colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
			colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
			colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
			colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
			colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
			colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
			colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
			colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
			colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
			colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
			colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
			colors[ImGuiCol_Button] = ImVec4(0.30f, 0.30f, 0.30f, 0.54f);
			colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
			colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
			colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
			colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
			colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
            colors[ImGuiCol_Separator] = ImVec4(0.33f, 0.33f, 0.33f, 0.59f);
			colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
			colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
			colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
			colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
			colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 0.00f, 0.00f, 0.50f);
			colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
			colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
			colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
			colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
			colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
            colors[ImGuiCol_DockingPreview] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
			colors[ImGuiCol_DockingPreview] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
			colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
			colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
			colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
			colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
			colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
			colors[ImGuiCol_TableHeaderBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
			colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
			colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
			colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
			colors[ImGuiCol_TextSelectedBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.27f);
			colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
			colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
			colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
			colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
			colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.22f, 0.22f, 0.22f, 0.373f);

			style.WindowPadding = ImVec2(8.00f, 8.00f);
			style.FramePadding = ImVec2(5.00f, 2.00f);
			style.CellPadding = ImVec2(6.00f, 6.00f);
			style.ItemSpacing = ImVec2(6.00f, 6.00f);
			style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
			style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
			style.IndentSpacing = 25;
			style.ScrollbarSize = 15;
			style.GrabMinSize = 10;
			style.WindowBorderSize = 1;
			style.ChildBorderSize = 1;
			style.PopupBorderSize = 1;
			style.FrameBorderSize = 0;
			style.TabBorderSize = 1;
			style.WindowRounding = 7;
			style.ChildRounding = 4;
			style.FrameRounding = 2;
			style.PopupRounding = 4;
			style.ScrollbarRounding = 9;
			style.GrabRounding = 3;
			style.LogSliderDeadzone = 4;
			style.TabRounding = 4;
			style.WindowMenuButtonPosition = ImGuiDir_None;

			ApplyHue(style, hue);

			return style;
		}

		static void RenderArrowIcon()
		{
			ImVec2 arrowPos = ImVec2(
				ImGui::GetWindowPos().x + ImGui::GetWindowSize().x - ImGui::GetStyle().ItemSpacing.x * 2 - ImGui::CalcTextSize(">").x,
				ImGui::GetCursorScreenPos().y - ImGui::GetTextLineHeight() - ImGui::GetStyle().ItemSpacing.y // Adjust to align with text height
			);

			ImGui::RenderArrow(
				ImGui::GetWindowDrawList(),
				arrowPos,
				ImGui::GetColorU32(ImGuiCol_Text),
				ImGuiDir_Right
			);
		}

		static void HelpMarker(const char* desc, bool sameLine = true)
		{
			if (sameLine) ImGui::SameLine();
			ImGui::TextDisabled("(?)");
			if (ImGui::BeginItemTooltip())
			{
				ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
				ImGui::TextUnformatted(desc);
				ImGui::PopTextWrapPos();
				ImGui::EndTooltip();
			}
		}

		void DragButton2(const char* txt, glm::vec2& v) // make this a bool func
		{
			// Define a fixed width for the buttons
			const float buttonWidth = 20.0f;

			// Calculate the available width for the input fields
			float inputWidth = (ImGui::GetContentRegionAvail().x - 2 - ImGui::GetStyle().ItemSpacing.x - buttonWidth * 2) * 0.65f / 2;

			// Draw colored button for "X"
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 0.5f)); // Red color
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.6f)); // Red color
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.0f, 0.0f, 0.75f)); // Red color
			if (ImGui::Button((std::string("X##X") + txt).c_str(), ImVec2(buttonWidth, 0)))
			{
				if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) { v.x -= 1.0f; }
				else { v.x += 1.0f; }
			}
			ImGui::PopStyleColor(3);
			ImGui::SameLine(0, 0);

			ImGui::SetNextItemWidth(inputWidth);
			ImGui::DragFloat((std::string("##X") + txt).c_str(), &v.x, 0.1f);

			ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);

			// Draw colored button for "Y"
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.5f)); // Green color
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.6f)); // Green color
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.75f)); // Green color
			if (ImGui::Button((std::string("Y##Y") + txt).c_str(), ImVec2(buttonWidth, 0)))
			{
				if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) { v.y -= 1.0f; }
				else { v.y += 1.0f; }
			}
			ImGui::PopStyleColor(3);
			ImGui::SameLine(0, 0);

			ImGui::SetNextItemWidth(inputWidth);
			ImGui::DragFloat((std::string("##Y") + txt).c_str(), &v.y, 0.1f);


			ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
			ImGui::AlignTextToFramePadding();
			ImGui::Text(txt);

			//HelpMarker("Pressing SHIFT makes the step for the buttons -1.0, instead of 1.0");
		}

		void Separator()
		{
			ImGui::Separator();
		}

		void Separator(const std::string& label)
		{
			ImGui::SeparatorText(label.c_str());
		}

		void Text(const std::string& text)
		{
			ImGui::TextUnformatted(text.c_str());
		}

		void HelpMarker(const std::string& desc)
		{
			ImGui::TextDisabled("(?)");
			if (ImGui::BeginItemTooltip())
			{
				ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
				Text(desc);
				ImGui::PopTextWrapPos();
				ImGui::EndTooltip();
			}
		}

		bool Drag(const std::string& label, float& v, float v_speed = 1.f, float v_min = 0.f, float v_max = 0.f, const char* format = "%.3f", ImGuiSliderFlags flags = 0)
		{
			return ImGui::DragScalar(label.c_str(), ImGuiDataType_Float, &v, v_speed, &v_min, &v_max, format, flags);
		}

		bool Drag2(const std::string& label, float* v, float v_speed = 1.f, float v_min = 0.f, float v_max = 0.f, const char* format = "%.3f", ImGuiSliderFlags flags = 0)
		{
			return ImGui::DragScalarN(label.c_str(), ImGuiDataType_Float, v, 2, v_speed, &v_min, &v_max, format, flags);
		}

		bool Drag3(const std::string& label, float* v, float v_speed = 1.f, float v_min = 0.f, float v_max = 0.f, const char* format = "%.3f", ImGuiSliderFlags flags = 0)
		{
			return ImGui::DragScalarN(label.c_str(), ImGuiDataType_Float, v, 3, v_speed, &v_min, &v_max, format, flags);
		}

		bool Drag4(const std::string& label, float* v, float v_speed = 1.f, float v_min = 0.f, float v_max = 0.f, const char* format = "%.3f", ImGuiSliderFlags flags = 0)
		{
			return ImGui::DragScalarN(label.c_str(), ImGuiDataType_Float, v, 4, v_speed, &v_min, &v_max, format, flags);
		}

		//bool DragFloatRange2(const char* label, float* v_current_min, float* v_current_max, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* format = "%.3f", const char* format_max = NULL, ImGuiSliderFlags flags = 0);
		bool Drag(const std::string& label, int& v, float v_speed = 1.f, int v_min = 0, int v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0)
		{
			return ImGui::DragScalar(label.c_str(), ImGuiDataType_S32, &v, v_speed, &v_min, &v_max, format, flags);
		}

		bool Drag2(const std::string& label, int* v, float v_speed = 1.f, int v_min = 0, int v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0)
		{
			return ImGui::DragScalarN(label.c_str(), ImGuiDataType_S32, v, 2, v_speed, &v_min, &v_max, format, flags);
		}

		bool Drag3(const std::string& label, int* v, float v_speed = 1.f, int v_min = 0, int v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0)
		{
			return ImGui::DragScalarN(label.c_str(), ImGuiDataType_S32, v, 3, v_speed, &v_min, &v_max, format, flags);
		}
		bool Drag4(const std::string& label, int* v, float v_speed = 1.f, int v_min = 0, int v_max = 0, const char* format = "%d", ImGuiSliderFlags flags = 0)
		{
			return ImGui::DragScalarN(label.c_str(), ImGuiDataType_S32, v, 4, v_speed, &v_min, &v_max, format, flags);
		}


		bool Drag(const std::string& label, uint32_t& v, float v_speed = 1.f, uint32_t v_min = 0, uint32_t v_max = 0, const char* format = "%u", ImGuiSliderFlags flags = 0)
		{
			return ImGui::DragScalar(label.c_str(), ImGuiDataType_U32, &v, v_speed, &v_min, &v_max, format, flags);
		}
		bool Drag2(const std::string& label, uint32_t* v, float v_speed = 1.f, uint32_t v_min = 0, uint32_t v_max = 0, const char* format = "%u", ImGuiSliderFlags flags = 0)
		{
			return ImGui::DragScalarN(label.c_str(), ImGuiDataType_U32, v, 2, v_speed, &v_min, &v_max, format, flags);
		}
		bool Drag3(const std::string& label, uint32_t* v, float v_speed = 1.f, uint32_t v_min = 0, uint32_t v_max = 0, const char* format = "%u", ImGuiSliderFlags flags = 0)
		{
			return ImGui::DragScalarN(label.c_str(), ImGuiDataType_U32, v, 3, v_speed, &v_min, &v_max, format, flags);
		}
		bool Drag4(const std::string& label, uint32_t* v, float v_speed = 1.f, uint32_t v_min = 0, uint32_t v_max = 0, const char* format = "%u", ImGuiSliderFlags flags = 0)
		{
			return ImGui::DragScalarN(label.c_str(), ImGuiDataType_U32, v, 4, v_speed, &v_min, &v_max, format, flags);
		}


		bool Input(const std::string& label, float& v, float step = 0.f, float step_fast = 0.f, const char* format = "%.3f", ImGuiInputTextFlags flags = 0)
		{
			return ImGui::InputScalar(label.c_str(), ImGuiDataType_Float, (void*)&v, (void*)(step > 0.0f ? &step : NULL), (void*)(step_fast > 0.0f ? &step_fast : NULL), format, flags);
		}

		bool Input2(const std::string& label, float* v, const char* format = "%.3f", ImGuiInputTextFlags flags = 0)
		{
			return ImGui::InputScalarN(label.c_str(), ImGuiDataType_Float, v, 2, NULL, NULL, format, flags);
		}

		bool Input3(const std::string& label, float* v, const char* format = "%.3f", ImGuiInputTextFlags flags = 0)
		{
			return ImGui::InputScalarN(label.c_str(), ImGuiDataType_Float, v, 3, NULL, NULL, format, flags);
		}

		bool Input4(const std::string& label, float* v, const char* format = "%.3f", ImGuiInputTextFlags flags = 0)
		{
			return ImGui::InputScalarN(label.c_str(), ImGuiDataType_Float, v, 4, NULL, NULL, format, flags);
		}

		bool Input(const std::string& label, int& v, int step = 1, int step_fast = 100, ImGuiInputTextFlags flags = 0)
		{
			// Hexadecimal input provided as a convenience but the flag name is awkward. Typically you'd use InputText() to parse your own data, if you want to handle prefixes.
			const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%08X" : "%d";
			return ImGui::InputScalar(label.c_str(), ImGuiDataType_S32, (void*)v, (void*)(step > 0 ? &step : NULL), (void*)(step_fast > 0 ? &step_fast : NULL), format, flags);
		}

		bool Input2(const std::string& label, int* v, ImGuiInputTextFlags flags = 0)
		{
			return ImGui::InputScalarN(label.c_str(), ImGuiDataType_S32, v, 2, NULL, NULL, "%d", flags);
		}

		bool Input3(const std::string& label, int* v, ImGuiInputTextFlags flags = 0)
		{
			return ImGui::InputScalarN(label.c_str(), ImGuiDataType_S32, v, 3, NULL, NULL, "%d", flags);
		}

		bool Input4(const std::string& label, int* v, ImGuiInputTextFlags flags = 0)
		{
			return ImGui::InputScalarN(label.c_str(), ImGuiDataType_S32, v, 4, NULL, NULL, "%d", flags);
		}

		bool InputUInt(const std::string& label, uint32_t& v, int step = 1, int step_fast = 100, ImGuiInputTextFlags flags = 0)
		{
			// Hexadecimal input provided as a convenience but the flag name is awkward. Typically you'd use InputText() to parse your own data, if you want to handle prefixes.
			const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%08X" : "%d";
			return ImGui::InputScalar(label.c_str(), ImGuiDataType_U32, &v, (void*)(step > 0 ? &step : NULL), (void*)(step_fast > 0 ? &step_fast : NULL), format, flags);
		}

		bool InputDouble(const std::string& label, double& v, double step = 0.0, double step_fast = 0.0, const char* format = "%.6f", ImGuiInputTextFlags flags = 0)
		{
			return ImGui::InputScalar(label.c_str(), ImGuiDataType_Double, (void*)&v, (void*)(step > 0.0 ? &step : NULL), (void*)(step_fast > 0.0 ? &step_fast : NULL), format, flags);
		}

		bool Checkbox(const std::string& label, bool& v)
		{
			return ImGui::Checkbox(label.c_str(), &v);
		}
		//bool DragIntRange2(const char* label, int* v_current_min, int* v_current_max, float v_speed = 1.0f, int v_min = 0, int v_max = 0, const char* format = "%d", const char* format_max = NULL, ImGuiSliderFlags flags = 0);
		//bool DragScalar(const char* label, ImGuiDataType data_type, void* p_data, float v_speed = 1.0f, const void* p_min = NULL, const void* p_max = NULL, const char* format = NULL, ImGuiSliderFlags flags = 0);
		//bool DragScalarN(const char* label, ImGuiDataType data_type, void* p_data, int components, float v_speed = 1.0f, const void* p_min = NULL, const void* p_max = NULL, const char* format = NULL, ImGuiSliderFlags flags = 0);
	}
}