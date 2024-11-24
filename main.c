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

typedef struct App
{
    GLFWwindow *window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
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
    APP_ERROR_VALIDATION_LAYER_NOT_FOUND = 7,
    APP_ERROR_NO_PHYSICAL_DEVICE = 8,
    APP_ERROR_VULKAN_ENUM_PHYSICAL_DEVICE = 9,
} AppResult;

AppResult initGLFW(App *app);
AppResult initVulkan(App *app);
AppResult checkValidationLayerSupport(void);
AppResult selectPhysicalDevice(App *app);
uint32_t computeDeviceScore(VkPhysicalDevice device);
AppResult deviceHasGraphicsQueueFamily(VkPhysicalDevice device, bool *hasGraphicsQueueFamily);
AppResult cleanup(App *app, AppResult result);

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

    result = selectPhysicalDevice(&app);
    if (result != APP_SUCCESS)
        return cleanup(&app, result);

    // VkDevice device;
    // uint32_t *graphicsQueueIndices = NULL;

    // Main loop
    while (!glfwWindowShouldClose(app.window))
    {
        glfwPollEvents();
    }

    return (int)cleanup(&app, APP_SUCCESS);
} // main

AppResult selectPhysicalDevice(App *app)
{
    uint32_t deviceCount = 0;
    VkResult vkResult = vkEnumeratePhysicalDevices(app->instance, &deviceCount, NULL);
    if (vkResult != VK_SUCCESS || deviceCount == 0)
    {
        fprintf(stderr, "Failed to find GPUs with Vulkan support\n");
        return APP_ERROR_NO_PHYSICAL_DEVICE;
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
        if (hasGraphicsQueueFamily)
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
        return APP_ERROR_NO_PHYSICAL_DEVICE;
    }

    app->physicalDevice = selectedDevice;
    return APP_SUCCESS;
} // selectPhysicalDevice

AppResult deviceHasGraphicsQueueFamily(VkPhysicalDevice device, bool *hasGraphicsQueueFamily)
{
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
            return APP_ERROR_VALIDATION_LAYER_NOT_FOUND;
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

#ifdef __APPLE__
    // On macOS, we need to enable the VK_KHR_portability_enumeration extension
    // and set the VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR flag
    // https://vulkan-tutorial.com/en/Drawing_a_triangle/Setup/Instance cf the "Encountered
    // VK_ERROR_INCOMPATIBLE_DRIVER section"

    requiredExtensionCount += 1;
    requiredExtensions = malloc(sizeof(char *) * requiredExtensionCount);
    if (!requiredExtensions)
    {
        fprintf(stderr, "Failed to allocate memory for required extensions\n");
        return APP_ERROR_MALLOC;
    }

    for (uint32_t i = 0; i < glfwExtensionCount; ++i)
    {
        requiredExtensions[i] = glfwExtensions[i];
    }

    requiredExtensions[glfwExtensionCount] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
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
            return result;
        }
        createInfo.enabledLayerCount = sizeof(validationLayers) / sizeof(validationLayers[0]);
        createInfo.ppEnabledLayerNames = validationLayers;
    }

    VkResult instanceResult = vkCreateInstance(&createInfo, NULL, &app->instance);
    if (instanceResult != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create Vulkan instance: %d\n", instanceResult);
#ifdef __APPLE__
        free(requiredExtensions);
#endif
        return APP_ERROR_VULKAN_INSTANCE;
    }

    uint32_t extensionCount = 0;
    VkResult enumInstanceExtPropResult = vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
    if (enumInstanceExtPropResult != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to enumerate instance extension properties: %d\n", enumInstanceExtPropResult);
#ifdef __APPLE__
        free(requiredExtensions);
#endif
        return APP_ERROR_VULKAN_ENUM_INSTANCE_EXT_PROP;
    }

    VkExtensionProperties extensionArr[extensionCount];

    enumInstanceExtPropResult = vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensionArr);
    if (enumInstanceExtPropResult != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to enumerate instance extension properties: %d\n", enumInstanceExtPropResult);
#ifdef __APPLE__
        free(requiredExtensions);
#endif
        return APP_ERROR_VULKAN_ENUM_INSTANCE_EXT_PROP;
    }

#ifdef __APPLE__
    free(requiredExtensions);
#endif

    return APP_SUCCESS;
} // initVulkan

AppResult cleanup(App *app, AppResult result)
{
    if (app->instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(app->instance, NULL);
    }

    if (app->window != NULL)
    {
        glfwDestroyWindow(app->window);
    }

    glfwTerminate();

    return result;
} // cleanup
