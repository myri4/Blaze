#pragma once

#include <wc/Math/Camera.h>
#include <wc/Shader.h>
#include <wc/Descriptors.h>

#include "RenderData.h"
#include "wc/imgui_backend/imgui_impl_vulkan.h"

#include "Font.h"

#include "Rendergraph.h"

namespace wc
{
	uint32_t m_ComputeWorkGroupSize = 4; // @TODO: REMOVE!!!?

	struct BloomPass
	{
		Shader m_Shader;
		vk::Sampler m_Sampler;

		std::vector<VkDescriptorSet> m_DescriptorSets;

		uint32_t m_MipLevels = 1;

		struct
		{
			std::vector<vk::ImageView> imageViews;
			vk::Image image;
		} m_Buffers[3];

		auto GetOutput() { return m_Buffers[2].imageViews[0]; }

		void Init()
		{
			m_Shader.Create("assets/shaders/bloom.comp");
		}

		void CreateImages(glm::vec2 renderSize, uint32_t mipLevelCount)
		{
			glm::ivec2 bloomTexSize = renderSize * 0.5f;
			bloomTexSize += glm::ivec2(m_ComputeWorkGroupSize - bloomTexSize.x % m_ComputeWorkGroupSize, m_ComputeWorkGroupSize - bloomTexSize.y % m_ComputeWorkGroupSize);
			m_MipLevels = mipLevelCount - 4;

			vk::SamplerSpecification samplerSpec = {
				.magFilter = vk::Filter::LINEAR,
				.minFilter = vk::Filter::LINEAR,
				.mipmapMode = vk::SamplerMipmapMode::LINEAR,
				.addressModeU = vk::SamplerAddressMode::CLAMP_TO_EDGE,
				.addressModeV = vk::SamplerAddressMode::CLAMP_TO_EDGE,
				.addressModeW = vk::SamplerAddressMode::CLAMP_TO_EDGE,
				.maxLod = float(m_MipLevels),
			};

			m_Sampler.Create(samplerSpec);
			m_Sampler.SetName("BloomSampler");

			for (int i = 0; i < 3; i++)
			{
				auto& buffer = m_Buffers[i];
				vk::ImageSpecification imgInfo;

				imgInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;

				imgInfo.width = bloomTexSize.x;
				imgInfo.height = bloomTexSize.y;

				imgInfo.mipLevels = m_MipLevels;
				imgInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

				buffer.image.Create(imgInfo);

				auto& views = buffer.imageViews;
				views.reserve(imgInfo.mipLevels);

				VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
				createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				createInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
				createInfo.flags = 0;
				createInfo.image = buffer.image;
				createInfo.subresourceRange.layerCount = 1;
				createInfo.subresourceRange.levelCount = imgInfo.mipLevels;
				createInfo.subresourceRange.baseMipLevel = 0;
				createInfo.subresourceRange.baseArrayLayer = 0;
				createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				{ // Creating the first image view
					vk::ImageView& imageView = views.emplace_back();
					imageView.Create(createInfo);
				}

				// Create The rest
				createInfo.subresourceRange.levelCount = 1;
				for (uint32_t mip = 1; mip < imgInfo.mipLevels; mip++)
				{
					createInfo.subresourceRange.baseMipLevel = mip;
					vk::ImageView& imageView = views.emplace_back();
					imageView.Create(createInfo);
				}

				buffer.image.SetName(std::format("m_BloomBuffers[{}]", i));
			}

			vk::SyncContext::ImmediateSubmit([&](VkCommandBuffer cmd)
				{
					VkImageSubresourceRange range;
					range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					range.baseArrayLayer = 0;
					range.baseMipLevel = 0;
					range.layerCount = 1;
					range.levelCount = m_MipLevels;
					for (int i = 0; i < 3; i++)
						m_Buffers[i].image.SetLayout(cmd, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, range);
				});
		}

