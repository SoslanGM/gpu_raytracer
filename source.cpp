
// SSSTANDARD
#include <stdio.h>
#include <stdlib.h>
#include <shlwapi.h>

#include "D:/RenderDoc/renderdoc_app.h"
RENDERDOC_API_1_1_2 *rdoc_api = NULL;

// VULKANSSS
#define VK_USE_PLATFORM_WIN32_KHR
#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"
#include "vulkan/vk_platform.h"

// SSSTB
#define STB_IMAGE_IMPLEMENTATION
#include "../lib/stb_image.h"

// PRECIOUSSS
#include "M:/types.h"
#include "M:/special_symbols.h"
#include "source.h"
#include "vk_extract.h"
#include "file_io.h"
#include "M:/tokenizer.cpp"
#include "vk_prepare.h"
//#include "vk_setup.h"



void GetVulkanInstance(char **layer_names, u32 layer_count,
                       char **ext_names,   u32 ext_count)
{
    VkInstanceCreateInfo instance_ci = {};
    instance_ci.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_ci.pNext                   = NULL;
    instance_ci.flags                   = 0;
    instance_ci.pApplicationInfo        = NULL;
    instance_ci.enabledLayerCount       = layer_count;
    instance_ci.ppEnabledLayerNames     = layer_names;
    instance_ci.enabledExtensionCount   = ext_count;
    instance_ci.ppEnabledExtensionNames = ext_names;
    
    VkResult result;
    result = vkCreateInstance(&instance_ci, NULL, &vk.instance);
    ODS_RES("Instance creation: %s\n");
}


VKAPI_ATTR VkBool32 VKAPI_CALL
DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
              VkDebugUtilsMessageTypeFlagsEXT types,
              const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
              void *user_data)
{
    ODS(">>> MessageIdName: %s\n", callback_data->pMessageIdName);
    ODS(">>> MessageIdNum:  %d\n", callback_data->messageIdNumber);
    ODS(">>> Message:       %s\n", callback_data->pMessage);
    ODS("\n");
    
    return VK_FALSE;
}

void SetupDebugging()
{
    VkResult result;
    
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_ci = {};
    debug_messenger_ci.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_messenger_ci.pNext           = NULL;
    debug_messenger_ci.flags           = 0;
    debug_messenger_ci.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT|
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT|
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT|
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_messenger_ci.messageType     =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_messenger_ci.pfnUserCallback = DebugCallback;
    debug_messenger_ci.pUserData       = NULL;
    
    result = vkCreateDebugUtilsMessengerEXT(vk.instance, &debug_messenger_ci, NULL, &vk.debug_messenger);
    ODS("Debug utils messenger creation: %s\n", RevEnum(vk_enums.result_enum, result));
}


