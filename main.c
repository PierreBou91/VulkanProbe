#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef __APPLE__
#ifndef VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
#define VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME "VK_KHR_get_physical_device_properties2"
#endif
#ifndef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#define VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME "VK_KHR_portability_subset"
#endif
#endif

// User-defined constants
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const char *TITLE = "Vulkan Probe";

const char *validationLayers[1] = {
    "VK_LAYER_KHRONOS_validation",
};

const char *requiredInstanceExtensions[2] = {
#ifdef __APPLE__
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
#else
    NULL,
#endif
};

#ifdef __APPLE__
const char *requiredDeviceExtensions[2] = {
    VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};
#else
const char *requiredDeviceExtensions[1] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};
#endif

#define ARRAY_LEN(x) (sizeof(x) / sizeof(x[0]))

#ifdef NDEBUG // pass -DNDEBUG to the compiler when building in release mode
const bool debug = false;
const bool enableValidationLayers = false;
#else
const bool debug = true;
const bool enableValidationLayers = true;
#endif

#ifdef VERBOSE // pass -DVERBOSE to the compiler to enable verbose output
const bool verbose = true;
#else
const bool verbose = false;
#endif
typedef struct App
{
    GLFWwindow *window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR surface;
    VkQueue graphicsQueue;
    uint32_t graphicsQueueFamilyIndex;
    float graphicsQueuePriority;
    VkQueue presentationQueue;
    uint32_t presentationQueueFamilyIndex;
    float presentationQueuePriority;
    VkDeviceQueueCreateInfo pQueueCreateInfos[2];
    uint32_t queueCreateInfoCount;
    VkDevice logicalDevice;
    VkSurfaceFormatKHR selectedDeviceSurfaceFormat;
    VkPresentModeKHR selectedDevicePresentMode;
    VkSurfaceCapabilitiesKHR selectedDeviceSurfaceCapabilities;
    VkExtent2D swapChainExtent;
    VkSwapchainKHR swapChain;
    VkImage *swapChainImages;
    uint32_t swapChainImageCount;
    VkImageView *swapChainImageViews;
} App;

typedef enum AppResult
{
    APP_SUCCESS = 0,
    APP_ERROR_GLFW_INIT = 1,
    APP_ERROR_GLFW_WINDOW = 2,
    APP_ERROR_VULKAN_ENUM_INSTANCE_EXT_PROP = 3,
    APP_ERROR_REQUIRED_EXTENSION_NOT_SUPPORTED = 4,
    APP_ERROR_VULKAN_ENUM_INSTANCE_LAYER_PROP = 5,
    APP_ERROR_VULKAN_VALIDATION_LAYER_NOT_FOUND = 6,
    APP_ERROR_VULKAN_CREATE_INSTANCE = 7,
    APP_ERROR_GLFW_CREATE_SURFACE = 8,
    APP_ERROR_VULKAN_NO_PHYSICAL_DEVICE = 9,
    APP_ERROR_VULKAN_ENUM_PHYSICAL_DEVICE = 10,
    APP_ERROR_VULKAN_ENUM_QUEUE_FAMILY_PROP = 11,
    APP_ERROR_VULKAN_CANNOT_GET_PRESENTATION_SUPPORT = 12,
    APP_ERROR_VULKAN_NO_GRAPHICS_QUEUE_FAMILY = 13,
    APP_ERROR_VULKAN_NO_PRESENTATION_QUEUE_FAMILY = 14,
    APP_ERROR_VULKAN_CREATE_LOGICAL_DEVICE = 15,
    APP_ERROR_VULKAN_GET_PHYS_DEV_SURFACE_FORMATS = 16,
    APP_ERROR_VULKAN_GET_PHYS_DEV_PRESENT_MODES = 17,
    APP_ERROR_VULKAN_GET_PHYS_DEV_SURFACE_CAPABILITIES = 18,
    APP_ERROR_VULKAN_CREATE_SWAP_CHAIN = 19,
    APP_ERROR_VULKAN_GET_SWAP_CHAIN_IMAGES = 20,
    APP_ERROR_VULKAN_ALLOC_SWAP_CHAIN_IMAGE_VIEWS = 21,
    APP_ERROR_VULKAN_CREATE_IMAGE_VIEW = 22,
} AppResult;

AppResult initGLFW(App *app);
AppResult initVulkan(App *app);
AppResult checkValidationLayerSupport(void);
AppResult createSurface(App *app);
AppResult createVulkanInstance(App *app);
AppResult selectPhysicalDevice(App *app);
bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
bool deviceHasSwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
bool deviceHasRequiredExtensions(VkPhysicalDevice device);
bool deviceHasPresentationQueueFamily(VkPhysicalDevice device, VkSurfaceKHR surface);
bool deviceHasGraphicsQueueFamily(VkPhysicalDevice device);
uint32_t computeDeviceScore(VkPhysicalDevice device);
AppResult createLogicalDevice(App *app);
AppResult getDeviceQueues(App *app);
AppResult createSwapChain(App *app);
AppResult setOptimalSwapChainParameters(App *app);
AppResult createImageViews(App *app);
AppResult cleanup(App *app, AppResult result);

