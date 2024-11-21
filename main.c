#include <stdio.h>
#include <stdlib.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const char *TITLE = "Vulkan Probe";

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
} AppResult;

AppResult initGLFW(App *app);
AppResult initVulkan(App *app);
AppResult cleanup(App *app, AppResult result);

int main(void)
{
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
}

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
}

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

#ifdef __APPLE__
    free(requiredExtensions);
#endif

    free(extensions);

    return APP_SUCCESS;
}

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
}