void GetGPU()
{
    u32 gpu_count = 0;
    vkEnumeratePhysicalDevices(vk.instance, &gpu_count, NULL);
    VkPhysicalDevice *gpus = (VkPhysicalDevice *)calloc(gpu_count, sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(vk.instance, &gpu_count, gpus);
    
    if(gpu_count == 1)
        vk.gpu = gpus[0];
}

void CreateSurface()
{
    VkResult result;
    
    VkWin32SurfaceCreateInfoKHR surface_ci = {};
    surface_ci.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_ci.pNext     = NULL;
    surface_ci.flags     = 0;
    surface_ci.hinstance = app.instance;
    surface_ci.hwnd      = app.window;
    
    result = vkCreateWin32SurfaceKHR(vk.instance, &surface_ci, NULL, &vk.surface);
    ODS("Surface creation: %s\n", RevEnum(vk_enums.result_enum, result));
}
void SetupQueue()
{
    VkResult result;
    
    // ---
    u32 queuefam_propcount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vk.gpu, &queuefam_propcount, NULL);
    VkQueueFamilyProperties *queue_famprops = (VkQueueFamilyProperties *)malloc(sizeof(VkQueueFamilyProperties) * queuefam_propcount);
    vkGetPhysicalDeviceQueueFamilyProperties(vk.gpu, &queuefam_propcount, queue_famprops);
    
    ODS("> Queue family count: %d\n", queuefam_propcount);
    for(u32 i = 0; i < queuefam_propcount; i++)
    {
        ODS("Family %d:\n", i);
        ODS("%3d queues\n", queue_famprops[i].queueCount);
        ODS("Minimum gran: %-4d x %-4d\n", queue_famprops[i].minImageTransferGranularity.width, queue_famprops[i].minImageTransferGranularity.height);
        ODS("- graphics:  %s\n", (queue_famprops[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) ? "YES" : "NO");
        ODS("- compute:   %s\n", (queue_famprops[i].queueFlags & VK_QUEUE_COMPUTE_BIT)  ? "YES" : "NO");
        ODS("- transfer:  %s\n", (queue_famprops[i].queueFlags & VK_QUEUE_TRANSFER_BIT) ? "YES" : "NO");
        ODS("- sparse:    %s\n", (queue_famprops[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) ? "YES" : "NO");
        ODS("- protected: %s\n", (queue_famprops[i].queueFlags & VK_QUEUE_PROTECTED_BIT) ? "YES" : "NO");
        ODS("\n");
    }
    // ---
    
    // ---
    u32 queue_famindex = 0;
    
    for(u32 i = 0; i < queuefam_propcount; i++)
    {
        u32 can_graphics;
        u32 can_present;
        
        can_graphics = queue_famprops[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
        vkGetPhysicalDeviceSurfaceSupportKHR(vk.gpu, i, vk.surface, &can_present);
        
        if(can_graphics && can_present)
            queue_famindex = i;
    }
    
    vk.queue_family_index = queue_famindex;
    // ---
}
void GetVulkanDevice(char **dev_ext_names, u32 dev_ext_count,
                     VkPhysicalDeviceFeatures features)
{
    r32 queue_priority = 1.0f;
    
    VkDeviceQueueCreateInfo queue_ci = {};
    queue_ci.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_ci.pNext            = NULL;
    queue_ci.flags            = 0;
    queue_ci.queueFamilyIndex = vk.queue_family_index;
    queue_ci.queueCount       = 1;
    queue_ci.pQueuePriorities = &queue_priority;
    
    VkDeviceCreateInfo device_ci = {};
    device_ci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_ci.pNext                   = NULL;
    device_ci.flags                   = 0;
    device_ci.queueCreateInfoCount    = 1;
    device_ci.pQueueCreateInfos       = &queue_ci;
    device_ci.enabledLayerCount       = 0;
    device_ci.ppEnabledLayerNames     = NULL;
    device_ci.enabledExtensionCount   = dev_ext_count;
    device_ci.ppEnabledExtensionNames = dev_ext_names;
    device_ci.pEnabledFeatures        = &features;
    
    vkCreateDevice(vk.gpu, &device_ci, NULL, &vk.device);
    
    vkGetDeviceQueue(vk.device, vk.queue_family_index, 0, &vk.queue);
}

VkShaderModule GetShaderModule(char *shaderfile)
{
    // open the doggone shader file
    FILE *f = fopen(shaderfile, "rb");
    
    fseek(f, 0, SEEK_END);
    u32 size = ftell(f);
    rewind(f);
    
    char *code = (char *)calloc(size, sizeof(char));
    fread(code, size, 1, f);
    
    fclose(f);
    
    
    // vk bureaucracy
    VkShaderModuleCreateInfo shadermodule_ci = {};
    shadermodule_ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shadermodule_ci.pNext    = NULL;
    shadermodule_ci.flags    = 0;
    shadermodule_ci.codeSize = size;
    shadermodule_ci.pCode    = (u32 *)code;
    
    VkShaderModule shadermodule;
    vkCreateShaderModule(vk.device, &shadermodule_ci, NULL, &shadermodule);
    
    return shadermodule;
}


void GetComputePipeline(char *shader)
{
    VkPipelineShaderStageCreateInfo stage_ci = {};
    stage_ci.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_ci.pNext               = NULL;
    stage_ci.flags               = 0;
    stage_ci.stage               = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_ci.module              = GetShaderModule(shader);
    stage_ci.pName               = "main";
    stage_ci.pSpecializationInfo = NULL;
    
    
    VkDescriptorSetLayoutBinding binding = {};
    binding.binding            = 0;
    binding.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    binding.descriptorCount    = 1;
    binding.stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    binding.pImmutableSamplers = NULL;
    
    VkDescriptorSetLayoutCreateInfo dsl_ci = {};
    dsl_ci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsl_ci.pNext        = NULL;
    dsl_ci.flags        = 0;
    dsl_ci.bindingCount = 1;
    dsl_ci.pBindings    = &binding;
    VkDescriptorSetLayout dslayout;
    vkCreateDescriptorSetLayout(vk.device, &dsl_ci, NULL, &dslayout);
    
    VkDescriptorPoolSize dspool_size = {};
    dspool_size.type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    dspool_size.descriptorCount = 1;
    
    VkDescriptorPoolCreateInfo dspool_ci = {};
    dspool_ci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dspool_ci.pNext         = NULL;
    dspool_ci.flags         = 0;
    dspool_ci.maxSets       = 1;
    dspool_ci.poolSizeCount = 1;
    dspool_ci.pPoolSizes    = &dspool_size;
    VkDescriptorPool dspool;
    vkCreateDescriptorPool(vk.device, &dspool_ci, NULL, &dspool);
    
    VkDescriptorSetAllocateInfo dsl_ai = {};
    dsl_ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsl_ai.pNext              = NULL;
    dsl_ai.descriptorPool     = dspool;
    dsl_ai.descriptorSetCount = 1;
    dsl_ai.pSetLayouts        = &dslayout;
    vkAllocateDescriptorSets(vk.device, &dsl_ai, &vk.dsl);
    
    VkPipelineLayoutCreateInfo layout_ci = {};
    layout_ci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_ci.pNext                  = NULL;
    layout_ci.flags                  = 0;
    layout_ci.setLayoutCount         = 1;
    layout_ci.pSetLayouts            = &dslayout;
    layout_ci.pushConstantRangeCount = 0;
    layout_ci.pPushConstantRanges    = NULL;
    vkCreatePipelineLayout(vk.device, &layout_ci, NULL, &vk.pipelayout);
    
    
    VkComputePipelineCreateInfo compipe_ci = {};
    compipe_ci.sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    compipe_ci.pNext              = NULL;
    compipe_ci.flags              = 0;
    compipe_ci.stage              = stage_ci;
    compipe_ci.layout             = vk.pipelayout;
    compipe_ci.basePipelineHandle = NULL;
    compipe_ci.basePipelineIndex  = 0;
    
    vkCreateComputePipelines(vk.device, NULL, 1, &compipe_ci, NULL, &vk.compipe);
}


char *DecToBin(u64 n)
{
    u32 width = 64;
    
    char *binnum = (char *)malloc(width+1);
    binnum[width] = '\0';
    
    for(int i = 0; i < width; i++)
    {
        if(n >> i & 1)
            binnum[width-i-1] = '1';
        else
            binnum[width-i-1] = '0';
    }
    
    return binnum;
}


void CheckDeviceMemoryProperties()
{
    vkGetPhysicalDeviceMemoryProperties(vk.gpu, &vk.gpu_memprops);
    
    ODS("> Memory properties: \n");
    ODS("- memory heap count: %d\n", vk.gpu_memprops.memoryHeapCount);
    for(u32 i = 0; i < vk.gpu_memprops.memoryHeapCount; i++)
    {
        ODS("-- heap: %d: \n", i);
        ODS("size:  %zd MB\n", vk.gpu_memprops.memoryHeaps[i].size / (1024 * 1024));
        ODS("flags: %s  \n", DecToBin(vk.gpu_memprops.memoryHeaps[i].flags));
        ODS("\n");
    }
    ODS("- memory type count: %d\n", vk.gpu_memprops.memoryTypeCount);
    for(u32 i = 0; i < vk.gpu_memprops.memoryTypeCount; i++)
    {
        ODS("-- memory: %d: \n", i);
        ODS("heap index: %d \n", vk.gpu_memprops.memoryTypes[i].heapIndex);
        ODS("type:       %s \n", DecToBin(vk.gpu_memprops.memoryTypes[i].propertyFlags));
        ODS("\n");
    }
    ODS("\n");
}


// Finding a memory type index
u32 FindMemoryIndex(u32 possibleMemoryIndexes, u32 requiredProperties,
                    VkPhysicalDeviceMemoryProperties memoryProperties)
{
    ODS("Possible indexes:\n%.*s\n", 64,        DecToBin(possibleMemoryIndexes));
    ODS("Looking for these flags:\n%.*s\n", 64, DecToBin(requiredProperties));
    
    u32 memoryTypeCount = memoryProperties.memoryTypeCount;
    // iterate over all of the memory types,
    for(int i = 0; i < memoryTypeCount; i++)
    {
        // if we encountered a bit that's one of the returned from memoryreqs,
        if((possibleMemoryIndexes >> i) & 1)
        {
            // check if it has our required memory properties.
            u32 memoryTypePropertyFlags = memoryProperties.memoryTypes[i].propertyFlags;
            if((memoryTypePropertyFlags & requiredProperties) == requiredProperties)
            {
                ODS("selected index: %d\n", i);
                
                return i;
            }
        }
    }
    
    ODS("Couldn't find anything suitable\n");
    return -1;
}

VkBuffer CreateBuffer(u32 size, u32 usage, u32 required_memprops,
                      VkDevice device, VkPhysicalDeviceMemoryProperties gpu_memprops, VkDeviceMemory *memory)
{
    // Possible sub-functional breakdown:
    // ---
    VkBufferCreateInfo buffer_ci = {};
    buffer_ci.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_ci.pNext                 = NULL;
    buffer_ci.flags                 = 0;
    buffer_ci.size                  = size;
    buffer_ci.usage                 = usage;
    buffer_ci.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    
    VkBuffer buffer;
    vkCreateBuffer(device, &buffer_ci, NULL, &buffer);
    // ---
    
    // ---
    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(device, buffer, &mem_reqs);
    
    VkMemoryAllocateInfo mem_ai = {};
    mem_ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_ai.pNext           = NULL;
    mem_ai.allocationSize  = mem_reqs.size;
    mem_ai.memoryTypeIndex = FindMemoryIndex(mem_reqs.memoryTypeBits,
                                             required_memprops,
                                             gpu_memprops);
    
    vkAllocateMemory(device, &mem_ai, NULL, memory);
    // ---
    
    vkBindBufferMemory(device, buffer, *memory, 0);
    
    return buffer;
}


void CreateCommandPool(VkCommandPool *command_pool)
{
    VkResult result;
    
    VkCommandPoolCreateInfo commandpool_ci = {};
    commandpool_ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandpool_ci.pNext            = NULL;
    commandpool_ci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandpool_ci.queueFamilyIndex = vk.queue_family_index;
    
    result = vkCreateCommandPool(vk.device, &commandpool_ci, NULL, &(*command_pool));
    ODS("Command pool result: %s\n", RevEnum(vk_enums.result_enum, result));
}

void AllocateCommandBuffer(VkCommandPool command_pool, VkCommandBuffer *command_buffer)
{
    VkResult result;
    
    VkCommandBufferAllocateInfo cb_ai = {};
    cb_ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cb_ai.pNext              = NULL;
    cb_ai.commandPool        = command_pool;
    cb_ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cb_ai.commandBufferCount = 1;
    
    result = vkAllocateCommandBuffers(vk.device, &cb_ai, &vk.cbuffer);
    ODS("Command buffer allocation: %s\n", RevEnum(vk_enums.result_enum, result));
}



void SwapchainCreate()
{
    VkResult result;
    
    VkSwapchainCreateInfoKHR swapchain_ci = {};
    swapchain_ci.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_ci.pNext                 = NULL;
    swapchain_ci.flags                 = 0;
    swapchain_ci.surface               = vk.surface;
    swapchain_ci.minImageCount         = data.image_count;
    swapchain_ci.imageFormat           = vk.format;
    swapchain_ci.imageColorSpace       = vk.color_space;
    swapchain_ci.imageExtent           = vk.screenextent;
    swapchain_ci.imageArrayLayers      = 1;
    swapchain_ci.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_STORAGE_BIT;
    swapchain_ci.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_ci.queueFamilyIndexCount = 0;
    swapchain_ci.pQueueFamilyIndices   = NULL;
    swapchain_ci.preTransform          = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchain_ci.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_ci.presentMode           = VK_PRESENT_MODE_MAILBOX_KHR;
    swapchain_ci.clipped               = VK_TRUE;
    swapchain_ci.oldSwapchain          = NULL;
    
    result = vkCreateSwapchainKHR(vk.device, &swapchain_ci, NULL, &vk.swapchain);
    ODS("Swapchain result: %s\n", RevEnum(vk_enums.result_enum, result));
}
void GetSwapchainImages()
{
    VkResult result;
    
    u32 swapchain_imagecount = 0;
    result = vkGetSwapchainImagesKHR(vk.device, vk.swapchain, &swapchain_imagecount, NULL);
    ODS("Swapchain images(count): %s\n", RevEnum(vk_enums.result_enum, result));
    vk.swapchain_images = (VkImage *)malloc(sizeof(VkImage) * swapchain_imagecount);
    result = vkGetSwapchainImagesKHR(vk.device, vk.swapchain, &swapchain_imagecount, vk.swapchain_images);
    ODS("Swapchain images(fill):  %s\n", RevEnum(vk_enums.result_enum, result));
}

void CreateSwapchainImageViews()
{
    VkResult result;
    
    VkImageViewCreateInfo imageview_ci = {};
    imageview_ci.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageview_ci.pNext            = NULL;
    imageview_ci.flags            = 0;
    imageview_ci.viewType         = VK_IMAGE_VIEW_TYPE_2D;
    imageview_ci.format           = vk.format;
    imageview_ci.components       = vk.components;
    imageview_ci.subresourceRange = vk.color_sr;
    
    vk.swapchain_imageviews = (VkImageView *)malloc(sizeof(VkImageView) * data.image_count);
    for(u32 i = 0; i < data.image_count; i++)
    {
        imageview_ci.image = vk.swapchain_images[i];
        result = vkCreateImageView(vk.device, &imageview_ci, NULL, &vk.swapchain_imageviews[i]);
        ODS("Swapchain imageview result: %s\n", RevEnum(vk_enums.result_enum, result));
    }
}



LRESULT CALLBACK WndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_KEYDOWN:
        {
            u32 VKCode = wParam;
            if(VKCode == VK_ESCAPE)
                PostQuitMessage(0);
        } break;
        
        case WM_PAINT:
        {
            
        } break;
        
        case WM_CLOSE:
        {
            PostQuitMessage(0);
        } break;
        
        case WM_SIZE:
        {
            if(wParam == SIZE_MINIMIZED)
            {
                //display = false;
            }
            if(wParam == SIZE_RESTORED)
            {
                //display = true;
            }
        } break;
        
        default:
        {
            
        } break;
    }
    
    return DefWindowProc(window, message, wParam, lParam);
}


HWND WindowCreate()
{
    COLORREF window_color = 0x00000000;
    HBRUSH window_brush = CreateSolidBrush(window_color);
    
    WNDCLASSEX windowClass = {};
    windowClass.cbSize        = sizeof(WNDCLASSEX);
    windowClass.style         = CS_OWNDC|CS_VREDRAW|CS_HREDRAW;
    windowClass.lpfnWndProc   = WndProc;
    windowClass.hInstance     = app.instance;
    windowClass.lpszClassName = "WindowClass";
    windowClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = window_brush;
    ATOM windowclass = RegisterClassEx(&windowClass);
    
    u32 scrw = GetSystemMetrics(SM_CXSCREEN);
    u32 scrh = GetSystemMetrics(SM_CYSCREEN);
    u32 cx = scrw/2;
    u32 cy = scrh/2;
    
    RECT clirect;
    clirect.left  = cx-app.window_width/2;
    clirect.right = cx+app.window_width/2;
    clirect.top    = cy-app.window_height/2;
    clirect.bottom = cy+app.window_height/2;
    
    AdjustWindowRect(&clirect, WS_BORDER|WS_CAPTION, false);
    u32 winw = clirect.right - clirect.left;
    u32 winh = clirect.bottom - clirect.top;
    
    HWND window = CreateWindowEx(NULL, windowClass.lpszClassName, "",
                                 WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                 (scrw-winw)/2,
                                 (scrh-winh)/2,
                                 winw,
                                 winh,
                                 NULL, NULL, app.instance, NULL);
    return window;
}


VkSubmitInfo GenerateSubmitInfo(u32 wait_count,      VkSemaphore *wait_semaphores,
                                VkPipelineStageFlags *wait_mask,
                                u32 command_buffer_count, VkCommandBuffer *command_buffers,
                                u32 signal_count,    VkSemaphore *signal_semaphores)
{
    VkSubmitInfo result = {};
    result.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    result.pNext                = NULL;
    result.waitSemaphoreCount   = wait_count;
    result.pWaitSemaphores      = wait_semaphores;
    result.pWaitDstStageMask    = wait_mask;
    result.commandBufferCount   = command_buffer_count;
    result.pCommandBuffers      = command_buffers;
    result.signalSemaphoreCount = signal_count;
    result.pSignalSemaphores    = signal_semaphores;
    
    return result;
}

void TransitImageLayout(VkImageLayout old_layout, VkImageLayout new_layout, VkImage image,
                        VkQueue queue, VkCommandBuffer command_buffer, VkFence fence,
                        u32 wait_semaphore_count,   VkSemaphore *wait_semaphores,
                        u32 signal_semaphore_count, VkSemaphore *signal_semaphores)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext               = NULL;
    barrier.srcAccessMask       = VK_ACCESS_HOST_READ_BIT;
    barrier.dstAccessMask       = VK_ACCESS_HOST_WRITE_BIT;
    barrier.oldLayout           = old_layout;
    barrier.newLayout           = new_layout;
    barrier.image               = image;
    barrier.subresourceRange    = vk.color_sr;
    
    VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkSubmitInfo queue_si = GenerateSubmitInfo(wait_semaphore_count, wait_semaphores,
                                               &wait_stage_mask,
                                               1, &command_buffer,
                                               signal_semaphore_count, signal_semaphores);
    
    void *checkpoint_marker = calloc(1, sizeof(void *));
    
    VkCommandBufferBeginInfo commandbuffer_bi = {};
    commandbuffer_bi.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandbuffer_bi.pNext            = NULL;
    commandbuffer_bi.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(command_buffer, &commandbuffer_bi);
    vkCmdPipelineBarrier(command_buffer,
                         VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                         0,
                         0, NULL,
                         0, NULL,
                         1, &barrier);
    vkEndCommandBuffer(command_buffer);
    vkQueueSubmit(queue, 1, &queue_si, fence);
    
    ODS("Waiting on transit fence\n");
    vkWaitForFences(vk.device, 1, &fence, VK_TRUE, UINT64_MAX);
    VkResult result = vkGetFenceStatus(vk.device, fence);
    ODS("Transition fence result: %s\n", RevEnum(vk_enums.result_enum, result));
    
    ODS("> %s -> %s\n", RevEnum(vk_enums.imagelayout_enum, old_layout), RevEnum(vk_enums.imagelayout_enum, new_layout));
    
    ODS("Resetting transit fence\n");
    vkResetFences(vk.device, 1, &fence);
}