int main(void)
{
    if (debug)
        printf("Running in debug mode\n");
    else
        printf("Running in release mode\n");

    App app = {0};

    AppResult result = {0};

    result = initGLFW(&app);
    if (result != APP_SUCCESS)
        return cleanup(&app, result);

    if (verbose)
    {
        printf("=========================================\n");
        printf("#########################################\n");
        printf("#             GLFW INITIALIZED          #\n");
        printf("#########################################\n");
    }

    result = initVulkan(&app);
    if (result != APP_SUCCESS)
        return cleanup(&app, result);

    // Main loop
    while (!glfwWindowShouldClose(app.window))
    {
        glfwPollEvents();
        if (glfwGetKey(app.window, GLFW_KEY_SPACE) == GLFW_PRESS)
            glfwSetWindowShouldClose(app.window, GLFW_TRUE);
    }

    return (int)cleanup(&app, APP_SUCCESS);
} // main

AppResult initVulkan(App *app)
{
    AppResult appResult = {0};

    // The first thing we need to do is to create the Vulkan instance
    appResult = createVulkanInstance(app);
    if (appResult != APP_SUCCESS)
        return appResult;

    if (verbose)
    {
        printf("=========================================\n");
        printf("#########################################\n");
        printf("#        VULKAN INSTANCE CREATED        #\n");
        printf("#########################################\n");
    }

    // Then we need to create the surface
    appResult = createSurface(app);
    if (appResult != APP_SUCCESS)
        return appResult;

    // Then we need to select the physical device
    appResult = selectPhysicalDevice(app);
    if (appResult != APP_SUCCESS)
        return appResult;

    if (verbose)
    {
        printf("=========================================\n");
        printf("#########################################\n");
        printf("#       PHYSICAL DEVICE SELECTED        #\n");
        printf("#########################################\n");
    }

    // Now we can create the logical device
    appResult = createLogicalDevice(app);
    if (appResult != APP_SUCCESS)
        return appResult;

    if (verbose)
    {
        printf("=========================================\n");
        printf("#########################################\n");
        printf("#        LOGICAL DEVICE CREATED         #\n");
        printf("#########################################\n");
    }

    // Then we wand to choose the right swap chain settings
    appResult = createSwapChain(app);
    if (appResult != APP_SUCCESS)
        return appResult;

    if (verbose)
    {
        printf("=========================================\n");
        printf("#########################################\n");
        printf("#         SWAP CHAIN CREATED            #\n");
        printf("#########################################\n");
    }

    // The next step is to setup the image views
    appResult = createImageViews(app);
    if (appResult != APP_SUCCESS)
        return appResult;

    if (verbose)
    {
        printf("=========================================\n");
        printf("#########################################\n");
        printf("#        IMAGE VIEWS CREATED            #\n");
        printf("#########################################\n");
    }

    // // Print the app Struct
    // if (verbose)
    // {
    //     printf("=========================================\n");
    //     printf("App Struct:\n");
    //     printf("\tWindow: %p\n", (void *)app->window);
    //     printf("\tInstance: %p\n", (void *)app->instance);
    //     printf("\tPhysical Device: %p\n", (void *)app->physicalDevice);
    //     printf("\tSurface: %p\n", (void *)app->surface);
    //     printf("\tGraphics Queue: %p\n", (void *)app->graphicsQueue);
    //     printf("\tGraphics Queue Family Index: %u\n", app->graphicsQueueFamilyIndex);
    //     printf("\tGraphics Queue Priority: %f\n", app->graphicsQueuePriority);
    //     printf("\tPresentation Queue: %p\n", (void *)app->presentationQueue);
    //     printf("\tPresentation Queue Family Index: %u\n", app->presentationQueueFamilyIndex);
    //     printf("\tPresentation Queue Priority: %f\n", app->presentationQueuePriority);
    //     printf("\tLogical Device: %p\n", (void *)app->logicalDevice);
    // }

    return APP_SUCCESS;
} // initVulkan

