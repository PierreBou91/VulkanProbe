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
} AppResult;

AppResult initGLFW(App *app);
AppResult initVulkan(App *app);
AppResult checkValidationLayerSupport(void);
AppResult cleanup(App *app, AppResult result);

int main(void)
{
#ifdef NDEBUG // pass -DNDEBUG to the compiler when building in release mode
    printf("Running in release mode\n");
#else
    printf("Running in debug mode\n");
#endif

    App app = {0};

    AppResult result = {0};

    result = initGLFW(&app);
    if (result != APP_SUCCESS)
        return cleanup(&app, result);

    result = initVulkan(&app);
    if (result != APP_SUCCESS)
        return cleanup(&app, result);

    // Main loop
    while (!glfwWindowShouldClose(app.window))
    {
        glfwPollEvents();
    }

    return cleanup(&app, APP_SUCCESS);
} // main

AppResult checkValidationLayerSupport(void)
{
    uint32_t layerCount;
    VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, NULL);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to enumerate instance layer properties: %d\n", result);
        return APP_ERROR_VULKAN_ENUM_INSTANCE_LAYER_PROP;
    }

    VkLayerProperties *availableLayers = malloc(sizeof(VkLayerProperties) * layerCount);
    if (!availableLayers)
    {
        fprintf(stderr, "Failed to allocate memory for available layers\n");
        return APP_ERROR_MALLOC;
    }

    result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to enumerate instance layer properties: %d\n", result);
        free(availableLayers);
        return APP_ERROR_VULKAN_ENUM_INSTANCE_LAYER_PROP;
    }

    for (uint32_t i = 0; i < sizeof(validationLayers) / sizeof(validationLayers[0]); ++i)
    {
        bool layerFound = false;
        for (uint32_t j = 0; j < layerCount; ++j)
        {
            if (strcmp(validationLayers[i], availableLayers[j].layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            fprintf(stderr, "ERROR: Validation layer %s not found\n", validationLayers[i]);
            free(availableLayers);
            return APP_ERROR_VALIDATION_LAYER_NOT_FOUND;
        }
    }

    free(availableLayers);
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
    // https://vulkan-tutorial.com/en/Drawing_a_triangle/Setup/Instance cf the ¨Encountered
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

    VkExtensionProperties *extensions = malloc(sizeof(VkExtensionProperties) * extensionCount);
    if (!extensions)
    {
        fprintf(stderr, "Failed to allocate memory for extensions\n");
#ifdef __APPLE__
        free(requiredExtensions);
#endif
        return APP_ERROR_MALLOC;
    }
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions);

    printf("Available Vulkan extensions:\n");
    for (uint32_t i = 0; i < extensionCount; ++i)
    {
        printf("\t%s\n", extensions[i].extensionName);
    }

    printf("Required glfw extensions:\n");
    for (uint32_t i = 0; i < glfwExtensionCount; ++i)
    {
        printf("\t%s\n", glfwExtensions[i]);
    }

#ifdef __APPLE__
    free(requiredExtensions);
#endif

    free(extensions);

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