void GetFormatAndColorspace()
{
    VkResult result;
    
    u32 surface_formatcount = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(vk.gpu, vk.surface, &surface_formatcount, NULL);
    ODS("Surface formats(count): %s\n", RevEnum(vk_enums.result_enum, result));
    VkSurfaceFormatKHR *surface_formats = (VkSurfaceFormatKHR *)malloc(sizeof(VkSurfaceFormatKHR) * surface_formatcount);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(vk.gpu, vk.surface, &surface_formatcount, surface_formats);
    ODS("Surface formats(fill):  %s\n", RevEnum(vk_enums.result_enum, result));
    
    ODS("> Surface formats:\n");
    for(u32 i = 0; i < surface_formatcount; i++)
    {
        ODS("FormatKHR %d:\n", i);
        ODS("- format:      %s\n", RevEnum(vk_enums.format_enum,     surface_formats[i].format));
        ODS("- color space: %s\n", RevEnum(vk_enums.colorspace_enum, surface_formats[i].colorSpace));
    }
    ODS("\n");
    
    ODS("> Surface properties:\n");
    for(u32 i = 0; i < surface_formatcount; i++)
    {
        VkFormatProperties surface_props;
        vkGetPhysicalDeviceFormatProperties(vk.gpu, surface_formats[i].format, &surface_props);
        
        ODS("> Format %d:\n", i);
        ODS("Linear  tiling:  %s\n", DecToBin(surface_props.linearTilingFeatures));
        ODS("Optimal tiling:  %s\n", DecToBin(surface_props.optimalTilingFeatures));
        ODS("Buffer features: %s\n", DecToBin(surface_props.bufferFeatures));
        ODS("\n");
    }
    
    vk.format = surface_formats[0].format;
    vk.color_space = surface_formats[0].colorSpace;
}