AppResult createImageViews(App *app)
{
    // First we need to create the image views
    // Destroyed in the cleanup function
    app->swapChainImageViews = malloc(app->swapChainImageCount * sizeof(VkImageView));
    if (app->swapChainImageViews == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for swap chain image views\n");
        return APP_ERROR_VULKAN_ALLOC_SWAP_CHAIN_IMAGE_VIEWS;
    }

    for (uint32_t i = 0; i < app->swapChainImageCount; ++i)
    {
        VkImageViewCreateInfo imageViewCreateInfo = {0};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = app->swapChainImages[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = app->selectedDeviceSurfaceFormat.format;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        VkResult vkResult = vkCreateImageView(app->logicalDevice, &imageViewCreateInfo, NULL, &app->swapChainImageViews[i]);
        if (vkResult != VK_SUCCESS)
        {
            fprintf(stderr, "Failed to create image view: %d\n", vkResult);
            return APP_ERROR_VULKAN_CREATE_IMAGE_VIEW;
        }
    }

    return APP_SUCCESS;
}

AppResult createSwapChain(App *app)
{
    // First we need to set the optimal swap chain parameters
    AppResult appResult = setOptimalSwapChainParameters(app);
    if (appResult != APP_SUCCESS)
        return appResult;

    // Then we have to decide how many images we want in the swap chain, a good value
    // is to have at least one more than the minimum while not exceeding the maximum
    // 0 in maxImageCount means that there is no maximum
    uint32_t imageCount = 0;
    if (app->selectedDeviceSurfaceCapabilities.maxImageCount > 0)
        imageCount = fmin(app->selectedDeviceSurfaceCapabilities.maxImageCount, app->selectedDeviceSurfaceCapabilities.minImageCount + 1);
    else
        imageCount = app->selectedDeviceSurfaceCapabilities.minImageCount + 1;

    // Now we can create the swap chain create info struct
    VkSwapchainCreateInfoKHR swapChainCreateInfo = {0};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = app->surface;
    swapChainCreateInfo.minImageCount = imageCount;
    swapChainCreateInfo.imageFormat = app->selectedDeviceSurfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = app->selectedDeviceSurfaceFormat.colorSpace;
    swapChainCreateInfo.imageExtent = app->swapChainExtent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    /*
    From the vulkan tutorial:
    https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain
    "we need to specify how to handle swap chain images that will be used across multiple queue families. That will be the case in our application if the graphics queue family is different from the presentation queue. We'll be drawing on the images in the swap chain from the graphics queue and then submitting them on the presentation queue. There are two ways to handle images that are accessed from multiple queues:

    - VK_SHARING_MODE_EXCLUSIVE: An image is owned by one queue family at a time and ownership must be explicitly transferred before using it in another queue family. This option offers the best performance.
    - VK_SHARING_MODE_CONCURRENT: Images can be used across multiple queue families without explicit ownership transfers.

    If the queue families differ, then we'll be using the concurrent mode in this tutorial to avoid having to do the ownership chapters, because these involve some concepts that are better explained at a later time. Concurrent mode requires you to specify in advance between which queue families ownership will be shared using the queueFamilyIndexCount and pQueueFamilyIndices parameters. If the graphics queue family and presentation queue family are the same, which will be the case on most hardware, then we should stick to exclusive mode, because concurrent mode requires you to specify at least two distinct queue families"
    */
    if (app->graphicsQueueFamilyIndex != app->presentationQueueFamilyIndex)
    {
        uint32_t queueFamilyIndices[] = {app->graphicsQueueFamilyIndex, app->presentationQueueFamilyIndex};
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.queueFamilyIndexCount = 0;
        swapChainCreateInfo.pQueueFamilyIndices = NULL;
    }

    swapChainCreateInfo.preTransform = app->selectedDeviceSurfaceCapabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.presentMode = app->selectedDevicePresentMode;
    swapChainCreateInfo.clipped = VK_TRUE;
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult vkResult = vkCreateSwapchainKHR(app->logicalDevice, &swapChainCreateInfo, NULL, &app->swapChain);
    if (vkResult != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create swap chain: %d\n", vkResult);
        return APP_ERROR_VULKAN_CREATE_SWAP_CHAIN;
    }

    // Now we can get the swap chain images
    vkResult = vkGetSwapchainImagesKHR(app->logicalDevice, app->swapChain, &app->swapChainImageCount, NULL);
    if (vkResult != VK_SUCCESS || app->swapChainImageCount == 0)
    {
        fprintf(stderr, "Failed to get swap chain images: %d\n", vkResult);
        return APP_ERROR_VULKAN_GET_SWAP_CHAIN_IMAGES;
    }

    // Images are destroyed when the swap chain is destroyed
    app->swapChainImages = malloc(app->swapChainImageCount * sizeof(VkImage));
    vkResult = vkGetSwapchainImagesKHR(app->logicalDevice, app->swapChain, &app->swapChainImageCount, app->swapChainImages);
    if (vkResult != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to get swap chain images: %d\n", vkResult);
        return APP_ERROR_VULKAN_GET_SWAP_CHAIN_IMAGES;
    }

    return APP_SUCCESS;
} // createSwapChain

AppResult setOptimalSwapChainParameters(App *app)
{
    // First chose the ideal format
    uint32_t surfaceFormatCount = 0;
    VkResult vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(app->physicalDevice, app->surface, &surfaceFormatCount, NULL);
    if (vkResult != VK_SUCCESS || surfaceFormatCount == 0)
    {
        fprintf(stderr, "Failed to get physical device surface formats: %d\n", vkResult);
        return APP_ERROR_VULKAN_GET_PHYS_DEV_SURFACE_FORMATS;
    }
    // VLA is ok here for simplicity.
    VkSurfaceFormatKHR surfaceFormatsArr[surfaceFormatCount];
    vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(app->physicalDevice, app->surface, &surfaceFormatCount, surfaceFormatsArr);

    // Ideally we want a SRGB color format and color space
    bool foundIdealFormat = false;
    for (uint32_t i = 0; i < surfaceFormatCount; ++i)
    {
        if (surfaceFormatsArr[i].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormatsArr[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            app->selectedDeviceSurfaceFormat = surfaceFormatsArr[i];
            foundIdealFormat = true;
            break;
        }
    }

    if (!foundIdealFormat)
    {
        // If we didn't find the ideal format we just take the first one
        app->selectedDeviceSurfaceFormat = surfaceFormatsArr[0];
    }

    if (verbose)
    {
        printf("=========================================\n");
        if (foundIdealFormat)
        {
            printf("Ideal format found:\n");
            printf("\tFormat: VK_FORMAT_B8G8R8A8_SRGB\n");
            printf("\tColor space: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR\n");
        }
        else
        {
            printf("Ideal format not found, taking the first one:\n");
            printf("\tFormat: %u\n", surfaceFormatsArr[0].format);
            printf("\tColor space: %u\n", surfaceFormatsArr[0].colorSpace);
        }
    }

    // Then we want to chose the presentation mode
    uint32_t presentModeCount = 0;
    vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(app->physicalDevice, app->surface, &presentModeCount, NULL);
    if (vkResult != VK_SUCCESS || presentModeCount == 0)
    {
        fprintf(stderr, "Failed to get physical device surface present modes: %d\n", vkResult);
        return APP_ERROR_VULKAN_GET_PHYS_DEV_PRESENT_MODES;
    }
    // VLA is ok here for simplicity.
    VkPresentModeKHR presentModesArr[presentModeCount];
    vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(app->physicalDevice, app->surface, &presentModeCount, presentModesArr);

    // Ideally we want a mailbox present mode
    bool foundIdealPresentMode = false;
    for (uint32_t i = 0; i < presentModeCount; ++i)
    {
        if (presentModesArr[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            foundIdealPresentMode = true;
            app->selectedDevicePresentMode = presentModesArr[i];
            break;
        }
    }

    if (!foundIdealPresentMode)
    {
        // If we didn't find the ideal present mode we just take the FIFO one that is
        // guaranteed to be available
        app->selectedDevicePresentMode = VK_PRESENT_MODE_FIFO_KHR;
    }

    if (verbose)
    {
        printf("=========================================\n");
        if (foundIdealPresentMode)
        {
            printf("Ideal present mode found:\n");
            printf("\tVK_PRESENT_MODE_MAILBOX_KHR\n");
        }
        else
        {
            printf("Ideal present mode not found, taking VK_PRESENT_MODE_FIFO_KHR\n");
        }
    }

    // Then we have to setup app surface capabilities
    vkResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(app->physicalDevice, app->surface, &app->selectedDeviceSurfaceCapabilities);
    if (vkResult != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to get physical device surface capabilities: %d\n", vkResult);
        return APP_ERROR_VULKAN_GET_PHYS_DEV_SURFACE_CAPABILITIES;
    }

    // and set the swap chain extent
    if (app->selectedDeviceSurfaceCapabilities.currentExtent.width != UINT32_MAX)
    {
        app->swapChainExtent = app->selectedDeviceSurfaceCapabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(app->window, &width, &height);
        VkExtent2D actualExtent = {
            .width = (uint32_t)width,
            .height = (uint32_t)height,
        };
        actualExtent.width = fmax(app->selectedDeviceSurfaceCapabilities.minImageExtent.width, fmin(app->selectedDeviceSurfaceCapabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = fmax(app->selectedDeviceSurfaceCapabilities.minImageExtent.height, fmin(app->selectedDeviceSurfaceCapabilities.maxImageExtent.height, actualExtent.height));
        app->swapChainExtent = actualExtent;
    }

    return APP_SUCCESS;
} // setOptimalSwapChainParameters

AppResult createLogicalDevice(App *app)
{
    AppResult appResult = {0};
    appResult = getDeviceQueues(app);
    if (appResult != APP_SUCCESS)
        return appResult;

    // No specific features for now
    VkPhysicalDeviceFeatures deviceFeatures = {0};

    // Now we can create the logical device
    VkDeviceCreateInfo deviceCreateInfo = {0};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = NULL;
    deviceCreateInfo.queueCreateInfoCount = app->queueCreateInfoCount;
    deviceCreateInfo.pQueueCreateInfos = app->pQueueCreateInfos;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = ARRAY_LEN(requiredDeviceExtensions);
    deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtensions;

    VkResult vkResult = vkCreateDevice(app->physicalDevice, &deviceCreateInfo, NULL, &app->logicalDevice);
    if (vkResult != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create logical device: %d\n", vkResult);
        return APP_ERROR_VULKAN_CREATE_LOGICAL_DEVICE;
    }

    // We can finally assign the queues
    vkGetDeviceQueue(app->logicalDevice, app->graphicsQueueFamilyIndex, 0, &app->graphicsQueue);
    vkGetDeviceQueue(app->logicalDevice, app->presentationQueueFamilyIndex, 0, &app->presentationQueue);

    return APP_SUCCESS;
} // createLogicalDevice

AppResult getDeviceQueues(App *app)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(app->physicalDevice, &queueFamilyCount, NULL);

    if (queueFamilyCount == 0)
    {
        fprintf(stderr, "Failed to get queue family count\n");
        return APP_ERROR_VULKAN_ENUM_QUEUE_FAMILY_PROP;
    }

    // VLA is ok here for simplicity.
    VkQueueFamilyProperties queueFamiliesArr[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(app->physicalDevice, &queueFamilyCount, queueFamiliesArr);

    if (verbose)
    {
        printf("=========================================\n");
        printf("Queue families:\n");
        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            printf("\tQueue family %i:\n", i);
            printf("\t\tQueue flags: %u\n", queueFamiliesArr[i].queueFlags);
            printf("\t\tQueue count: %u\n", queueFamiliesArr[i].queueCount);
            printf("\t\tTimestamp valid bits: %u\n", queueFamiliesArr[i].timestampValidBits);
            printf("\t\tMin image transfer granularity: %u, %u, %u\n", queueFamiliesArr[i].minImageTransferGranularity.width, queueFamiliesArr[i].minImageTransferGranularity.height, queueFamiliesArr[i].minImageTransferGranularity.depth);
        }
    }

    // For the moment we are only interested in queue families that support graphics
    // or presentation so we will loop through the queue families and select the first
    // one that supports graphics and same for presentation queue family
    bool graphicsQueueFamilyFound = false;
    bool presentationQueueFamilyFound = false;
    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        if (!graphicsQueueFamilyFound && queueFamiliesArr[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            app->graphicsQueueFamilyIndex = i;
            graphicsQueueFamilyFound = true;
        }

        VkBool32 presentationSupported = VK_FALSE;
        VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(app->physicalDevice, i, app->surface, &presentationSupported);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "Failed to check presentation support: %d\n", result);
            return APP_ERROR_VULKAN_CANNOT_GET_PRESENTATION_SUPPORT;
        }

        if (!presentationQueueFamilyFound && presentationSupported)
        {
            app->presentationQueueFamilyIndex = i;
            presentationQueueFamilyFound = true;
        }

        if (graphicsQueueFamilyFound && presentationQueueFamilyFound)
        {
            break;
        }
    }

    if (!graphicsQueueFamilyFound)
    {
        fprintf(stderr, "No graphics queue family found\n");
        return APP_ERROR_VULKAN_NO_GRAPHICS_QUEUE_FAMILY;
    }

    if (!presentationQueueFamilyFound)
    {
        fprintf(stderr, "No presentation queue family found\n");
        return APP_ERROR_VULKAN_NO_PRESENTATION_QUEUE_FAMILY;
    }

    // Set the app queue priorities to 1.0f
    app->graphicsQueuePriority = 1.0f;
    app->presentationQueuePriority = 1.0f;

    // Now we can build the VkDeviceQueueCreateInfo but first we need to check if the queue
    // family indices are different
    if (app->graphicsQueueFamilyIndex == app->presentationQueueFamilyIndex)
    {
        // In this case we only need to create one queue info
        app->queueCreateInfoCount = 1;
        VkDeviceQueueCreateInfo queueCreateInfo = {0};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.pNext = NULL;
        queueCreateInfo.queueFamilyIndex = app->graphicsQueueFamilyIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &app->graphicsQueuePriority;
        app->pQueueCreateInfos[0] = queueCreateInfo;
    }
    else
    {
        // In this case we need to create two queue infos
        app->queueCreateInfoCount = 2;
        VkDeviceQueueCreateInfo graphicsQueueCreateInfo = {0};
        graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        graphicsQueueCreateInfo.pNext = NULL;
        graphicsQueueCreateInfo.queueFamilyIndex = app->graphicsQueueFamilyIndex;
        graphicsQueueCreateInfo.queueCount = 1;
        graphicsQueueCreateInfo.pQueuePriorities = &app->graphicsQueuePriority;

        VkDeviceQueueCreateInfo presentationQueueCreateInfo = {0};
        presentationQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        presentationQueueCreateInfo.pNext = NULL;
        presentationQueueCreateInfo.queueFamilyIndex = app->presentationQueueFamilyIndex;
        presentationQueueCreateInfo.queueCount = 1;
        presentationQueueCreateInfo.pQueuePriorities = &app->presentationQueuePriority;

        app->pQueueCreateInfos[0] = graphicsQueueCreateInfo;
        app->pQueueCreateInfos[1] = presentationQueueCreateInfo;
    }

    return APP_SUCCESS;
} // getDeviceQueues

AppResult selectPhysicalDevice(App *app)
{
    // First we need to get the number of physical devices and enumerate them
    uint32_t deviceCount = 0;
    if (vkEnumeratePhysicalDevices(app->instance, &deviceCount, NULL) != VK_SUCCESS || deviceCount == 0)
    {
        fprintf(stderr, "Failed to find GPUs with Vulkan support\n");
        return APP_ERROR_VULKAN_NO_PHYSICAL_DEVICE;
    }
    // VLA is ok here for simplicity.
    VkPhysicalDevice deviceArr[deviceCount];
    if (vkEnumeratePhysicalDevices(app->instance, &deviceCount, deviceArr) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to enumerate physical devices\n");
        return APP_ERROR_VULKAN_ENUM_PHYSICAL_DEVICE;
    }

    if (verbose)
    {
        printf("=========================================\n");
        printf("Physical devices:\n");
    }

    // Now we need to select the best physical device
    VkPhysicalDevice selectedDevice = VK_NULL_HANDLE;
    uint32_t maxScore = 0;
    for (uint32_t i = 0; i < deviceCount; ++i)
    {
        if (verbose)
        {

            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(deviceArr[i], &deviceProperties);
            printf("\tDevice %i: %s\n", i + 1, deviceProperties.deviceName);
            printf("\t\tAPI version: %u\n", deviceProperties.apiVersion);
            printf("\t\tDriver version: %u\n", deviceProperties.driverVersion);
            printf("\t\tVendor ID: %u\n", deviceProperties.vendorID);
            printf("\t\tDevice ID: %u\n", deviceProperties.deviceID);
            if (deviceProperties.deviceType == 0)
                printf("\t\tDevice type: VK_PHYSICAL_DEVICE_TYPE_OTHER\n");
            else if (deviceProperties.deviceType == 1)
                printf("\t\tDevice type: VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU\n");
            else if (deviceProperties.deviceType == 2)
                printf("\t\tDevice type: VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU\n");
            else if (deviceProperties.deviceType == 3)
                printf("\t\tDevice type: VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU\n");
            else if (deviceProperties.deviceType == 4)
                printf("\t\tDevice type: VK_PHYSICAL_DEVICE_TYPE_CPU\n");
        }

        if (isDeviceSuitable(deviceArr[i], app->surface))
        {
            uint32_t score = computeDeviceScore(deviceArr[i]);
            if (score > maxScore)
            {
                maxScore = score;
                selectedDevice = deviceArr[i];
            }
            if (verbose)
            {
                printf("\t\tScore: %u\n", score);
                printf("\t\tDevice is suitable\n");
            }
        }
        else
        {
            if (verbose)
            {
                printf("\t\tDevice is not suitable\n");
            }
        }
    }

    if (selectedDevice == VK_NULL_HANDLE)
    {
        fprintf(stderr, "Failed to find a suitable GPU\n");
        return APP_ERROR_VULKAN_NO_PHYSICAL_DEVICE;
    }

    app->physicalDevice = selectedDevice;

    if (verbose)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(selectedDevice, &deviceProperties);
        printf("=========================================\n");
        printf("Selected device: %s\n", deviceProperties.deviceName);
    }

    return APP_SUCCESS;
} // selectPhysicalDevice

uint32_t computeDeviceScore(VkPhysicalDevice device)
{
    // For now, we prefer discrete GPUs over integrated GPUs, and we also prefer GPUs over CPUs
    // We also prefer GPUs with a higher maxImageDimension2D
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

bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    // With the snippet below we can filter out some devices to test other PhysicalDevice
    // VkPhysicalDeviceProperties deviceProperties;
    // vkGetPhysicalDeviceProperties(device, &deviceProperties);
    // if (strcmp(deviceProperties.deviceName, "NVIDIA GeForce RTX 4090") == 0)
    // {
    //     return false;
    // }

    // We need to check if the device has a graphics and presentation queue family
    if (!deviceHasGraphicsQueueFamily(device))
        return false;
    if (!deviceHasPresentationQueueFamily(device, surface))
        return false;

    // If it supports the required extensions
    if (!deviceHasRequiredExtensions(device))
        return false;

    // And if its swap chain is valid, note that it's important to check this AFTER having
    // ensured that the device has the Swapchain extension
    if (!deviceHasSwapChainSupport(device, surface))
        return false;

    return true;
} // isDeviceSuitable

bool deviceHasSwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    // For now the only thing we need to do is to ensure is that the device has at least
    // one supported surface format and present mode given the surface.

    // First get the physical device surface capabilities
    VkSurfaceCapabilitiesKHR surfaceCapabilities = {0};
    VkResult vkResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surfaceCapabilities);
    if (vkResult != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to get physical device surface capabilities: %d\n", vkResult);
        return false;
    }

    if (verbose)
    {
        printf("\t\tSurface capabilities:\n");
        printf("\t\t\tMin image count: %u\n", surfaceCapabilities.minImageCount);
        printf("\t\t\tMax image count: %u\n", surfaceCapabilities.maxImageCount);
        printf("\t\t\tCurrent extent: %u, %u\n", surfaceCapabilities.currentExtent.width, surfaceCapabilities.currentExtent.height);
        printf("\t\t\tMin image extent: %u, %u\n", surfaceCapabilities.minImageExtent.width, surfaceCapabilities.minImageExtent.height);
        printf("\t\t\tMax image extent: %u, %u\n", surfaceCapabilities.maxImageExtent.width, surfaceCapabilities.maxImageExtent.height);
        printf("\t\t\tMax image array layers: %u\n", surfaceCapabilities.maxImageArrayLayers);
        printf("\t\t\tSupported transforms: %x\n", surfaceCapabilities.supportedTransforms);
        printf("\t\t\tCurrent transform: %x\n", surfaceCapabilities.currentTransform);
        printf("\t\t\tSupported composite alpha: %x\n", surfaceCapabilities.supportedCompositeAlpha);
        printf("\t\t\tSupported usage flags: %x\n", surfaceCapabilities.supportedUsageFlags);
    }

    // Then get the physical device surface formats
    uint32_t surfaceFormatCount = 0;
    vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surfaceFormatCount, NULL);
    if (vkResult != VK_SUCCESS || surfaceFormatCount == 0)
    {
        fprintf(stderr, "Failed to get physical device surface formats: %d\n", vkResult);
        return false;
    }
    // VLA is ok here for simplicity.
    VkSurfaceFormatKHR surfaceFormatsArr[surfaceFormatCount];
    vkResult = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surfaceFormatCount, surfaceFormatsArr);
    if (vkResult != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to get physical device surface formats: %d\n", vkResult);
        return false;
    }

    if (verbose)
    {
        printf("\t\tSurface formats:\n");
        for (uint32_t i = 0; i < surfaceFormatCount; ++i)
        {
            printf("\t\t\tSurface format %i:\n", i);
            printf("\t\t\t\tFormat: %u\n", surfaceFormatsArr[i].format);
            printf("\t\t\t\tColor space: %u\n", surfaceFormatsArr[i].colorSpace);
        }
    }

    // Then get the physical device surface present modes
    uint32_t presentModeCount = 0;
    vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, NULL);
    if (vkResult != VK_SUCCESS || presentModeCount == 0)
    {
        fprintf(stderr, "Failed to get physical device surface present modes: %d\n", vkResult);
        return false;
    }
    // VLA is ok here for simplicity.
    VkPresentModeKHR presentModesArr[presentModeCount];
    vkResult = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, presentModesArr);
    if (vkResult != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to get physical device surface present modes: %d\n", vkResult);
        return false;
    }

    if (verbose)
    {
        printf("\t\tPresent modes:\n");
        for (uint32_t i = 0; i < presentModeCount; ++i)
        {
            printf("\t\t\tPresent mode %i: %u\n", i, presentModesArr[i]);
        }
    }

    return true;
} // deviceHasSwapChainSupport

