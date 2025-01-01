#pragma once

#pragma warning(push, 0)
#define VK_NO_PROTOTYPES
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#define WC_GRAPHICS_VALIDATION 0
#define WC_SYNCHRONIZATION_VALIDATION 1
#define WC_SHADER_DEBUG_PRINT 0

#include <Volk/volk.h>
#include <GLFW/glfw3.h>
#include <vma/vk_mem_alloc.h>

#pragma warning(pop)
#include <magic_enum.hpp>
#include <set>
#include <unordered_set>
#include <glm/glm.hpp>
#include "../Utils/Log.h"

template<class T>
VkObjectType GetObjectType()
{
	if (typeid(T) == typeid(VkInstance)) return VK_OBJECT_TYPE_INSTANCE;
	if (typeid(T) == typeid(VkPhysicalDevice)) return VK_OBJECT_TYPE_PHYSICAL_DEVICE;
	if (typeid(T) == typeid(VkDevice)) return VK_OBJECT_TYPE_DEVICE;
	if (typeid(T) == typeid(VkQueue)) return VK_OBJECT_TYPE_QUEUE;
	if (typeid(T) == typeid(VkSemaphore)) return VK_OBJECT_TYPE_SEMAPHORE;
	if (typeid(T) == typeid(VkCommandBuffer)) return VK_OBJECT_TYPE_COMMAND_BUFFER;
	if (typeid(T) == typeid(VkFence)) return VK_OBJECT_TYPE_FENCE;
	if (typeid(T) == typeid(VkDeviceMemory)) return VK_OBJECT_TYPE_DEVICE_MEMORY;
	if (typeid(T) == typeid(VkBuffer)) return VK_OBJECT_TYPE_BUFFER;
	if (typeid(T) == typeid(VkImage)) return VK_OBJECT_TYPE_IMAGE;
	if (typeid(T) == typeid(VkEvent)) return VK_OBJECT_TYPE_EVENT;
	if (typeid(T) == typeid(VkQueryPool)) return VK_OBJECT_TYPE_QUERY_POOL;
	if (typeid(T) == typeid(VkBufferView)) return VK_OBJECT_TYPE_BUFFER_VIEW;
	if (typeid(T) == typeid(VkImageView)) return VK_OBJECT_TYPE_IMAGE_VIEW;
	if (typeid(T) == typeid(VkShaderModule)) return VK_OBJECT_TYPE_SHADER_MODULE;
	if (typeid(T) == typeid(VkPipelineCache)) return VK_OBJECT_TYPE_PIPELINE_CACHE;
	if (typeid(T) == typeid(VkPipelineLayout)) return VK_OBJECT_TYPE_PIPELINE_LAYOUT;
	if (typeid(T) == typeid(VkRenderPass)) return VK_OBJECT_TYPE_RENDER_PASS;
	if (typeid(T) == typeid(VkPipeline)) return VK_OBJECT_TYPE_PIPELINE;
	if (typeid(T) == typeid(VkDescriptorSetLayout)) return VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;
	if (typeid(T) == typeid(VkSampler)) return VK_OBJECT_TYPE_SAMPLER;
	if (typeid(T) == typeid(VkDescriptorPool)) return VK_OBJECT_TYPE_DESCRIPTOR_POOL;
	if (typeid(T) == typeid(VkDescriptorSet)) return VK_OBJECT_TYPE_DESCRIPTOR_SET;
	if (typeid(T) == typeid(VkFramebuffer)) return VK_OBJECT_TYPE_FRAMEBUFFER;
	if (typeid(T) == typeid(VkCommandPool)) return VK_OBJECT_TYPE_COMMAND_POOL;

	return VK_OBJECT_TYPE_UNKNOWN;
}

namespace VulkanContext
{
	void SetObjectName(VkObjectType object_type, uint64_t object_handle, const char* object_name);
}

namespace vk
{
	template <typename T>
	struct VkObject
	{
	protected:
		T m_Handle = VK_NULL_HANDLE;
	public:
		VkObject() = default;
		VkObject(T handle) { m_Handle = handle; }
	
		operator bool() const { return m_Handle != VK_NULL_HANDLE; }
		VkObjectType GetType() const { return GetObjectType<T>(); }
	
