#include "imgui/imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_vulkan.h"
#include <wc/Texture.h>
#include <wc/Shader.h>

#include <stdio.h>

// Visual Studio warnings
#ifdef _MSC_VER
#pragma warning (disable: 4127) // condition expression is constant
#endif

// Render function
void ImGui_ImplVulkan_RenderDrawData(ImDrawData* draw_data, VkCommandBuffer cmd, VkRenderPassBeginInfo rpInfo)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;

    ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();

    // Allocate array to store enough vertex/index buffers. Each unique viewport gets its own storage.
    ImGui_ImplVulkan_ViewportData* viewport_renderer_data = (ImGui_ImplVulkan_ViewportData*)draw_data->OwnerViewport->RendererUserData;
    IM_ASSERT(viewport_renderer_data != nullptr);
    ImGui_ImplVulkan_WindowRenderBuffers* wrb = &viewport_renderer_data->RenderBuffers;
    if (!wrb->FrameRenderBuffers)
    {
        wrb->Index = 0;
        wrb->FrameRenderBuffers.Allocate(3); // @TODO: Change this
        memset(wrb->FrameRenderBuffers.Data, 0, sizeof(ImGui_ImplVulkan_FrameRenderBuffers) * wrb->FrameRenderBuffers.Count);
    }
    wrb->Index = (wrb->Index + 1) % wrb->FrameRenderBuffers.Count;
    ImGui_ImplVulkan_FrameRenderBuffers* rb = &wrb->FrameRenderBuffers[wrb->Index];

    if (draw_data->TotalVtxCount > 0)
    {
        // Create or resize the vertex/index buffers
        size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
        size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
        if (rb->VertexBuffer == VK_NULL_HANDLE) rb->VertexBuffer.Allocate(vertex_size, vk::DEVICE_ADDRESS);
        else if (rb->VertexBuffer.Size() < vertex_size)
        {
            rb->VertexBuffer.Free();
            rb->VertexBuffer.Allocate(vertex_size, vk::DEVICE_ADDRESS);
        }

        if (rb->IndexBuffer == VK_NULL_HANDLE) rb->IndexBuffer.Allocate(index_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        else if (rb->IndexBuffer.Size() < index_size)
        {
            rb->IndexBuffer.Free();
            rb->IndexBuffer.Allocate(index_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        }

        vk::StagingBuffer vBuffer, iBuffer;
        vBuffer.Allocate(vertex_size);
        iBuffer.Allocate(index_size);
        ImDrawVert* vtx_dst = (ImDrawVert*)vBuffer.Map();
        ImDrawIdx* idx_dst = (ImDrawIdx*)iBuffer.Map();
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtx_dst += cmd_list->VtxBuffer.Size;
            idx_dst += cmd_list->IdxBuffer.Size;
        }

        vk::SyncContext::ImmediateSubmit([&](VkCommandBuffer cmd) {
            rb->VertexBuffer.SetData(cmd, vBuffer, vertex_size);
            rb->IndexBuffer.SetData(cmd, iBuffer, index_size);
            });
        
        vBuffer.Unmap();
        iBuffer.Unmap();
        vBuffer.Free();
        iBuffer.Free();
    }
	VkCommandBufferBeginInfo begInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

	begInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(cmd, &begInfo);
    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, bd->Shader.GetPipeline());

		if (draw_data->TotalVtxCount > 0)
			vkCmdBindIndexBuffer(cmd, rb->IndexBuffer, 0, sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);

		// Setup viewport:
		VkViewport viewport;
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = (float)fb_width;
		viewport.height = (float)fb_height;
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		struct
		{
			float scale[2];
			float translate[2];
            VkDeviceAddress vertexBuffer;
		}u_Data;
		u_Data.scale[0] = 2.f / draw_data->DisplaySize.x;
		u_Data.scale[1] = 2.f / draw_data->DisplaySize.y;
		u_Data.translate[0] = -1.f - draw_data->DisplayPos.x * u_Data.scale[0];
		u_Data.translate[1] = -1.f - draw_data->DisplayPos.y * u_Data.scale[1];
        u_Data.vertexBuffer = rb->VertexBuffer.GetDeviceAddress();
		vkCmdPushConstants(cmd, bd->Shader.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(u_Data), &u_Data);
	}

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr && pcmd->UserCallback != ImDrawCallback_ResetRenderState) pcmd->UserCallback(cmd_list, pcmd);
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);

                // Clamp to viewport as vkCmdSetScissor() won't accept values that are off bounds
                if (clip_min.x < 0.f) { clip_min.x = 0.f; }
                if (clip_min.y < 0.f) { clip_min.y = 0.f; }
                if (clip_max.x > fb_width) { clip_max.x = (float)fb_width; }
                if (clip_max.y > fb_height) { clip_max.y = (float)fb_height; }
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                // Apply scissor/clipping rectangle
                VkRect2D scissor;
                scissor.offset.x = (int32_t)(clip_min.x);
                scissor.offset.y = (int32_t)(clip_min.y);
                scissor.extent.width = (uint32_t)(clip_max.x - clip_min.x);
                scissor.extent.height = (uint32_t)(clip_max.y - clip_min.y);
                vkCmdSetScissor(cmd, 0, 1, &scissor);

                // Bind DescriptorSet with font or user texture
                VkDescriptorSet desc_set = (VkDescriptorSet)pcmd->TextureId;
                if (sizeof(ImTextureID) < sizeof(ImU64))
                {
                    // We don't support texture switches if ImTextureID hasn't been redefined to be 64-bit. Do a flaky check that other textures haven't been used.
                    IM_ASSERT(pcmd->TextureId == (ImTextureID)bd->FontDescriptorSet);
                    desc_set = bd->FontDescriptorSet;
                }
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, bd->Shader.GetPipelineLayout(), 0, 1, &desc_set, 0, nullptr);

                // Draw
                vkCmdDrawIndexed(cmd, pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
            }
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }

    VkRect2D scissor = { { 0, 0 }, { (uint32_t)fb_width, (uint32_t)fb_height } };
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);
}