bool deviceHasRequiredExtensions(VkPhysicalDevice device)
{
    // It is the same as for the instance, we need to get the available extensions and compare them
    // with the required ones
    uint32_t availableExtensionCount = 0;
    VkResult vkResult = vkEnumerateDeviceExtensionProperties(device, NULL, &availableExtensionCount, NULL);
    if (vkResult != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to enumerate device extension properties: %d\n", vkResult);
        return false;
    }

    // VLA is ok here for simplicity.
    VkExtensionProperties availableExtensionsArr[availableExtensionCount];
    vkResult = vkEnumerateDeviceExtensionProperties(device, NULL, &availableExtensionCount, availableExtensionsArr);
    if (vkResult != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to enumerate device extension properties: %d\n", vkResult);
        return false;
    }

    // Too many extensions
    // if (verbose)
    // {
    //     for (uint32_t i = 0; i < availableExtensionCount; ++i)
    //     {
    //         printf("\t\tExtensions %i.:%s\n", i + 1, availableExtensionsArr[i].extensionName);
    //     }
    // }

    for (uint32_t i = 0; i < ARRAY_LEN(requiredDeviceExtensions); ++i)
    {
        bool extensionFound = false;
        for (uint32_t j = 0; j < availableExtensionCount; ++j)
        {
            if (strcmp(requiredDeviceExtensions[i], availableExtensionsArr[j].extensionName) == 0)
            {
                extensionFound = true;
                break;
            }
        }

        if (!extensionFound)
        {
            fprintf(stderr, "Required extension %s is not supported\n", requiredDeviceExtensions[i]);
            return false;
        }
    }

    return true;
} // deviceHasRequiredExtensions

