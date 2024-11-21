# Vulkan Probe

## Resources

[- Vulkan tutorial website](https://vulkan-tutorial.com/)
[- Vulkan API repo](https://github.com/KhronosGroup/Vulkan-Docs)

## Notes

From the [tutorial overview](https://vulkan-tutorial.com/en/Overview) page

> So in short, to draw the first triangle we need to:
>
> - Create a VkInstance
> - Select a supported graphics card (VkPhysicalDevice)
> - Create a VkDevice and VkQueue for drawing and presentation
> - Create a window, window surface and swap chain
> - Wrap the swap chain images into VkImageView
> - Create a render pass that specifies the render targets and usage
> - Create framebuffers for the render pass
> - Set up the graphics pipeline
> - Allocate and record a command buffer with the draw commands for every possible swap chain image
> - Draw frames by acquiring images, submitting the right draw command buffer and returning the images back to the swap chain

## Setup (Linux)

From the [tutorial dev environment](https://vulkan-tutorial.com/en/Development_environment) page

### Vulkan Packages:

```bash
sudo apt install vulkan-tools
sudo apt install libvulkan-dev
sudo apt install vulkan-validationlayers-dev spirv-tools
```

### GLFW:

```bash
sudo apt install libglfw3-dev
```

### CGLM: (C linear algebra library)

The tutorial recommends the GLM library, but I'm using the C version: CGLM.
[CGLM repo](https://github.com/recp/cglm)
[CGLM docs](https://cglm.readthedocs.io/)

```bash
sudo apt install libcglm-dev
```

### Shader compiler:

> Two popular shader compilers are Khronos Group's glslangValidator and Google's glslc. The latter has a familiar GCC- and Clang-like usage, so we'll go with that: on Ubuntu, download Google's unofficial binaries and copy glslc to your /usr/local/bin. Note you may need to sudo depending on your permissions. On Fedora use sudo dnf install glslc, while on Arch Linux run sudo pacman -S shaderc. To test, run glslc and it should rightfully complain we didn't pass any shaders to compile.
> [Unofficial glslc binaries](https://github.com/google/shaderc/blob/main/downloads.md)

### Other libs:

```bash
sudo apt install libxxf86vm-dev libxi-dev
```
