#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const char *TITLE = "Vulkan Probe";

const char *validationLayers[] = {
    "VK_LAYER_KHRONOS_validation",
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

#ifdef __APPLE__
#ifndef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#define VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME "VK_KHR_portability_subset"
#endif
#endif

typedef struct App
{
    GLFWwindow *window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
} App;

typedef enum AppResult
{
    APP_SUCCESS = 0,
    APP_ERROR_MALLOC = 1,
    APP_ERROR_GLFW_INIT = 2,
    APP_ERROR_GLFW_WINDOW = 3,
    APP_ERROR_VULKAN_INSTANCE = 4,
    APP_ERROR_VULKAN_ENUM_INSTANCE_EXT_PROP = 5,
    APP_ERROR_VULKAN_ENUM_INSTANCE_LAYER_PROP = 6,
    APP_ERROR_VULKAN_VALIDATION_LAYER_NOT_FOUND = 7,
    APP_ERROR_VULKAN_NO_PHYSICAL_DEVICE = 8,
    APP_ERROR_VULKAN_ENUM_PHYSICAL_DEVICE = 9,
    APP_ERROR_VULKAN_LOGICAL_DEVICE = 10,
    APP_ERROR_VULKAN_NO_GRAPHICS_QUEUE_FAMILY = 11,
    APP_ERROR_VULKAN_CREATE_SURFACE = 12,
    APP_ERROR_VULKAN_PRESENTATION_SUPPORT = 13,
} AppResult;

AppResult initGLFW(App *app);
AppResult initVulkan(App *app);
AppResult checkValidationLayerSupport(void);
AppResult selectPhysicalDevice(App *app);
AppResult getGraphicsQueueFamilyIndices(VkPhysicalDevice device, uint32_t *queueFamilyIndicesCount, uint32_t *queueFamilyIndices);
uint32_t computeDeviceScore(VkPhysicalDevice device);
AppResult deviceHasGraphicsQueueFamily(VkPhysicalDevice device, bool *hasGraphicsQueueFamily);
AppResult createLogicalDevice(App *app);
AppResult createSurface(App *app);
AppResult cleanup(App *app, AppResult result);
AppResult deviceHasPresentationQueueFamily(VkPhysicalDevice device, VkSurfaceKHR surface, bool *hasPresentationQueueFamily);
AppResult getPresentationQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t *pQueueFamilyIndicesCount, uint32_t *pQueueFamilyIndices);

int main(void)
{
#ifdef NDEBUG // pass -DNDEBUG to the compiler when building in release mode
    printf("Running in release mode\n");
#else
    printf("Running in debug mode\n");
#endif

    App app = {0};

    AppResult result;

    result = initGLFW(&app);
    if (result != APP_SUCCESS)
        return cleanup(&app, result);

    result = initVulkan(&app);
    if (result != APP_SUCCESS)
        return cleanup(&app, result);

    // Below could be moved inside initVulkan
    result = createSurface(&app);
    if (result != APP_SUCCESS)
        return cleanup(&app, result);

    result = selectPhysicalDevice(&app);
    if (result != APP_SUCCESS)
        return cleanup(&app, result);

    result = createLogicalDevice(&app);
    if (result != APP_SUCCESS)
        return cleanup(&app, result);

    // Graphics queue family idices
    printf("Graphics queue family indices:\n");
    uint32_t graphicsQueueIndicesCount = 0;
    result = getGraphicsQueueFamilyIndices(app.physicalDevice, &graphicsQueueIndicesCount, NULL);
    if (result != APP_SUCCESS)
        return cleanup(&app, result);
    uint32_t graphicsQueueIndices[graphicsQueueIndicesCount];
    result = getGraphicsQueueFamilyIndices(app.physicalDevice, &graphicsQueueIndicesCount, graphicsQueueIndices);
    if (result != APP_SUCCESS)
        return cleanup(&app, result);
    for (uint32_t i = 0; i < graphicsQueueIndicesCount; ++i)
    {
        printf("Graphics queue index: %d\n", graphicsQueueIndices[i]);
    }

    // Presentation queue family idices
    printf("Presentation queue family indices:\n");
    uint32_t presentationQueueIndicesCount = 0;
    result = getPresentationQueueFamilyIndices(app.physicalDevice, app.surface, &presentationQueueIndicesCount, NULL);
    if (result != APP_SUCCESS)
        return cleanup(&app, result);
    uint32_t presentationQueueIndices[presentationQueueIndicesCount];
    result = getPresentationQueueFamilyIndices(app.physicalDevice, app.surface, &presentationQueueIndicesCount, presentationQueueIndices);
    if (result != APP_SUCCESS)
        return cleanup(&app, result);
    for (uint32_t i = 0; i < presentationQueueIndicesCount; ++i)
    {
        printf("Presentation queue index: %d\n", presentationQueueIndices[i]);
    }

    // Above could be moved inside initVulkan

    // Main loop
    while (!glfwWindowShouldClose(app.window))
    {
        glfwPollEvents();
    }

    return (int)cleanup(&app, APP_SUCCESS);
} // main