bool deviceHasPresentationQueueFamily(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

    // VLA is ok here for simplicity.
    VkQueueFamilyProperties queueFamiliesArr[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamiliesArr);

    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        VkBool32 presentationSupported = VK_FALSE;
        VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupported);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "Failed to check presentation support: %d\n", result);
            return false;
        }

        if (presentationSupported)
        {
            return true;
        }
    }

    return false;
} // deviceHasPresentationQueueFamily

bool deviceHasGraphicsQueueFamily(VkPhysicalDevice device)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

    // VLA is ok here for simplicity.
    VkQueueFamilyProperties queueFamiliesArr[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamiliesArr);

    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        if (queueFamiliesArr[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            return true;
        }
    }

    return false;
} // deviceHasGraphicsQueueFamily

AppResult createSurface(App *app)
{
    if (glfwCreateWindowSurface(app->instance, app->window, NULL, &app->surface) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create window surface\n");
        return APP_ERROR_GLFW_CREATE_SURFACE;
    }
    return APP_SUCCESS;
} // createSurface

AppResult createVulkanInstance(App *app)
{
    // First we need a VkApplicationInfo
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = TITLE,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };

    // Then we need to get the required extensions from GLFW and from the user, this can be
    // the case for apple users for example. They are defined in the
    // requiredInstanceExtensions
    uint32_t glfwRequiredExtensionCount = 0;
    const char **glfwRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionCount);

    if (verbose)
    {
        printf("=========================================\n");
        printf("Required extension(s) for the Vulkan instance:\n");
        printf("\tFrom GLFW:\n");
        if (glfwRequiredExtensionCount == 0)
        {
            printf("\t\tNone\n");
        }
        else
        {
            for (uint32_t i = 0; i < glfwRequiredExtensionCount; ++i)
            {
                printf("\t\t%i. %s\n", i + 1, glfwRequiredExtensions[i]);
            }
        }

        printf("\tFrom the user:\n");
        if (requiredInstanceExtensions[0] == NULL)
        {
            printf("\t\tNone\n");
        }
        else
        {
            for (uint32_t i = 0; i < ARRAY_LEN(requiredInstanceExtensions); ++i)
            {
                printf("\t\t%i. %s\n", i + 1, requiredInstanceExtensions[i]);
            }
        }
    }

    // We need to concatenate the required extensions together so we first compute the total
    // count
    uint32_t requiredInstanceExtensionsCount = 0;
    if (requiredInstanceExtensions[0] == NULL)
    {
        requiredInstanceExtensionsCount = glfwRequiredExtensionCount;
    }
    else
    {
        requiredInstanceExtensionsCount = glfwRequiredExtensionCount + ARRAY_LEN(requiredInstanceExtensions);
    }

    // VLA is ok here for simplicity.
    const char *concatenatedRequiredInstanceExtensions[requiredInstanceExtensionsCount];
    // Now we can concatenate the required extensions together...
    for (uint32_t i = 0; i < glfwRequiredExtensionCount; ++i)
    {
        concatenatedRequiredInstanceExtensions[i] = glfwRequiredExtensions[i];
    }
    if (concatenatedRequiredInstanceExtensions[0] != NULL)
    {
        for (uint32_t i = 0; i < ARRAY_LEN(requiredInstanceExtensions); ++i)
        {
            concatenatedRequiredInstanceExtensions[glfwRequiredExtensionCount + i] = requiredInstanceExtensions[i];
        }
    }

    if (verbose)
    {
        printf("=========================================\n");
        printf("Concatenated required extension(s) for the Vulkan instance:\n");
        for (uint32_t i = 0; i < requiredInstanceExtensionsCount; ++i)
        {
            printf("\t%i. %s\n", i + 1, concatenatedRequiredInstanceExtensions[i]);
        }
    }

    // ... and then get all the available extensions in order to compare
    uint32_t availableInstanceExtensionsCount = 0;
    VkResult vkResult = vkEnumerateInstanceExtensionProperties(NULL, &availableInstanceExtensionsCount, NULL);
    if (vkResult != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to enumerate instance extension properties: %d\n", vkResult);
        return APP_ERROR_VULKAN_ENUM_INSTANCE_EXT_PROP;
    }

    // VLA is ok here for simplicity.
    VkExtensionProperties availableInstanceExtensions[availableInstanceExtensionsCount];
    vkResult = vkEnumerateInstanceExtensionProperties(NULL, &availableInstanceExtensionsCount, availableInstanceExtensions);
    if (vkResult != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to enumerate instance extension properties: %d\n", vkResult);
        return APP_ERROR_VULKAN_ENUM_INSTANCE_EXT_PROP;
    }

    if (verbose)
    {
        printf("=========================================\n");
        printf("Available extension(s) for the Vulkan instance:\n");
        for (uint32_t i = 0; i < availableInstanceExtensionsCount; ++i)
        {
            printf("\t%i. %s\n", i + 1, availableInstanceExtensions[i].extensionName);
        }
    }

    // Check if all the required extensions are supported
    for (uint32_t i = 0; i < requiredInstanceExtensionsCount; ++i)
    {
        bool extensionFound = false;
        for (uint32_t j = 0; j < availableInstanceExtensionsCount; ++j)
        {
            if (strcmp(concatenatedRequiredInstanceExtensions[i], availableInstanceExtensions[j].extensionName) == 0)
            {
                extensionFound = true;
                break;
            }
        }

        if (!extensionFound)
        {
            fprintf(stderr, "Required extension %s is not supported\n", concatenatedRequiredInstanceExtensions[i]);
            return APP_ERROR_REQUIRED_EXTENSION_NOT_SUPPORTED;
        }
    }

    // Now that we have ensured that the required extension were avalaible we can build the VkInstanceCreateInfo
    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