		void SetUp(const vk::ImageView& input)
		{
			m_DescriptorSets.reserve(m_MipLevels * 3 - 1);

			uint32_t usingSets = 0;

			auto GenerateDescriptor = [&](const vk::ImageView& outputView, const vk::ImageView& inputView)
				{
					VkDescriptorSet* descriptor = nullptr;
					if (usingSets < m_DescriptorSets.size())
						descriptor = &m_DescriptorSets[usingSets];
					else
					{
						descriptor = &m_DescriptorSets.emplace_back();
						vk::descriptorAllocator.Allocate(*descriptor, m_Shader.GetDescriptorLayout());
					}
					usingSets++;

					vk::DescriptorWriter writer(*descriptor);
					writer.BindImage(0, m_Sampler, outputView, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
					writer.BindImage(1, m_Sampler, inputView, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
					writer.BindImage(2, m_Sampler, m_Buffers[2].imageViews[0], VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
				};

			GenerateDescriptor(m_Buffers[0].imageViews[0], input);

			for (uint32_t currentMip = 1; currentMip < m_MipLevels; currentMip++)
			{
				// Ping
				GenerateDescriptor(m_Buffers[1].imageViews[currentMip], m_Buffers[0].imageViews[0]);

				// Pong
				GenerateDescriptor(m_Buffers[0].imageViews[currentMip], m_Buffers[1].imageViews[0]);
			}


			// First Upsample
			GenerateDescriptor(m_Buffers[2].imageViews[m_MipLevels - 1], m_Buffers[0].imageViews[0]);

			for (int currentMip = m_MipLevels - 2; currentMip >= 0; currentMip--)
				GenerateDescriptor(m_Buffers[2].imageViews[currentMip], m_Buffers[0].imageViews[0]);
		}

		void Execute(CommandEncoder& cmd, float Threshold = 1.2f, float Knee = 0.6f)
		{
			cmd.BindShader(m_Shader);
			uint32_t counter = 0;

			enum
			{
				Prefilter,
				Downsample,
				UpsampleFirst,
				Upsample
			};

			struct
			{
				glm::vec4 Params = glm::vec4(1.f); // (x) threshold, (y) threshold - knee, (z) knee * 2, (w) 0.25 / knee
				float LOD = 0.f;
				int Mode = Prefilter;
			} settings;

			settings.Params = glm::vec4(Threshold, Threshold - Knee, Knee * 2.f, 0.25f / Knee);
			cmd.PushConstants(settings);
			cmd.BindDescriptorSet(m_DescriptorSets[counter++]);
			cmd.Dispatch(glm::ceil(glm::vec2(m_Buffers[0].image.GetSize()) / glm::vec2(m_ComputeWorkGroupSize)));

			settings.Mode = Downsample;
			for (uint32_t currentMip = 1; currentMip < m_MipLevels; currentMip++)
			{
				glm::vec2 dispatchSize = glm::ceil((glm::vec2)m_Buffers[0].image.GetMipSize(currentMip) / glm::vec2(m_ComputeWorkGroupSize));

				// Ping
				settings.LOD = float(currentMip - 1);
				cmd.PushConstants(settings);

				cmd.BindDescriptorSet(m_DescriptorSets[counter++]);
				cmd.Dispatch(dispatchSize);

				// Pong
				settings.LOD = float(currentMip);
				cmd.PushConstants(settings);

				cmd.BindDescriptorSet(m_DescriptorSets[counter++]);
				cmd.Dispatch(dispatchSize);
			}

			// First Upsample
			settings.LOD = float(m_MipLevels - 2);
			settings.Mode = UpsampleFirst;
			cmd.PushConstants(settings);

			cmd.BindDescriptorSet(m_DescriptorSets[counter++]);
			cmd.Dispatch(glm::ceil((glm::vec2)m_Buffers[2].image.GetMipSize(m_MipLevels - 1) / glm::vec2(m_ComputeWorkGroupSize)));

			settings.Mode = Upsample;
			for (int currentMip = m_MipLevels - 2; currentMip >= 0; currentMip--)
			{
				settings.LOD = float(currentMip);
				cmd.PushConstants(settings);

				cmd.BindDescriptorSet(m_DescriptorSets[counter++]);
				cmd.Dispatch(glm::ceil((glm::vec2)m_Buffers[2].image.GetMipSize(currentMip) / glm::vec2(m_ComputeWorkGroupSize)));
			}
		}

		void DestroyImages()
		{
			for (int i = 0; i < 3; i++)
			{
				for (auto& view : m_Buffers[i].imageViews)
					view.Destroy();
				m_Buffers[i].image.Destroy();
				m_Buffers[i].imageViews.clear();
			}
			m_Sampler.Destroy();
		}

		void Deinit()
		{
			m_Shader.Destroy();
		}
	};

	struct CompositePass
	{
		Shader m_Shader;
		VkDescriptorSet m_DescriptorSet;

		void Init()
		{
			m_Shader.Create("assets/shaders/composite.comp");
			vk::descriptorAllocator.Allocate(m_DescriptorSet, m_Shader.GetDescriptorLayout());
		}

		void SetUp(vk::Sampler sampler, vk::ImageView output, vk::ImageView input, vk::ImageView bloomInput)
		{
			vk::DescriptorWriter writer(m_DescriptorSet);
			writer.BindImage(0, sampler, output, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
				.BindImage(1, sampler, input, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				.BindImage(2, sampler, bloomInput, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		}

		void Execute(CommandEncoder& cmd, glm::ivec2 size)
		{
			cmd.BindShader(m_Shader);
			cmd.BindDescriptorSet(m_DescriptorSet);
			struct
			{
				uint32_t Bloom;
			}data;
			data.Bloom = 1;
			cmd.PushConstants(data);
			cmd.Dispatch(glm::ceil((glm::vec2)size / glm::vec2(m_ComputeWorkGroupSize)));
		}

		void Deinit()
		{
			m_Shader.Destroy();
		}
	};

	struct CRTPass
	{
		Shader m_Shader;
		VkDescriptorSet m_DescriptorSet;

		void Init()
		{
			m_Shader.Create("assets/shaders/crt.comp");
			vk::descriptorAllocator.Allocate(m_DescriptorSet, m_Shader.GetDescriptorLayout());
		}

		void SetUp(vk::Sampler sampler, vk::ImageView output, vk::ImageView input)
		{
			vk::DescriptorWriter writer(m_DescriptorSet);
			writer.BindImage(0, sampler, output, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
				.BindImage(1, sampler, input, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		}

		void Execute(CommandEncoder& cmd, glm::ivec2 size, float time)
		{
			cmd.BindShader(m_Shader);
			cmd.BindDescriptorSet(m_DescriptorSet);
			struct {
				float time = 0.f;
				uint32_t CRT;
				uint32_t Vignete;
				float Brighness;
			} m_Data;
			m_Data.time = time;
			m_Data.CRT = 1;
			m_Data.Vignete = 1;
			m_Data.Brighness = 1.f;

			cmd.PushConstants(m_Data);
			cmd.Dispatch(glm::ceil((glm::vec2)size / glm::vec2(m_ComputeWorkGroupSize)));
		}

		void Deinit()
		{
			m_Shader.Destroy();
		}
	};

	struct Renderer2D
	{
		glm::vec2 m_RenderSize;

		// Rendering
		VkFramebuffer m_Framebuffer;
		VkRenderPass m_RenderPass;

		vk::Image m_OutputImage;
		vk::ImageView m_OutputImageView;


		Shader m_Shader;
		VkDescriptorSet m_DescriptorSet;

		Shader m_LineShader;



		// Post processing
		BloomPass bloom;
		CompositePass composite;
		CRTPass crt;

		float time = 0.f;

		vk::Image m_FinalImage[2];
		vk::ImageView m_FinalImageView[2];
		vk::Sampler m_ScreenSampler;

		VkCommandBuffer m_Cmd[FRAME_OVERLAP];
		VkCommandBuffer m_ComputeCmd[FRAME_OVERLAP];
		VkDescriptorSet m_ImguiImageID = VK_NULL_HANDLE;

		OrthographicCamera* camera = nullptr;

		auto GetImguiImageID() { return (uint64_t)m_ImguiImageID; }
		auto GetOutputImage() const { return m_OutputImage; }

		auto GetRenderSize() const { return m_RenderSize; }
		auto GetAspectRatio() const { return m_OutputImage.GetAspectRatio(); }

		auto GetHalfSize() const { return (m_RenderSize / 128.f) * camera->Zoom; }
		auto GetHalfSize(glm::vec2 size) const { return size * camera->Zoom; }

		auto ScreenToWorld(glm::vec2 coords) const
		{
			float camX = ((2.f * coords.x / m_RenderSize.x) - 1.f);
			float camY = (1.f - (2.f * coords.y / m_RenderSize.y));
			return glm::vec2(camX, camY) * GetHalfSize();
		}

		auto WorldToScreen(glm::vec2 worldCoords) const
		{
			glm::vec2 relativeCoords = worldCoords / GetHalfSize();

			float screenX = ((relativeCoords.x + 1.f) / 2.f) * m_RenderSize.x;
			float screenY = ((1.f - relativeCoords.y) / 2.f) * m_RenderSize.y;

			return glm::vec2(screenX, screenY);
		}

		void Init(const AssetManager& assetManager)
		{
			bloom.Init();
			composite.Init();
			crt.Init();

			for (uint32_t i = 0; i < FRAME_OVERLAP; i++)
			{
				vk::SyncContext::GraphicsCommandPool.Allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_Cmd[i]);
				vk::SyncContext::ComputeCommandPool.Allocate(VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_ComputeCmd[i]);
			}

			{
				VkAttachmentDescription attachmentDescription = {
					.format = VK_FORMAT_R32G32B32A32_SFLOAT, // @WARNING: if the image format is changed this should also be changed
					.samples = VK_SAMPLE_COUNT_1_BIT,
					.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
					.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
					.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
					.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.finalLayout = VK_IMAGE_LAYOUT_GENERAL,
				};

				// Collect attachment references
				std::vector<VkAttachmentReference> colorReferences;

				colorReferences.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
				VkSubpassDescription subpass = {
					.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
					.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size()),
					.pColorAttachments = colorReferences.data(),
				};

				// Use subpass dependencies for attachment layout transitions
				std::array<VkSubpassDependency, 2> dependencies;

				dependencies[0] = {
					.srcSubpass = VK_SUBPASS_EXTERNAL,
					.dstSubpass = 0,
					.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
					.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
				};

				dependencies[1] = {
					.srcSubpass = 0,
					.dstSubpass = VK_SUBPASS_EXTERNAL,
					.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
					.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
					.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
				};

				// Create render pass
				VkRenderPassCreateInfo renderPassInfo = {
					.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
					.attachmentCount = 1,
					.pAttachments = &attachmentDescription,
					.subpassCount = 1,
					.pSubpasses = &subpass,
					.dependencyCount = dependencies.size(),
					.pDependencies = dependencies.data(),
				};
				vkCreateRenderPass(VulkanContext::GetLogicalDevice(), &renderPassInfo, VulkanContext::GetAllocator(), &m_RenderPass);
			}

			VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

			{
				ShaderCreateInfo createInfo;
				ReadBinary("assets/shaders/Renderer2D.vert", createInfo.binaries[0]);
				ReadBinary("assets/shaders/Renderer2D.frag", createInfo.binaries[1]);
				createInfo.renderSize = m_RenderSize;
				createInfo.renderPass = m_RenderPass;

				VkDescriptorBindingFlags flags[1];
				memset(flags, 0, sizeof(VkDescriptorBindingFlags) * (std::size(flags) - 1));
				flags[std::size(flags) - 1] = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

				uint32_t count = (uint32_t)assetManager.GetTextures().size();

				VkDescriptorSetVariableDescriptorCountAllocateInfo set_counts = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO };
				set_counts.descriptorSetCount = 1;
				set_counts.pDescriptorCounts = &count;

				createInfo.bindingFlags = flags;
				createInfo.bindingFlagCount = (uint32_t)std::size(flags);

				createInfo.dynamicDescriptorCount = count;

				createInfo.dynamicStateCount = std::size(dynamicStates);
				createInfo.dynamicState = dynamicStates;

				m_Shader.Create(createInfo);

				vk::descriptorAllocator.Allocate(m_DescriptorSet, m_Shader.GetDescriptorLayout(), &set_counts, set_counts.descriptorSetCount);

				vk::DescriptorWriter writer(m_DescriptorSet);


				std::vector<VkDescriptorImageInfo> infos;
				for (auto& image : assetManager.GetTextures())
					infos.push_back({ image.GetSampler(), image.GetView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });

				writer.BindImages(0, infos, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			}

			{
				ShaderCreateInfo createInfo;
				ReadBinary("assets/shaders/Line.vert", createInfo.binaries[0]);
				ReadBinary("assets/shaders/Line.frag", createInfo.binaries[1]);
				createInfo.renderSize = m_RenderSize;
				createInfo.renderPass = m_RenderPass;
				createInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
				createInfo.dynamicStateCount = std::size(dynamicStates);
				createInfo.dynamicState = dynamicStates;

				m_LineShader.Create(createInfo);





			}
		}

		void CreateScreen(glm::vec2 size)
		{
			m_RenderSize = size;

			camera->Update(GetHalfSize());

			{
				vk::ImageSpecification imageInfo;
				imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
				imageInfo.width = m_RenderSize.x;
				imageInfo.height = m_RenderSize.y;
				imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT /*| VK_IMAGE_USAGE_STORAGE_BIT*/;

				m_OutputImage.Create(imageInfo);
				m_OutputImage.SetName(std::format("Renderer2D::OutputImage"));

				m_OutputImageView.Create(m_OutputImage);
				m_OutputImageView.SetName(std::format("Renderer2D::OutputImage"));
			}

			VkFramebufferCreateInfo framebufferInfo = {
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass = m_RenderPass,
				.attachmentCount = 1,
				.pAttachments = &m_OutputImageView,
				.width = (uint32_t)m_RenderSize.x,
				.height = (uint32_t)m_RenderSize.y,
				.layers = 1,
			};
			vkCreateFramebuffer(VulkanContext::GetLogicalDevice(), &framebufferInfo, VulkanContext::GetAllocator(), &m_Framebuffer);

			for (int i = 0; i < ARRAYSIZE(m_FinalImage); i++)
			{
				vk::ImageSpecification imageInfo;
				imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
				imageInfo.width = m_RenderSize.x;
				imageInfo.height = m_RenderSize.y;
				imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

				m_FinalImage[i].Create(imageInfo);
				m_FinalImage[i].SetName(std::format("Renderer2D::FinalImage[{}]", i));

				m_FinalImageView[i].Create(m_FinalImage[i]);
				m_FinalImageView[i].SetName(std::format("Renderer2D::FinalImageView[{}]", i));
			}

			vk::SyncContext::ImmediateSubmit([&](VkCommandBuffer cmd) {
				for (int i = 0; i < ARRAYSIZE(m_FinalImage); i++)
					m_FinalImage[i].SetLayout(cmd, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
				});

			bloom.CreateImages(m_RenderSize, m_OutputImage.GetMipLevelCount());

			// For now we are using the same sampler for sampling the screen and the bloom images but maybe it should be separated
			vk::SamplerSpecification sampler;
			sampler.magFilter = vk::Filter::LINEAR;
			sampler.minFilter = vk::Filter::LINEAR;
			sampler.mipmapMode = vk::SamplerMipmapMode::LINEAR;
			sampler.addressModeU = vk::SamplerAddressMode::CLAMP_TO_EDGE;
			sampler.addressModeV = vk::SamplerAddressMode::CLAMP_TO_EDGE;
			sampler.addressModeW = vk::SamplerAddressMode::CLAMP_TO_EDGE;

			m_ScreenSampler.Create(sampler);

			int m_PassCount = 0; // @NOTE: Not sure if this is working properly
			int m_FinalPass = 0;

			auto GetImageBuffer = [&]()
				{
					m_FinalPass = m_PassCount++ % 2;
					return m_FinalImageView[m_FinalPass];
				};

			bloom.SetUp(m_OutputImageView);
			composite.SetUp(m_ScreenSampler, GetImageBuffer(), m_OutputImageView, bloom.GetOutput());
			//{
			//
			//	auto output = GetImageBuffer();
			//	auto input = GetImageBuffer();
			//	crt.SetUp(m_ScreenSampler, output, input);
			//}

			m_ImguiImageID = MakeImGuiDescriptor(m_ImguiImageID, { m_ScreenSampler, /*m_FinalImageView[m_FinalPass]*/m_OutputImageView, VK_IMAGE_LAYOUT_GENERAL });
		}

		void Resize(glm::vec2 newSize)
		{
			DestroyScreen();
			CreateScreen(newSize);
		}

		void DestroyScreen()
		{
			vkDestroyFramebuffer(VulkanContext::GetLogicalDevice(), m_Framebuffer, VulkanContext::GetAllocator());
			m_Framebuffer = VK_NULL_HANDLE;

			m_OutputImage.Destroy();
			m_OutputImageView.Destroy();

			m_ScreenSampler.Destroy();

			for (int i = 0; i < ARRAYSIZE(m_FinalImage); i++)
			{
				m_FinalImage[i].Destroy();
				m_FinalImageView[i].Destroy();
			}

			bloom.DestroyImages();
		}

		void Deinit()
		{
			bloom.Deinit();
			composite.Deinit();
			crt.Deinit();

			m_Shader.Destroy();
			m_LineShader.Destroy();

			DestroyScreen();

			vkDestroyRenderPass(VulkanContext::GetLogicalDevice(), m_RenderPass, VulkanContext::GetAllocator());
			m_RenderPass = VK_NULL_HANDLE;
		}

		void Flush(RenderData& renderData)
		{
			//if (!m_IndexCount && !m_LineVertexCount) return;

			time += Globals.deltaTime;

			{
				VkCommandBuffer& cmd = m_Cmd[CURRENT_FRAME];
				vkResetCommandBuffer(cmd, 0);

				VkCommandBufferBeginInfo begInfo = {
					.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
					.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
				};

				vkBeginCommandBuffer(cmd, &begInfo);
				VkRenderPassBeginInfo rpInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };

				rpInfo.renderPass = m_RenderPass;
				rpInfo.framebuffer = m_Framebuffer;
				rpInfo.clearValueCount = 1;
				VkClearValue clearValue;
				clearValue.color = { 0.f, 0.f, 0.f, 1.f };
				rpInfo.pClearValues = &clearValue;

				rpInfo.renderArea.extent = { (uint32_t)m_RenderSize.x, (uint32_t)m_RenderSize.y };

				vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

				VkViewport viewport = {
					.x = 0.f,
					.y = 0.f,
					//.y = createInfo.renderSize.y; // change this to 0 to invert
					.width = m_RenderSize.x,
					.height = m_RenderSize.y,
					//.height = -createInfo.renderSize.y; // remove the - to invert
					.minDepth = 0.f,
					.maxDepth = 1.f,
				};

				VkRect2D scissor = {
					.offset = { 0, 0 },
					.extent = { (uint32_t)m_RenderSize.x, (uint32_t)m_RenderSize.y },
				};


				vkCmdSetViewport(cmd, 0, 1, &viewport);
				vkCmdSetScissor(cmd, 0, 1, &scissor);

				struct
				{
					glm::mat4 ViewProj;
					VkDeviceAddress vertexBuffer;
				} m_data;
				m_data.ViewProj = camera->GetViewProjectionMatrix();
				if (renderData.GetIndexCount())
				{
					renderData.UploadVertexData();
					m_data.vertexBuffer = renderData.GetVertexBuffer().GetDeviceAddress();
					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Shader.GetPipeline());
					vkCmdPushConstants(cmd, m_Shader.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(m_data), &m_data);

					vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Shader.GetPipelineLayout(), 0, 1, &m_DescriptorSet, 0, nullptr);
					vkCmdBindIndexBuffer(cmd, renderData.GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
					vkCmdDrawIndexed(cmd, renderData.GetIndexCount(), 1, 0, 0, 0);
				}

				if (renderData.GetLineVertexCount())
				{
					renderData.UploadLineVertexData();
					m_data.vertexBuffer = renderData.GetLineVertexBuffer().GetDeviceAddress();

					vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_LineShader.GetPipeline());
					vkCmdPushConstants(cmd, m_Shader.GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(m_data), &m_data);

					vkCmdDraw(cmd, renderData.GetLineVertexCount(), 1, 0, 0);
				}


				vkCmdEndRenderPass(cmd);
				vkEndCommandBuffer(cmd);

				vk::SyncContext::Submit(cmd, vk::SyncContext::GetGraphicsQueue());
			}

			{
				CommandEncoder cmd;
				//bloom.Execute(cmd);
				//composite.Execute(cmd, m_RenderSize);
				//crt.Execute(cmd, m_RenderSize, time);

				//cmd.ExecuteCompute(m_ComputeCmd[CURRENT_FRAME]);
			}
		}
	};
}