AppResult createSurface(App *app)
{
    if (glfwCreateWindowSurface(app->instance, app->window, NULL, &app->surface) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create window surface\n");
        return APP_ERROR_VULKAN_CREATE_SURFACE;
    }
    return APP_SUCCESS;
}

AppResult createLogicalDevice(App *app)
{
    uint32_t graphicsQueueIndicesCount = 0;
    AppResult result = getGraphicsQueueFamilyIndices(app->physicalDevice, &graphicsQueueIndicesCount, NULL);
    if (result != APP_SUCCESS)
        return result;

    if (graphicsQueueIndicesCount == 0)
    {
        fprintf(stderr, "No graphics queue families found\n");
        return APP_ERROR_VULKAN_NO_GRAPHICS_QUEUE_FAMILY;
    }

    uint32_t *graphicsQueueIndices = malloc(sizeof(uint32_t) * graphicsQueueIndicesCount);
    if (!graphicsQueueIndices)
    {
        fprintf(stderr, "Failed to allocate memory for graphics queue indices\n");
        return APP_ERROR_MALLOC;
    }

    result = getGraphicsQueueFamilyIndices(app->physicalDevice, &graphicsQueueIndicesCount, graphicsQueueIndices);
    if (result != APP_SUCCESS)
    {
        free(graphicsQueueIndices);
        return result;
    }

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = graphicsQueueIndices[0], // Use the first graphics queue family
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };

    VkPhysicalDeviceFeatures deviceFeatures = {0}; // No special features for now

    // Enumerate device extensions
    uint32_t deviceExtensionCount = 0;
    vkEnumerateDeviceExtensionProperties(app->physicalDevice, NULL, &deviceExtensionCount, NULL);

    VkExtensionProperties *availableDeviceExtensions = malloc(sizeof(VkExtensionProperties) * deviceExtensionCount);
    if (!availableDeviceExtensions)
    {
        fprintf(stderr, "Failed to allocate memory for device extensions\n");
        free(graphicsQueueIndices);
        return APP_ERROR_MALLOC;
    }

    vkEnumerateDeviceExtensionProperties(app->physicalDevice, NULL, &deviceExtensionCount, availableDeviceExtensions);

    // Determine required device extensions
    uint32_t requiredDeviceExtensionCount = 0;
    const char **requiredDeviceExtensions = NULL;

#ifdef __APPLE__
    bool portabilitySubsetExtensionFound = false;
    for (uint32_t i = 0; i < deviceExtensionCount; ++i)
    {
        if (strcmp(availableDeviceExtensions[i].extensionName, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME) == 0)
        {
            portabilitySubsetExtensionFound = true;
            break;
        }
    }

    if (portabilitySubsetExtensionFound)
    {
        requiredDeviceExtensionCount = 1;
        requiredDeviceExtensions = malloc(sizeof(char *) * requiredDeviceExtensionCount);
        if (!requiredDeviceExtensions)
        {
            fprintf(stderr, "Failed to allocate memory for required device extensions\n");
            free(availableDeviceExtensions);
            free(graphicsQueueIndices);
            return APP_ERROR_MALLOC;
        }
        requiredDeviceExtensions[0] = VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME;
    }