bool ImGui_ImplVulkan_CreateFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();

    // Destroy existing texture (if any)
    if (bd->FontView || bd->FontImage || bd->FontDescriptorSet)
    {
        vk::SyncContext::GetGraphicsQueue().WaitIdle();
        ImGui_ImplVulkan_DestroyFontsTexture();
    }

    uint8_t* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    size_t upload_size = width * height * 4 * sizeof(char);

    // Create the Image:
    vk::ImageSpecification imageSpec = 
    {
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .width = (uint32_t)width,
        .height = (uint32_t)height,
        .mipLevels = 1,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    };
	bd->FontImage.Create(imageSpec);
	bd->FontImage.SetName("imgui_font_image");

    // Create the Image View:
    VkImageViewCreateInfo viewInfo = { 
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, 
        .image = bd->FontImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        }
    };
    bd->FontView.Create(viewInfo);
    bd->FontView.SetName("imgui_font_image_view");

    // Create the Descriptor Set:
    bd->FontDescriptorSet = (VkDescriptorSet)MakeImGuiDescriptor(bd->FontDescriptorSet, { bd->FontSampler, bd->FontView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });

    // Create the Upload Buffer:
    vk::StagingBuffer upload_buffer;
    upload_buffer.Allocate(upload_size);
    upload_buffer.SetData(pixels, upload_size);

    // Copy to Image:
    vk::SyncContext::ImmediateSubmit([&](VkCommandBuffer cmd) {
        VkImageMemoryBarrier copy_barrier = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = bd->FontImage,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            }
        };
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &copy_barrier);

        VkBufferImageCopy region = {
            .imageSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount = 1,
            },
            .imageExtent = {
                .width = (uint32_t)width,
                .height = (uint32_t)height,
                .depth = 1,
            }
        };
        vkCmdCopyBufferToImage(cmd, upload_buffer, bd->FontImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        VkImageMemoryBarrier use_barrier = { 
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = bd->FontImage,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            }
        };
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &use_barrier);
        });
    upload_buffer.Free();

    // Store our identifier
    io.Fonts->SetTexID((ImTextureID)bd->FontDescriptorSet);

    return true;
}

void ImGui_ImplVulkan_DestroyFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();

    io.Fonts->SetTexID(0);

    bd->FontView.Destroy();
    bd->FontImage.Destroy();
}