void CheckSurfaceCapabilities()
{
    VkResult result;
    
    VkSurfaceCapabilitiesKHR surface_caps;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.gpu, vk.surface, &surface_caps);
    ODS("Physical device surface capabilities result: %s\n", RevEnum(vk_enums.result_enum, result));
    
    ODS("> Surface capabilities:\n");
    ODS("- min images: %d\n", surface_caps.minImageCount);
    ODS("- max images: %d\n", surface_caps.maxImageCount);
    ODS("- current extent: %-4d x %-4d\n", surface_caps.currentExtent.width,  surface_caps.currentExtent.height);
    ODS("- minimal extent: %-4d x %-4d\n", surface_caps.minImageExtent.width, surface_caps.minImageExtent.height);
    ODS("- maximal extent: %-4d x %-4d\n", surface_caps.maxImageExtent.width, surface_caps.maxImageExtent.height);
    ODS("- max image arrays: %d\n", surface_caps.maxImageArrayLayers);
    ODS("- Supported transforms:      %s\n", DecToBin(surface_caps.supportedTransforms));
    ODS("- Current transform:         %s\n", DecToBin(surface_caps.currentTransform));
    ODS("- Supported composite alpha: %s\n", DecToBin(surface_caps.supportedCompositeAlpha));
    ODS("- Supported usage flags:     %s\n", DecToBin(surface_caps.supportedUsageFlags));
    ODS("\n");
}

void SetPresentMode()
{
    VkResult result;
    
    u32 present_modecount;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(vk.gpu, vk.surface, &present_modecount, NULL);
    ODS("Surface formats(count): %s\n", RevEnum(vk_enums.result_enum, result));
    VkPresentModeKHR *present_modes = (VkPresentModeKHR *)malloc(sizeof(VkPresentModeKHR) * present_modecount);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(vk.gpu, vk.surface, &present_modecount, present_modes);
    ODS("Surface formats(fill):  %s\n", RevEnum(vk_enums.result_enum, result));
    
    ODS("> Present modes:\n");
    for(u32 i = 0; i < present_modecount; i++)
    {
        char *mode = RevEnum(vk_enums.presentmode_enum, present_modes[i]);
        ODS("Mode %d: %s\n", i, mode);
        
        if(strstr(mode, "MAILBOX"))
            vk.present_mode = present_modes[i];
    }
    ODS("Chosen present mode: %s\n", RevEnum(vk_enums.presentmode_enum, vk.present_mode));
    ODS("\n");
}

void CreateImage(VkImage *image, VkDeviceMemory *memory,
                 s32 width, s32 height,
                 VkImageTiling tiling, u32 usage,
                 VkImageLayout layout, VkMemoryPropertyFlags memory_properties)
{
    VkExtent3D extent = {};
    extent.width  = width;
    extent.height = height;
    extent.depth  = 1;
    
    // ---
    VkImageCreateInfo image_ci = {};
    image_ci.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_ci.pNext                 = NULL;
    image_ci.flags                 = 0;
    image_ci.imageType             = VK_IMAGE_TYPE_2D;
    image_ci.format                = vk.format;
    image_ci.extent                = extent;
    image_ci.mipLevels             = 1;
    image_ci.arrayLayers           = 1;
    image_ci.samples               = VK_SAMPLE_COUNT_1_BIT;
    image_ci.tiling                = tiling;
    image_ci.usage                 = usage;
    image_ci.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    image_ci.queueFamilyIndexCount = 0;
    image_ci.pQueueFamilyIndices   = NULL;
    image_ci.initialLayout         = layout;
    
    vkCreateImage(vk.device, &image_ci, NULL, image);
    // ---
    
    // ---
    VkMemoryRequirements memreqs;
    vkGetImageMemoryRequirements(vk.device, *image, &memreqs);
    
    VkMemoryAllocateInfo mem_ai = {};
    mem_ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_ai.pNext           = NULL;
    mem_ai.allocationSize  = memreqs.size;
    mem_ai.memoryTypeIndex = FindMemoryIndex(memreqs.memoryTypeBits,
                                             memory_properties,
                                             vk.gpu_memprops);
    
    VkResult result;
    result = vkAllocateMemory(vk.device, &mem_ai, NULL, memory);
    ODS("Memory allocation: %s\n", RevEnum(vk_enums.result_enum, result));
    
    vkBindImageMemory(vk.device, *image, *memory, 0);
    // ---
}

void CreateImageView(VkImage *image, VkImageView *imageview)
{
    VkComponentMapping components = {};
    components.r = VK_COMPONENT_SWIZZLE_B;
    components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    components.b = VK_COMPONENT_SWIZZLE_R;
    components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    
    VkImageViewCreateInfo imageview_ci = {};
    imageview_ci.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageview_ci.pNext            = NULL;
    imageview_ci.flags            = 0;
    imageview_ci.image            = *image;
    imageview_ci.viewType         = VK_IMAGE_VIEW_TYPE_2D;
    imageview_ci.format           = vk.format;
    imageview_ci.components       = components;
    imageview_ci.subresourceRange = vk.color_sr;
    
    vkCreateImageView(vk.device, &imageview_ci, NULL, imageview);
}

void CreateSampler(VkSampler *sampler)
{
    VkSamplerCreateInfo sampler_ci = {};
    sampler_ci.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_ci.pNext                   = NULL;
    sampler_ci.flags                   = 0;
    sampler_ci.magFilter               = VK_FILTER_LINEAR;
    sampler_ci.minFilter               = VK_FILTER_LINEAR;
    sampler_ci.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_ci.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.mipLodBias              = 0.0f;
    sampler_ci.anisotropyEnable        = VK_FALSE;
    sampler_ci.compareEnable           = VK_FALSE;
    sampler_ci.minLod                  = 1.0f;
    sampler_ci.maxLod                  = 1.0f;
    sampler_ci.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_ci.unnormalizedCoordinates = VK_FALSE;
    
    vkCreateSampler(vk.device, &sampler_ci, NULL, sampler);
}


u8 *ReadImage(char *filename, u32 *width, u32 *height)
{
    s32 image_channels;
    s32 force_channels = 4;
    u8 *pixels = stbi_load(filename, (s32 *)width, (s32 *)height, &image_channels, force_channels);
    
    return pixels;
}