#else
    requiredDeviceExtensionCount = 0;
    requiredDeviceExtensions = NULL;
#endif

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .pEnabledFeatures = &deviceFeatures,
        .enabledExtensionCount = requiredDeviceExtensionCount,
        .ppEnabledExtensionNames = requiredDeviceExtensions,
    };

    if (enableValidationLayers)
    {
        deviceCreateInfo.enabledLayerCount = sizeof(validationLayers) / sizeof(validationLayers[0]);
        deviceCreateInfo.ppEnabledLayerNames = validationLayers;
    }
    else
    {
        deviceCreateInfo.enabledLayerCount = 0;
        deviceCreateInfo.ppEnabledLayerNames = NULL;
    }

    VkResult vkResult = vkCreateDevice(app->physicalDevice, &deviceCreateInfo, NULL, &app->logicalDevice);
    if (vkResult != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create logical device: %d\n", vkResult);
#ifdef __APPLE__
        free(requiredDeviceExtensions);
#endif
        free(availableDeviceExtensions);
        free(graphicsQueueIndices);
        return APP_ERROR_VULKAN_LOGICAL_DEVICE;
    }

    vkGetDeviceQueue(app->logicalDevice, graphicsQueueIndices[0], 0, &app->graphicsQueue);

#ifdef __APPLE__
    free(requiredDeviceExtensions);
#endif
    free(availableDeviceExtensions);
    free(graphicsQueueIndices);

    return APP_SUCCESS;
}

AppResult getGraphicsQueueFamilyIndices(VkPhysicalDevice device, uint32_t *pQueueFamilyIndicesCount, uint32_t *pQueueFamilyIndices)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

    VkQueueFamilyProperties queueFamiliesArr[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamiliesArr);

    uint32_t count = 0;
    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        if (queueFamiliesArr[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            if (pQueueFamilyIndices != NULL)
            {
                pQueueFamilyIndices[count] = i;
            }
            count++;
        }
    }

    *pQueueFamilyIndicesCount = count;

    return APP_SUCCESS;
} // getGraphicsQueueFamilyIndices

AppResult getPresentationQueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t *pQueueFamilyIndicesCount, uint32_t *pQueueFamilyIndices)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

    VkQueueFamilyProperties queueFamiliesArr[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamiliesArr);

    uint32_t count = 0;
    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        VkBool32 presentationSupported = VK_FALSE;
        VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupported);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "Failed to check presentation support: %d\n", result);
            return APP_ERROR_VULKAN_ENUM_PHYSICAL_DEVICE;
        }

        if (presentationSupported)
        {
            if (pQueueFamilyIndices != NULL)
            {
                pQueueFamilyIndices[count] = i;
            }
            count++;
        }
    }

    *pQueueFamilyIndicesCount = count;

    return APP_SUCCESS;
} // getPresentationQueueFamilyIndices