//--------------------------------------------------------------------------------------------------------
// MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
//--------------------------------------------------------------------------------------------------------

static void ImGui_ImplVulkan_CreateWindow(ImGuiViewport* viewport)
{
    ImGui_ImplVulkan_ViewportData* vd = IM_NEW(ImGui_ImplVulkan_ViewportData)();
    viewport->RendererUserData = vd;
    ImGui_ImplVulkanH_Window* wd = &vd->Window;

	// Create surface
	ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    VkResult err = (VkResult)platform_io.Platform_CreateVkSurface(viewport, (ImU64)VulkanContext::GetInstance(), (const void*)VulkanContext::GetAllocator(), (ImU64*)&wd->Swapchain.surface);

    // Create SwapChain, RenderPass, Framebuffer, etc.
    wd->ClearEnable = (viewport->Flags & ImGuiViewportFlags_NoRendererClear) ? false : true;
    wd->CreateOrResize((int)viewport->Size.x, (int)viewport->Size.y);
    vd->WindowOwned = true;
}

static void ImGui_ImplVulkan_DestroyWindow(ImGuiViewport* viewport)
{
    // The main viewport (owned by the application) will always have RendererUserData == 0 since we didn't create the data for it.
    if (ImGui_ImplVulkan_ViewportData* vd = (ImGui_ImplVulkan_ViewportData*)viewport->RendererUserData)
    {
        if (vd->WindowOwned)
            vd->Window.Destroy();
        vd->RenderBuffers.Destroy();
        IM_DELETE(vd);
    }
    viewport->RendererUserData = nullptr;
}

static void ImGui_ImplVulkan_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    ImGui_ImplVulkan_ViewportData* vd = (ImGui_ImplVulkan_ViewportData*)viewport->RendererUserData;
    if (vd == nullptr) // This is nullptr for the main viewport (which is left to the user/app to handle)
        return;
    vd->Window.ClearEnable = (viewport->Flags & ImGuiViewportFlags_NoRendererClear) ? false : true;
    vd->Window.CreateOrResize((int)size.x, (int)size.y);
}

static void ImGui_ImplVulkan_RenderWindow(ImGuiViewport* viewport, void*)
{
    ImGui_ImplVulkan_ViewportData* vd = (ImGui_ImplVulkan_ViewportData*)viewport->RendererUserData;
    ImGui_ImplVulkanH_Window* wd = &vd->Window;

    ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
    ImGui_ImplVulkanH_FrameSemaphores* fsd = &wd->FrameSemaphores[wd->SemaphoreIndex];

    auto err = wd->Swapchain.AcquireNextImage(wd->swapchainImageIndex, fsd->ImageAcquiredSemaphore);
    fd = &wd->Frames[wd->swapchainImageIndex];
    fd->Fence.Wait();
    fd->Fence.Reset();

    if (err == VK_ERROR_OUT_OF_DATE_KHR)
    {
        return;
    }

    wd->ClearValue.color = { 0.f, 0.f, 0.f, 1.f };
    vkResetCommandBuffer(fd->CommandBuffer, 0);
    {
        VkRenderPassBeginInfo info = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        info.renderPass = wd->Swapchain.RenderPass;
        info.framebuffer = wd->Swapchain.Framebuffers[wd->swapchainImageIndex];
        info.renderArea.extent.width = wd->Width;
        info.renderArea.extent.height = wd->Height;
        if (wd->ClearEnable)
        {
            info.clearValueCount = 1;
            info.pClearValues = &wd->ClearValue;
        }

        ImGui_ImplVulkan_RenderDrawData(viewport->DrawData, fd->CommandBuffer, info);
    }

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &fsd->ImageAcquiredSemaphore;
    info.pWaitDstStageMask = &wait_stage;
    info.commandBufferCount = 1;
    info.pCommandBuffers = &fd->CommandBuffer;
    info.signalSemaphoreCount = 1;
    info.pSignalSemaphores = &fsd->RenderCompleteSemaphore;

    vk::SyncContext::GetGraphicsQueue().Submit(info, fd->Fence);
}