		void SetName(const char* name) const { VulkanContext::SetObjectName(GetType(), (uint64_t)m_Handle, name); }
		void SetName(const std::string& name) const { SetName(name.c_str()); }
	
		operator T& () { return m_Handle; }
		operator const T& () const { return m_Handle; }
	
		T* operator*() { return &m_Handle; }
		T const* operator*() const { return &m_Handle; }
	
		T* operator&() { return &m_Handle; }
		const T* operator&() const { return &m_Handle; }
	};

	class PhysicalDevice : public VkObject<VkPhysicalDevice>
	{
		VkPhysicalDeviceFeatures features = {};
		VkPhysicalDeviceProperties properties = {};
		VkPhysicalDeviceProperties2 properties2 = {};
		VkPhysicalDeviceMemoryProperties memoryProperties = {};
	public:

		using VkObject<VkPhysicalDevice>::VkObject;

		void SetDevice(VkPhysicalDevice physicalDevice)
		{
			m_Handle = physicalDevice;
			vkGetPhysicalDeviceFeatures(m_Handle, &features);
			vkGetPhysicalDeviceProperties(m_Handle, &properties);
			vkGetPhysicalDeviceMemoryProperties(m_Handle, &memoryProperties);
		}

		VkResult GetImageFormatProperties(VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties)
		{
			return vkGetPhysicalDeviceImageFormatProperties(m_Handle, format, type, tiling, usage, flags, pImageFormatProperties);
		}

		void GetQueueFamilyProperties(uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties* pQueueFamilyProperties)
		{
			vkGetPhysicalDeviceQueueFamilyProperties(m_Handle, pQueueFamilyPropertyCount, pQueueFamilyProperties);
		}

		std::vector<VkQueueFamilyProperties> GetQueueFamilyProperties()
		{
			uint32_t count = 0;
			GetQueueFamilyProperties(&count, nullptr); // Query the count of queue family properties

			std::vector<VkQueueFamilyProperties> data(count);
			GetQueueFamilyProperties(&count, data.data());

			return data;
		}

		VkFormatProperties GetFormatProperties(VkFormat format)
		{
			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(m_Handle, format, &formatProperties);
			return formatProperties;
		}

		void QueryProperties2(void* pNext = nullptr)
		{
			properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			properties2.pNext = pNext;
			vkGetPhysicalDeviceProperties2(m_Handle, &properties2);
		}

		auto GetFeatures() const { return features; }

		auto GetProperties() const { return properties; }
		auto GetProperties2() const { return properties2; }

		auto GetMemoryProperties() const { return memoryProperties; }
	};


	struct LogicalDevice : public VkObject<VkDevice> 
	{
		VkResult Create(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo& createInfo) { return vkCreateDevice(physicalDevice, &createInfo, nullptr, &m_Handle); }

		PFN_vkVoidFunction GetProcAddress(const char* pName) { return vkGetDeviceProcAddr(m_Handle, pName); }

		void WaitIdle()	{ vkDeviceWaitIdle(m_Handle); }

		void Destroy() { vkDestroyDevice(m_Handle, nullptr); }
	};

	struct Queue : public vk::VkObject<VkQueue>
	{
		uint32_t queueFamily = 0;

		VkResult Submit(const VkSubmitInfo& submit_info, VkFence fence = VK_NULL_HANDLE) const { return vkQueueSubmit(m_Handle, 1, &submit_info, fence); }

		VkResult PresentKHR(const VkPresentInfoKHR& present_info) const { return vkQueuePresentKHR(m_Handle, &present_info); }

		void WaitIdle() const { vkQueueWaitIdle(m_Handle); }
	};

	inline Queue GraphicsQueue;
	//inline Queue PresentQueue;
	inline Queue ComputeQueue;
	inline Queue TransferQueue;
}

namespace VulkanContext
{
	inline VkInstance instance;
	inline vk::PhysicalDevice physicalDevice;
	inline vk::LogicalDevice logicalDevice;

	inline VmaAllocator MemoryAllocator;

	inline auto& GetInstance() { return instance; }
	inline auto& GetPhysicalDevice() { return physicalDevice; }
	inline auto& GetLogicalDevice() { return logicalDevice; }
	inline auto& GetMemoryAllocator() { return MemoryAllocator; }
	inline VkAllocationCallbacks* GetAllocator() { return nullptr; }
	inline auto GetProperties() { return physicalDevice.GetProperties(); }
	inline auto GetSupportedFeatures() { return physicalDevice.GetFeatures(); }		

#if WC_GRAPHICS_VALIDATION
	inline bool bValidationLayers = false;
	inline VkDebugUtilsMessengerEXT debug_messenger;
#endif	