AppResult selectPhysicalDevice(App *app)
{
    uint32_t deviceCount = 0;
    VkResult vkResult = vkEnumeratePhysicalDevices(app->instance, &deviceCount, NULL);
    if (vkResult != VK_SUCCESS || deviceCount == 0)
    {
        fprintf(stderr, "Failed to find GPUs with Vulkan support\n");
        return APP_ERROR_VULKAN_NO_PHYSICAL_DEVICE;
    }

    VkPhysicalDevice deviceArr[deviceCount];
    vkResult = vkEnumeratePhysicalDevices(app->instance, &deviceCount, deviceArr);
    if (vkResult != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to enumerate physical devices: %d\n", vkResult);
        return APP_ERROR_VULKAN_ENUM_PHYSICAL_DEVICE;
    }

    VkPhysicalDevice selectedDevice = VK_NULL_HANDLE;
    uint32_t maxScore = 0;
    for (uint32_t i = 0; i < deviceCount; ++i)
    {
        bool hasGraphicsQueueFamily = false;
        AppResult appResult = deviceHasGraphicsQueueFamily(deviceArr[i], &hasGraphicsQueueFamily);
        if (appResult != APP_SUCCESS)
        {
            return appResult;
        }

        bool hasPresentationQueueFamily = false;
        appResult = deviceHasPresentationQueueFamily(deviceArr[i], app->surface, &hasPresentationQueueFamily);
        if (appResult != APP_SUCCESS)
        {
            return appResult;
        }

        if (hasGraphicsQueueFamily && hasPresentationQueueFamily)
        {
            uint32_t deviceScore = computeDeviceScore(deviceArr[i]);
            if (deviceScore > maxScore)
            {
                maxScore = deviceScore;
                selectedDevice = deviceArr[i];
            }
        }
    }

    if (selectedDevice == VK_NULL_HANDLE)
    {
        fprintf(stderr, "Failed to find a suitable device\n");
        return APP_ERROR_VULKAN_NO_PHYSICAL_DEVICE;
    }

    app->physicalDevice = selectedDevice;
    return APP_SUCCESS;
} // selectPhysicalDevice
AppResult deviceHasPresentationQueueFamily(VkPhysicalDevice device, VkSurfaceKHR surface, bool *hasPresentationQueueFamily)
{
    *hasPresentationQueueFamily = false;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

    VkQueueFamilyProperties queueFamiliesArr[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamiliesArr);

    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        VkBool32 presentationSupported = VK_FALSE;
        VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupported);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "Failed to check presentation support: %d\n", result);
            return APP_ERROR_VULKAN_PRESENTATION_SUPPORT;
        }

        if (presentationSupported)
        {
            *hasPresentationQueueFamily = true;
            break;
        }
    }

    return APP_SUCCESS;
}

AppResult deviceHasGraphicsQueueFamily(VkPhysicalDevice device, bool *hasGraphicsQueueFamily)
{
    *hasGraphicsQueueFamily = false;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

    VkQueueFamilyProperties queueFamiliesArr[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamiliesArr);

    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        if (queueFamiliesArr[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            *hasGraphicsQueueFamily = true;
            break;
        }
    }

    return APP_SUCCESS;
} // deviceHasGraphicsQueueFamily

uint32_t computeDeviceScore(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    uint32_t deviceScore = 0;

    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        deviceScore += 1000;
    }
    else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    {
        deviceScore += 500;
    }
    else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
    {
        deviceScore += 250;
    }
    else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
    {
        deviceScore += 100;
    }

    deviceScore += deviceProperties.limits.maxImageDimension2D;

    return deviceScore;
} // computeDeviceScore

