#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>
#include <map>
#include <optional>
// global const
const int		WIDTH		= 800;
const int		HEIGHT		= 600;

// for validation layer
const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

#ifdef _DEBUG
const bool enableValidationLayer = true;
#else
const bool enableValidationLayer = false;
#endif // #ifdef NDEBUG

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, 
									  const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
									  const VkAllocationCallbacks* pAllocator, 
									  VkDebugUtilsMessengerEXT* pDebugMessenger) 
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, 
								   VkDebugUtilsMessengerEXT debugMessenger, 
								   const VkAllocationCallbacks* pAllocator) 
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

class HelloTriangleApplication
{
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;

		bool isComplete()
		{
			return graphicsFamily.has_value();
		}
	};
public:
	void Run()
	{
		_initWindow();
		_initVulkan();
		_mainLoop();
		_cleanup();
	}

private:
	GLFWwindow*							_window;

	// vulkan
	VkInstance							_instance;

	// debug messenger
	VkDebugUtilsMessengerEXT			debugMessenger;

	// physical device-->gpu graphics card
	VkPhysicalDevice					_physicalDevice = VK_NULL_HANDLE;

	// logic device
	VkDevice							_device;

	// queue handle
	VkQueue								_graphicsQueue;
private:

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserdata)
	{
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}
	void _initWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);	// no openGL api
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);		// no resize

		_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	}

	bool _checkValidationLayerSupport()
	{
		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availibleLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availibleLayers.data());

		bool layerFound = false;
		for (const char* layerName : validationLayers)
		{
			for (const auto& layerProperties : availibleLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

		}
		if (!layerFound)
			return false;

		return true;
	}

	std::vector<const char*> getRequiredExtensions()
	{
		uint32_t gfwExtensionCount = 0;
		const char** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&gfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + gfwExtensionCount);
		if (enableValidationLayer)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	void _createInstance()
	{
		if (enableValidationLayer && !_checkValidationLayerSupport())
		{
			throw std::runtime_error("validation layers requested, but not availible");
		}
		VkApplicationInfo appInfo = {};
		appInfo.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName	= "Hello Triangle";
		appInfo.applicationVersion	= VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName			= "No Engine";
		appInfo.engineVersion		= VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion			= VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType			= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = getRequiredExtensions();

		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames  = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (enableValidationLayer)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			_populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		
		if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create instance!");
		}		
	}

	void _populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
	}

	void _initVulkan()
	{
		_createInstance();
		_setupMessenger();
		_pickPhysicalDevice();
		_createLogicDevice();
	}

	//====================== Physical Device ==========================
	void _pickPhysicalDevice() // graphics card choose(GPU)
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr); // get the number of GPUs first

		// do we have any GPUs with Vulkan support?
		if (deviceCount == 0)
		{
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");
		}

		// fill with all the GPUs
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

		for (const auto& device : devices)
		{
			if (_isDeviceSuitable(device))
			{
				_physicalDevice = device;
				break;
			}
		}

		// if no suitable GPU
		if (_physicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("Failed to find a suitable GPU!");
		}

		// use an ordered map to automatically sort candidates by increasing score
		std::multimap<int, VkPhysicalDevice> candidates;

		for (const auto& device : devices)
		{
			int score = _rateDeviceSuitability(device);
			candidates.insert(std::make_pair(score, device));
		}

		// check if the best candidate is suitable alt all
		if (candidates.rbegin()->first > 0)
		{
			_physicalDevice = candidates.rbegin()->second;
		}
		else
		{
			throw std::runtime_error("Failed to find a suitalbe GPU");
		}
	}

	int _rateDeviceSuitability(VkPhysicalDevice device)
	{
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
		int score = 0;

		// discrete GPUs have a significant performance advantage
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			score += 1000;
		}

		// maximum possible size of textures affects graphics quality
		score += deviceProperties.limits.maxImageDimension2D;

		// application can't funcion without geometry shaders
		if (!deviceFeatures.geometryShader)
		{
			return 0;
		}

		return score;
	}

	bool _isDeviceSuitable(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices = _findQueueFamily(device);

		return indices.isComplete();
	}

	//====================== Queue Family ==========================
	QueueFamilyIndices _findQueueFamily(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
			}
			if (indices.isComplete())
				break;
			i++;
		}
		return indices;
	}

	//====================== Logic Device ==========================
	void _createLogicDevice()
	{
		QueueFamilyIndices indices = _findQueueFamily(_physicalDevice);

		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value(); // only graphics queue now
		queueCreateInfo.queueCount = 1;

		float queuePriority = 1.0f;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		VkPhysicalDeviceFeatures deviceFeatures = {};

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
		deviceCreateInfo.queueCreateInfoCount = 1;

		deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

		deviceCreateInfo.enabledExtensionCount= 0;

		if (enableValidationLayer)
		{
			deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			deviceCreateInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(_physicalDevice, &deviceCreateInfo, nullptr, &_device) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create logical device");
		}

		// retrieving queue handle
		vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &_graphicsQueue);
	}
	void _setupMessenger()
	{
		if (!enableValidationLayer) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		_populateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(_instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to set up debug messenger");
		}
	}

	void _mainLoop()
	{
		while (!glfwWindowShouldClose(_window))
		{

		}
	}

	void _cleanup()
	{
		vkDestroyDevice(_device, nullptr);

		if (enableValidationLayer)
		{
			DestroyDebugUtilsMessengerEXT(_instance, debugMessenger, nullptr);
		}

		vkDestroyInstance(_instance, nullptr);

		glfwDestroyWindow(_window);

		glfwTerminate();
	}
};

int main()
{
	HelloTriangleApplication app;

	try
	{
		app.Run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