	inline void BeginLabel(VkCommandBuffer cmd, const char* labelName, const glm::vec4& color = glm::vec4(1.f))
	{
#if WC_GRAPHICS_VALIDATION
		if (bValidationLayers)
		{
			VkDebugUtilsLabelEXT label = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
			label.pLabelName = labelName;
			label.color[0] = color[0];
			label.color[1] = color[1];
			label.color[2] = color[2];
			label.color[3] = color[3];
			vkCmdBeginDebugUtilsLabelEXT(cmd, &label);
		}
#endif
	}

	inline void InsertLabel(VkCommandBuffer cmd, const char* labelName, const glm::vec4& color = glm::vec4(1.f))
	{
#if WC_GRAPHICS_VALIDATION
		if (bValidationLayers)
		{
			VkDebugUtilsLabelEXT label = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
			label.pLabelName = labelName;
			label.color[0] = color[0];
			label.color[1] = color[1];
			label.color[2] = color[2];
			label.color[3] = color[3];
			vkCmdInsertDebugUtilsLabelEXT(cmd, &label);
		}
#endif
	}

	inline void EndLabel(VkCommandBuffer cmd) 
	{
#if WC_GRAPHICS_VALIDATION
		if (bValidationLayers) vkCmdEndDebugUtilsLabelEXT(cmd);
#endif
	}


	inline void BeginLabel(VkQueue queue, const char* labelName, const glm::vec4& color = glm::vec4(1.f))
	{
#if WC_GRAPHICS_VALIDATION
		if (bValidationLayers)
		{
			VkDebugUtilsLabelEXT label = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
			label.pLabelName = labelName;
			label.color[0] = color[0];
			label.color[1] = color[1];
			label.color[2] = color[2];
			label.color[3] = color[3];
			vkQueueBeginDebugUtilsLabelEXT(queue, &label);
		}
#endif
	}

	inline void InsertLabel(VkQueue queue, const char* labelName, const glm::vec4& color = glm::vec4(1.f))
	{
#if WC_GRAPHICS_VALIDATION
		if (bValidationLayers)
		{
			VkDebugUtilsLabelEXT label = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
			label.pLabelName = labelName;
			label.color[0] = color[0];
			label.color[1] = color[1];
			label.color[2] = color[2];
			label.color[3] = color[3];
			vkQueueInsertDebugUtilsLabelEXT(queue, &label);
		}
#endif
	}

	inline void EndLabel(VkQueue queue) 
	{
#if WC_GRAPHICS_VALIDATION
		if (bValidationLayers) vkQueueEndDebugUtilsLabelEXT(queue);
#endif
	}

	inline void SetObjectName(VkObjectType type, uint64_t handle, const char* name)
	{
#if WC_GRAPHICS_VALIDATION
		if (bValidationLayers)
		{
			VkDebugUtilsObjectNameInfoEXT name_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
			name_info.objectType = type;
			name_info.objectHandle = handle;
			name_info.pObjectName = name;
			vkSetDebugUtilsObjectNameEXT(GetLogicalDevice(), &name_info);
		}
#endif
	}

	template<class T>
	inline void SetObjectName(T handle, const char* name) { SetObjectName(GetObjectType<T>(), (uint64_t)handle, name); }

	template<class T>
	inline void SetObjectName(T handle, const std::string& name) { SetObjectName(handle, name.c_str()); }	

	inline bool Create()
	{
		if (volkInitialize() != VK_SUCCESS)
		{
			WC_CORE_ERROR("Failed to initialise volk!");
			return false;
		}
		// Create Instance
#if WC_GRAPHICS_VALIDATION
		std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers)
			for (const auto& layerProperties : availableLayers)
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					bValidationLayers = true;
					break;
				}

		if (!bValidationLayers)
		{
			WC_CORE_ERROR("Validation layers requested, but not available!");
			return false;
		}
