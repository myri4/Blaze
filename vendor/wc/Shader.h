#pragma once

#pragma warning(push, 0)
#include <magic_enum.hpp>
#pragma warning(pop)

#include "vk/Buffer.h"
#include "vk/Pipeline.h"
#include "Descriptors.h"

#include <fstream>
#include <filesystem>
#include <spirv_cross/spirv_cross.hpp>

namespace wc
{
	inline void ReadBinary(const std::string& filename, std::vector<uint32_t>& buffer)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
			WC_CORE_ERROR("Cant open file at location {}", filename);

		size_t fileSize = (size_t)file.tellg();

		buffer.resize(fileSize / sizeof(uint32_t));

		file.seekg(0);

		file.read((char*)buffer.data(), fileSize);

		file.close();
	}

	inline auto CreateBlendAttachment(bool enable = true)
	{
		VkPipelineColorBlendAttachmentState colorBlend = {
			.blendEnable = enable,
			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,

			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.alphaBlendOp = VK_BLEND_OP_ADD,

			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		};
		return colorBlend;
	}

	struct ShaderCreateInfo
	{
		std::vector<uint32_t> binaries[2];

		glm::vec2 renderSize = glm::vec2(0.f);
		VkRenderPass renderPass;

		std::vector<VkPipelineColorBlendAttachmentState> blendAttachments;
		bool depthTest = false;

		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		VkFrontFace frontFace = VK_FRONT_FACE_CLOCKWISE;

		VkDescriptorBindingFlags* bindingFlags = nullptr;
		uint32_t bindingFlagCount = 0;
		uint32_t dynamicDescriptorCount = 0;

		VkDynamicState* dynamicState = nullptr;
		uint32_t dynamicStateCount = 0;
	};

	struct ComputeShaderCreateInfo
	{
		ComputeShaderCreateInfo() = default;
		ComputeShaderCreateInfo(const std::string& path) { wc::ReadBinary(path, binary); }
		ComputeShaderCreateInfo(const char* path) { wc::ReadBinary(path, binary); }

		std::vector<uint32_t> binary;

		VkDescriptorBindingFlags* bindingFlags = nullptr;
		uint32_t bindingFlagCount = 0;
		uint32_t dynamicDescriptorCount = 0;
	};

	struct Shader
	{	
	private:
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_DescriptorLayout = VK_NULL_HANDLE;
				
	public:

		auto GetPipeline() const { return m_Pipeline; }
		auto GetPipelineLayout() const { return m_PipelineLayout; }
		auto GetDescriptorLayout() const { return m_DescriptorLayout; }

		VkDescriptorSet AllocateDescriptorSet()
		{
			VkDescriptorSet descriptor;
			vk::descriptorAllocator.Allocate(descriptor, GetDescriptorLayout());
			return descriptor;
		}

		void Destroy()
		{
			vkDestroyPipeline(VulkanContext::GetLogicalDevice(), m_Pipeline, VulkanContext::GetAllocator());
			m_Pipeline = VK_NULL_HANDLE;

			vkDestroyPipelineLayout(VulkanContext::GetLogicalDevice(), m_PipelineLayout, VulkanContext::GetAllocator());
			m_PipelineLayout = VK_NULL_HANDLE;

			vkDestroyDescriptorSetLayout(VulkanContext::GetLogicalDevice(), m_DescriptorLayout, VulkanContext::GetAllocator());
			m_DescriptorLayout = VK_NULL_HANDLE;
		}

		void Create(const ShaderCreateInfo& createInfo)
		{
			std::array<VkShaderModule, 2> shaderModules = {};
			std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {};
						
			for (int i = 0; i < 2; i++)
			{
				auto& binary = createInfo.binaries[i];
				VkShaderModuleCreateInfo moduleCreateInfo = { 
					.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
					.codeSize = binary.size() * sizeof(uint32_t),
					.pCode = binary.data(),
				};

				vkCreateShaderModule(VulkanContext::GetLogicalDevice(), &moduleCreateInfo, VulkanContext::GetAllocator(), &shaderModules[i]);
			}

			{ // Reflection
				std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
				std::vector<VkPushConstantRange> ranges;

				for (uint32_t i = 0; i < shaderModules.size(); i++)
				{
					spirv_cross::Compiler compiler(createInfo.binaries[i]);
					spirv_cross::ShaderResources resources = compiler.get_shader_resources();

					spirv_cross::EntryPoint entryPoint = compiler.get_entry_points_and_stages()[0];
					VkShaderStageFlags shaderStage = VkShaderStageFlagBits(1 << entryPoint.execution_model);

					for (auto& resource : resources.push_constant_buffers)
					{
						auto& baseType = compiler.get_type(resource.base_type_id);
						uint32_t bufferSize = (uint32_t)compiler.get_declared_struct_size(baseType);

						uint32_t offset = 0;
						if (ranges.size())
							offset = ranges.back().offset + ranges.back().size;

						auto& pushConstantRange = ranges.emplace_back() = {
							.stageFlags = shaderStage,
							.offset = 0,
							.size = bufferSize
						};
					}


					for (auto& resource : resources.uniform_buffers)
					{
						bool add = true;
						VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
						for (auto& layoutBinding : layoutBindings)
							if (layoutBinding.descriptorType == descriptorType && layoutBinding.binding == binding)
							{
								add = false;
								layoutBinding.stageFlags |= shaderStage;
								break;
							}

						if (add)
						{
							const auto& type = compiler.get_type(resource.type_id);

							uint32_t descriptorCount = 1;
							if (type.array[0] > 0) descriptorCount = type.array[0];

							auto& layoutBinding = layoutBindings.emplace_back() = {
								.binding = binding,
								.descriptorType = descriptorType,
								.descriptorCount = descriptorCount,
								.stageFlags = shaderStage,
								.pImmutableSamplers = nullptr,
							};
						}
					}

					for (auto& resource : resources.storage_buffers)
					{
						bool add = true;
						VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
						uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
						for (auto& layoutBinding : layoutBindings)
							if (layoutBinding.descriptorType == descriptorType && layoutBinding.binding == binding)
							{
								add = false;
								layoutBinding.stageFlags |= shaderStage;
								break;
							}

						if (add)
						{
							//const auto& type = compiler.get_type(resource.type_id);

							uint32_t descriptorCount = 1;
							// @TODO: report this as an issue with the new spirv-cross reflection as this is probably a nullptr and gives random garbage which is not supposed to happen
							//if (type.array[0] > 0) descriptorCount = type.array[0]; 

							auto& layoutBinding = layoutBindings.emplace_back() = {
								.binding = binding,
								.descriptorType = descriptorType,
								.descriptorCount = descriptorCount,
								.stageFlags = shaderStage,
								.pImmutableSamplers = nullptr,
							};
						}
					}

					if (shaderStage == VK_SHADER_STAGE_FRAGMENT_BIT)
					{
						for (auto& resource : resources.storage_images)
						{
							const auto& type = compiler.get_type(resource.type_id);

							uint32_t descriptorCount = 1;
							//if (type.array[0] > 0) descriptorCount = type.array[0];

							auto& layoutBinding = layoutBindings.emplace_back() = {
								.binding = compiler.get_decoration(resource.id, spv::DecorationBinding),
								.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
								.descriptorCount = descriptorCount,
								.stageFlags = shaderStage,
								.pImmutableSamplers = nullptr,
							};
						}

						for (auto& resource : resources.sampled_images)
						{
							const auto& type = compiler.get_type(resource.type_id);

							uint32_t descriptorCount = 1;
							//if (type.array[0] > 0) descriptorCount = type.array[0];

							auto& layoutBinding = layoutBindings.emplace_back() = {
								.binding = compiler.get_decoration(resource.id, spv::DecorationBinding),
								.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
								.descriptorCount = descriptorCount,
								.stageFlags = shaderStage,
								.pImmutableSamplers = nullptr,
							};
						}
					}

					VkPipelineShaderStageCreateInfo stage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };

					stage.stage = (VkShaderStageFlagBits)shaderStage;
					stage.module = shaderModules[i];
					stage.pName = "main";
					shaderStages[i] = stage;
				}

				if (createInfo.dynamicDescriptorCount)
					layoutBindings[layoutBindings.size() - 1].descriptorCount = createInfo.dynamicDescriptorCount;

				VkDescriptorSetLayoutCreateInfo layoutInfo = { 
					.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
					.bindingCount = static_cast<uint32_t>(layoutBindings.size()),
					.pBindings = layoutBindings.data(),
				};

				VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };

				if (createInfo.bindingFlagCount > 0)
				{
					binding_flags.bindingCount = createInfo.bindingFlagCount;
					binding_flags.pBindingFlags = createInfo.bindingFlags;

					layoutInfo.pNext = &binding_flags;
				}

				vkCreateDescriptorSetLayout(VulkanContext::GetLogicalDevice(), &layoutInfo, VulkanContext::GetAllocator(), &m_DescriptorLayout);

				VkPipelineLayoutCreateInfo info = { 
					.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
					.setLayoutCount = 1,
					.pSetLayouts = &m_DescriptorLayout,
					.pushConstantRangeCount = (uint32_t)ranges.size(),
					.pPushConstantRanges = ranges.data(),
				};

				vkCreatePipelineLayout(VulkanContext::GetLogicalDevice(), &info, VulkanContext::GetAllocator(), &m_PipelineLayout);
			}

			VkViewport viewport = {
				.x = 0.f,
				.y = 0.f,
				//.y = createInfo.renderSize.y; // change this to 0 to invert
				.width = createInfo.renderSize.x,
				.height = createInfo.renderSize.y,
				//.height = -createInfo.renderSize.y; // remove the - to invert
				.minDepth = 0.f,
				.maxDepth = 1.f,
			};

			VkRect2D scissor = {
				.offset = { 0, 0 },
				.extent = { (uint32_t)createInfo.renderSize.x, (uint32_t)createInfo.renderSize.y },
			};			

			VkPipelineDepthStencilStateCreateInfo depthStencil = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,

				.depthTestEnable = createInfo.depthTest,
				.depthWriteEnable = createInfo.depthTest, // should be depth write
				.depthCompareOp = createInfo.depthTest ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_ALWAYS, // should be changeable
				.depthBoundsTestEnable = false,
				.stencilTestEnable = false,
				.minDepthBounds = 0.f, // Optional
				.maxDepthBounds = 1.f, // Optional
			};

			VkPipelineViewportStateCreateInfo viewportState = { 
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,

				.viewportCount = 1,
				.pViewports = &viewport,
				.scissorCount = 1,
				.pScissors = &scissor,
			};			

			VkPipelineColorBlendStateCreateInfo colorBlending = { 
				.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,

				.logicOpEnable = false,
				.logicOp = VK_LOGIC_OP_COPY,
				.attachmentCount = (uint32_t)createInfo.blendAttachments.size(),
				.pAttachments = createInfo.blendAttachments.data(),
				.blendConstants = { 1.f, 1.f, 1.f, 1.f }
			};

			VkPipelineVertexInputStateCreateInfo vertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

			VkPipelineInputAssemblyStateCreateInfo inputAssembly = { 
				.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
				.topology = createInfo.topology,
				.primitiveRestartEnable = false,
			};

			VkPipelineRasterizationStateCreateInfo rasterizer = { 
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,

				.depthClampEnable = false,
				.rasterizerDiscardEnable = false,

				.polygonMode = VK_POLYGON_MODE_FILL,
				.cullMode = VK_CULL_MODE_NONE,
				.frontFace = createInfo.frontFace,
				.depthBiasEnable = false,
				.depthBiasConstantFactor = 0.f,
				.depthBiasClamp = 0.f,
				.depthBiasSlopeFactor = 0.f,
				.lineWidth = 1.f,
			};

			VkPipelineMultisampleStateCreateInfo multisampling = { 
				.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,

				.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
				.sampleShadingEnable = false,
				.minSampleShading = 1.f,
				.pSampleMask = nullptr,
				.alphaToCoverageEnable = false,
				.alphaToOneEnable = false,
			};

			VkGraphicsPipelineCreateInfo pipelineInfo = { 
				.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
				.stageCount = (uint32_t)shaderStages.size(),
				.pStages = shaderStages.data(),
				.pVertexInputState = &vertexInputState,
				.pInputAssemblyState = &inputAssembly,
				.pViewportState = &viewportState,
				.pRasterizationState = &rasterizer,
				.pMultisampleState = &multisampling,

				.pDepthStencilState = &depthStencil,
				.pColorBlendState = &colorBlending,
				.layout = m_PipelineLayout,
				.renderPass = createInfo.renderPass,
				.subpass = 0,
				.basePipelineHandle = nullptr,
			};

			VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
			if (createInfo.dynamicStateCount > 0)
			{
				dynamicState.dynamicStateCount = createInfo.dynamicStateCount;
				dynamicState.pDynamicStates = createInfo.dynamicState;
				pipelineInfo.pDynamicState = &dynamicState;
			}

			vkCreateGraphicsPipelines(VulkanContext::GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, VulkanContext::GetAllocator(), &m_Pipeline);

			for (uint32_t i = 0; i < shaderModules.size(); i++) vkDestroyShaderModule(VulkanContext::GetLogicalDevice(), shaderModules[i], VulkanContext::GetAllocator());
		}

		void Create(const ComputeShaderCreateInfo& createInfo)
		{
			//if (createInfo.infoPath.size() == 0) WC_CORE_WARN("Shader info for shader with path {} is empty!", createInfo.path);

			{ // Reflection

				std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

				spirv_cross::Compiler compiler(createInfo.binary);
				spirv_cross::ShaderResources resources = compiler.get_shader_resources();
				VkShaderStageFlags shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;

				for (auto& resource : resources.uniform_buffers)
				{
					const auto& type = compiler.get_type(resource.type_id);

					uint32_t descriptorCount = 1;
					//if (type.array[0] > 0) descriptorCount = type.array[0];

					auto& layoutBinding = layoutBindings.emplace_back() = {
						.binding = compiler.get_decoration(resource.id, spv::DecorationBinding),
						.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
						.descriptorCount = descriptorCount,
						.stageFlags = shaderStage,
						.pImmutableSamplers = nullptr,
					};
				}

				for (auto& resource : resources.storage_buffers)
				{
					const auto& type = compiler.get_type(resource.type_id);

					uint32_t descriptorCount = 1;
					//if (type.array[0] > 0) descriptorCount = type.array[0];

					auto& layoutBinding = layoutBindings.emplace_back() = {
						.binding = compiler.get_decoration(resource.id, spv::DecorationBinding),
						.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
						.descriptorCount = descriptorCount,
						.stageFlags = shaderStage,
						.pImmutableSamplers = nullptr,
					};
				}


				for (auto& resource : resources.storage_images)
				{
					const auto& type = compiler.get_type(resource.type_id);

					uint32_t descriptorCount = 1;
					//if (type.array[0] > 0) descriptorCount = type.array[0];

					auto& layoutBinding = layoutBindings.emplace_back() = {
						.binding = compiler.get_decoration(resource.id, spv::DecorationBinding),
						.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
						.descriptorCount = descriptorCount,
						.stageFlags = shaderStage,
						.pImmutableSamplers = nullptr,
					};
				}

				for (auto& resource : resources.sampled_images)
				{
					const auto& type = compiler.get_type(resource.type_id);

					uint32_t descriptorCount = 1;
					//if (type.array[0] > 0) descriptorCount = type.array[0];

					auto& layoutBinding = layoutBindings.emplace_back() = {
						.binding = compiler.get_decoration(resource.id, spv::DecorationBinding),
						.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
						.descriptorCount = descriptorCount,
						.stageFlags = shaderStage,
						.pImmutableSamplers = nullptr,
					};
				}

				if (createInfo.dynamicDescriptorCount)
					layoutBindings[layoutBindings.size() - 1].descriptorCount = createInfo.dynamicDescriptorCount;

				VkDescriptorSetLayoutCreateInfo layoutInfo = { 
					.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
					.bindingCount = static_cast<uint32_t>(layoutBindings.size()),
					.pBindings = layoutBindings.data(),
				};

				VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };

				if (createInfo.bindingFlagCount > 0)
				{
					binding_flags.bindingCount = createInfo.bindingFlagCount;
					binding_flags.pBindingFlags = createInfo.bindingFlags;

					layoutInfo.pNext = &binding_flags;
				}

				vkCreateDescriptorSetLayout(VulkanContext::GetLogicalDevice(), &layoutInfo, VulkanContext::GetAllocator(), &m_DescriptorLayout);

				VkPipelineLayoutCreateInfo info = { 
					.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,

					.setLayoutCount = 1,
					.pSetLayouts = &m_DescriptorLayout,
				};

				VkPushConstantRange range = {
					.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
					.offset = 0,
					.size = 0,
				};
				if (resources.push_constant_buffers.size() > 0)
				{
					auto& baseType = compiler.get_type(resources.push_constant_buffers[0].base_type_id);
					range.size = (uint32_t)compiler.get_declared_struct_size(baseType);

					info.pPushConstantRanges = &range;
					info.pushConstantRangeCount = 1;
				}

				vkCreatePipelineLayout(VulkanContext::GetLogicalDevice(), &info, VulkanContext::GetAllocator(), &m_PipelineLayout);
			}

			VkShaderModuleCreateInfo moduleCreateInfo = { 
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,

				.codeSize = createInfo.binary.size() * sizeof(uint32_t),
				.pCode = createInfo.binary.data(),
			};

			VkShaderModule shaderModule;
			vkCreateShaderModule(VulkanContext::GetLogicalDevice(), &moduleCreateInfo, VulkanContext::GetAllocator(), &shaderModule);

			VkPipelineShaderStageCreateInfo stage = { 
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,

				.stage = VK_SHADER_STAGE_COMPUTE_BIT,
				.module = shaderModule,
				.pName = "main",
			};

			VkComputePipelineCreateInfo pipelineCreateInfo = { 
				.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
				.stage = stage,
				.layout = m_PipelineLayout,
			};

			vkCreateComputePipelines(VulkanContext::GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, VulkanContext::GetAllocator(), &m_Pipeline);

			vkDestroyShaderModule(VulkanContext::GetLogicalDevice(), shaderModule, VulkanContext::GetAllocator());
		}
	};
}