#ifdef __APPLE__
        .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
#else
        .flags = 0,
#endif
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .enabledExtensionCount = requiredInstanceExtensionsCount,
        .ppEnabledExtensionNames = concatenatedRequiredInstanceExtensions,
    };

    // then can add the validation layers if necessary
    if (enableValidationLayers)
    {
        AppResult appResult = checkValidationLayerSupport();
        if (appResult != APP_SUCCESS)
            return appResult;
        createInfo.enabledLayerCount = ARRAY_LEN(validationLayers);
        createInfo.ppEnabledLayerNames = validationLayers;
    }

    // Finally we can create the Vulkan instance
    VkResult result = vkCreateInstance(&createInfo, NULL, &app->instance);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create Vulkan instance: %d\n", result);
        return APP_ERROR_VULKAN_CREATE_INSTANCE;
    }

    return APP_SUCCESS;
} // createVulkanInstance

AppResult checkValidationLayerSupport(void)
{
    // This simply queries the available layers and compares them with the user-defined required layers
    // in the validationLayers const
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

    if (verbose)
    {
        printf("=========================================\n");
        printf("Available validation layers:\n");
        for (uint32_t i = 0; i < layerCount; ++i)
        {
            printf("\t%i. %s\n", i + 1, availableLayersArr[i].layerName);
        }
        printf("=========================================\n");
        printf("Required validation layer(s) from the user:\n");
        for (uint32_t i = 0; i < ARRAY_LEN(validationLayers); ++i)
        {
            printf("\t%i. %s\n", i + 1, validationLayers[i]);
        }
    }

    for (uint32_t i = 0; i < ARRAY_LEN(validationLayers); ++i)
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

AppResult cleanup(App *app, AppResult result)
{
    if (app->swapChainImageViews != NULL)
    {
        for (uint32_t i = 0; i < app->swapChainImageCount; ++i)
        {
            vkDestroyImageView(app->logicalDevice, app->swapChainImageViews[i], NULL);
        }
        free(app->swapChainImageViews);
    }

    if (app->swapChain != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(app->logicalDevice, app->swapChain, NULL);

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