AppResult checkValidationLayerSupport(void)
{
    uint32_t layerCount;
    VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, NULL);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to enumerate instance layer properties: %d\n", result);
        return APP_ERROR_VULKAN_ENUM_INSTANCE_LAYER_PROP;
    }

    VkLayerProperties availableLayersArr[layerCount];
    result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayersArr);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to enumerate instance layer properties: %d\n", result);
        return APP_ERROR_VULKAN_ENUM_INSTANCE_LAYER_PROP;
    }

    for (uint32_t i = 0; i < sizeof(validationLayers) / sizeof(validationLayers[0]); ++i)
    {
        bool layerFound = false;
        for (uint32_t j = 0; j < layerCount; ++j)
        {
            if (strcmp(validationLayers[i], availableLayersArr[j].layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            fprintf(stderr, "ERROR: Validation layer %s not found\n", validationLayers[i]);
            return APP_ERROR_VULKAN_VALIDATION_LAYER_NOT_FOUND;
        }
    }

    return APP_SUCCESS;
} // checkValidationLayerSupport

AppResult initGLFW(App *app)
{
    if (!glfwInit())
    {
        fprintf(stderr, "ERROR: Failed to initialize GLFW\n");
        return APP_ERROR_GLFW_INIT;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    app->window = glfwCreateWindow(WIDTH, HEIGHT, TITLE, NULL, NULL);
    if (!app->window)
    {
        fprintf(stderr, "ERROR: Failed to create GLFW window\n");
        return APP_ERROR_GLFW_WINDOW;
    }

    return APP_SUCCESS;
} // initGLFW

AppResult initVulkan(App *app)
{
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = TITLE,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };

    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    uint32_t requiredExtensionCount = glfwExtensionCount;
    const char **requiredExtensions = NULL;

    // Enumerate available instance extensions
    uint32_t availableExtensionCount = 0;
    VkResult result = vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, NULL);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to enumerate instance extension properties: %d\n", result);
        return APP_ERROR_VULKAN_ENUM_INSTANCE_EXT_PROP;
    }

    VkExtensionProperties *availableExtensions = malloc(sizeof(VkExtensionProperties) * availableExtensionCount);
    if (!availableExtensions)
    {
        fprintf(stderr, "Failed to allocate memory for instance extensions\n");
        return APP_ERROR_MALLOC;
    }

    result = vkEnumerateInstanceExtensionProperties(NULL, &availableExtensionCount, availableExtensions);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to enumerate instance extension properties: %d\n", result);
        free(availableExtensions);
        return APP_ERROR_VULKAN_ENUM_INSTANCE_EXT_PROP;
    }

#ifdef __APPLE__
// Define the missing macro if necessary
#ifndef VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
#define VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME "VK_KHR_get_physical_device_properties2"
#endif

    // Additional required extensions
    const char *additionalExtensions[] = {
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};

    uint32_t additionalExtensionCount = sizeof(additionalExtensions) / sizeof(additionalExtensions[0]);

    requiredExtensionCount += additionalExtensionCount;
    requiredExtensions = malloc(sizeof(char *) * requiredExtensionCount);
    if (!requiredExtensions)
    {
        fprintf(stderr, "Failed to allocate memory for required extensions\n");
        free(availableExtensions);
        return APP_ERROR_MALLOC;
    }

    for (uint32_t i = 0; i < glfwExtensionCount; ++i)
    {
        requiredExtensions[i] = glfwExtensions[i];
    }

    for (uint32_t i = 0; i < additionalExtensionCount; ++i)
    {
        requiredExtensions[glfwExtensionCount + i] = additionalExtensions[i];
    }
#else
    requiredExtensions = glfwExtensions;
#endif

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = requiredExtensionCount,
        .ppEnabledExtensionNames = requiredExtensions,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .pNext = NULL,
#ifdef __APPLE__
        .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
#else
        .flags = 0,
#endif
    };

    if (enableValidationLayers)
    {
        AppResult result = checkValidationLayerSupport();
        if (result != APP_SUCCESS)
        {
            free(availableExtensions);
#ifdef __APPLE__
            free(requiredExtensions);
#endif
            return result;
        }
        createInfo.enabledLayerCount = sizeof(validationLayers) / sizeof(validationLayers[0]);
        createInfo.ppEnabledLayerNames = validationLayers;
    }

    result = vkCreateInstance(&createInfo, NULL, &app->instance);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create Vulkan instance: %d\n", result);
#ifdef __APPLE__
        free(requiredExtensions);
#endif
        free(availableExtensions);
        return APP_ERROR_VULKAN_INSTANCE;
    }

#ifdef __APPLE__
    free(requiredExtensions);
#endif
    free(availableExtensions);

    return APP_SUCCESS;
} // initVulkan

AppResult cleanup(App *app, AppResult result)
{
    if (app->logicalDevice != VK_NULL_HANDLE)
        vkDestroyDevice(app->logicalDevice, NULL);

    if (app->surface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(app->instance, app->surface, NULL);

    if (app->instance != VK_NULL_HANDLE)
        vkDestroyInstance(app->instance, NULL);

    if (app->window != NULL)
        glfwDestroyWindow(app->window);

    glfwTerminate();

    return result;
} // cleanup