#define RD 0

int CALLBACK WinMain(HINSTANCE instance,
                     HINSTANCE prevInstance,
                     LPSTR commandLine,
                     int showMode)
{
#if RD
    HMODULE mod = NULL;
    if(mod = LoadLibrary("D:/RenderDoc/renderdoc.dll"))
    {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
        int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_4_0, (void **)&rdoc_api);
        if(ret != 1)
        {
            ODS("Can't get RD API\n");
            exit(0);
        }
    }
    else
    {
        ODS("Can't load RD");
        exit(0);
    }
#endif
    
    
    app.instance = instance;
    app.window_width = 1280;
    app.window_height = 720;
    app.window = WindowCreate();  // should be in a struct somewhere...
    
    
    vk.format = VK_FORMAT_B8G8R8A8_UNORM;
    
    vk.components.r = VK_COMPONENT_SWIZZLE_R;
    vk.components.g = VK_COMPONENT_SWIZZLE_G;
    vk.components.b = VK_COMPONENT_SWIZZLE_B;
    vk.components.a = VK_COMPONENT_SWIZZLE_A;
    
    vk.color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    
    vk.color_sr.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    vk.color_sr.baseMipLevel   = 0;
    vk.color_sr.levelCount     = 1;
    vk.color_sr.baseArrayLayer = 0;
    vk.color_sr.layerCount     = 1;
    
    
    LoadVkEnums();
    
    Vulkan_LoadInstanceFunctions();
    EnumerateGlobalExtensions();
    EnumerateLayerExtensions();
    
    char *layer_names[] = {
        "VK_LAYER_LUNARG_assistant_layer",
        "VK_LAYER_KHRONOS_validation"
    };
    char *ext_names[] = {
        "VK_KHR_surface",
        "VK_KHR_win32_surface",
        "VK_EXT_debug_utils"
    };
    u32 layer_count = sizeof(layer_names)/sizeof(layer_names[0]);
    u32 ext_count   = sizeof(ext_names)/sizeof(ext_names[0]);
    
    GetVulkanInstance(layer_names, layer_count,
                      ext_names,   ext_count);
    ODS("Here's a Vulkan instance: 0x%p\n", &vk.instance);
    Vulkan_LoadExtensionFunctions(vk.instance);
    
    RENDERDOC_DevicePointer RD_device = RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(vk.instance);
    
    SetupDebugging();
    GetGPU();
    
    CreateSurface();
    
    SetupQueue();
    char *dev_ext_names[] = {
        "VK_KHR_swapchain"
    };
    u32 dev_ext_count = sizeof(dev_ext_names) / sizeof(dev_ext_names[0]);
    VkPhysicalDeviceFeatures features = {};
    GetVulkanDevice(dev_ext_names, dev_ext_count, features);
    ODS("Here's a Vulkan device:   0x%p\n", &vk.device);
    
    vkGetDeviceQueue(vk.device, vk.queue_family_index, 0, &vk.queue);
    
    CheckDeviceMemoryProperties();
    
    vk.screenextent.width  = app.window_width;
    vk.screenextent.height = app.window_height;
    
    CheckSurfaceCapabilities();
    GetFormatAndColorspace();
    SetPresentMode();
    
    SwapchainCreate();
    GetSwapchainImages();
    CreateSwapchainImageViews();
    
    
    
    VkCommandPoolCreateInfo commandpool_ci = {};
    commandpool_ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandpool_ci.pNext            = NULL;
    commandpool_ci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandpool_ci.queueFamilyIndex = 0;
    VkCommandPool commandpool;
    vkCreateCommandPool(vk.device, &commandpool_ci, NULL, &commandpool);
    
    VkCommandBufferAllocateInfo commandbuffer_ai = {};
    commandbuffer_ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandbuffer_ai.pNext              = NULL;
    commandbuffer_ai.commandPool        = commandpool;
    commandbuffer_ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandbuffer_ai.commandBufferCount = 1;
    
    VkCommandBuffer commandbuffer;
    vkAllocateCommandBuffers(vk.device, &commandbuffer_ai, &commandbuffer);
    
    VkCommandBufferBeginInfo commandbuffer_bi = {};
    commandbuffer_bi.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandbuffer_bi.pNext            = NULL;
    commandbuffer_bi.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    
    
    
    VkFenceCreateInfo fence_ci = {};
    fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_ci.pNext = NULL;
    fence_ci.flags = 0;
    
    VkFence fence;
    vkCreateFence(vk.device, &fence_ci, NULL, &fence);
    
    
    VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkSubmitInfo transit_si = {};
    transit_si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    transit_si.pNext                = NULL;
    transit_si.pWaitDstStageMask    = &wait_stage_mask;
    transit_si.commandBufferCount   = 1;
    transit_si.pCommandBuffers      = &commandbuffer;
    
    VkImageMemoryBarrier swap0_barrier = {};
    swap0_barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    swap0_barrier.pNext               = NULL;
    swap0_barrier.srcAccessMask       = VK_ACCESS_HOST_READ_BIT;
    swap0_barrier.dstAccessMask       = VK_ACCESS_HOST_WRITE_BIT;
    swap0_barrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    swap0_barrier.newLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    swap0_barrier.image               = vk.swapchain_images[0];
    swap0_barrier.subresourceRange    = vk.color_sr;
    
    VkImageMemoryBarrier swap1_barrier = {};
    swap1_barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    swap1_barrier.pNext               = NULL;
    swap1_barrier.srcAccessMask       = VK_ACCESS_HOST_READ_BIT;
    swap1_barrier.dstAccessMask       = VK_ACCESS_HOST_WRITE_BIT;
    swap1_barrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    swap1_barrier.newLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    swap1_barrier.image               = vk.swapchain_images[1];
    swap1_barrier.subresourceRange    = vk.color_sr;
    
    
    vkBeginCommandBuffer(commandbuffer, &commandbuffer_bi);
    vkCmdPipelineBarrier(commandbuffer,
                         VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                         0,
                         0, NULL,
                         0, NULL,
                         1, &swap0_barrier);
    vkCmdPipelineBarrier(commandbuffer,
                         VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                         0,
                         0, NULL,
                         0, NULL,
                         1, &swap1_barrier);
    vkEndCommandBuffer(commandbuffer);
    vkQueueSubmit(vk.queue, 1, &transit_si, fence);
    
    vkWaitForFences(vk.device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vk.device, 1, &fence);
    
    
    
    // Resources
    // - open the rabbit
    // - load the vertexes
    
    
    
    // Compute
    VkImage computed_image;
    VkImageView computed_imageview;
    VkDeviceMemory computed_imagememory;
    
    CreateImage(&computed_image, &computed_imagememory, app.window_width, app.window_height,
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    CreateImageView(&computed_image, &computed_imageview);
    
    VkImageMemoryBarrier barrier = {};
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext               = NULL;
    barrier.srcAccessMask       = VK_ACCESS_HOST_READ_BIT;
    barrier.dstAccessMask       = VK_ACCESS_HOST_WRITE_BIT;
    barrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout           = VK_IMAGE_LAYOUT_GENERAL;
    barrier.image               = computed_image;
    barrier.subresourceRange    = vk.color_sr;
    
    vkBeginCommandBuffer(commandbuffer, &commandbuffer_bi);
    vkCmdPipelineBarrier(commandbuffer,
                         VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                         0,
                         0, NULL,
                         0, NULL,
                         1, &barrier);
    vkEndCommandBuffer(commandbuffer);
    vkQueueSubmit(vk.queue, 1, &transit_si, fence);
    
    vkWaitForFences(vk.device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vk.device, 1, &fence);
    
    
    
    
    
    char *shader = "../code/shader_comp.spv";
    
    VkPipelineShaderStageCreateInfo stage_ci = {};
    stage_ci.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_ci.pNext               = NULL;
    stage_ci.flags               = 0;
    stage_ci.stage               = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_ci.module              = GetShaderModule(shader);
    stage_ci.pName               = "main";
    stage_ci.pSpecializationInfo = NULL;
    
    
    VkDescriptorSetLayoutBinding binding = {};
    binding.binding            = 0;
    binding.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    binding.descriptorCount    = 1;
    binding.stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    binding.pImmutableSamplers = NULL;
    
    VkDescriptorSetLayoutCreateInfo dsl_ci = {};
    dsl_ci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsl_ci.pNext        = NULL;
    dsl_ci.flags        = 0;
    dsl_ci.bindingCount = 1;
    dsl_ci.pBindings    = &binding;
    VkDescriptorSetLayout dslayout;
    vkCreateDescriptorSetLayout(vk.device, &dsl_ci, NULL, &dslayout);
    
    VkDescriptorPoolSize dspool_size = {};
    dspool_size.type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    dspool_size.descriptorCount = 1;
    
    VkDescriptorPoolCreateInfo dspool_ci = {};
    dspool_ci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dspool_ci.pNext         = NULL;
    dspool_ci.flags         = 0;
    dspool_ci.maxSets       = 1;
    dspool_ci.poolSizeCount = 1;
    dspool_ci.pPoolSizes    = &dspool_size;
    VkDescriptorPool dspool;
    vkCreateDescriptorPool(vk.device, &dspool_ci, NULL, &dspool);
    
    VkDescriptorSetAllocateInfo dsl_ai = {};
    dsl_ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsl_ai.pNext              = NULL;
    dsl_ai.descriptorPool     = dspool;
    dsl_ai.descriptorSetCount = 1;
    dsl_ai.pSetLayouts        = &dslayout;
    vkAllocateDescriptorSets(vk.device, &dsl_ai, &vk.dsl);
    
    VkPipelineLayoutCreateInfo layout_ci = {};
    layout_ci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_ci.pNext                  = NULL;
    layout_ci.flags                  = 0;
    layout_ci.setLayoutCount         = 1;
    layout_ci.pSetLayouts            = &dslayout;
    layout_ci.pushConstantRangeCount = 0;
    layout_ci.pPushConstantRanges    = NULL;
    vkCreatePipelineLayout(vk.device, &layout_ci, NULL, &vk.pipelayout);
    
    
    VkComputePipelineCreateInfo compipe_ci = {};
    compipe_ci.sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    compipe_ci.pNext              = NULL;
    compipe_ci.flags              = 0;
    compipe_ci.stage              = stage_ci;
    compipe_ci.layout             = vk.pipelayout;
    compipe_ci.basePipelineHandle = NULL;
    compipe_ci.basePipelineIndex  = 0;
    
    vkCreateComputePipelines(vk.device, NULL, 1, &compipe_ci, NULL, &vk.compipe);
    
    
    VkDescriptorImageInfo computetexture_info = {};
    computetexture_info.sampler     = NULL;
    computetexture_info.imageView   = computed_imageview;
    computetexture_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    
    VkWriteDescriptorSet compute_write = {};
    compute_write.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    compute_write.pNext            = NULL;
    compute_write.dstSet           = vk.dsl;
    compute_write.dstBinding       = 0;
    compute_write.dstArrayElement  = 0;
    compute_write.descriptorCount  = 1;
    compute_write.descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    compute_write.pImageInfo       = &computetexture_info;
    compute_write.pBufferInfo      = NULL;
    compute_write.pTexelBufferView = NULL;
    vkUpdateDescriptorSets(vk.device, 1, &compute_write, 0, NULL);
    
    
    u32 xdim = ceil(r32(app.window_width) / 32.0f);
    u32 ydim = ceil(r32(app.window_height) / 32.0f);
    
    vkBeginCommandBuffer(commandbuffer, &commandbuffer_bi);
    vkCmdBindDescriptorSets(commandbuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vk.pipelayout, 0, 1, &vk.dsl, 0, NULL);
    vkCmdBindPipeline(commandbuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vk.compipe);
    vkCmdDispatch(commandbuffer, xdim, ydim, 1);
    vkEndCommandBuffer(commandbuffer);
    
    
    
    VkSubmitInfo compute_si = {};
    compute_si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    compute_si.pNext                = NULL;
    compute_si.pWaitDstStageMask    = &wait_stage_mask;
    compute_si.commandBufferCount   = 1;
    compute_si.pCommandBuffers      = &commandbuffer;
    vkQueueSubmit(vk.queue, 1, &compute_si, fence);
    
    
    // Graphics
    VkShaderModule vert_module = GetShaderModule("../code/shader_vert.spv");
    
    VkPipelineShaderStageCreateInfo vert_stage = {};
    vert_stage.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_stage.pNext               = NULL;
    vert_stage.flags               = 0;
    vert_stage.stage               = VK_SHADER_STAGE_VERTEX_BIT;
    vert_stage.module              = vert_module;
    vert_stage.pName               = "main";
    vert_stage.pSpecializationInfo = NULL;
    
    VkShaderModule frag_module = GetShaderModule("../code/shader_frag.spv");
    
    VkPipelineShaderStageCreateInfo frag_stage = {};
    frag_stage.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_stage.pNext               = NULL;
    frag_stage.flags               = 0;
    frag_stage.stage               = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_stage.module              = frag_module;
    frag_stage.pName               = "main";
    frag_stage.pSpecializationInfo = NULL;
    
    VkPipelineShaderStageCreateInfo shader_stages[] = { vert_stage, frag_stage };
    
    
    VkVertexInputBindingDescription vertex_binding = {};
    vertex_binding.binding   = 0;
    vertex_binding.stride    = 6 * sizeof(float);
    vertex_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    VkVertexInputAttributeDescription vert_description = {};
    vert_description.location = 0;
    vert_description.binding  = vertex_binding.binding;
    vert_description.format   = VK_FORMAT_R32G32B32A32_SFLOAT;
    vert_description.offset   = 0;
    
    VkVertexInputAttributeDescription uv_description = {};
    uv_description.location = 1;
    uv_description.binding  = vertex_binding.binding;
    uv_description.format   = VK_FORMAT_R32G32_SFLOAT;
    uv_description.offset   = 4 * sizeof(float);
    
    VkVertexInputAttributeDescription vertex_attributes[] = { vert_description, uv_description };
    
    VkPipelineVertexInputStateCreateInfo vertex_state = {};
    vertex_state.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_state.pNext                           = NULL;
    vertex_state.flags                           = 0;
    vertex_state.vertexBindingDescriptionCount   = 1;
    vertex_state.pVertexBindingDescriptions      = &vertex_binding;
    vertex_state.vertexAttributeDescriptionCount = sizeof(vertex_attributes)/sizeof(vertex_attributes[0]);
    vertex_state.pVertexAttributeDescriptions    = vertex_attributes;
    ODS("Vert attr count: %d\n", vertex_state.vertexAttributeDescriptionCount);
    
    
    VkPipelineInputAssemblyStateCreateInfo inputassembly_state = {};
    inputassembly_state.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputassembly_state.pNext                  = NULL;
    inputassembly_state.flags                  = 0;
    inputassembly_state.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputassembly_state.primitiveRestartEnable = NULL;
    
    
    VkViewport viewport = {};
    viewport.x        = 0;
    viewport.y        = 0;
    viewport.width    = app.window_width;
    viewport.height   = app.window_height;
    viewport.minDepth = 0;
    viewport.maxDepth = 1;
    
    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width  = app.window_width;
    scissor.extent.height = app.window_height;
    
    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.pNext         = NULL;
    viewport_state.flags         = 0;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports    = &viewport;
    viewport_state.scissorCount  = 1;
    viewport_state.pScissors     = &scissor;
    
    
    VkPipelineRasterizationStateCreateInfo raster_state = {};
    raster_state.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster_state.pNext                   = NULL;
    raster_state.flags                   = 0;
    raster_state.polygonMode             = VK_POLYGON_MODE_FILL;
    raster_state.cullMode                = VK_CULL_MODE_BACK_BIT;
    raster_state.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster_state.lineWidth               = 1.0f;
    
    
    VkPipelineMultisampleStateCreateInfo multisample_state = {};
    multisample_state.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state.pNext                 = NULL;
    multisample_state.flags                 = 0;
    multisample_state.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    
    
    VkPipelineColorBlendAttachmentState colorblend_attachment = {};
    colorblend_attachment.blendEnable         = VK_FALSE;
    colorblend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorblend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorblend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
    colorblend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorblend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorblend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;
    colorblend_attachment.colorWriteMask      = 0xF;
    
    VkPipelineColorBlendStateCreateInfo colorblend_state = {};
    colorblend_state.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorblend_state.pNext           = NULL;
    colorblend_state.flags           = 0;
    colorblend_state.attachmentCount = 1;
    colorblend_state.pAttachments    = &colorblend_attachment;
    
    
    
    // descriptor set layout
    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.binding            = 0;
    dsl_binding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    dsl_binding.descriptorCount    = 1;
    dsl_binding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo descriptorset_layoutci = {};
    descriptorset_layoutci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorset_layoutci.pNext        = NULL;
    descriptorset_layoutci.flags        = 0;
    descriptorset_layoutci.bindingCount = 1;
    descriptorset_layoutci.pBindings    = &dsl_binding;
    
    
    VkDescriptorSetLayout descriptorset_layout;
    vkCreateDescriptorSetLayout(vk.device, &descriptorset_layoutci, NULL, &descriptorset_layout);
    
    VkPipelineLayoutCreateInfo pipelinelayout_ci = {};
    pipelinelayout_ci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelinelayout_ci.pNext                  = NULL;
    pipelinelayout_ci.flags                  = 0;
    pipelinelayout_ci.setLayoutCount         = 1;
    pipelinelayout_ci.pSetLayouts            = &descriptorset_layout;
    
    VkPipelineLayout pipelinelayout;
    vkCreatePipelineLayout(vk.device, &pipelinelayout_ci, NULL, &pipelinelayout);
    
    
    
    VkAttachmentReference subpass_attachmentreference = {};
    subpass_attachmentreference.attachment = 0;
    subpass_attachmentreference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass = {};
    subpass.flags                   = 0;
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &subpass_attachmentreference;
    
    
    VkAttachmentDescription renderpass_attachment = {};
    renderpass_attachment.flags          = 0;
    renderpass_attachment.format         = vk.format;
    renderpass_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    renderpass_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD;
    renderpass_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    renderpass_attachment.initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    renderpass_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkRenderPassCreateInfo renderpass_ci = {};
    renderpass_ci.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderpass_ci.pNext           = NULL;
    renderpass_ci.flags           = 0;
    renderpass_ci.attachmentCount = 1;
    renderpass_ci.pAttachments    = &renderpass_attachment;
    renderpass_ci.subpassCount    = 1;
    renderpass_ci.pSubpasses      = &subpass;
    
    VkRenderPass renderpass;
    vkCreateRenderPass(vk.device, &renderpass_ci, NULL, &renderpass);
    
    
    VkGraphicsPipelineCreateInfo graphics_pipeline_ci = {};
    graphics_pipeline_ci.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_ci.pNext               = NULL;
    graphics_pipeline_ci.flags               = 0;
    graphics_pipeline_ci.stageCount          = 2;
    graphics_pipeline_ci.pStages             = shader_stages;
    graphics_pipeline_ci.pVertexInputState   = &vertex_state;
    graphics_pipeline_ci.pInputAssemblyState = &inputassembly_state;
    graphics_pipeline_ci.pTessellationState  = NULL;
    graphics_pipeline_ci.pViewportState      = &viewport_state;
    graphics_pipeline_ci.pRasterizationState = &raster_state;
    graphics_pipeline_ci.pMultisampleState   = &multisample_state;
    graphics_pipeline_ci.pDepthStencilState  = NULL;
    graphics_pipeline_ci.pColorBlendState    = &colorblend_state;
    graphics_pipeline_ci.pDynamicState       = NULL;
    graphics_pipeline_ci.layout              = pipelinelayout;
    graphics_pipeline_ci.renderPass          = renderpass;
    graphics_pipeline_ci.subpass             = 0;
    
    VkPipeline graphics_pipeline;
    vkCreateGraphicsPipelines(vk.device, NULL, 1, &graphics_pipeline_ci, NULL, &graphics_pipeline);
    
    
    VkDescriptorPoolSize descriptorpool_size = {};
    descriptorpool_size.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorpool_size.descriptorCount = 1;
    
    VkDescriptorPoolCreateInfo descriptorpool_ci = {};
    descriptorpool_ci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorpool_ci.pNext         = NULL;
    descriptorpool_ci.flags         = 0;
    descriptorpool_ci.maxSets       = 1;
    descriptorpool_ci.poolSizeCount = 1;
    descriptorpool_ci.pPoolSizes    = &descriptorpool_size;
    
    VkDescriptorPool descriptorpool;
    vkCreateDescriptorPool(vk.device, &descriptorpool_ci, NULL, &descriptorpool);
    
    
    VkDescriptorSetAllocateInfo descriptorset_ai = {};
    descriptorset_ai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorset_ai.pNext              = NULL;
    descriptorset_ai.descriptorPool     = descriptorpool;
    descriptorset_ai.descriptorSetCount = 1;
    descriptorset_ai.pSetLayouts        = &descriptorset_layout;
    
    VkDescriptorSet descriptorset;
    vkAllocateDescriptorSets(vk.device, &descriptorset_ai, &descriptorset);
    
    
    
    
    VkPhysicalDeviceMemoryProperties gpu_memprops;
    vkGetPhysicalDeviceMemoryProperties(vk.gpu, &gpu_memprops);
    
    VkDeviceMemory staging_memory;
    VkBuffer staging_buffer = CreateBuffer(10 * 1024 * 1024,
                                           VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                           vk.device,
                                           gpu_memprops,
                                           &staging_memory);
    
    u32 width = 0, height = 0;
    u8 *girlpixels = ReadImage("../assets/image.jpg", &width, &height);
    u32 girlbytes = app.window_width * app.window_height * 4;
    
    void *stagingmapptr;
    vkMapMemory(vk.device, staging_memory, 0, girlbytes, 0, &stagingmapptr);
    memcpy(stagingmapptr, girlpixels, girlbytes);
    vkUnmapMemory(vk.device, staging_memory);
    
    
    VkImage girlimage;
    VkDeviceMemory girlmemory;
    VkImageView girlview;
    CreateImage(&girlimage, &girlmemory, app.window_width, app.window_height,
                VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                VK_IMAGE_LAYOUT_PREINITIALIZED, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    CreateImageView(&girlimage, &girlview);
    
    VkFence stagingfence;
    vkCreateFence(vk.device, &fence_ci, NULL, &stagingfence);
    VkFence transitfence;
    vkCreateFence(vk.device, &fence_ci, NULL, &transitfence);
    
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext               = NULL;
    barrier.srcAccessMask       = VK_ACCESS_HOST_READ_BIT;
    barrier.dstAccessMask       = VK_ACCESS_HOST_WRITE_BIT;
    barrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.image               = girlimage;
    barrier.subresourceRange    = vk.color_sr;
    
    vkBeginCommandBuffer(commandbuffer, &commandbuffer_bi);
    vkCmdPipelineBarrier(commandbuffer,
                         VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                         0,
                         0, NULL,
                         0, NULL,
                         1, &barrier);
    vkEndCommandBuffer(commandbuffer);
    vkQueueSubmit(vk.queue, 1, &transit_si, fence);
    
    vkWaitForFences(vk.device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vk.device, 1, &fence);
    
    VkImageSubresourceLayers srl = {};
    srl.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    srl.mipLevel       = 0;
    srl.baseArrayLayer = 0;
    srl.layerCount     = 1;
    
    VkBufferImageCopy region = {};
    region.bufferOffset       = 0;
    region.bufferRowLength    = 0;
    region.bufferImageHeight  = 0;
    region.imageSubresource   = srl;
    region.imageOffset.x      = 0;
    region.imageOffset.y      = 0;
    region.imageOffset.z      = 0;
    region.imageExtent.width  = app.window_width;
    region.imageExtent.height = app.window_height;
    region.imageExtent.depth  = 1;
    
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    vkBeginCommandBuffer(commandbuffer, &commandbuffer_bi);
    vkCmdCopyBufferToImage(commandbuffer, staging_buffer, girlimage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    vkCmdPipelineBarrier(commandbuffer,
                         VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                         0,
                         0, NULL,
                         0, NULL,
                         1, &barrier);
    vkEndCommandBuffer(commandbuffer);
    vkQueueSubmit(vk.queue, 1, &transit_si, fence);
    
    vkWaitForFences(vk.device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vk.device, 1, &fence);
    
    
    
    
    VkSampler sampler;
    CreateSampler(&sampler);
    
    VkDescriptorImageInfo computed_image_info = {};
    computed_image_info.sampler     = sampler;
    //computed_image_info.imageView   = girlview;
    computed_image_info.imageView   = computed_imageview;
    computed_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    VkWriteDescriptorSet descriptor_write = {};
    descriptor_write.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.pNext            = NULL;
    descriptor_write.dstSet           = descriptorset;
    descriptor_write.dstBinding       = 0;
    descriptor_write.dstArrayElement  = 0;
    descriptor_write.descriptorCount  = 1;
    descriptor_write.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_write.pImageInfo       = &computed_image_info;
    descriptor_write.pBufferInfo      = NULL;
    descriptor_write.pTexelBufferView = NULL;
    vkUpdateDescriptorSets(vk.device, 1, &descriptor_write, 0, NULL);
    
    
    
    VkSemaphoreCreateInfo sem_ci = {};
    sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    sem_ci.pNext = NULL;
    sem_ci.flags = 0;
    
    VkSemaphore semaphore_acquired;
    vkCreateSemaphore(vk.device, &sem_ci, NULL, &semaphore_acquired);
    
    
#if RD
    rdoc_api->StartFrameCapture(RD_device, app.window);
#endif
    
    
    u32 present_index = 0;
    VkResult acqres = vkAcquireNextImageKHR(vk.device, vk.swapchain, UINT64_MAX, semaphore_acquired, NULL, &present_index);
    ODS("Acquisition result: %s\n", RevEnum(vk_enums.result_enum, acqres));
    
    vk.framebuffers = (VkFramebuffer *)calloc(2, sizeof(VkFramebuffer));
    
    VkFramebufferCreateInfo framebuffer_ci = {};
    framebuffer_ci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_ci.pNext           = NULL;
    framebuffer_ci.flags           = 0;
    framebuffer_ci.renderPass      = renderpass;
    framebuffer_ci.attachmentCount = 1;
    framebuffer_ci.width           = app.window_width;
    framebuffer_ci.height          = app.window_height;
    framebuffer_ci.layers          = 1;
    for(u32 i = 0; i < 2; i++)
    {
        framebuffer_ci.pAttachments = &vk.swapchain_imageviews[i];
        vkCreateFramebuffer(vk.device, &framebuffer_ci, NULL, &vk.framebuffers[i]);
    }
    
    
    VkDeviceMemory vertex_memory;
    VkBuffer vertex_buffer = CreateBuffer(4 * 6 * sizeof(float),
                                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                          vk.device,
                                          gpu_memprops,
                                          &vertex_memory);
    
    struct vertex
    {
        r32 x, y, z, w, u, v;
    };
    vertex v0 = { -1.0f, -1.0f, 0.0f, 1.0f,  0.0f, 0.0f };
    vertex v1 = { -1.0f,  1.0f, 0.0f, 1.0f,  0.0f, 1.0f };
    vertex v2 = {  1.0f, -1.0f, 0.0f, 1.0f,  1.0f, 0.0f };
    vertex v3 = {  1.0f,  1.0f, 0.0f, 1.0f,  1.0f, 1.0f };
    vertex quad[] = { v0, v1, v2, v3 };
    
    void *vertex_mapptr;
    vkMapMemory(vk.device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &vertex_mapptr);
    memcpy(vertex_mapptr, quad, sizeof(float) * 6 * 4);
    vkUnmapMemory(vk.device, vertex_memory);
    
    
    VkDeviceMemory index_memory;
    VkBuffer index_buffer = CreateBuffer(6 * sizeof(u32),
                                         VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                         vk.device,
                                         gpu_memprops,
                                         &index_memory);
    
    u32 indexes[] = { 0, 1, 2, 1, 3, 2 };
    
    void *index_mapptr;
    vkMapMemory(vk.device, index_memory, 0, VK_WHOLE_SIZE, 0, &index_mapptr);
    memcpy(index_mapptr, indexes, sizeof(u32) * 6);
    vkUnmapMemory(vk.device, index_memory);
    
    
    VkClearValue clear_color = { 1.0f, 0.4f, 0.7f, 1.0f };
    
    VkRenderPassBeginInfo renderpass_bi = {};
    renderpass_bi.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpass_bi.pNext           = NULL;
    renderpass_bi.renderPass      = renderpass;
    renderpass_bi.framebuffer     = vk.framebuffers[present_index];
    renderpass_bi.renderArea      = scissor;
    renderpass_bi.clearValueCount = 1;
    renderpass_bi.pClearValues    = &clear_color;
    
    
    VkDeviceSize offset = 0;
    
    vkBeginCommandBuffer(commandbuffer, &commandbuffer_bi);
    vkCmdBindVertexBuffers(commandbuffer, 0, 1, &vertex_buffer, &offset);
    vkCmdBindIndexBuffer(commandbuffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
    vkCmdBindDescriptorSets(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinelayout, 0, 1, &descriptorset, 0, NULL);
    vkCmdBeginRenderPass(commandbuffer, &renderpass_bi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdDrawIndexed(commandbuffer, 6, 1, 0, 0, 0);
    vkCmdEndRenderPass(commandbuffer);
    vkEndCommandBuffer(commandbuffer);
    
    
    VkFence fence_rendered;
    vkCreateFence(vk.device, &fence_ci, NULL, &fence_rendered);
    
    VkPipelineStageFlags waitmask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    
    VkSubmitInfo si = {};
    si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.pNext                = NULL;
    si.pWaitDstStageMask    = &waitmask;
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &commandbuffer;
    vkQueueSubmit(vk.queue, 1, &si, fence_rendered);
    VkResult res = vkWaitForFences(vk.device, 1, &fence_rendered, VK_TRUE, UINT64_MAX);
    ODS("Render wait result: %s\n", RevEnum(vk_enums.result_enum, res));
    
    
    VkResult result;
    VkPresentInfoKHR pi = {};
    pi.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.pNext              = NULL;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores    = &semaphore_acquired;
    pi.swapchainCount     = 1;
    pi.pSwapchains        = &vk.swapchain;
    pi.pImageIndices      = &present_index;
    pi.pResults           = &result;
    vkQueuePresentKHR(vk.queue, &pi);
    ODS("Present result: %s\n", RevEnum(vk_enums.result_enum, result));
    
#if RD
    rdoc_api->EndFrameCapture(RD_device, app.window);
    rdoc_api->GetCapture(0, "V:/core/capture.doc", NULL, NULL);
#endif
    
    MSG msg;
    bool done = false;
    while(!done)
    {
        PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE);
        if(msg.message == WM_QUIT)
            done = true;
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        //InputProcessing();
        //DataProcessing();
        RedrawWindow(app.window, NULL, NULL, RDW_INTERNALPAINT);
    }
    
    return msg.wParam;
}