#endif

		VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
		appInfo.pApplicationName = "WC Application";
		appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 2, 0);
		appInfo.pEngineName = "WC Engine";
		appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 2, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		VkInstanceCreateInfo instanceCreateInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		instanceCreateInfo.pApplicationInfo = &appInfo;

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#if WC_GRAPHICS_VALIDATION
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (bValidationLayers)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);


			instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();

			debugCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
			debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT; // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT?

#if WC_SHADER_DEBUG_PRINT
			debugCreateInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
#endif

			debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debugCreateInfo.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT severity,
				VkDebugUtilsMessageTypeFlagsEXT messageType,
				const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
				void* pUserData) -> VkBool32 VKAPI_CALL
				{
					std::string type;
					switch (messageType)
					{
						case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
							type = "General";
							break;
						case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
							type = "Validation";
							break;
						case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
							type = "Performance";
							break;
						default:
							type = "Unknown";
							break;
					}

					switch (severity)
					{
					case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
						WC_CORE_ERROR("[{}] {}", type, pCallbackData->pMessage);
						//WC_DEBUGBREAK();
						//OutputDebugString(pCallbackData->pMessage);
						break;

					case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
						WC_CORE_WARN("[{}] {}", type, pCallbackData->pMessage);
						break;

					case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
						WC_CORE_TRACE("[{}] {}", type, pCallbackData->pMessage);
						break;

					case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
						//WC_CORE_TRACE(pCallbackData->pMessage);
						break;
					}

					return true;
				};

			VkValidationFeatureEnableEXT enabledFeatures[] = {
				VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
#if WC_SYNCHRONIZATION_VALIDATION
				VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
#endif
				//VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT
			};

			//VkValidationFeatureDisableEXT disabledFeatures[] = {};

			VkValidationFeaturesEXT validationFeatures = {
				.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
				.pNext = nullptr,
				.enabledValidationFeatureCount = (uint32_t)std::size(enabledFeatures),
				.pEnabledValidationFeatures = enabledFeatures,			
				//.disabledValidationFeatureCount = (uint32_t)std::size(disabledFeatures);
				//.pDisabledValidationFeatures = disabledFeatures;
			};

			debugCreateInfo.pNext = &validationFeatures;

			instanceCreateInfo.pNext = &debugCreateInfo;
		}
#endif


		instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

		if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance) != VK_SUCCESS)
		{
			WC_CORE_ERROR("Failed to create instance!");
			return false;
		}

		volkLoadInstance(instance);		

		const std::vector<const char*> deviceExtensions = {	VK_KHR_SWAPCHAIN_EXTENSION_NAME, };

#if WC_GRAPHICS_VALIDATION
		if (bValidationLayers)
		{
			debugCreateInfo.pNext = nullptr; // by spec definition
			if (vkCreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, VulkanContext::GetAllocator(), &debug_messenger) != VK_SUCCESS)
			{
				WC_CORE_ERROR("Failed to set up debug messenger!");
				return false;
			}

#if WC_SHADER_DEBUG_PRINT
			deviceExtensions.push_back(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME);
#endif
		}
