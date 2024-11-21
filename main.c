// std
#include <stdio.h>
#include <stdlib.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const char *TITLE = "Vulkan Probe";

int main(void)
{
    // Initialize GLFW
    if (!glfwInit())
    {
        printf("Failed to initialize GLFW\n");
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // Create GLFW window
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, TITLE, NULL, NULL);
    if (!window)
    {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return 2;
    }

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
        printf("Failed to allocate memory for required extensions\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 3;
    }

    for (uint32_t i = 0; i < glfwExtensionCount; ++i)
    {
        requiredExtensions[i] = glfwExtensions[i];
    }

    requiredExtensions[glfwExtensionCount] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;

#else
    requiredExtensions = glfwExtensions;
#endif

    // Vulkan instance creation info
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

    // Create Vulkan instance
    VkInstance instance;
    VkResult result = vkCreateInstance(&createInfo, NULL, &instance);

    if (result != VK_SUCCESS)
    {
        printf("Failed to create Vulkan instance!\n");
#ifdef __APPLE__
        free(requiredExtensions);
#endif
        glfwDestroyWindow(window);
        glfwTerminate();
        return 4;
    }

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    // Cleanup
    vkDestroyInstance(instance, NULL);
#ifdef __APPLE__
    free(requiredExtensions);
#endif
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