static void ImGui_ImplVulkan_SwapBuffers(ImGuiViewport* viewport, void*)
{
    ImGui_ImplVulkan_ViewportData* vd = (ImGui_ImplVulkan_ViewportData*)viewport->RendererUserData;
    ImGui_ImplVulkanH_Window* wd = &vd->Window;

    ImGui_ImplVulkanH_FrameSemaphores* fsd = &wd->FrameSemaphores[wd->SemaphoreIndex];
    auto err = wd->Swapchain.Present(wd->swapchainImageIndex, fsd->RenderCompleteSemaphore);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
        vd->Window.CreateOrResize((int)viewport->Size.x, (int)viewport->Size.y);


    wd->FrameIndex = (wd->FrameIndex + 1) % wd->Frames.Count;
    wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->FrameSemaphores.Count;
}

bool ImGui_ImplVulkan_Init(VkRenderPass rp)
{
    ImGuiIO& io = ImGui::GetIO();

    // Setup backend capabilities flags
    ImGui_ImplVulkan_Data* bd = IM_NEW(ImGui_ImplVulkan_Data)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName = "imgui_impl_vulkan";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;  // We can create multi-viewports on the Renderer side (optional)

    bd->RenderPass = rp;

	// Bilinear sampling is required by default. Set 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to allow point/nearest sampling.
	vk::SamplerSpecification samplerSpec = {
		.magFilter = vk::Filter::LINEAR,
		.minFilter = vk::Filter::LINEAR,
		.mipmapMode = vk::SamplerMipmapMode::LINEAR,
		.addressModeU = vk::SamplerAddressMode::REPEAT,
		.addressModeV = vk::SamplerAddressMode::REPEAT,
		.addressModeW = vk::SamplerAddressMode::REPEAT,
		.minLod = -1000,
		.maxLod = 1000,
	};
	bd->FontSampler.Create(samplerSpec);
    bd->FontSampler.SetName("imgui_font_sampler");

	//color_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	wc::ShaderCreateInfo createInfo;
	createInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	wc::ReadBinary("assets/shaders/imgui.vert", createInfo.binaries[0]);
	wc::ReadBinary("assets/shaders/imgui.frag", createInfo.binaries[1]);
	createInfo.renderPass = bd->RenderPass;
	VkDynamicState dynamic_states[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	createInfo.dynamicStateCount = std::size(dynamic_states);
	createInfo.dynamicState = dynamic_states;

	bd->Shader.Create(createInfo);

    // Our render function expect RendererUserData to be storing the window render buffer we need (for the main viewport we won't use ->Window)
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->RendererUserData = IM_NEW(ImGui_ImplVulkan_ViewportData)();

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
		ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
		IM_ASSERT(platform_io.Platform_CreateVkSurface != nullptr && "Platform needs to setup the CreateVkSurface handler.");
		platform_io.Renderer_CreateWindow = ImGui_ImplVulkan_CreateWindow;
		platform_io.Renderer_DestroyWindow = ImGui_ImplVulkan_DestroyWindow;
		platform_io.Renderer_SetWindowSize = ImGui_ImplVulkan_SetWindowSize;
		platform_io.Renderer_RenderWindow = ImGui_ImplVulkan_RenderWindow;
		platform_io.Renderer_SwapBuffers = ImGui_ImplVulkan_SwapBuffers;
    }

    return true;
}

void ImGui_ImplVulkan_Shutdown()
{
    ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    // First destroy objects in all viewports
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    for (int n = 0; n < platform_io.Viewports.Size; n++)
        if (ImGui_ImplVulkan_ViewportData* vd = (ImGui_ImplVulkan_ViewportData*)platform_io.Viewports[n]->RendererUserData)
            vd->RenderBuffers.Destroy();

    ImGui_ImplVulkan_DestroyFontsTexture();

    bd->FontSampler.Destroy();
    bd->Shader.Destroy();

    // Manually delete main viewport render data in-case we haven't initialized for viewports
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    if (ImGui_ImplVulkan_ViewportData* vd = (ImGui_ImplVulkan_ViewportData*)main_viewport->RendererUserData)
        IM_DELETE(vd);
    main_viewport->RendererUserData = nullptr;

    // Clean up windows    
    ImGui::DestroyPlatformWindows();

    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasViewports);
    IM_DELETE(bd);
}
//-----------------------------------------------------------------------------

#endif // #ifndef IMGUI_DISABLE
