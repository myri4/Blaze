#pragma once

#include <imgui/imgui.h>

namespace wc
{
	ImVec2 ImConv(glm::vec2 v)
	{
		return ImVec2(v.x, v.y);
	}

	namespace Widgets
	{

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

		void PositionUI(glm::vec2& pos)
		{
			float position[2] = { pos.x, pos.y };

			// Define a fixed width for the buttons
			const float buttonWidth = 20.0f;

			// Calculate the available width for the input fields
			float totalWidth = ImGui::GetContentRegionAvail().x;
			float labelWidth = buttonWidth * 2 + ImGui::CalcTextSize("(?)").x + ImGui::GetStyle().ItemSpacing.x * 2;
			float inputWidth = (totalWidth - labelWidth) / 2;

			ImGui::AlignTextToFramePadding();

			// Draw colored button for "X"
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 0.5f)); // Red color
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 0.6f)); // Red color
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.0f, 0.0f, 0.75f)); // Red color
			if (ImGui::Button("X", ImVec2(buttonWidth, 0)))
			{
				if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) { pos.x -= 1.0f; }
				else { pos.x += 1.0f; }
			}
			ImGui::PopStyleColor(3);
			ImGui::SameLine(0, 0);

			ImGui::PushItemWidth(inputWidth);
			if (ImGui::InputFloat("##XPosition", &position[0], 1.0f, 0.1f, "%.3f")) { pos.x = position[0]; }
			ImGui::PopItemWidth();
			ImGui::SameLine();

			ImGui::AlignTextToFramePadding();

			// Draw colored button for "Y"
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 1.0f, 0.0f, 0.5f)); // Green color
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 0.6f)); // Green color
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 1.0f, 0.0f, 0.75f)); // Green color
			if (ImGui::Button("Y", ImVec2(buttonWidth, 0)))
			{
				if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) { pos.y -= 1.0f; }
				else { pos.y += 1.0f; }
			}
			ImGui::PopStyleColor(3);
			ImGui::SameLine(0, 0);

			ImGui::PushItemWidth(inputWidth);
			if (ImGui::InputFloat("##YPosition", &position[1], 1.0f, 0.1f, "%.3f")) { pos.y = position[1]; }
			ImGui::PopItemWidth();

			HelpMarker("HOLD Ctrl for less step");

			return;
		}

		void ScaleUI(glm::vec2& scale)
		{
			float scaleValue[2] = { scale.x, scale.y };

			ImGui::Text("Scale");
			ImGui::SameLine();

			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

			if (ImGui::DragFloat2("##Scale", scaleValue))
			{
				scale.x = scaleValue[0];
				scale.y = scaleValue[1];
			}

			ImGui::PopItemWidth();
			return;
		}

	}

	namespace UI
	{
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


		bool InputFloat(const std::string& label, float& v, float step = 0.0f, float step_fast = 0.0f, const char* format = "%.3f", ImGuiInputTextFlags flags = 0)
		{
			return ImGui::InputScalar(label.c_str(), ImGuiDataType_Float, (void*)&v, (void*)(step > 0.0f ? &step : NULL), (void*)(step_fast > 0.0f ? &step_fast : NULL), format, flags);
		}

		bool InputFloat2(const std::string& label, float* v, const char* format = "%.3f", ImGuiInputTextFlags flags = 0)
		{
			return ImGui::InputScalarN(label.c_str(), ImGuiDataType_Float, v, 2, NULL, NULL, format, flags);
		}

		bool InputFloat3(const std::string& label, float* v, const char* format = "%.3f", ImGuiInputTextFlags flags = 0)
		{
			return ImGui::InputScalarN(label.c_str(), ImGuiDataType_Float, v, 3, NULL, NULL, format, flags);
		}

		bool InputFloat4(const std::string& label, float* v, const char* format = "%.3f", ImGuiInputTextFlags flags = 0)
		{
			return ImGui::InputScalarN(label.c_str(), ImGuiDataType_Float, v, 4, NULL, NULL, format, flags);
		}

		bool InputInt(const std::string& label, int& v, int step = 1, int step_fast = 100, ImGuiInputTextFlags flags = 0)
		{
			// Hexadecimal input provided as a convenience but the flag name is awkward. Typically you'd use InputText() to parse your own data, if you want to handle prefixes.
			const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%08X" : "%d";
			return ImGui::InputScalar(label.c_str(), ImGuiDataType_S32, (void*)v, (void*)(step > 0 ? &step : NULL), (void*)(step_fast > 0 ? &step_fast : NULL), format, flags);
		}

		bool InputInt2(const std::string& label, int* v, ImGuiInputTextFlags flags = 0)
		{
			return ImGui::InputScalarN(label.c_str(), ImGuiDataType_S32, v, 2, NULL, NULL, "%d", flags);
		}

		bool InputInt3(const std::string& label, int* v, ImGuiInputTextFlags flags = 0)
		{
			return ImGui::InputScalarN(label.c_str(), ImGuiDataType_S32, v, 3, NULL, NULL, "%d", flags);
		}

		bool InputInt4(const std::string& label, int* v, ImGuiInputTextFlags flags = 0)
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