#endif		

		// Pick physical device 
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0)
		{
			WC_CORE_ERROR("Failed to find GPUs with Vulkan support!");
			return false;
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		struct
		{
			std::optional<uint32_t> graphicsFamily;
			//std::optional<uint32_t> presentFamily;
			std::optional<uint32_t> computeFamily;
			std::optional<uint32_t> transferFamily;

			bool IsComplete() { return graphicsFamily.has_value() && /*presentFamily.has_value() &&*/ computeFamily.has_value() && transferFamily.has_value(); }
			void Reset()
			{
				graphicsFamily.reset();
				//presentFamily.reset();
				computeFamily.reset();
				transferFamily.reset();
			}
		} indices;							

		for (const auto& physDevice : devices)
		{				
			uint32_t extensionCount;
			vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, nullptr);

			std::vector<VkExtensionProperties> availableExtensions(extensionCount);
			vkEnumerateDeviceExtensionProperties(physDevice, nullptr, &extensionCount, availableExtensions.data());

			bool extensionsSupported = true;
			for (const auto& requiredExtension : deviceExtensions)
			{
				bool found = false;
				for (const auto& availableExtension : availableExtensions)
				{
					if (requiredExtension == std::string(availableExtension.extensionName))
					{
						found = true;
						break;
					}
				}

				if (!found)
				{
					extensionsSupported = false;
					break;
				}
			}
						
			indices.Reset();
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, queueFamilies.data());

			for (uint32_t i = 0; i < queueFamilies.size(); i++)
			{
				if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)  indices.graphicsFamily = i;
				if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)   indices.computeFamily = i;
				if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)  indices.transferFamily = i;

				if (indices.IsComplete())
					break;
			}

			//bool swapChainAdequate = false;
			//if (extensionsSupported) {
			//	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
			//	swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
			//}

			if (indices.IsComplete() && extensionsSupported /*&& swapChainAdequate*/)
			{
				physicalDevice = physDevice;

				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE)
		{
			WC_CORE_ERROR("Failed to find a suitable GPU!");
			return false;
		}
		
		{ // Create Logical Device	
			VkPhysicalDeviceFeatures deviceFeatures = {};
			if (physicalDevice.GetFeatures().samplerAnisotropy) deviceFeatures.samplerAnisotropy = true;

			VkPhysicalDeviceVulkan12Features features12 = { 
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,

				.shaderSampledImageArrayNonUniformIndexing = true,
				.descriptorBindingPartiallyBound = true,
				.descriptorBindingVariableDescriptorCount = true,
				.runtimeDescriptorArray = true,
				.scalarBlockLayout = true,
				.timelineSemaphore = true,
				.bufferDeviceAddress = true,
			};

			VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };

			std::set<uint32_t> uniqueQueueFamilies = {
				indices.graphicsFamily.value(),
				//indices.presentFamily.value(),
				indices.computeFamily.value(),
				indices.transferFamily.value(),
			};

			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
			float queuePriorities[] = { 1.f };
			for (const auto& queueFamily : uniqueQueueFamilies)
			{
				VkDeviceQueueCreateInfo queueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
				queueCreateInfo.queueFamilyIndex = queueFamily;
				queueCreateInfo.queueCount = (uint32_t)std::size(queuePriorities);
				queueCreateInfo.pQueuePriorities = queuePriorities;

				queueCreateInfos.push_back(queueCreateInfo);
			}

			createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
			createInfo.pQueueCreateInfos = queueCreateInfos.data();

			createInfo.pEnabledFeatures = &deviceFeatures;

			createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
			createInfo.ppEnabledExtensionNames = deviceExtensions.data();

			createInfo.pNext = &features12;

#if WC_GRAPHICS_VALIDATION
			if (bValidationLayers)
			{
				createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
				createInfo.ppEnabledLayerNames = validationLayers.data();
			}
#endif

			if (logicalDevice.Create(physicalDevice, createInfo) != VK_SUCCESS)
			{
				WC_CORE_ERROR("Failed to create logical device!");
				return false;
			}

			auto GetDeviceQueue = [](vk::Queue& q, uint32_t family)
				{
					q.queueFamily = family;
					vkGetDeviceQueue(GetLogicalDevice(), family, 0, &q);
				};

			GetDeviceQueue(vk::GraphicsQueue, indices.graphicsFamily.value());
			//presentQueue.GetDeviceQueue(indices.presentFamily.value());
			GetDeviceQueue(vk::ComputeQueue, indices.computeFamily.value());
			GetDeviceQueue(vk::TransferQueue, indices.transferFamily.value());

			vk::GraphicsQueue.SetName("GraphicsQueue");
			vk::ComputeQueue.SetName("ComputeQueue");
			vk::TransferQueue.SetName("TransferQueue");
		}

		VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.vulkanApiVersion = appInfo.apiVersion;
		allocatorInfo.physicalDevice = physicalDevice;
		allocatorInfo.device = GetLogicalDevice();
		allocatorInfo.instance = instance;
		allocatorInfo.pVulkanFunctions = &vulkanFunctions;
		allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		if (vmaCreateAllocator(&allocatorInfo, &MemoryAllocator) != VK_SUCCESS)
		{
			WC_CORE_ERROR("Failed to create a memory allocator!");
			return false;
		}

		return true;
	}

	inline void Destroy()
	{
		vmaDestroyAllocator(MemoryAllocator);
		logicalDevice.Destroy();

#if WC_GRAPHICS_VALIDATION
		if (bValidationLayers) vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, VulkanContext::GetAllocator());
#endif
		vkDestroyInstance(instance, nullptr);
		volkFinalize();
	}
}