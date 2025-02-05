#pragma once
#ifndef IMGUI_DISABLE
#include "imgui/imgui.h"      // IMGUI_IMPL_API

// Vulkan includes
#include <wc/Memory/Buffer.h>

#include <wc/vk/Buffer.h>
#include <wc/vk/Commands.h>
#include <wc/vk/Image.h>
#include <wc/vk/SyncContext.h>
#include <wc/Descriptors.h>
#include <wc/Shader.h>
#include <wc/Swapchain.h>

// Called by user code
void         ImGui_ImplVulkan_Init(VkRenderPass rp);
void         ImGui_ImplVulkan_Shutdown();
void         ImGui_ImplVulkan_RenderDrawData(ImDrawData* draw_data, VkCommandBuffer cmd, VkRenderPassBeginInfo rpInfo);
bool         ImGui_ImplVulkan_CreateFontsTexture();
void         ImGui_ImplVulkan_DestroyFontsTexture();

VkDescriptorSet MakeImGuiDescriptor(VkDescriptorSet dSet, const VkDescriptorImageInfo& imageInfo);

#endif // #ifndef IMGUI_DISABLE
