
// SSSTANDARD
#include <stdio.h>
#include <stdlib.h>
#include <shlwapi.h>



// VULKANSSS
#define VK_USE_PLATFORM_WIN32_KHR
#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"
#include "vulkan/vk_platform.h"

VkResult result;

// LIBSSS
#define OBJ_PARSE_IMPLEMENTATION
#include "../lib/obj_parse.h"

// SSSTB
#define STB_IMAGE_IMPLEMENTATION
#include "../lib/stb_image.h"

// PRECIOUSSS
#include "types.h"
#include "special_symbols.h"
#include "source.h"
#include "vk_extract.h"
#include "file_io.h"
#include "tokenizer.cpp"
#include "vk_prepare.h"
#include "time.h"
//#include "vk_setup.h"


void GetVulkanInstance(char **ext_names,   u32 ext_count,
                       char **layer_names, u32 layer_count)
{
    VkApplicationInfo appinfo = {};
    appinfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appinfo.pNext              = NULL;
    appinfo.pApplicationName   = "Toy RT";
    appinfo.applicationVersion = 0;
    appinfo.pEngineName        = "Toy RT Engine";
    appinfo.engineVersion      = 0;
    appinfo.apiVersion         = VK_MAKE_VERSION(1, 1, 126);
    
    VkInstanceCreateInfo instance_ci = {};
    instance_ci.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_ci.pNext                   = NULL;
    instance_ci.flags                   = 0;
    instance_ci.pApplicationInfo        = NULL;
    instance_ci.enabledLayerCount       = layer_count;
    instance_ci.ppEnabledLayerNames     = layer_names;
    instance_ci.enabledExtensionCount   = ext_count;
    instance_ci.ppEnabledExtensionNames = ext_names;
    
    result = vkCreateInstance(&instance_ci, NULL, &vk.instance);
    
    RevEnum(vk_enums.result_enum, result);
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
    RevEnum(vk_enums.result_enum, result);
    ODS_RES("Debug utils messenger creation: %s\n");
}


void GetGPU()
{
    u32 gpu_count = 0;
    vkEnumeratePhysicalDevices(vk.instance, &gpu_count, NULL);
    VkPhysicalDevice *gpus = (VkPhysicalDevice *)calloc(gpu_count, sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(vk.instance, &gpu_count, gpus);
    
    if(gpu_count == 1)
        vk.gpu = gpus[0];
    
    free(gpus);
}

void CreateSurface()
{
    VkWin32SurfaceCreateInfoKHR surface_ci = {};
    surface_ci.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_ci.pNext     = NULL;
    surface_ci.flags     = 0;
    surface_ci.hinstance = app.instance;
    surface_ci.hwnd      = app.window;
    
    result = vkCreateWin32SurfaceKHR(vk.instance, &surface_ci, NULL, &vk.surface);
    RevEnum(vk_enums.result_enum, result);
    ODS_RES("Surface creation: %s\n");
}
void SetupQueue()
{
    // ---
    u32 queuefam_propcount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vk.gpu, &queuefam_propcount, NULL);
    VkQueueFamilyProperties *queue_famprops = (VkQueueFamilyProperties *)malloc(sizeof(VkQueueFamilyProperties) * queuefam_propcount);  // FREE
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
    
    free(queue_famprops);
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

#if 0
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
#endif

char *DecToBin(u64 n, u32 width)
{
    char *binnum = (char *)malloc(width+1);  // FREE after ODS
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
char *DecToBin(u64 n)
{
    return DecToBin(n, 64);
}


void CheckDeviceMemoryProperties()
{
    vkGetPhysicalDeviceMemoryProperties(vk.gpu, &vk.gpu_memprops);
    
    ODS("> Memory properties: \n");
    ODS("- memory heap count: %d\n", vk.gpu_memprops.memoryHeapCount);
    for(u32 i = 0; i < vk.gpu_memprops.memoryHeapCount; i++)
    {
        char *flags = DecToBin(vk.gpu_memprops.memoryHeaps[i].flags);
        
        ODS("-- heap: %d: \n", i);
        ODS("size:  %zd MB\n", vk.gpu_memprops.memoryHeaps[i].size / (1024 * 1024));
        ODS("flags: %s  \n", flags);
        ODS("\n");
        
        free(flags);
    }
    ODS("- memory type count: %d\n", vk.gpu_memprops.memoryTypeCount);
    for(u32 i = 0; i < vk.gpu_memprops.memoryTypeCount; i++)
    {
        char *type = DecToBin(vk.gpu_memprops.memoryTypes[i].propertyFlags);
        
        ODS("-- memory: %d: \n", i);
        ODS("heap index: %d \n", vk.gpu_memprops.memoryTypes[i].heapIndex);
        ODS("type:       %s \n", type);
        ODS("\n");
        
        free(type);
    }
    ODS("\n");
}


// Finding a memory type index
u32 FindMemoryIndex(u32 possibleMemoryIndexes, u32 requiredProperties,
                    VkPhysicalDeviceMemoryProperties memoryProperties)
{
    char *possible_indexes = DecToBin(possibleMemoryIndexes);
    char *flags            = DecToBin(requiredProperties);
    ODS("Possible indexes:\n%.*s\n", 64, possible_indexes);
    ODS("Looking for these flags:\n%.*s\n", 64, flags);
    free(possible_indexes);
    free(flags);
    
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
    VkCommandPoolCreateInfo commandpool_ci = {};
    commandpool_ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandpool_ci.pNext            = NULL;
    commandpool_ci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandpool_ci.queueFamilyIndex = vk.queue_family_index;
    
    result = vkCreateCommandPool(vk.device, &commandpool_ci, NULL, &(*command_pool));
    ODS_RES("Command pool result: %s\n");
}

void AllocateCommandBuffer(VkCommandPool command_pool, VkCommandBuffer *command_buffer)
{
    VkCommandBufferAllocateInfo cb_ai = {};
    cb_ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cb_ai.pNext              = NULL;
    cb_ai.commandPool        = command_pool;
    cb_ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cb_ai.commandBufferCount = 1;
    
    result = vkAllocateCommandBuffers(vk.device, &cb_ai, &vk.cbuffer);
    ODS_RES("Command buffer allocation: %s\n");
}



void SwapchainCreate()
{
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
    ODS_RES("Swapchain result: %s\n");
}
void GetSwapchainImages()
{
    u32 swapchain_imagecount = 0;
    result = vkGetSwapchainImagesKHR(vk.device, vk.swapchain, &swapchain_imagecount, NULL);
    ODS_RES("Swapchain images(count): %s\n");
    
    vk.swapchain_images = (VkImage *)malloc(sizeof(VkImage) * swapchain_imagecount);
    result = vkGetSwapchainImagesKHR(vk.device, vk.swapchain, &swapchain_imagecount, vk.swapchain_images);
    ODS_RES("Swapchain images(fill):  %s\n");
}

void CreateSwapchainImageViews()
{
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
        ODS_RES("Swapchain imageview result: %s\n");
    }
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
    vk.commandbuffer_bi.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vk.commandbuffer_bi.pNext            = NULL;
    vk.commandbuffer_bi.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(command_buffer, &vk.commandbuffer_bi);
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
    result = vkGetFenceStatus(vk.device, fence);
    ODS_RES("Transition fence result: %s\n");
    
    char *oldlayoutstring = RevEnum_outstr(vk_enums.imagelayout_enum, old_layout);
    char *newlayoutstring = RevEnum_outstr(vk_enums.imagelayout_enum, new_layout);
    ODS("> %s -> %s\n", oldlayoutstring, newlayoutstring);
    free(oldlayoutstring);
    free(newlayoutstring);
    
    ODS("Resetting transit fence\n");
    vkResetFences(vk.device, 1, &fence);
}


void Render()
{
    u32 present_index = 0;
    result = vkAcquireNextImageKHR(vk.device, vk.swapchain, UINT64_MAX, vk.semaphore_acquired, NULL, &present_index);
    ODS_RES("Acquisition result: %s\n");
    
    
    
    VkFramebufferCreateInfo framebuffer_ci = {};
    framebuffer_ci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_ci.pNext           = NULL;
    framebuffer_ci.flags           = 0;
    framebuffer_ci.renderPass      = vk.renderpass;
    framebuffer_ci.attachmentCount = 1;
    framebuffer_ci.width           = app.window_width;
    framebuffer_ci.height          = app.window_height;
    framebuffer_ci.layers          = 1;
    for(u32 i = 0; i < 2; i++)
    {
        framebuffer_ci.pAttachments = &vk.swapchain_imageviews[i];
        vkCreateFramebuffer(vk.device, &framebuffer_ci, NULL, &vk.framebuffers[i]);
    }
    
    
    
    VkClearValue clear_color = { 1.0f, 0.4f, 0.7f, 1.0f };
    
    VkRenderPassBeginInfo renderpass_bi = {};
    renderpass_bi.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpass_bi.pNext           = NULL;
    renderpass_bi.renderPass      = vk.renderpass;
    renderpass_bi.framebuffer     = vk.framebuffers[present_index];
    renderpass_bi.renderArea      = vk.scissor;
    renderpass_bi.clearValueCount = 1;
    renderpass_bi.pClearValues    = &clear_color;
    
    
    
    VkDeviceSize offset = 0;
    
    vkBeginCommandBuffer(vk.commandbuffer, &vk.commandbuffer_bi);
    vkCmdBindVertexBuffers(vk.commandbuffer, 0, 1, &rendering.vertex_buffer, &offset);
    vkCmdBindIndexBuffer(vk.commandbuffer, rendering.index_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(vk.commandbuffer, vk.bindpoint_graphics, vk.graphics_pipeline);
    vkCmdBindDescriptorSets(vk.commandbuffer, vk.bindpoint_graphics, vk.pipelinelayout, 0, 1, &vk.descriptorset, 0, NULL);
    vkCmdBeginRenderPass(vk.commandbuffer, &renderpass_bi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdDrawIndexed(vk.commandbuffer, 6, 1, 0, 0, 0);
    vkCmdEndRenderPass(vk.commandbuffer);
    vkEndCommandBuffer(vk.commandbuffer);
    
    
    VkFence fence_rendered;
    vkCreateFence(vk.device, &vk.fence_ci, NULL, &fence_rendered);
    
    VkPipelineStageFlags waitmask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    
    VkSubmitInfo si = {};
    si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.pNext                = NULL;
    si.pWaitDstStageMask    = &waitmask;
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &vk.commandbuffer;
    vkQueueSubmit(vk.queue, 1, &si, fence_rendered);
    result = vkWaitForFences(vk.device, 1, &fence_rendered, VK_TRUE, UINT64_MAX);
    ODS_RES("Render wait result: %s\n");
    
    
    VkPresentInfoKHR pi = {};
    pi.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.pNext              = NULL;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores    = &vk.semaphore_acquired;
    pi.swapchainCount     = 1;
    pi.pSwapchains        = &vk.swapchain;
    pi.pImageIndices      = &present_index;
    pi.pResults           = &result;
    vkQueuePresentKHR(vk.queue, &pi);
    ODS_RES("Present result: %s\n");
    
    vkQueueWaitIdle(vk.queue);
    
    // transit from present_src_khr to color_attachment_optimal
    TransitImageLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, vk.swapchain_images[present_index],
                       vk.queue, vk.commandbuffer, vk.fence,
                       0, NULL,
                       0, NULL);
    
    // cleanup
    vkDestroyFramebuffer(vk.device, vk.framebuffers[0], NULL);
    vkDestroyFramebuffer(vk.device, vk.framebuffers[1], NULL);
    vkDestroyFence(vk.device, fence_rendered, NULL);
}


bool display = false;

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
            if(display)
                Render();
        } break;
        
        case WM_CLOSE:
        {
            PostQuitMessage(0);
        } break;
        
        case WM_SIZE:
        {
            if(wParam == SIZE_MINIMIZED)
            {
                display = false;
            }
            if(wParam == SIZE_RESTORED)
            {
                display = true;
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


void GetFormatAndColorspace()
{
    u32 surface_formatcount = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(vk.gpu, vk.surface, &surface_formatcount, NULL);
    RevEnum(vk_enums.result_enum, result);
    ODS_RES("Surface formats(count): %s\n");
    VkSurfaceFormatKHR *surface_formats = (VkSurfaceFormatKHR *)malloc(sizeof(VkSurfaceFormatKHR) * surface_formatcount);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(vk.gpu, vk.surface, &surface_formatcount, surface_formats);
    
    RevEnum(vk_enums.result_enum, result);
    ODS_RES("Surface formats(fill):  %s\n");
    
    ODS("> Surface formats:\n");
    for(u32 i = 0; i < surface_formatcount; i++)
    {
        RevEnum(vk_enums.format_enum, surface_formats[i].format);
        RevEnum(vk_enums.colorspace_enum, surface_formats[i].colorSpace);
        
        ODS("FormatKHR %d:\n", i);
        ODS_RES("- format:      %s\n");      // var and free
        ODS_RES("- color space: %s\n");  // var and free
    }
    ODS("\n");
    
    ODS("> Surface properties:\n");
    for(u32 i = 0; i < surface_formatcount; i++)
    {
        VkFormatProperties surface_props;
        vkGetPhysicalDeviceFormatProperties(vk.gpu, surface_formats[i].format, &surface_props);
        
        char *linear_tiling   = DecToBin(surface_props.linearTilingFeatures);
        char *optimal_tiling  = DecToBin(surface_props.optimalTilingFeatures);
        char *buffer_features = DecToBin(surface_props.bufferFeatures);
        
        ODS("> Format %d:\n", i);
        ODS("Linear  tiling:  %s\n", linear_tiling);
        ODS("Optimal tiling:  %s\n", optimal_tiling);
        ODS("Buffer features: %s\n", buffer_features);
        ODS("\n");
        
        free(linear_tiling);
        free(optimal_tiling);
        free(buffer_features);
    }
    
    vk.format = surface_formats[0].format;
    vk.color_space = surface_formats[0].colorSpace;
    
    free(surface_formats);
}

void CheckSurfaceCapabilities()
{
    VkSurfaceCapabilitiesKHR surface_caps;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk.gpu, vk.surface, &surface_caps);
    RevEnum(vk_enums.result_enum, result);
    ODS_RES("Physical device surface capabilities result: %s\n");
    
    char *supported_transforms      = DecToBin(surface_caps.supportedTransforms);
    char *current_transforms        = DecToBin(surface_caps.currentTransform);
    char *supported_composite_alpha = DecToBin(surface_caps.supportedCompositeAlpha);
    char *supported_usage_flags     = DecToBin(surface_caps.supportedUsageFlags);
    
    ODS("> Surface capabilities:\n");
    ODS("- min images: %d\n", surface_caps.minImageCount);
    ODS("- max images: %d\n", surface_caps.maxImageCount);
    ODS("- current extent: %-4d x %-4d\n", surface_caps.currentExtent.width,  surface_caps.currentExtent.height);
    ODS("- minimal extent: %-4d x %-4d\n", surface_caps.minImageExtent.width, surface_caps.minImageExtent.height);
    ODS("- maximal extent: %-4d x %-4d\n", surface_caps.maxImageExtent.width, surface_caps.maxImageExtent.height);
    ODS("- max image arrays: %d\n", surface_caps.maxImageArrayLayers);
    ODS("- Supported transforms:      %s\n", supported_transforms);
    ODS("- Current transform:         %s\n", current_transforms);
    ODS("- Supported composite alpha: %s\n", supported_composite_alpha);
    ODS("- Supported usage flags:     %s\n", supported_usage_flags);
    ODS("\n");
    
    free(supported_transforms);
    free(current_transforms);
    free(supported_composite_alpha);
    free(supported_usage_flags);
}

void SetPresentMode()
{
    u32 present_modecount;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(vk.gpu, vk.surface, &present_modecount, NULL);
    RevEnum(vk_enums.result_enum, result);
    ODS_RES("Surface formats(count): %s\n");
    
    VkPresentModeKHR *present_modes = (VkPresentModeKHR *)malloc(sizeof(VkPresentModeKHR) * present_modecount);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(vk.gpu, vk.surface, &present_modecount, present_modes);
    RevEnum(vk_enums.result_enum, result);
    ODS_RES("Surface formats(fill):  %s\n");
    
    ODS("> Present modes:\n");
    for(u32 i = 0; i < present_modecount; i++)
    {
        RevEnum(vk_enums.presentmode_enum, present_modes[i]);
        ODS("Mode %d: %s\n", i, revenum_buffer);
        
        if(strstr(revenum_buffer, "MAILBOX"))
            vk.present_mode = present_modes[i];
    }
    RevEnum(vk_enums.presentmode_enum, vk.present_mode);
    ODS_RES("Chosen present mode: %s\n");
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
    
    result = vkAllocateMemory(vk.device, &mem_ai, NULL, memory);
    RevEnum(vk_enums.result_enum, result);
    ODS_RES("Memory allocation: %s\n");
    
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



void CheckGPUFeatures()
{
    VkPhysicalDeviceFeatures gpu_features;
    vkGetPhysicalDeviceFeatures(vk.gpu, &gpu_features);
    
    // This was previously generated with a metaprogramming utility.
    // To be showcased in a future portfolio entry :)
    ODS("- %-40s: %s\n", "robustBufferAccess", (gpu_features.robustBufferAccess & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "fullDrawIndexUint32", (gpu_features.fullDrawIndexUint32 & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "imageCubeArray", (gpu_features.imageCubeArray & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "independentBlend", (gpu_features.independentBlend & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "geometryShader", (gpu_features.geometryShader & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "tessellationShader", (gpu_features.tessellationShader & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "sampleRateShading", (gpu_features.sampleRateShading & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "dualSrcBlend", (gpu_features.dualSrcBlend & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "logicOp", (gpu_features.logicOp & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "multiDrawIndirect", (gpu_features.multiDrawIndirect & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "drawIndirectFirstInstance", (gpu_features.drawIndirectFirstInstance & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "depthClamp", (gpu_features.depthClamp & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "depthBiasClamp", (gpu_features.depthBiasClamp & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "fillModeNonSolid", (gpu_features.fillModeNonSolid & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "depthBounds", (gpu_features.depthBounds & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "wideLines", (gpu_features.wideLines & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "largePoints", (gpu_features.largePoints & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "alphaToOne", (gpu_features.alphaToOne & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "multiViewport", (gpu_features.multiViewport & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "samplerAnisotropy", (gpu_features.samplerAnisotropy & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "textureCompressionETC2", (gpu_features.textureCompressionETC2 & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "textureCompressionASTC_LDR", (gpu_features.textureCompressionASTC_LDR & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "textureCompressionBC", (gpu_features.textureCompressionBC & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "occlusionQueryPrecise", (gpu_features.occlusionQueryPrecise & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "pipelineStatisticsQuery", (gpu_features.pipelineStatisticsQuery & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "vertexPipelineStoresAndAtomics", (gpu_features.vertexPipelineStoresAndAtomics & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "fragmentStoresAndAtomics", (gpu_features.fragmentStoresAndAtomics & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "shaderTessellationAndGeometryPointSize", (gpu_features.shaderTessellationAndGeometryPointSize & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "shaderImageGatherExtended", (gpu_features.shaderImageGatherExtended & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "shaderStorageImageExtendedFormats", (gpu_features.shaderStorageImageExtendedFormats & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "shaderStorageImageMultisample", (gpu_features.shaderStorageImageMultisample & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "shaderStorageImageReadWithoutFormat", (gpu_features.shaderStorageImageReadWithoutFormat & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "shaderStorageImageWriteWithoutFormat", (gpu_features.shaderStorageImageWriteWithoutFormat & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "shaderUniformBufferArrayDynamicIndexing", (gpu_features.shaderUniformBufferArrayDynamicIndexing & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "shaderSampledImageArrayDynamicIndexing", (gpu_features.shaderSampledImageArrayDynamicIndexing & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "shaderStorageBufferArrayDynamicIndexing", (gpu_features.shaderStorageBufferArrayDynamicIndexing & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "shaderStorageImageArrayDynamicIndexing", (gpu_features.shaderStorageImageArrayDynamicIndexing & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "shaderClipDistance", (gpu_features.shaderClipDistance & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "shaderCullDistance", (gpu_features.shaderCullDistance & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "shaderFloat64", (gpu_features.shaderFloat64 & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "shaderInt64", (gpu_features.shaderInt64 & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "shaderInt16", (gpu_features.shaderInt16 & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "shaderResourceResidency", (gpu_features.shaderResourceResidency & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "shaderResourceMinLod", (gpu_features.shaderResourceMinLod & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "sparseBinding", (gpu_features.sparseBinding & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "sparseResidencyBuffer", (gpu_features.sparseResidencyBuffer & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "sparseResidencyImage2D", (gpu_features.sparseResidencyImage2D & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "sparseResidencyImage3D", (gpu_features.sparseResidencyImage3D & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "sparseResidency2Samples", (gpu_features.sparseResidency2Samples & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "sparseResidency4Samples", (gpu_features.sparseResidency4Samples & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "sparseResidency8Samples", (gpu_features.sparseResidency8Samples & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "sparseResidency16Samples", (gpu_features.sparseResidency16Samples & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "sparseResidencyAliased", (gpu_features.sparseResidencyAliased & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "variableMultisampleRate", (gpu_features.variableMultisampleRate & 1) ? "YES" : "NO");
    ODS("- %-40s: %s\n", "inheritedQueries", (gpu_features.inheritedQueries & 1) ? "YES" : "NO");
    ODS("\n");
}


r32 abs(r32 v)
{
    r32 a[2] = { -v, v };
    return a[v > 0];
}

u32 Part1By2(u32 x)
{
    x &= 0x000003ff;                  // x = ---- ---- ---- ---- ---- --98 7654 3210
    x = (x ^ (x << 16)) & 0xff0000ff; // x = ---- --98 ---- ---- ---- ---- 7654 3210
    x = (x ^ (x <<  8)) & 0x0300f00f; // x = ---- --98 ---- ---- 7654 ---- ---- 3210
    x = (x ^ (x <<  4)) & 0x030c30c3; // x = ---- --98 ---- 76-- --54 ---- 32-- --10
    x = (x ^ (x <<  2)) & 0x09249249; // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
    return x;
}

u32 EncodeMorton3(u32 x, u32 y, u32 z)
{
    return (Part1By2(z) << 2) + (Part1By2(y) << 1) + Part1By2(x);
}



void CheckMortonSorting(u32 *data, u32 count)
{
    for(u32 i = 0; i < count-1; i++)
    {
        if(data[i] > data[i+1])
        {
            ODS("Debug! \n");
            exit(0);
        }
    }
    ODS("All good. \n");
}
bool CheckMortonSorting_Digit(u32 *data, u32 count, u32 digit)
{
    for(u32 i = 0; i < count-1; i++)
    {
        u32 firstbit  = ((data[i]   & (1 << digit)) >> digit);
        u32 secondbit = ((data[i+1] & (1 << digit)) >> digit);
        if(firstbit > secondbit)
        {
            ODS("Debug! \n");
            return false;
        }
    }
    return true;
}





typedef struct
{
    VkDescriptorType type;
    VkBuffer buffer;
    VkDeviceMemory memory;
} resource_record;
typedef struct
{
    VkDescriptorType type;
    VkImageView imageview;
} imageresource_record;
typedef struct
{
    string *shader_file;
    u32 resource_count;
    resource_record *resources;
    u32 pcr_size;
    u32 *pcr_data;  // u32 or r32 isn't important, important is the size of 4 bytes
} in_struct;
typedef struct
{
    string *shader_file;
    u32 resource_count;
    resource_record *resources;
    u32 imageresource_count;
    imageresource_record *imageresources;
    
    u32 pcr_size;
    u32 *pcr_data;  // u32 or r32 isn't important, important is the size of 4 bytes
} in_struct_v2;
typedef struct
{
    u32 descwrite_count;
    VkWriteDescriptorSet *descwrites;
    VkDescriptorPool pool;
    VkShaderModule module;
    VkDescriptorSet pipe_dset;
    VkDescriptorSetLayout pipe_dslayout;
    VkPipelineLayout pipe_layout;
    VkPipeline pipe;
} out_struct;

// v1
out_struct *CreateComputePipeline(in_struct *in)
{
    out_struct *out = (out_struct *)calloc(1, sizeof(out_struct));
    
    // v1 - easy mode, only one descriptor type.
    // TO DO: v2 - count the various descriptor types, and accordingly create descriptor pools
    VkDescriptorPoolSize poolsize;
    poolsize.type            = in->resources[0].type;
    poolsize.descriptorCount = in->resource_count;
    
    VkDescriptorPoolCreateInfo dspoolci = {};
    dspoolci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dspoolci.pNext         = NULL;
    dspoolci.flags         = 0;
    dspoolci.maxSets       = in->resource_count;
    dspoolci.poolSizeCount = 1;
    dspoolci.pPoolSizes    = &poolsize;
    //VkDescriptorPool dspool;
    result = vkCreateDescriptorPool(vk.device, &dspoolci, NULL, &out->pool);  // <--- crashes here without validation layers
    ODS_RES("Compute pipeline descriptor pool creation: %s \n");
    
    
    u32 running_binding_index = 0;
    VkDescriptorSetLayoutBinding *bindings = (VkDescriptorSetLayoutBinding *)calloc(in->resource_count, sizeof(VkDescriptorSetLayoutBinding));
    for(u32 i = 0; i < in->resource_count; i++)
    {
        bindings[i].binding         = i;
        bindings[i].descriptorType  = in->resources[i].type;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
    }
    
    VkDescriptorSetLayoutCreateInfo dslayout_ci = {};
    dslayout_ci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dslayout_ci.pNext        = NULL;
    dslayout_ci.flags        = 0;
    dslayout_ci.bindingCount = in->resource_count;
    dslayout_ci.pBindings    = bindings;
    result = vkCreateDescriptorSetLayout(vk.device, &dslayout_ci, NULL, &out->pipe_dslayout);
    ODS_RES("Compute pipeline descriptor set layout creation: %s \n");
    
    
    VkPipelineLayoutCreateInfo layout_ci = {};
    layout_ci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_ci.pNext                  = NULL;
    layout_ci.flags                  = 0;
    layout_ci.setLayoutCount         = 1;
    layout_ci.pSetLayouts            = &out->pipe_dslayout;
    if(in->pcr_size > 0)
    {
        VkPushConstantRange pcr = {};
        pcr.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pcr.offset     = 0;
        pcr.size       = in->pcr_size * sizeof(u32);
        
        layout_ci.pushConstantRangeCount = 1;
        layout_ci.pPushConstantRanges    = &pcr;  // thread count, digit place
    }
    else
    {
        layout_ci.pushConstantRangeCount = 0;
        layout_ci.pPushConstantRanges    = NULL;
    }
    result = vkCreatePipelineLayout(vk.device, &layout_ci, NULL, &out->pipe_layout);  // <--- crashes here with them
    ODS_RES("Compute pipeline layout creation: %s \n");
    
    
    
    //VkShaderModule module = GetShaderModule(in->shader_file->ptr);
    out->module = GetShaderModule(in->shader_file->ptr);
    
    VkPipelineShaderStageCreateInfo stage = {};
    stage.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.pNext               = NULL;
    stage.flags               = 0;
    stage.stage               = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.module              = out->module;
    stage.pName               = "main";
    
    VkComputePipelineCreateInfo ci = {};
    ci.sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    ci.pNext              = NULL;
    ci.flags              = 0;
    ci.stage              = stage;
    ci.layout             = out->pipe_layout;
    vkCreateComputePipelines(vk.device, NULL, 1, &ci, NULL, &out->pipe);
    
    
    
    VkDescriptorSetAllocateInfo dsai = {};
    dsai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsai.pNext              = NULL;
    dsai.descriptorPool     = out->pool;
    dsai.descriptorSetCount = 1;
    dsai.pSetLayouts        = &out->pipe_dslayout;
    result = vkAllocateDescriptorSets(vk.device, &dsai, &out->pipe_dset);
    ODS_RES("Descriptor set allocation: %s \n");
    
    out->descwrite_count = in->resource_count;
    out->descwrites = (VkWriteDescriptorSet *)calloc(out->descwrite_count, sizeof(VkWriteDescriptorSet));
    for(u32 i = 0; i < out->descwrite_count; i++)
    {
        VkDescriptorBufferInfo *bi = (VkDescriptorBufferInfo *)calloc(1, sizeof(VkDescriptorBufferInfo));
        bi->buffer = in->resources[i].buffer;
        bi->offset = 0;
        bi->range  = VK_WHOLE_SIZE;
        
        out->descwrites[i].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        out->descwrites[i].pNext            = NULL;
        out->descwrites[i].dstSet           = out->pipe_dset;
        out->descwrites[i].dstBinding       = i;
        out->descwrites[i].dstArrayElement  = 0;
        out->descwrites[i].descriptorCount  = 1;
        out->descwrites[i].descriptorType   = in->resources[i].type;
        out->descwrites[i].pBufferInfo      = bi;
    }
    
    return out;
}
out_struct *CreateComputePipeline_v2(in_struct_v2 *in)
{
    out_struct *out = (out_struct *)calloc(1, sizeof(out_struct));
    
    // count the various descriptor types, and accordingly create descriptor pools
    VkDescriptorPoolSize buffer_poolsize;
    buffer_poolsize.type            = in->resources[0].type;
    buffer_poolsize.descriptorCount = in->resource_count;
    
    VkDescriptorPoolSize image_poolsize;
    image_poolsize.type            = in->imageresources[0].type;
    image_poolsize.descriptorCount = in->imageresource_count;
    
    VkDescriptorPoolSize poolsizes[2] = { buffer_poolsize, image_poolsize };
    
    VkDescriptorPoolCreateInfo dspoolci = {};
    dspoolci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dspoolci.pNext         = NULL;
    dspoolci.flags         = 0;
    dspoolci.maxSets       = 1;
    dspoolci.poolSizeCount = 2;
    dspoolci.pPoolSizes    = poolsizes;
    result = vkCreateDescriptorPool(vk.device, &dspoolci, NULL, &out->pool);
    ODS_RES("Compute pipeline descriptor pool creation: %s \n");
    
    
    VkDescriptorSetLayoutBinding *bindings = (VkDescriptorSetLayoutBinding *)calloc(in->resource_count+in->imageresource_count, sizeof(VkDescriptorSetLayoutBinding));
    u32 running_binding_index = 0;
    for(u32 i = 0; i < in->resource_count; i++, running_binding_index++)
    {
        bindings[running_binding_index].binding         = running_binding_index;
        bindings[running_binding_index].descriptorType  = in->resources[i].type;
        bindings[running_binding_index].descriptorCount = 1;
        bindings[running_binding_index].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
    }
    
    for(u32 i = 0; i < in->imageresource_count; i++, running_binding_index++)
    {
        bindings[running_binding_index].binding         = running_binding_index;
        bindings[running_binding_index].descriptorType  = in->imageresources[i].type;
        bindings[running_binding_index].descriptorCount = 1;
        bindings[running_binding_index].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
    }
    
    
    
    VkDescriptorSetLayoutCreateInfo dslayout_ci = {};
    dslayout_ci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dslayout_ci.pNext        = NULL;
    dslayout_ci.flags        = 0;
    dslayout_ci.bindingCount = in->resource_count+in->imageresource_count;
    dslayout_ci.pBindings    = bindings;
    result = vkCreateDescriptorSetLayout(vk.device, &dslayout_ci, NULL, &out->pipe_dslayout);
    ODS_RES("Compute pipeline descriptor set layout creation: %s \n");
    
    
    VkPipelineLayoutCreateInfo layout_ci = {};
    layout_ci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_ci.pNext                  = NULL;
    layout_ci.flags                  = 0;
    layout_ci.setLayoutCount         = 1;
    layout_ci.pSetLayouts            = &out->pipe_dslayout;
    if(in->pcr_size > 0)
    {
        VkPushConstantRange pcr = {};
        pcr.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pcr.offset     = 0;
        pcr.size       = in->pcr_size * sizeof(u32);
        
        layout_ci.pushConstantRangeCount = 1;
        layout_ci.pPushConstantRanges    = &pcr;
    }
    else
    {
        layout_ci.pushConstantRangeCount = 0;
        layout_ci.pPushConstantRanges    = NULL;
    }
    result = vkCreatePipelineLayout(vk.device, &layout_ci, NULL, &out->pipe_layout);
    ODS_RES("Compute pipeline layout creation: %s \n");
    
    
    
    out->module = GetShaderModule(in->shader_file->ptr);
    
    VkPipelineShaderStageCreateInfo stage = {};
    stage.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.pNext               = NULL;
    stage.flags               = 0;
    stage.stage               = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.module              = out->module;
    stage.pName               = "main";
    
    VkComputePipelineCreateInfo ci = {};
    ci.sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    ci.pNext              = NULL;
    ci.flags              = 0;
    ci.stage              = stage;
    ci.layout             = out->pipe_layout;
    vkCreateComputePipelines(vk.device, NULL, 1, &ci, NULL, &out->pipe);
    
    
    
    VkDescriptorSetAllocateInfo dsai = {};
    dsai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsai.pNext              = NULL;
    dsai.descriptorPool     = out->pool;
    dsai.descriptorSetCount = 1;
    dsai.pSetLayouts        = &out->pipe_dslayout;
    result = vkAllocateDescriptorSets(vk.device, &dsai, &out->pipe_dset);
    ODS_RES("Descriptor set allocation: %s \n");
    
    out->descwrite_count = in->resource_count + in->imageresource_count;
    out->descwrites = (VkWriteDescriptorSet *)calloc(out->descwrite_count, sizeof(VkWriteDescriptorSet));
    u32 descwrite_runningindex = 0;
    for(u32 i = 0; i < in->resource_count; i++, descwrite_runningindex++)
    {
        VkDescriptorBufferInfo *bi = (VkDescriptorBufferInfo *)calloc(1, sizeof(VkDescriptorBufferInfo));
        bi->buffer = in->resources[descwrite_runningindex].buffer;
        bi->offset = 0;
        bi->range  = VK_WHOLE_SIZE;
        
        out->descwrites[descwrite_runningindex].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        out->descwrites[descwrite_runningindex].pNext            = NULL;
        out->descwrites[descwrite_runningindex].dstSet           = out->pipe_dset;
        out->descwrites[descwrite_runningindex].dstBinding       = descwrite_runningindex;
        out->descwrites[descwrite_runningindex].dstArrayElement  = 0;
        out->descwrites[descwrite_runningindex].descriptorCount  = 1;
        out->descwrites[descwrite_runningindex].descriptorType   = in->resources[i].type;
        out->descwrites[descwrite_runningindex].pBufferInfo      = bi;
    }
    
    for(u32 i = 0; i < in->imageresource_count; i++, descwrite_runningindex++)
    {
        VkDescriptorImageInfo *ii = (VkDescriptorImageInfo *)calloc(1, sizeof(VkDescriptorImageInfo));
        ii->sampler     = NULL;
        ii->imageView   = in->imageresources[i].imageview;
        ii->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        
        out->descwrites[descwrite_runningindex].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        out->descwrites[descwrite_runningindex].pNext           = NULL;
        out->descwrites[descwrite_runningindex].dstSet          = out->pipe_dset;
        out->descwrites[descwrite_runningindex].dstBinding      = descwrite_runningindex;
        out->descwrites[descwrite_runningindex].dstArrayElement = 0;
        out->descwrites[descwrite_runningindex].descriptorCount = 1;
        out->descwrites[descwrite_runningindex].descriptorType  = in->imageresources[i].type;
        out->descwrites[descwrite_runningindex].pImageInfo      = ii;
    }
    
    return out;
}


void CleanPipeline(out_struct *out)
{
    vkDestroyShaderModule(vk.device, out->module, NULL);
    vkDestroyDescriptorPool(vk.device, out->pool, NULL);
    vkDestroyDescriptorSetLayout(vk.device, out->pipe_dslayout, NULL);
    vkDestroyPipelineLayout(vk.device, out->pipe_layout, NULL);
    vkDestroyPipeline(vk.device, out->pipe, NULL);
    free(out->descwrites);
    free(out);
}

s32 delta(u32 *keys, u32 n, u32 i, u32 j)
{
    if((j < 0) || (j > (n-1)))
        return -1;
    if(keys[i] == keys[j])
        return 32;
    else
        return 32-(__lzcnt(keys[i]^keys[j]))-1;
}



// I should really stop just piling them willy-nilly like that. Make a separate, probably not source.h file.
typedef struct
{
    r32 x, y, z;
} vertex3;
typedef struct
{
    vertex3 lower;
    vertex3 upper;
} AABB;  // CPU-side

typedef struct
{
    r32 x, y, z;
    u32 pad;
} vertex3_padded;
typedef struct
{
    vertex3_padded lower;
    vertex3_padded upper;
} AABB_padded;  // GPU-side

typedef struct
{
    //s32 d;
    //s32 delta_min;
    //s32 delta_node;
    s32 parent;
    s32 left;
    s32 right;
} tree_entry;
typedef struct
{
    //s32 d;
    //s32 delta_min;
    //s32 delta_node;
    s32 parent;
    s32 left;
    s32 right;
    s32 pad;
} tree_entry_padded;

typedef struct 
{
    vertex3 o;
    vertex3 d;
    r32 t;
} ray;

r32 t_min = 0.001f;
r32 t_max = 1000.0f;

// RayPlane() would be some form of the plan equation, probably coefficients.
bool RayBox(ray r, AABB_padded b, r32 *t_result)
{
    bool result = false;
    
    // check against the six planes; pick one coord, defining an axis-aligned plane, 
    // first plane: zmin.
    r32 x_min = b.lower.x;
    r32 x_max = b.upper.x;
    r32 y_min = b.lower.y;
    r32 y_max = b.upper.y;
    r32 z_min = b.lower.z;
    r32 z_max = b.upper.z;
    
    u32 t_counter = 0;
    r32 t_values[6];
    
    
    // x_min
    r32 t = (x_min - r.o.x)/r.d.x;
    if((t_min < t) && (t < t_max))
    {
        r32 py = r.o.y + t * r.d.y;
        r32 pz = r.o.z + t * r.d.z;
        
        // check bounds
        if(((b.lower.y < py)&&(py < b.upper.y)) &&
           ((b.lower.z < pz)&&(pz < b.upper.z)))
        {
            t_values[t_counter++] = t;
        }
    }
    
    // x_max
    t = (x_max - r.o.x)/r.d.x;
    if((t_min < t) && (t < t_max))
    {
        r32 py = r.o.y + t * r.d.y;
        r32 pz = r.o.z + t * r.d.z;
        
        // check bounds
        if(((b.lower.x < py)&&(py < b.upper.y)) &&
           ((b.lower.y < pz)&&(pz < b.upper.z)))
        {
            t_values[t_counter++] = t;
        }
    }
    
    
    // y_min
    t = (y_min - r.o.y)/r.d.y;
    if((t_min < t) && (t < t_max))
    {
        r32 px = r.o.x + t * r.d.x;
        r32 pz = r.o.z + t * r.d.z;
        
        // check bounds
        if(((b.lower.x < px)&&(px < b.upper.x)) &&
           ((b.lower.z < pz)&&(pz < b.upper.z)))
        {
            t_values[t_counter++] = t;
        }
    }
    
    // y_max
    t = (y_max - r.o.y)/r.d.y;
    if((t_min < t) && (t < t_max))
    {
        r32 px = r.o.x + t * r.d.x;
        r32 pz = r.o.z + t * r.d.z;
        
        // check bounds
        if(((b.lower.x < px)&&(px < b.upper.x)) &&
           ((b.lower.z < pz)&&(pz < b.upper.z)))
        {
            t_values[t_counter++] = t;
        }
    }
    
    
    // z_min
    t = (z_min - r.o.z)/r.d.z;
    if((t_min < t) && (t < t_max))
    {
        r32 px = r.o.x + t * r.d.x;
        r32 py = r.o.y + t * r.d.y;
        
        // check bounds
        if(((b.lower.x < px)&&(px < b.upper.x)) &&
           ((b.lower.y < py)&&(py < b.upper.y)))
        {
            t_values[t_counter++] = t;
        }
    }
    
    // z_max
    t = (z_max - r.o.z)/r.d.z;
    if((t_min < t) && (t < t_max))
    {
        r32 px = r.o.x + t * r.d.x;
        r32 py = r.o.y + t * r.d.y;
        
        // check bounds
        if(((b.lower.x < px)&&(px < b.upper.x)) &&
           ((b.lower.y < py)&&(py < b.upper.y)))
        {
            t_values[t_counter++] = t;
        }
    }
    
    
    r32 minimal_t = t_max;
    
    // now look through the values and select the minimal
    for(u32 i = 0; i < t_counter; i++)
    {
        if(minimal_t > t_values[i])
            minimal_t = t_values[i];
    }
    
    if(t_counter > 0)
    {
        result = true;
        *t_result = minimal_t;
    }
    return result;
}

#define RD 0 

int CALLBACK WinMain(HINSTANCE instance,
                     HINSTANCE prevInstance,
                     LPSTR commandLine,
                     int showMode)
{
    app.instance = instance;
    app.window_width = 1280;
    app.window_height = 720;
    app.window = WindowCreate();  // should be in a struct somewhere...
    
    display = true;
    
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
    
    char *ext_names[] = {
        "VK_KHR_surface",
        "VK_KHR_win32_surface",
        "VK_EXT_debug_utils"
    };
    u32 ext_count = sizeof(ext_names)/sizeof(ext_names[0]);
    
    char *layer_names[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    u32 layer_count = sizeof(layer_names)/sizeof(layer_names[0]);
    
#define val 1
    GetVulkanInstance(ext_names,   ext_count,
                      #if val
                      layer_names, layer_count);
#elif !val
    NULL, 0);
#endif
    
    ODS("Here's a Vulkan instance: 0x%p\n", &vk.instance);
    Vulkan_LoadExtensionFunctions(vk.instance);
    
    SetupDebugging();
    GetGPU();
    
    CheckGPUFeatures();
    
    CreateSurface();
    
    SetupQueue();
    char *dev_ext_names[] = {
        "VK_KHR_swapchain"
    };
    u32 dev_ext_count = sizeof(dev_ext_names) / sizeof(dev_ext_names[0]);
    VkPhysicalDeviceFeatures features = {};
    features.robustBufferAccess = true;
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
    
    //VkCommandBuffer commandbuffer;
    vkAllocateCommandBuffers(vk.device, &commandbuffer_ai, &vk.commandbuffer);
    
    VkCommandBufferBeginInfo commandbuffer_bi = {};
    vk.commandbuffer_bi.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vk.commandbuffer_bi.pNext            = NULL;
    vk.commandbuffer_bi.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    
    
    
    //VkFenceCreateInfo fence_ci = {};
    vk.fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vk.fence_ci.pNext = NULL;
    vk.fence_ci.flags = 0;
    
    //VkFence fence;
    vkCreateFence(vk.device, &vk.fence_ci, NULL, &vk.fence);
    
    
    VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkSubmitInfo transit_si = {};
    transit_si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    transit_si.pNext                = NULL;
    transit_si.pWaitDstStageMask    = &wait_stage_mask;
    transit_si.commandBufferCount   = 1;
    transit_si.pCommandBuffers      = &vk.commandbuffer;
    
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
    
    
    vkBeginCommandBuffer(vk.commandbuffer, &vk.commandbuffer_bi);
    vkCmdPipelineBarrier(vk.commandbuffer,
                         VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                         0,
                         0, NULL,
                         0, NULL,
                         1, &swap0_barrier);
    vkCmdPipelineBarrier(vk.commandbuffer,
                         VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                         0,
                         0, NULL,
                         0, NULL,
                         1, &swap1_barrier);
    vkEndCommandBuffer(vk.commandbuffer);
    vkQueueSubmit(vk.queue, 1, &transit_si, vk.fence);
    
    vkWaitForFences(vk.device, 1, &vk.fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vk.device, 1, &vk.fence);
    
    
    TimerSetup();
    
    // --- application setup is done.
    
    
    
    
    
    // - Resources
    //char *model_file = "../assets/bunny.obj";
    char *model_file = "../assets/suzanne.obj";
    
    ParsedOBJ model_obj = LoadOBJ(model_file);
    ParsedOBJRenderable model = model_obj.renderables[0];
    
    
    ODS("Vertex count:      %d\n", model.vertex_count);
    ODS("Material count:    %d\n", model_obj.material_library_count);
    for(u32 i = 0; i < model_obj.material_library_count; i++)
        ODS("- material %d: %s\n", i, model_obj.material_libraries[i]);
    
    ODS("Floats per vertex: %d\n", model.floats_per_vertex);
    ODS("Index count:       %d\n", model.index_count);
    ODS("Triangle count:    %d\n", model.index_count/3);
    ODS("\n");
    
    u32 vertex_datasize = (model.vertex_count * model.floats_per_vertex);
    
    
    // - making room for each of coord components
    u32 model_tricount = model.index_count / 3;
    r32 *xs = (r32 *)malloc(model_tricount * sizeof(r32));  // FREE after you're done with global AABB
    r32 *ys = (r32 *)malloc(model_tricount * sizeof(r32));
    r32 *zs = (r32 *)malloc(model_tricount * sizeof(r32));
    for(u32 i = 0; i < model_tricount; i++)
    {
        xs[i] = model.vertices[8*i];
        ys[i] = model.vertices[8*i+1];
        zs[i] = model.vertices[8*i+2];
    }
    
    
    u32 compute_bufsize = model_tricount;
    
    r32 x_max, x_min, y_max, y_min, z_max, z_min;
    x_max =  y_max =  z_max = -10000.0f;
    x_min =  y_min =  z_min =  10000.0f;
    
    u32 block_size = 32;
    
    
    
    u64 CPU_calc_start = TimerRead();
    
    for(u32 i = 0; i < model_tricount; i++)
    {
        if(x_min > xs[i]) x_min = xs[i];
        if(y_min > ys[i]) y_min = ys[i];
        if(z_min > zs[i]) z_min = zs[i];
        
        if(x_max < xs[i]) x_max = xs[i];
        if(y_max < ys[i]) y_max = ys[i];
        if(z_max < zs[i]) z_max = zs[i];
    }
    ODS("AABB: \n");
    ODS("X: [ %+-7.3f | %+-7.3f ]\n", x_min, x_max);
    ODS("Y: [ %+-7.3f | %+-7.3f ]\n", y_min, y_max);
    ODS("Z: [ %+-7.3f | %+-7.3f ]\n", z_min, z_max);
    
    r64 CPU_time_elapsed_s  = TimerElapsedFrom(CPU_calc_start, SECONDS);
    r64 CPU_time_elapsed_ms = TimerElapsedFrom(CPU_calc_start, MILLISECONDS);
    r64 CPU_time_elapsed_us = TimerElapsedFrom(CPU_calc_start, MICROSECONDS);
    r64 CPU_time_elapsed_ns = TimerElapsedFrom(CPU_calc_start, NANOSECONDS);
    
    ODS("s:  %f\nms: %f\nus: %f\nns: %f\n", CPU_time_elapsed_s, CPU_time_elapsed_ms, CPU_time_elapsed_us, CPU_time_elapsed_ns);
    
    
    
    VkDeviceMemory xs_memory;
    VkDeviceMemory ys_memory;
    VkDeviceMemory zs_memory;
    
    VkPhysicalDeviceMemoryProperties gpu_memprops;
    vkGetPhysicalDeviceMemoryProperties(vk.gpu, &gpu_memprops);
    
    VkBuffer xs_buffer = CreateBuffer(sizeof(float) * compute_bufsize,
                                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                      vk.device, gpu_memprops,
                                      &xs_memory);
    VkBuffer ys_buffer = CreateBuffer(sizeof(float) * compute_bufsize,
                                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                      vk.device, gpu_memprops,
                                      &ys_memory);
    VkBuffer zs_buffer = CreateBuffer(sizeof(float) * compute_bufsize,
                                      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                      vk.device, gpu_memprops,
                                      &zs_memory);
    
    void *xs_mapptr;
    vkMapMemory(vk.device, xs_memory, 0, VK_WHOLE_SIZE, 0, &xs_mapptr);
    memcpy(xs_mapptr, xs, sizeof(float) * compute_bufsize);
    //vkUnmapMemory(vk.device, xs_memory);
    
    void *ys_mapptr;
    vkMapMemory(vk.device, ys_memory, 0, VK_WHOLE_SIZE, 0, &ys_mapptr);
    memcpy(ys_mapptr, ys, sizeof(float) * compute_bufsize);
    //vkUnmapMemory(vk.device, ys_memory);
    
    void *zs_mapptr;
    vkMapMemory(vk.device, zs_memory, 0, VK_WHOLE_SIZE, 0, &zs_mapptr);
    memcpy(zs_mapptr, zs, sizeof(float) * compute_bufsize);
    //vkUnmapMemory(vk.device, zs_memory);
    
    
    free(xs);
    free(ys);
    free(zs);
    
    // ---
    
    
    
    
    
    // Compute
    // - prepare a render target
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
    
    vkBeginCommandBuffer(vk.commandbuffer, &vk.commandbuffer_bi);
    vkCmdPipelineBarrier(vk.commandbuffer,
                         VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                         0,
                         0, NULL,
                         0, NULL,
                         1, &barrier);
    vkEndCommandBuffer(vk.commandbuffer);
    vkQueueSubmit(vk.queue, 1, &transit_si, vk.fence);
    
    vkWaitForFences(vk.device, 1, &vk.fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vk.device, 1, &vk.fence);
    // ---
    
    
    // - pipeline and resources
    //u32 block_size = 32;
    
    //char *shader = "../code/shader_comp.spv";
    char *shader = "../code/minmax_comp.spv";
    
    VkPipelineShaderStageCreateInfo stage_ci = {};
    stage_ci.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_ci.pNext               = NULL;
    stage_ci.flags               = 0;
    stage_ci.stage               = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_ci.module              = GetShaderModule(shader);
    stage_ci.pName               = "main";
    stage_ci.pSpecializationInfo = NULL;
    
    VkDescriptorSetLayoutBinding binding_x = {};
    binding_x.binding            = 0;
    binding_x.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding_x.descriptorCount    = 1;
    binding_x.stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    binding_x.pImmutableSamplers = NULL;
    
    VkDescriptorSetLayoutBinding binding_y = {};
    binding_y.binding            = 1;
    binding_y.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding_y.descriptorCount    = 1;
    binding_y.stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    binding_y.pImmutableSamplers = NULL;
    
    VkDescriptorSetLayoutBinding binding_z = {};
    binding_z.binding            = 2;
    binding_z.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding_z.descriptorCount    = 1;
    binding_z.stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    binding_z.pImmutableSamplers = NULL;
    
    VkDescriptorSetLayoutBinding bindings[] = { binding_x, binding_y, binding_z };
    
    
    VkDescriptorSetLayoutCreateInfo dsl_ci = {};
    dsl_ci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsl_ci.pNext        = NULL;
    dsl_ci.flags        = 0;
    dsl_ci.bindingCount = 3;
    dsl_ci.pBindings    = bindings;
    VkDescriptorSetLayout dslayout;
    vkCreateDescriptorSetLayout(vk.device, &dsl_ci, NULL, &dslayout);
    
    VkDescriptorPoolSize dspool_size = {};
    dspool_size.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    dspool_size.descriptorCount = 3;
    
    VkDescriptorPoolCreateInfo dspool_ci = {};
    dspool_ci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dspool_ci.pNext         = NULL;
    dspool_ci.flags         = 0;
    dspool_ci.maxSets       = 3;
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
    
    VkPushConstantRange pcr = {};
    pcr.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pcr.offset     = 0;
    pcr.size       = 3 * sizeof(u32);
    
    VkPipelineLayoutCreateInfo layout_ci = {};
    layout_ci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_ci.pNext                  = NULL;
    layout_ci.flags                  = 0;
    layout_ci.setLayoutCount         = 1;
    layout_ci.pSetLayouts            = &dslayout;
    layout_ci.pushConstantRangeCount = 1;
    layout_ci.pPushConstantRanges    = &pcr;
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
    
    
#if 0
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
#endif
    
    VkDescriptorBufferInfo x_buffer_info = {};
    x_buffer_info.buffer = xs_buffer;
    x_buffer_info.offset = 0;
    x_buffer_info.range  = VK_WHOLE_SIZE;
    
    VkWriteDescriptorSet write_x = {};
    write_x.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_x.pNext            = NULL;
    write_x.dstSet           = vk.dsl;
    write_x.dstBinding       = 0;
    write_x.dstArrayElement  = 0;
    write_x.descriptorCount  = 1;
    write_x.descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write_x.pImageInfo       = NULL;
    write_x.pBufferInfo      = &x_buffer_info;
    write_x.pTexelBufferView = NULL;
    
    
    VkDescriptorBufferInfo y_buffer_info = {};
    y_buffer_info.buffer = ys_buffer;
    y_buffer_info.offset = 0;
    y_buffer_info.range  = VK_WHOLE_SIZE;
    
    VkWriteDescriptorSet write_y = {};
    write_y.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_y.pNext            = NULL;
    write_y.dstSet           = vk.dsl;
    write_y.dstBinding       = 1;
    write_y.dstArrayElement  = 0;
    write_y.descriptorCount  = 1;
    write_y.descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write_y.pImageInfo       = NULL;
    write_y.pBufferInfo      = &y_buffer_info;
    write_y.pTexelBufferView = NULL;
    
    
    VkDescriptorBufferInfo z_buffer_info = {};
    z_buffer_info.buffer = zs_buffer;
    z_buffer_info.offset = 0;
    z_buffer_info.range  = VK_WHOLE_SIZE;
    
    VkWriteDescriptorSet write_z = {};
    write_z.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_z.pNext            = NULL;
    write_z.dstSet           = vk.dsl;
    write_z.dstBinding       = 2;
    write_z.dstArrayElement  = 0;
    write_z.descriptorCount  = 1;
    write_z.descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write_z.pImageInfo       = NULL;
    write_z.pBufferInfo      = &z_buffer_info;
    write_z.pTexelBufferView = NULL;
    
    
    VkWriteDescriptorSet compute_writes[] = { write_x, write_y, write_z };
    
    vkUpdateDescriptorSets(vk.device, 3, compute_writes, 0, NULL);
    
#if 0
    u32 xdim = ceil(r32(app.window_width) / 32.0f);
    u32 ydim = ceil(r32(app.window_height) / 32.0f);
#endif
    
    VkMemoryBarrier dispatch_barrier = {};
    dispatch_barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    dispatch_barrier.pNext         = NULL;
    dispatch_barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    dispatch_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    
    
    
    
    // Compute stuff
    
    // === Step 1: Take a look at the model vertexes, find min/max of xyz, get the AABB of the model.
    vkBeginCommandBuffer(vk.commandbuffer, &vk.commandbuffer_bi);
    vkCmdBindDescriptorSets(vk.commandbuffer, vk.bindpoint_compute, vk.pipelayout, 0, 1, &vk.dsl, 0, NULL);
    vkCmdBindPipeline(vk.commandbuffer, vk.bindpoint_compute, vk.compipe);
    
    // --- This entire block is riddled with dangerous floaty maths.
    u32 thread_count = model_tricount;
    u32 block_count = (u32)ceil((r32)thread_count / (r32)block_size);
    
    struct
    {
        u32 thread_count;
        u32 read_offset;
        u32 write_offset;
    } controls;
    controls.thread_count = model_tricount;
    controls.read_offset  = 0;
    controls.write_offset = (u32)ceil((r32)block_count / (r32)block_size) * block_size;
    
    
    
    u64 GPU_calc_start = TimerRead();
    
    // - min/max
    ODS("Dispatch: %5d threads, %4d blocks; [offsets r:%4d | w:%4d]\n", thread_count, block_count, controls.read_offset, controls.write_offset);
    vkCmdPushConstants(vk.commandbuffer, vk.pipelayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(controls), &controls);  // is sizeof(controls) 12?
    vkCmdDispatch(vk.commandbuffer, block_count, 1, 1);
    for(; block_count > 1; )
    {
        thread_count = ceil((r32)thread_count / (r32)block_size);
        
        vkCmdPipelineBarrier(vk.commandbuffer,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0,
                             1, &dispatch_barrier,
                             0, NULL,
                             0, NULL);
        
        block_count = ceil((r32)thread_count / (r32)block_size);
        controls.thread_count = thread_count;
        controls.read_offset  = controls.write_offset;
        controls.write_offset = ceil((r32)block_count / 32.0f) * 32;
        ODS("Dispatch: %5d threads, %4d blocks; [offsets r:%4d | w:%4d]\n", thread_count, block_count, controls.read_offset, controls.write_offset);
        
        vkCmdPushConstants(vk.commandbuffer, vk.pipelayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(controls), &controls);
        vkCmdDispatch(vk.commandbuffer, block_count, 1, 1);
    }
    vkEndCommandBuffer(vk.commandbuffer);
    // ---
    
    
    VkSubmitInfo compute_si = {};
    compute_si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    compute_si.pNext                = NULL;
    compute_si.pWaitDstStageMask    = &wait_stage_mask;
    compute_si.commandBufferCount   = 1;
    compute_si.pCommandBuffers      = &vk.commandbuffer;
    vkQueueSubmit(vk.queue, 1, &compute_si, vk.fence);
    vkWaitForFences(vk.device, 1, &vk.fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vk.device, 1, &vk.fence);
    
    
    r64 GPU_time_elapsed_s  = TimerElapsedFrom(GPU_calc_start, SECONDS);
    r64 GPU_time_elapsed_ms = TimerElapsedFrom(GPU_calc_start, MILLISECONDS);
    r64 GPU_time_elapsed_us = TimerElapsedFrom(GPU_calc_start, MICROSECONDS);
    r64 GPU_time_elapsed_ns = TimerElapsedFrom(GPU_calc_start, NANOSECONDS);
    
    ODS("s:  %f\nms: %f\nus: %f\nns: %f\n", GPU_time_elapsed_s, GPU_time_elapsed_ms, GPU_time_elapsed_us, GPU_time_elapsed_ns);
    // ===
    
    
    // read results
    //u32 answer_size = compute_bufsize/32;
    
    u32 answer_range = controls.write_offset + 1;
    
    r32 *x_res = (r32 *)malloc(answer_range * sizeof(r32));       // FREE after... soon after.
    memcpy(x_res, (r32 *)xs_mapptr, answer_range * sizeof(r32));
    
    r32 *y_res = (r32 *)malloc(answer_range * sizeof(r32));
    memcpy(y_res, (r32 *)ys_mapptr, answer_range * sizeof(r32));
    
    r32 *z_res = (r32 *)malloc(answer_range * sizeof(r32));
    memcpy(z_res, (r32 *)zs_mapptr, answer_range * sizeof(r32));
    
    ODS("Trumpets: compute result\n");
    
    x_max = x_res[0]; x_min = x_res[controls.write_offset];
    y_max = y_res[0]; y_min = y_res[controls.write_offset];
    z_max = z_res[0]; z_min = z_res[controls.write_offset];
    
    r32 x_range = abs(x_min) + abs(x_max);
    r32 y_range = abs(y_min) + abs(y_max);
    r32 z_range = abs(z_min) + abs(z_max);
    
    ODS("X: [ %+-7.3f | %+-7.3f ]\n", x_min, x_max);
    ODS("Y: [ %+-7.3f | %+-7.3f ]\n", y_min, y_max);
    ODS("Z: [ %+-7.3f | %+-7.3f ]\n", z_min, z_max);
    
    
    
    free(x_res);
    free(y_res);
    free(z_res);
    
    
    // === Step 2: Go through all the triangles, calculate centroids, assign Morton codes.
    
    typedef struct
    {
        vertex3 cen;
        u32 morton_code;
        AABB_padded bounding_box;
    } bvhdata;
    
    
    // CPU impl
    bvhdata *step2_cpu = (bvhdata *)malloc(sizeof(bvhdata) * model_tricount);  // FREE after the loop
    
    for(u32 i = 0; i < model_tricount; i++)
    {
        // indexes
        u32 i0 = model.indices[3*i];
        u32 i1 = model.indices[3*i+1];
        u32 i2 = model.indices[3*i+2];
        
        // vertexes
        r32 v0_x = model.vertices[8*i0];
        r32 v0_y = model.vertices[8*i0+1];
        r32 v0_z = model.vertices[8*i0+2];
        
        r32 v1_x = model.vertices[8*i1];
        r32 v1_y = model.vertices[8*i1+1];
        r32 v1_z = model.vertices[8*i1+2];
        
        r32 v2_x = model.vertices[8*i2];
        r32 v2_y = model.vertices[8*i2+1];
        r32 v2_z = model.vertices[8*i2+2];
        
        // centroid
        r32 cen_x = (v0_x + v1_x + v2_x)/3.0f;
        r32 cen_y = (v0_y + v1_y + v2_y)/3.0f;
        r32 cen_z = (v0_z + v1_z + v2_z)/3.0f;
        
        step2_cpu[i].cen.x = cen_x;
        step2_cpu[i].cen.y = cen_y;
        step2_cpu[i].cen.z = cen_z;
        
        // map to [0;1023], calculate the Morton code
        r32 new_range = 1024.0f;  // DANGER: if coord components are 1, we're fucked 
        u32 morton_x = (u32)floor((cen_x+abs(x_min))*(new_range/x_range));
        u32 morton_y = (u32)floor((cen_y+abs(y_min))*(new_range/y_range));
        u32 morton_z = (u32)floor((cen_z+abs(z_min))*(new_range/z_range));
        u32 morton = EncodeMorton3(morton_x, morton_y, morton_z);
        step2_cpu[i].morton_code = morton;
        
        // AABB
        r32 lower_x = min(min(v0_x, v1_x), v2_x);
        r32 lower_y = min(min(v0_y, v1_y), v2_y);
        r32 lower_z = min(min(v0_z, v1_z), v2_z);
        
        r32 upper_x = max(max(v0_x, v1_x), v2_x);
        r32 upper_y = max(max(v0_y, v1_y), v2_y);
        r32 upper_z = max(max(v0_z, v1_z), v2_z);
        
        
        step2_cpu[i].bounding_box.lower.x   = lower_x;
        step2_cpu[i].bounding_box.lower.y   = lower_y;
        step2_cpu[i].bounding_box.lower.z   = lower_z;
        step2_cpu[i].bounding_box.lower.pad = 0;
        
        step2_cpu[i].bounding_box.upper.x   = upper_x;
        step2_cpu[i].bounding_box.upper.y   = upper_y;
        step2_cpu[i].bounding_box.upper.z   = upper_z;
        step2_cpu[i].bounding_box.upper.pad = 0;
    }
    
    // FREE step2_cpu here
    free(step2_cpu);
    //exit(0);
    
    
    
    // resources
    // TO DO: rename bvhdata into leafdata
    
    u32 index_datasize = sizeof(u32) * model.index_count;
    VkDeviceMemory model_index_memory;
    VkBuffer model_index_buffer = CreateBuffer(index_datasize,
                                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                               vk.device, gpu_memprops,
                                               &model_index_memory);
    
    vertex_datasize = sizeof(float) * model.floats_per_vertex * model.vertex_count;
    VkDeviceMemory model_vertex_memory;
    VkBuffer model_vertex_buffer = CreateBuffer(vertex_datasize,
                                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                vk.device, gpu_memprops,
                                                &model_vertex_memory);
    
    VkDeviceMemory model_bvhdata_memory;
    VkBuffer model_bvhdata_buffer = CreateBuffer(sizeof(bvhdata) * model_tricount,
                                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                 vk.device, gpu_memprops,
                                                 &model_bvhdata_memory);
    
    void *model_index_mapptr;
    vkMapMemory(vk.device, model_index_memory, 0, VK_WHOLE_SIZE, 0, &model_index_mapptr);
    memcpy(model_index_mapptr, model.indices, index_datasize);
    
    void *model_vertex_mapptr;
    vkMapMemory(vk.device, model_vertex_memory, 0, VK_WHOLE_SIZE, 0, &model_vertex_mapptr);
    memcpy(model_vertex_mapptr, model.vertices, vertex_datasize);
    
    //FreeParsedOBJ(&model_obj);
    
    
    
    
    
    
    // Pipeline with 6 push constants, 3 bound buffers: 2 in + 1 out
    char *shader2 = "../code/centroidmorton.spv";
    
    VkPipelineShaderStageCreateInfo compipe2_stageci = {};
    compipe2_stageci.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    compipe2_stageci.pNext               = NULL;
    compipe2_stageci.flags               = 0;
    compipe2_stageci.stage               = VK_SHADER_STAGE_COMPUTE_BIT;
    compipe2_stageci.module              = GetShaderModule(shader2);
    compipe2_stageci.pName               = "main";
    compipe2_stageci.pSpecializationInfo = NULL;
    
    
    // index
    VkDescriptorSetLayoutBinding compipe2_bindindex = {};
    compipe2_bindindex.binding            = 0;
    compipe2_bindindex.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    compipe2_bindindex.descriptorCount    = 1;
    compipe2_bindindex.stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    compipe2_bindindex.pImmutableSamplers = NULL;
    
    // vertex
    VkDescriptorSetLayoutBinding compipe2_bindvertex = {};
    compipe2_bindvertex.binding            = 1;
    compipe2_bindvertex.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    compipe2_bindvertex.descriptorCount    = 1;
    compipe2_bindvertex.stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    compipe2_bindvertex.pImmutableSamplers = NULL;
    
    // bvhdata
    VkDescriptorSetLayoutBinding compipe2_bindbvhdata = {};
    compipe2_bindbvhdata.binding            = 2;
    compipe2_bindbvhdata.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    compipe2_bindbvhdata.descriptorCount    = 1;
    compipe2_bindbvhdata.stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    compipe2_bindbvhdata.pImmutableSamplers = NULL;
    
    VkDescriptorSetLayoutBinding compipe2_bindings[] = { compipe2_bindindex, compipe2_bindvertex, compipe2_bindbvhdata };
    
    
    VkDescriptorSetLayoutCreateInfo compipe2_dslci = {};
    compipe2_dslci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    compipe2_dslci.pNext        = NULL;
    compipe2_dslci.flags        = 0;
    compipe2_dslci.bindingCount = 3;
    compipe2_dslci.pBindings    = compipe2_bindings;
    
    
    VkDescriptorSetLayout compipe2_dsl;
    vkCreateDescriptorSetLayout(vk.device, &compipe2_dslci, NULL, &compipe2_dsl);
    
    VkDescriptorPoolSize compipe2_dspoolsize = {};
    compipe2_dspoolsize.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    compipe2_dspoolsize.descriptorCount = 3;
    
    VkDescriptorPoolCreateInfo compipe2_dspci = {};
    compipe2_dspci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    compipe2_dspci.pNext         = NULL;
    compipe2_dspci.flags         = 0;
    compipe2_dspci.maxSets       = 3;
    compipe2_dspci.poolSizeCount = 1;
    compipe2_dspci.pPoolSizes    = &compipe2_dspoolsize;
    VkDescriptorPool compipe2_dspool;
    vkCreateDescriptorPool(vk.device, &compipe2_dspci, NULL, &compipe2_dspool);
    
    VkDescriptorSetAllocateInfo compipe2_dsai = {};
    compipe2_dsai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    compipe2_dsai.pNext              = NULL;
    compipe2_dsai.descriptorPool     = compipe2_dspool;
    compipe2_dsai.descriptorSetCount = 1;
    compipe2_dsai.pSetLayouts        = &compipe2_dsl;
    VkDescriptorSet compipe2_ds;
    vkAllocateDescriptorSets(vk.device, &compipe2_dsai, &compipe2_ds);
    
    
    VkPushConstantRange pcr2 = {};
    pcr2.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pcr2.offset     = 0;
    pcr2.size       = sizeof(r32) * 6 + sizeof(u32);
    
    VkPipelineLayoutCreateInfo compipe2_layoutci = {};
    compipe2_layoutci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    compipe2_layoutci.pNext                  = NULL;
    compipe2_layoutci.flags                  = 0;
    compipe2_layoutci.setLayoutCount         = 1;
    compipe2_layoutci.pSetLayouts            = &compipe2_dsl;
    compipe2_layoutci.pushConstantRangeCount = 1;
    compipe2_layoutci.pPushConstantRanges    = &pcr2;
    
    VkPipelineLayout compipe2_layout;
    vkCreatePipelineLayout(vk.device, &compipe2_layoutci, NULL, &compipe2_layout);
    
    VkComputePipelineCreateInfo compute2_ci = {};
    compute2_ci.sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    compute2_ci.pNext              = NULL;
    compute2_ci.flags              = 0;
    compute2_ci.stage              = compipe2_stageci;
    compute2_ci.layout             = compipe2_layout;
    compute2_ci.basePipelineHandle = NULL;
    compute2_ci.basePipelineIndex  = 0;
    
    VkPipeline compipe2;
    vkCreateComputePipelines(vk.device, NULL, 1, &compute2_ci, NULL, &compipe2);
    
    
    VkDescriptorBufferInfo compipe2_indexwbi = {};
    compipe2_indexwbi.buffer = model_index_buffer;
    compipe2_indexwbi.offset = 0;
    compipe2_indexwbi.range  = VK_WHOLE_SIZE;
    
    VkWriteDescriptorSet compipe2_indexwrite = {};
    compipe2_indexwrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    compipe2_indexwrite.pNext            = NULL;
    compipe2_indexwrite.dstSet           = compipe2_ds;
    compipe2_indexwrite.dstBinding       = 0;
    compipe2_indexwrite.dstArrayElement  = 0;
    compipe2_indexwrite.descriptorCount  = 1;
    compipe2_indexwrite.descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    compipe2_indexwrite.pImageInfo       = NULL;
    compipe2_indexwrite.pBufferInfo      = &compipe2_indexwbi;
    compipe2_indexwrite.pTexelBufferView = NULL;
    
    VkDescriptorBufferInfo compipe2_vertexwbi = {};
    compipe2_vertexwbi.buffer = model_vertex_buffer;
    compipe2_vertexwbi.offset = 0;
    compipe2_vertexwbi.range  = VK_WHOLE_SIZE;
    
    VkWriteDescriptorSet compipe2_vertexwrite = {};
    compipe2_vertexwrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    compipe2_vertexwrite.pNext            = NULL;
    compipe2_vertexwrite.dstSet           = compipe2_ds;
    compipe2_vertexwrite.dstBinding       = 1;
    compipe2_vertexwrite.dstArrayElement  = 0;
    compipe2_vertexwrite.descriptorCount  = 1;
    compipe2_vertexwrite.descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    compipe2_vertexwrite.pImageInfo       = NULL;
    compipe2_vertexwrite.pBufferInfo      = &compipe2_vertexwbi;
    compipe2_vertexwrite.pTexelBufferView = NULL;
    
    VkDescriptorBufferInfo compipe2_bvhdatawbi = {};
    compipe2_bvhdatawbi.buffer = model_bvhdata_buffer;
    compipe2_bvhdatawbi.offset = 0;
    compipe2_bvhdatawbi.range  = VK_WHOLE_SIZE;
    
    VkWriteDescriptorSet compipe2_bvhdatawrite = {};
    compipe2_bvhdatawrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    compipe2_bvhdatawrite.pNext            = NULL;
    compipe2_bvhdatawrite.dstSet           = compipe2_ds;
    compipe2_bvhdatawrite.dstBinding       = 2;
    compipe2_bvhdatawrite.dstArrayElement  = 0;
    compipe2_bvhdatawrite.descriptorCount  = 1;
    compipe2_bvhdatawrite.descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    compipe2_bvhdatawrite.pImageInfo       = NULL;
    compipe2_bvhdatawrite.pBufferInfo      = &compipe2_bvhdatawbi;
    compipe2_bvhdatawrite.pTexelBufferView = NULL;
    
    VkWriteDescriptorSet compipe2_descwrites[] = { compipe2_indexwrite, compipe2_vertexwrite, compipe2_bvhdatawrite };
    vkUpdateDescriptorSets(vk.device, 3, compipe2_descwrites, 0, NULL);
    
    struct
    {
        u32 thread_count;
        r32 x_min;
        r32 x_range;
        r32 y_min;
        r32 y_range;
        r32 z_min;
        r32 z_range;
    } step2_controls;
    step2_controls.thread_count = model_tricount;
    step2_controls.x_min   = x_min;
    step2_controls.x_range = x_range;
    step2_controls.y_min   = y_min;
    step2_controls.y_range = y_range;
    step2_controls.z_min   = z_min;
    step2_controls.z_range = z_range;
    
    ODS("Check: step2 controls\n");
    ODS("- Thread count: %d\n", step2_controls.thread_count);
    ODS("- x_min:   %f\n", step2_controls.x_min);
    ODS("- x_range: %f\n", step2_controls.x_range);
    ODS("- y_min:   %f\n", step2_controls.y_min);
    ODS("- y_range: %f\n", step2_controls.y_range);
    ODS("- z_min:   %f\n", step2_controls.z_min);
    ODS("- z_range: %f\n", step2_controls.z_range);
    
    block_count = (u32)ceil((r32)model_tricount / (r32)block_size);
    
    // --- generate mortons
    // command buffer
    vkBeginCommandBuffer(vk.commandbuffer, &vk.commandbuffer_bi);
    vkCmdBindDescriptorSets(vk.commandbuffer, vk.bindpoint_compute, compipe2_layout, 0, 1, &compipe2_ds, 0, NULL);
    vkCmdBindPipeline(vk.commandbuffer, vk.bindpoint_compute, compipe2);
    vkCmdPushConstants(vk.commandbuffer, compipe2_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(r32) * 6 + sizeof(u32), &step2_controls);
    vkCmdDispatch(vk.commandbuffer, block_count, 1, 1);
    vkEndCommandBuffer(vk.commandbuffer);
    
    // execution
    vkQueueSubmit(vk.queue, 1, &compute_si, vk.fence);
    vkWaitForFences(vk.device, 1, &vk.fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vk.device, 1, &vk.fence);
    
    
    bvhdata *step2_data = (bvhdata *)malloc(sizeof(bvhdata) * model_tricount);
    //u32 *step2_data = (u32 *)malloc(sizeof(u32) * model_tricount);
    
    void *bvhdata_mapptr;
    vkMapMemory(vk.device, model_bvhdata_memory, 0, VK_WHOLE_SIZE, 0, &bvhdata_mapptr);
    memcpy(step2_data, bvhdata_mapptr, sizeof(bvhdata) * model_tricount);
    //memcpy(step2_data, bvhdata_mapptr, sizeof(u32) * model_tricount);
    vkUnmapMemory(vk.device, model_bvhdata_memory);
    
    
    
    // precision difference check
#if 0
    for(u32 i = 0; i < model_tricount; i++)
    {
        r32 epsilon = 0.000001;
        r32 epsilon_x = abs(step2_cpu[i].cen_x - step2_data[i].cen_x);
        r32 epsilon_y = abs(step2_cpu[i].cen_y - step2_data[i].cen_y);
        r32 epsilon_z = abs(step2_cpu[i].cen_z - step2_data[i].cen_z);
        
        if ((epsilon_x > epsilon) ||
            (epsilon_y > epsilon) ||
            (epsilon_z > epsilon))
        {
            ODS("> set %5d \n", i);
        }
    }
#endif
#if 0
    for(u32 i = 0; i < model_tricount; i++)
    {
        r32 epsilon = 0.000001;
        r32 epsilon_x = abs(step2_cpu[i].cen.x - step2_data[i].cen.x);
        r32 epsilon_y = abs(step2_cpu[i].cen.y - step2_data[i].cen.y);
        r32 epsilon_z = abs(step2_cpu[i].cen.z - step2_data[i].cen.z);
        
        if ((epsilon_x > epsilon) ||
            (epsilon_y > epsilon) ||
            (epsilon_z > epsilon))
        {
            ODS("> set %5d \n", i);
        }
    }
#endif
    
    
    
    // Morton sort
    
#if 0
    u32 worksize = 32;//model_tricount;
    u32 groupcount = worksize / block_size;
    
    
    u32 morton_datasize = worksize * sizeof(u32);
    
    for(u32 i = 0; i < worksize; i++)
    {
        ODS("%3d : %s\n", i, DecToBin(morton_data[i], 32));
    }
    ODS("\n");
    
    
    u32 *opspace = (u32 *)calloc(worksize, sizeof(u32));
    u32 *bitsums = (u32 *)calloc(worksize, sizeof(u32));
    
    u32 number_width = 32;
    u32 radix = 2;
    u32 counters[2] = {};
    
    // we go through every number, THEN go to the next bit.
    // Just a single group first.
    for(u32 bit_index = 0; bit_index < number_width; bit_index++)
    {
        u32 *digits  = (u32 *)calloc(2 * worksize, sizeof(u32));
        u32 *scan    = (u32 *)calloc(2 * worksize, sizeof(u32));
        u32 *shuffle = (u32 *)calloc(2 * worksize, sizeof(u32));
        
        ODS("start \n");
        for(u32 i = 0; i < worksize; i++)
        {
            u32 bit_value = (morton_data[i] & (1 << bit_index)) >> bit_index;
            counters[bit_value]++;
            // this is only working because I had a single group;
            //  but the pattern remains: write all 0s, then all 1s.
            digits[bit_value*number_width+i] = 1;
        }
        
        // scan the digits
        u32 scanner = 0;
        for(u32 j = 0; j < 63; j++)
        {
            scanner += digits[j];
            scan[j+1] = scanner;
        }
        
        u32 counter = 0;
        for(u32 j = 0; j < 64; j++)
        {
            if(digits[j] == 1)
            {
                u32 index = j % 32;
                shuffle[counter] = index;
                counter++;
            }
        }
        
        for(u32 j = 0; j < worksize; j++)
        {
            u32 index = shuffle[j];
            u32 value = morton_data[index];
            opspace[j] = value;
            // opspace[j] = morton_data[shuffle[j]];
        }
        
        for(u32 j = 0; j < worksize; j++)
        {
            morton_data[j] = opspace[j];
        }
        ODS("end \n");
    }
    
    
    
    ODS("Sorted: \n");
    for(u32 i = 0; i < worksize; i++)
    {
        ODS("%s \n", DecToBin(morton_data[i], number_width));
    }
    
    CheckMortonSorting(morton_data, worksize);
#endif
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    // --- SPLIT KERNEL MORTON SORT ---
    // --- variables
    const u32 blocksize = 32;
    u32 worksize = model_tricount;
    //u32 worksize = 64;
    u32 blockcount = (u32)ceil((r32)worksize / (r32)blocksize);  // I see some padding in the future
    
    // for each of the digit places...
    u32 bitwidth = 32;
    u32 process_digitcount = bitwidth;
    //u32 process_digitcount = 1;
    
    // push constants: int+pointer.
    u32 step1_pcr[2];
    step1_pcr[0] = worksize;
    step1_pcr[1] = process_digitcount;
    
    VkPipelineBindPoint pipeline_bindpoint = VK_PIPELINE_BIND_POINT_COMPUTE;
    
    
    
    // - prepare vulkan structures
    // This time I'll make one "super-format" pipeline, and fill it sparsely...
    //  You still need to at least recreate the pipeline, if you want to switch shaders.
    // TO DO: multiple pipelines, with recreation from caches
    
    
    
    // --- abstraction
    
    // What do I want from pipeline abstraction:
    // - specify shader file.
    // - specify resources.
    //  That's it. Have each pipeline come up with its own pool etc.
    //   A better system would've counted all the slots across all the pipelines first and
    //   produced appropriate pools. ...Maybe just a single one?...
    //   But that's probably metaprogramming.
    
    // seems like it's a good idea to have some kind of a return structure as well.
    // - push constants stuff, bindpoint: copy from input
    // - pipe layout, pipeline, descriptor set, descriptor writes: output
    
    
    
    // step 1
    typedef struct 
    {
        u32 index;
        u32 code;
    } primitive_entry;
    
    //u32 *morton_data = (u32 *)malloc(worksize * sizeof(u32));
    primitive_entry *morton_data = (primitive_entry *)malloc(worksize * sizeof(primitive_entry));
    for(u32 i = 0; i < worksize; i++)
    {
        morton_data[i].index = i;
        morton_data[i].code  = step2_data[i].morton_code;
    }
    
    // FREE step2_data here
    free(step2_data);
    
#if 0
    for(u32 i = 0; i < worksize; i++)
    {
        char *morton = DecToBin(morton_data[i].code, 32);
        ODS("%5d %s \n", i, morton);
        free(morton);
    }
#endif
    
    // - prepare vulkan data
    // TO DO: replace CPU-side buffer with a staging buffer and a GPU-side buffer
    u32 workdata_size  = worksize * sizeof(u32);
    u32 entrydata_size = worksize * sizeof(primitive_entry); 
    // I really dislike this ^ convention...
    
    VkDeviceMemory mortondata_memory;
    VkBuffer mortondata_buffer = CreateBuffer(entrydata_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                              vk.device, gpu_memprops,
                                              &mortondata_memory);
    VkDeviceMemory flag_vector_zero_memory;
    VkBuffer flag_vector_zero_buffer = CreateBuffer(workdata_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                    vk.device, gpu_memprops,
                                                    &flag_vector_zero_memory);
    VkDeviceMemory flag_vector_one_memory;
    VkBuffer flag_vector_one_buffer = CreateBuffer(workdata_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                   vk.device, gpu_memprops,
                                                   &flag_vector_one_memory);
    
    void *mortondata_mapptr;
    vkMapMemory(vk.device, mortondata_memory, 0, VK_WHOLE_SIZE, 0, &mortondata_mapptr);
    memcpy(mortondata_mapptr, morton_data, entrydata_size);
    
    void *flag_vector_zero_mapptr;
    vkMapMemory(vk.device, flag_vector_zero_memory, 0, VK_WHOLE_SIZE, 0, &flag_vector_zero_mapptr);
    
    void *flag_vector_one_mapptr;
    vkMapMemory(vk.device, flag_vector_one_memory, 0, VK_WHOLE_SIZE, 0, &flag_vector_one_mapptr);
    
    
    
    in_struct *step1_in = (in_struct *)calloc(1, sizeof(in_struct));
    step1_in->shader_file = String("../code/step1_shader.spv");
    
    step1_in->resource_count = 3;
    step1_in->resources = (resource_record *)calloc(step1_in->resource_count, sizeof(resource_record));
    step1_in->resources[0].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step1_in->resources[0].buffer = mortondata_buffer;
    step1_in->resources[0].memory = mortondata_memory;
    step1_in->resources[1].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step1_in->resources[1].buffer = flag_vector_zero_buffer;
    step1_in->resources[1].memory = flag_vector_zero_memory;
    step1_in->resources[2].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step1_in->resources[2].buffer = flag_vector_one_buffer;
    step1_in->resources[2].memory = flag_vector_one_memory;
    
    step1_in->pcr_size = 2;
    step1_in->pcr_data = (u32 *)calloc(step1_in->pcr_size, sizeof(u32));
    step1_in->pcr_data[0] = worksize;
    
    out_struct *step1_out = CreateComputePipeline(step1_in);
    
    
    
    // step 2
    VkDeviceMemory scan_vector_zero_memory;
    VkBuffer scan_vector_zero_buffer = CreateBuffer(workdata_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                    vk.device, gpu_memprops,
                                                    &scan_vector_zero_memory);
    VkDeviceMemory scan_vector_one_memory;
    VkBuffer scan_vector_one_buffer = CreateBuffer(workdata_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                   vk.device, gpu_memprops,
                                                   &scan_vector_one_memory);
    
    void *scan_vector_zero_mapptr;
    vkMapMemory(vk.device, scan_vector_zero_memory, 0, VK_WHOLE_SIZE, 0, &scan_vector_zero_mapptr);
    
    void *scan_vector_one_mapptr;
    vkMapMemory(vk.device, scan_vector_one_memory, 0, VK_WHOLE_SIZE, 0, &scan_vector_one_mapptr);
    
    
    
    in_struct *step2_in = (in_struct *)calloc(1, sizeof(in_struct));
    step2_in->shader_file = String("../code/step2_shader.spv");
    
    step2_in->resource_count = 4;
    step2_in->resources = (resource_record *)calloc(step2_in->resource_count, sizeof(resource_record));
    step2_in->resources[0].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step2_in->resources[0].buffer = flag_vector_zero_buffer;
    step2_in->resources[0].memory = flag_vector_zero_memory;
    step2_in->resources[1].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step2_in->resources[1].buffer = flag_vector_one_buffer;
    step2_in->resources[1].memory = flag_vector_one_memory;
    step2_in->resources[2].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step2_in->resources[2].buffer = scan_vector_zero_buffer;
    step2_in->resources[2].memory = scan_vector_zero_memory;
    step2_in->resources[3].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step2_in->resources[3].buffer = scan_vector_one_buffer;
    step2_in->resources[3].memory = scan_vector_one_memory;
    
    step2_in->pcr_size = 1;
    step2_in->pcr_data = (u32 *)calloc(step2_in->pcr_size, sizeof(u32));
    step2_in->pcr_data[0] = worksize;
    
    out_struct *step2_out = CreateComputePipeline(step2_in);
    
    
    
    // step 3 - flag vector sums
    // in: 2 flag vectors, [datasize]
    // out: 2 sum vectors, [blockcount]
    VkDeviceMemory flag_sum_zero_memory;
    VkBuffer flag_sum_zero_buffer = CreateBuffer(blockcount * sizeof(u32), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                 vk.device, gpu_memprops,
                                                 &flag_sum_zero_memory);
    VkDeviceMemory flag_sum_one_memory;
    VkBuffer flag_sum_one_buffer = CreateBuffer(blockcount * sizeof(u32), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                vk.device, gpu_memprops,
                                                &flag_sum_one_memory);
    
    void *flag_sum_zero_mapptr;
    vkMapMemory(vk.device, flag_sum_zero_memory, 0, VK_WHOLE_SIZE, 0, &flag_sum_zero_mapptr);
    
    void *flag_sum_one_mapptr;
    vkMapMemory(vk.device, flag_sum_one_memory, 0, VK_WHOLE_SIZE, 0, &flag_sum_one_mapptr);
    
    
    
    in_struct *step3_in = (in_struct *)calloc(1, sizeof(in_struct));
    step3_in->shader_file = String("../code/step3_shader.spv");
    
    step3_in->resource_count = 4;
    
    step3_in->resources = (resource_record *)calloc(step3_in->resource_count, sizeof(resource_record));
    step3_in->resources[0].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step3_in->resources[0].buffer = flag_vector_zero_buffer;
    step3_in->resources[0].memory = flag_vector_zero_memory;
    step3_in->resources[1].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step3_in->resources[1].buffer = flag_vector_one_buffer;
    step3_in->resources[1].memory = flag_vector_one_memory;
    step3_in->resources[2].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step3_in->resources[2].buffer = flag_sum_zero_buffer;
    step3_in->resources[2].memory = flag_sum_zero_memory;
    step3_in->resources[3].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step3_in->resources[3].buffer = flag_sum_one_buffer;
    step3_in->resources[3].memory = flag_sum_one_memory;
    
    step3_in->pcr_size = 1;
    step3_in->pcr_data = (u32 *)calloc(step3_in->pcr_size, sizeof(u32));
    step3_in->pcr_data[0] = worksize;
    
    out_struct *step3_out = CreateComputePipeline(step3_in);
    
    
    
    // step 4 - sum scan
    VkDeviceMemory sum_scan_memory;
    VkBuffer sum_scan_buffer = CreateBuffer(2 * blockcount * sizeof(u32), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                            vk.device, gpu_memprops,
                                            &sum_scan_memory);
    
    void *sum_scan_mapptr;
    vkMapMemory(vk.device, sum_scan_memory, 0, VK_WHOLE_SIZE, 0, &sum_scan_mapptr);
    
    
    
    in_struct *step4_in = (in_struct *)calloc(1, sizeof(in_struct));
    step4_in->shader_file = String("../code/step4_shader.spv");
    
    step4_in->resource_count = 3;
    
    step4_in->resources = (resource_record *)calloc(step4_in->resource_count, sizeof(resource_record));
    step4_in->resources[0].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step4_in->resources[0].buffer = flag_sum_zero_buffer;
    step4_in->resources[0].memory = flag_sum_zero_memory;
    step4_in->resources[1].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step4_in->resources[1].buffer = flag_sum_one_buffer;
    step4_in->resources[1].memory = flag_sum_one_memory;
    step4_in->resources[2].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step4_in->resources[2].buffer = sum_scan_buffer;
    step4_in->resources[2].memory = sum_scan_memory;
    
    out_struct *step4_out = CreateComputePipeline(step4_in);
    
    
    
    // step 5
    in_struct *step5_in = (in_struct *)calloc(1, sizeof(in_struct));
    step5_in->shader_file = String("../code/step5_shader.spv");
    
    step5_in->resource_count = 3;
    
    step5_in->resources = (resource_record *)calloc(step5_in->resource_count, sizeof(resource_record));
    step5_in->resources[0].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step5_in->resources[0].buffer = scan_vector_zero_buffer;
    step5_in->resources[0].memory = scan_vector_zero_memory;
    step5_in->resources[1].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step5_in->resources[1].buffer = scan_vector_one_buffer;
    step5_in->resources[1].memory = scan_vector_one_memory;
    step5_in->resources[2].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step5_in->resources[2].buffer = sum_scan_buffer;
    step5_in->resources[2].memory = sum_scan_memory;
    
    step5_in->pcr_size = 1;
    step5_in->pcr_data = (u32 *)calloc(step5_in->pcr_size, sizeof(u32));
    step5_in->pcr_data[0] = worksize;
    
    out_struct *step5_out = CreateComputePipeline(step5_in);
    
    
    
    // step 6
    in_struct *step6_in = (in_struct *)calloc(1, sizeof(in_struct));
    step6_in->shader_file = String("../code/step6_shader.spv");
    
    step6_in->resource_count = 6;
    
    VkDeviceMemory sorted_mortons_memory;
    VkBuffer sorted_mortons_buffer = CreateBuffer(entrydata_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                  vk.device, gpu_memprops,
                                                  &sorted_mortons_memory);
    
    void *sorted_mortons_mapptr;
    vkMapMemory(vk.device, sorted_mortons_memory, 0, VK_WHOLE_SIZE, 0, &sorted_mortons_mapptr);
    
    
    step6_in->resources = (resource_record *)calloc(step6_in->resource_count, sizeof(resource_record));
    step6_in->resources[0].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step6_in->resources[0].buffer = flag_vector_zero_buffer;
    step6_in->resources[0].memory = flag_vector_zero_memory;
    step6_in->resources[1].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step6_in->resources[1].buffer = flag_vector_one_buffer;
    step6_in->resources[1].memory = flag_vector_one_memory;
    
    step6_in->resources[2].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step6_in->resources[2].buffer = scan_vector_zero_buffer;
    step6_in->resources[2].memory = scan_vector_zero_memory;
    step6_in->resources[3].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step6_in->resources[3].buffer = scan_vector_one_buffer;
    step6_in->resources[3].memory = scan_vector_one_memory;
    
    step6_in->resources[4].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step6_in->resources[4].buffer = mortondata_buffer;
    step6_in->resources[4].memory = mortondata_memory;
    step6_in->resources[5].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step6_in->resources[5].buffer = sorted_mortons_buffer;
    step6_in->resources[5].memory = sorted_mortons_memory;
    
    step6_in->pcr_size = 1;
    step6_in->pcr_data = (u32 *)calloc(step6_in->pcr_size, sizeof(u32));
    step6_in->pcr_data[0] = worksize;
    
    out_struct *step6_out = CreateComputePipeline(step6_in);
    
    
    
    // step 7 - copy sorted back into inputs
    in_struct *step7_in = (in_struct *)calloc(1, sizeof(in_struct));
    step7_in->shader_file = String("../code/step7_shader.spv");
    
    step7_in->resource_count = 2;
    
    step7_in->resources = (resource_record *)calloc(step7_in->resource_count, sizeof(resource_record));
    step7_in->resources[0].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step7_in->resources[0].buffer = sorted_mortons_buffer;
    step7_in->resources[0].memory = sorted_mortons_memory;
    step7_in->resources[1].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    step7_in->resources[1].buffer = mortondata_buffer;
    step7_in->resources[1].memory = mortondata_memory;
    
    step7_in->pcr_size = 1;
    step7_in->pcr_data = (u32 *)calloc(step7_in->pcr_size, sizeof(u32));
    step7_in->pcr_data[0] = worksize;
    
    out_struct *step7_out = CreateComputePipeline(step7_in);
    
    
#define PRINT 0
    u32 printsize = 32;
    
    // a lot of emancipation's about to happen here...
    for(u32 i = 0; i < process_digitcount; i++)
    {
        //ODS("> PROCESSING DIGIT %d \n", i);
        
        step1_in->pcr_data[1] = i;
        vkUpdateDescriptorSets(vk.device, step1_out->descwrite_count, step1_out->descwrites, 0, NULL);
        
        vkBeginCommandBuffer(vk.commandbuffer, &vk.commandbuffer_bi);
        vkCmdBindDescriptorSets(vk.commandbuffer, pipeline_bindpoint, step1_out->pipe_layout, 0, 1, &step1_out->pipe_dset, 0, NULL);
        vkCmdPushConstants(vk.commandbuffer, step1_out->pipe_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, step1_in->pcr_size * sizeof(u32), step1_in->pcr_data);
        vkCmdBindPipeline(vk.commandbuffer, pipeline_bindpoint, step1_out->pipe);
        vkCmdDispatch(vk.commandbuffer, blockcount, 1, 1);
        vkEndCommandBuffer(vk.commandbuffer);
        
        vkQueueSubmit(vk.queue, 1, &compute_si, vk.fence);
        vkWaitForFences(vk.device, 1, &vk.fence, VK_TRUE, UINT64_MAX);
        vkResetFences(vk.device, 1, &vk.fence);
        
        // - read output
        u32 *step1_output_flag_vector_zero = (u32 *)calloc(1, workdata_size);
        memcpy(step1_output_flag_vector_zero, flag_vector_zero_mapptr, workdata_size);
        
        u32 *step1_output_flag_vector_one  = (u32 *)calloc(1, workdata_size);
        memcpy(step1_output_flag_vector_one, flag_vector_one_mapptr, workdata_size);
        
        
        // - verify results
        u32 *step1_check_flag_vector_zero = (u32 *)malloc(workdata_size);  // FREE at the end of the loop
        u32 *step1_check_flag_vector_one  = (u32 *)malloc(workdata_size);
        for(u32 j = 0; j < worksize; j++)
        {
            u32 bit = ((morton_data[j].code & (1 << i)) >> i) & 1;
            step1_check_flag_vector_zero[j] = (bit == 0) ? 1 : 0;
            step1_check_flag_vector_one[j]  = bit;
        }
        
#if PRINT
        ODS("\n STEP 1 \n");
        ODS("Data: \n");
        for(u32 j = 0; j < printsize; j++)
        {
            ODS("%s \n", DecToBin(morton_data[j], 32));
        }
        ODS("Read by GPU: \n|");
        for(u32 j = 0; j < printsize; j++)
        {
            ODS("%2d|", step1_output_flag_vector_zero[j]);
        }
        ODS("\n|");
        for(u32 j = 0; j < printsize; j++)
        {
            ODS("%2d|", step1_output_flag_vector_one[j]);
        }
        ODS("\n");
        ODS("Read by CPU: \n|");
        for(u32 j = 0; j < printsize; j++)
        {
            ODS("%2d|", step1_check_flag_vector_zero[j]);
        }
        ODS("\n|");
        for(u32 j = 0; j < printsize; j++)
        {
            ODS("%2d|", step1_check_flag_vector_one[j]);
        }
        ODS("\n");
        
        ODS("Verification: ");
        bool correct = true;
        for(u32 j = 0; j < worksize; j++)
        {
            if(step1_output_flag_vector_zero[j] != step1_check_flag_vector_zero[j])
                correct = false;
            if(step1_output_flag_vector_one[j] != step1_check_flag_vector_one[j])
                correct = false;
        }
        ODS("%s \n", (correct == true) ? "CORRECT" : "INCORRECT");
#endif
        
        
        
        // Step 2: scan vectors
        vkUpdateDescriptorSets(vk.device, step2_out->descwrite_count, step2_out->descwrites, 0, NULL);
        
        vkBeginCommandBuffer(vk.commandbuffer, &vk.commandbuffer_bi);
        vkCmdBindDescriptorSets(vk.commandbuffer, pipeline_bindpoint, step2_out->pipe_layout, 0, 1, &step2_out->pipe_dset, 0, NULL);
        vkCmdPushConstants(vk.commandbuffer, step2_out->pipe_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, step2_in->pcr_size * sizeof(u32), step2_in->pcr_data);
        vkCmdBindPipeline(vk.commandbuffer, pipeline_bindpoint, step2_out->pipe);
        vkCmdDispatch(vk.commandbuffer, blockcount, 1, 1);
        vkEndCommandBuffer(vk.commandbuffer);
        
        vkQueueSubmit(vk.queue, 1, &compute_si, vk.fence);
        vkWaitForFences(vk.device, 1, &vk.fence, VK_TRUE, UINT64_MAX);
        vkResetFences(vk.device, 1, &vk.fence);
        
        // - read results
        u32 *step2_output_scan_vector_zero = (u32 *)calloc(1, workdata_size);
        memcpy(step2_output_scan_vector_zero, scan_vector_zero_mapptr, workdata_size);
        
        u32 *step2_output_scan_vector_one  = (u32 *)calloc(1, workdata_size);
        memcpy(step2_output_scan_vector_one,  scan_vector_one_mapptr,  workdata_size);
        
        
#if PRINT
        ODS("\n STEP 2 \n");
        ODS("Calculated by GPU: \n");
        ODS("- zero vector scan: \n|");
        for(u32 j = 0; j < printsize; j++)
        {
            ODS("%2d|", step2_output_scan_vector_zero[j]);
        }
        ODS("\n");
        ODS("- one vector scan: \n|");
        for(u32 j = 0; j < printsize; j++)
        {
            ODS("%2d|", step2_output_scan_vector_one[j]);
        }
        ODS("\n");
#endif
        
        
        u32 *step2_check_scan_vector_zero = (u32 *)calloc(1, workdata_size);
        u32 *step2_check_scan_vector_one  = (u32 *)calloc(1, workdata_size);
        
        u32 scanner0 = 0;
        for(u32 j = 0; j < worksize; j++)
        {
            step2_check_scan_vector_zero[j] = scanner0;
            scanner0 += step1_check_flag_vector_zero[j];
        }
        u32 scanner1 = 0;
        for(u32 j = 0; j < worksize; j++)
        {
            step2_check_scan_vector_one[j] = scanner1;
            scanner1 += step1_check_flag_vector_one[j];
        }
        
#if PRINT
        ODS("Calculated by CPU: \n");
        ODS("- zero vector scan: \n|");
        for(u32 j = 0; j < printsize; j++)
        {
            ODS("%2d|", step2_check_scan_vector_zero[j]);
        }
        ODS("\n");
        ODS("- one vector scan: \n|");
        for(u32 j = 0; j < printsize; j++)
        {
            ODS("%2d|", step2_check_scan_vector_one[j]);
        }
        ODS("\n");
        
        ODS("Verification: ");
        correct = true;
        for(u32 j = 0; j < worksize; j++)
        {
            if(step2_output_scan_vector_zero[j] != step2_check_scan_vector_zero[j])
                correct = false;
            if(step2_output_scan_vector_one[j] != step2_check_scan_vector_one[j])
                correct = false;
        }
        ODS("%s \n", (correct == true) ? "CORRECT" : "INCORRECT");
#endif
        
        
        
        // Step 3: block sums
        vkUpdateDescriptorSets(vk.device, step3_out->descwrite_count, step3_out->descwrites, 0, NULL);
        
        vkBeginCommandBuffer(vk.commandbuffer, &vk.commandbuffer_bi);
        vkCmdBindDescriptorSets(vk.commandbuffer, pipeline_bindpoint, step3_out->pipe_layout, 0, 1, &step3_out->pipe_dset, 0, NULL);
        vkCmdPushConstants(vk.commandbuffer, step3_out->pipe_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, step3_in->pcr_size * sizeof(u32), step3_in->pcr_data);
        vkCmdBindPipeline(vk.commandbuffer, pipeline_bindpoint, step3_out->pipe);
        vkCmdDispatch(vk.commandbuffer, blockcount, 1, 1);
        vkEndCommandBuffer(vk.commandbuffer);
        
        vkQueueSubmit(vk.queue, 1, &compute_si, vk.fence);
        vkWaitForFences(vk.device, 1, &vk.fence, VK_TRUE, UINT64_MAX);
        vkResetFences(vk.device, 1, &vk.fence);
        
        u32 *step3_output_sum_vector_zero = (u32 *)calloc(blockcount, sizeof(u32));
        memcpy(step3_output_sum_vector_zero, flag_sum_zero_mapptr, blockcount * sizeof(u32));
        
        u32 *step3_output_sum_vector_one  = (u32 *)calloc(blockcount, sizeof(u32));
        memcpy(step3_output_sum_vector_one,  flag_sum_one_mapptr,  blockcount * sizeof(u32));
        
        
#if PRINT
        ODS("\n Step 3 \n");
        ODS("GPU output: \n");
        ODS("- zero sums: \n|");
        for(u32 j = 0; j < blockcount; j++)
        {
            ODS("%2d|", step3_output_sum_vector_zero[j]);
        }
        ODS("\n");
        ODS("- one sums: \n|");
        for(u32 j = 0; j < blockcount; j++)
        {
            ODS("%2d|", step3_output_sum_vector_one[j]);
        }
        ODS("\n");
#endif
        
        
        u32 *step3_check_sum_vector_zero = (u32 *)calloc(blockcount, sizeof(u32));
        u32 *step3_check_sum_vector_one  = (u32 *)calloc(blockcount, sizeof(u32));
        
#if PRINT
        ODS("CPU verification: \n");
        for(u32 j = 0; j < worksize; j++)
        {
            u32 blockindex = j / blocksize;
            step3_check_sum_vector_zero[blockindex] += step1_check_flag_vector_zero[j];
            step3_check_sum_vector_one[blockindex]  += step1_check_flag_vector_one[j];
        }
        
        ODS("Verification results: ");
        correct = true;
        for(u32 j = 0; j < blockcount; j++)
        {
            if(step3_output_sum_vector_zero[j] != step3_check_sum_vector_zero[j])
                correct = false;
            if(step3_output_sum_vector_one[j] != step3_check_sum_vector_one[j])
                correct = false;
        }
        ODS("%s \n", (correct == true) ? "CORRECT" : "INCORRECT");
#endif
        
        
        
        // Step 4: sum scans
        vkUpdateDescriptorSets(vk.device, step4_out->descwrite_count, step4_out->descwrites, 0, NULL);
        
        vkBeginCommandBuffer(vk.commandbuffer, &vk.commandbuffer_bi);
        vkCmdBindDescriptorSets(vk.commandbuffer, pipeline_bindpoint, step4_out->pipe_layout, 0, 1, &step4_out->pipe_dset, 0, NULL);
        vkCmdBindPipeline(vk.commandbuffer, pipeline_bindpoint, step4_out->pipe);
        vkCmdDispatch(vk.commandbuffer, blockcount, 1, 1);
        vkEndCommandBuffer(vk.commandbuffer);
        
        vkQueueSubmit(vk.queue, 1, &compute_si, vk.fence);
        vkWaitForFences(vk.device, 1, &vk.fence, VK_TRUE, UINT64_MAX);
        vkResetFences(vk.device, 1, &vk.fence);
        
        
        u32 *step4_output_sum_scan = (u32 *)calloc(2 * blockcount, sizeof(u32));
        memcpy(step4_output_sum_scan, sum_scan_mapptr, 2 * blockcount * sizeof(u32));
        
#if PRINT
        ODS("\n STEP 4 \n");
        ODS("GPU output: \n");
        ODS("- sum scan: \n|");
        for(u32 j = 0; j < (2 * blockcount); j++)
        {
            ODS("%2d|", step4_output_sum_scan[j]);
        }
        ODS("\n");
        
        ODS("CPU check: \n");
#endif
        
        u32 *step4_check_sum_scan = (u32 *)calloc(2 * blockcount, sizeof(u32));
        u32 sum = 0;
        for(u32 j = 0; j < blockcount; j++)
        {
            step4_check_sum_scan[j] = sum;
            sum += step3_check_sum_vector_zero[j];
        }
        for(u32 j = 0; j < blockcount; j++)
        {
            step4_check_sum_scan[blockcount + j] = sum;
            sum += step3_check_sum_vector_one[j];
        }
        
#if PRINT
        ODS("- sum scan: \n|");
        for(u32 j = 0; j < (2 * blockcount); j++)
        {
            ODS("%2d|", step4_output_sum_scan[j]);
        }
        ODS("\n");
        
        ODS("Verification: ");
        correct = true;
        for(u32 j = 0; j < (2 * blockcount); j++)
        {
            if(step4_output_sum_scan[j] != step4_check_sum_scan[j])
                correct = false;
        }
        ODS("%s \n", (correct = true) ? "PASSED" : "FAILED" );
#endif
        
        
        
        // Step 5: modify flag scans with sum scans
        vkUpdateDescriptorSets(vk.device, step5_out->descwrite_count, step5_out->descwrites, 0, NULL);
        
        vkBeginCommandBuffer(vk.commandbuffer, &vk.commandbuffer_bi);
        vkCmdBindDescriptorSets(vk.commandbuffer, pipeline_bindpoint, step5_out->pipe_layout, 0, 1, &step5_out->pipe_dset, 0, NULL);
        vkCmdPushConstants(vk.commandbuffer, step5_out->pipe_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, step5_in->pcr_size * sizeof(u32), step5_in->pcr_data);
        vkCmdBindPipeline(vk.commandbuffer, pipeline_bindpoint, step5_out->pipe);
        vkCmdDispatch(vk.commandbuffer, blockcount, 1, 1);
        vkEndCommandBuffer(vk.commandbuffer);
        
        vkQueueSubmit(vk.queue, 1, &compute_si, vk.fence);
        vkWaitForFences(vk.device, 1, &vk.fence, VK_TRUE, UINT64_MAX);
        vkResetFences(vk.device, 1, &vk.fence);
        
        
        
        u32 *step5_output_scan_vector_zero = (u32 *)calloc(1, workdata_size);
        memcpy(step5_output_scan_vector_zero, scan_vector_zero_mapptr, workdata_size);
        
        u32 *step5_output_scan_vector_one  = (u32 *)calloc(1, workdata_size);
        memcpy(step5_output_scan_vector_one,  scan_vector_one_mapptr,  workdata_size);
        
        
#if PRINT
        ODS("\n STEP 5 \n");
        ODS("GPU output: \n");
        ODS("- modified flag scans: \n|");
        for(u32 j = 0; j < printsize; j++)
        {
            ODS("%2d|", step5_output_scan_vector_zero[j]);
        }
        ODS("\n|");
        for(u32 j = 0; j < printsize; j++)
        {
            ODS("%2d|", step5_output_scan_vector_one[j]);
        }
        ODS("\n");
#endif
        
        
        
        // Step 6: redistribute values
        vkUpdateDescriptorSets(vk.device, step6_out->descwrite_count, step6_out->descwrites, 0, NULL);
        
        vkBeginCommandBuffer(vk.commandbuffer, &vk.commandbuffer_bi);
        vkCmdBindDescriptorSets(vk.commandbuffer, pipeline_bindpoint, step6_out->pipe_layout, 0, 1, &step6_out->pipe_dset, 0, NULL);
        vkCmdPushConstants(vk.commandbuffer, step6_out->pipe_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, step6_in->pcr_size * sizeof(u32), step6_in->pcr_data);
        vkCmdBindPipeline(vk.commandbuffer, pipeline_bindpoint, step6_out->pipe);
        vkCmdDispatch(vk.commandbuffer, blockcount, 1, 1);
        vkEndCommandBuffer(vk.commandbuffer);
        
        vkQueueSubmit(vk.queue, 1, &compute_si, vk.fence);
        vkWaitForFences(vk.device, 1, &vk.fence, VK_TRUE, UINT64_MAX);
        vkResetFences(vk.device, 1, &vk.fence);
        
        
#if PRINT
        ODS("\n STEP 6 \n");
        u32 *step6_output_sorted_mortons = (u32 *)calloc(worksize, sizeof(u32));
        memcpy(step6_output_sorted_mortons, sorted_mortons_mapptr, worksize * sizeof(u32));
        
        ODS("GPU output: \n");
        ODS("- sorted morton values: \n");
        for(u32 j = 0; j < printsize; j++)
        {
            ODS("%s \n", DecToBin(step6_output_sorted_mortons[j], 32));
        }
        ODS("\n");
        
        free(step6_output_sorted_mortons);
#endif
        
        
        
        // Step 7: copy sorted mortons back into inputs
        vkUpdateDescriptorSets(vk.device, step7_out->descwrite_count, step7_out->descwrites, 0, NULL);
        
        vkBeginCommandBuffer(vk.commandbuffer, &vk.commandbuffer_bi);
        vkCmdBindDescriptorSets(vk.commandbuffer, pipeline_bindpoint, step7_out->pipe_layout, 0, 1, &step7_out->pipe_dset, 0, NULL);
        vkCmdPushConstants(vk.commandbuffer, step7_out->pipe_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, step7_in->pcr_size * sizeof(u32), step7_in->pcr_data);
        vkCmdBindPipeline(vk.commandbuffer, pipeline_bindpoint, step7_out->pipe);
        vkCmdDispatch(vk.commandbuffer, blockcount, 1, 1);
        vkEndCommandBuffer(vk.commandbuffer);
        
        vkQueueSubmit(vk.queue, 1, &compute_si, vk.fence);
        vkWaitForFences(vk.device, 1, &vk.fence, VK_TRUE, UINT64_MAX);
        vkResetFences(vk.device, 1, &vk.fence);
        
        
        
        // MEMORY CLEANUP(I think it's fine to have seven or two heap chunks at the same time)
        free(step1_output_flag_vector_zero);
        free(step1_output_flag_vector_one);
        free(step1_check_flag_vector_zero);
        free(step1_check_flag_vector_one);
        free(step2_output_scan_vector_zero);
        free(step2_output_scan_vector_one);
        free(step2_check_scan_vector_zero);
        free(step2_check_scan_vector_one);
        free(step3_output_sum_vector_zero);
        free(step3_output_sum_vector_one);
        free(step3_check_sum_vector_zero);
        free(step3_check_sum_vector_one);
        free(step4_output_sum_scan);
        free(step4_check_sum_scan);
        free(step5_output_scan_vector_zero);
        free(step5_output_scan_vector_one);
        
#if PRINT
        free(step6_output_sorted_mortons);
#endif
    }
    
    free(morton_data);
    
    
    
    // final check
    primitive_entry *sorted_mortons = (primitive_entry *)calloc(worksize, sizeof(primitive_entry));
    memcpy(sorted_mortons, mortondata_mapptr, worksize * sizeof(primitive_entry));
    
#if 1
    ODS("Sorted mortons: \n");
    for(u32 i = 0; i < worksize; i++)
    {
        char *morton = DecToBin(sorted_mortons[i].code, 32);
        ODS("%5d %5d %s \n", i, sorted_mortons[i].index, morton);
        free(morton);
    }
    ODS("\n");
#endif
    
    
    bool correct = true;
    for(u32 i = 0; i < worksize-1; i++)
    {
        if(sorted_mortons[i].code > sorted_mortons[i+1].code)
            correct = false;
    }
    ODS("FINAL VALIDATION: %s \n", (correct == true) ? "PASSED" : "FAILED");
    
    
    vkUnmapMemory(vk.device, flag_vector_zero_memory);
    vkUnmapMemory(vk.device, flag_vector_one_memory);
    vkUnmapMemory(vk.device, scan_vector_zero_memory);
    vkUnmapMemory(vk.device, scan_vector_one_memory);
    vkUnmapMemory(vk.device, flag_sum_zero_memory);
    vkUnmapMemory(vk.device, flag_sum_one_memory);
    vkUnmapMemory(vk.device, sum_scan_memory);
    vkUnmapMemory(vk.device, mortondata_memory);
    
    
    
    
    
    // --- build a tree over the Morton codes
    //worksize = 32;
    
    primitive_entry *sorted_keys = (primitive_entry *)calloc(worksize, sizeof(primitive_entry));
    memcpy(sorted_keys, sorted_mortons_mapptr, worksize*sizeof(primitive_entry));
    
    vkUnmapMemory(vk.device, sorted_mortons_memory);
    
    
    
    u32 tree_datasize = (2 * worksize - 1) * sizeof(tree_entry);
    
    VkDeviceMemory tree_memory;
    VkBuffer tree_buffer = CreateBuffer(tree_datasize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                        vk.device, gpu_memprops,
                                        &tree_memory);
    
    
    tree_entry *tree_data = (tree_entry *)calloc(2*worksize-1, sizeof(tree_entry));
    
    tree_data[0].parent = -1;  // the root has no parent
    
    // write -1 to left and right of leaves, as they have no children
    for(u32 i = worksize-1; i < 2*worksize-1; i++)
    {
        tree_data[i].left  = -1;
        tree_data[i].right = -1;
    }
    
    void *tree_mapptr;
    vkMapMemory(vk.device, tree_memory, 0, VK_WHOLE_SIZE, 0, &tree_mapptr);
    memcpy(tree_mapptr, tree_data, tree_datasize);
    
    
    // you realize how easy it is to META this? :)
    in_struct *tree_in = (in_struct *)calloc(1, sizeof(in_struct));
    tree_in->shader_file = String("../code/tree_shader.spv");
    
    tree_in->resource_count = 2;
    
    tree_in->resources = (resource_record *)calloc(tree_in->resource_count, sizeof(resource_record));
    tree_in->resources[0].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    tree_in->resources[0].buffer = sorted_mortons_buffer;
    tree_in->resources[0].memory = sorted_mortons_memory;
    tree_in->resources[1].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    tree_in->resources[1].buffer = tree_buffer;
    tree_in->resources[1].memory = tree_memory;
    
    tree_in->pcr_size = 1;
    tree_in->pcr_data = (u32 *)calloc(tree_in->pcr_size, sizeof(u32));
    tree_in->pcr_data[0] = worksize;
    
    out_struct *tree_out = CreateComputePipeline(tree_in);
    
    
    blockcount = worksize / blocksize;
    
    vkUpdateDescriptorSets(vk.device, tree_out->descwrite_count, tree_out->descwrites, 0, NULL);
    
    vkBeginCommandBuffer(vk.commandbuffer, &vk.commandbuffer_bi);
    vkCmdBindDescriptorSets(vk.commandbuffer, pipeline_bindpoint, tree_out->pipe_layout, 0, 1, &tree_out->pipe_dset, 0, NULL);
    vkCmdPushConstants(vk.commandbuffer, tree_out->pipe_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, tree_in->pcr_size * sizeof(u32), tree_in->pcr_data);
    vkCmdBindPipeline(vk.commandbuffer, pipeline_bindpoint, tree_out->pipe);
    vkCmdDispatch(vk.commandbuffer, blockcount, 1, 1);
    vkEndCommandBuffer(vk.commandbuffer);
    
    result = vkQueueSubmit(vk.queue, 1, &compute_si, vk.fence);
    ODS_RES("Execution: %s \n");
    result = vkWaitForFences(vk.device, 1, &vk.fence, VK_TRUE, UINT64_MAX);
    ODS_RES("Suspicious fence wait: %s \n");
    result = vkGetFenceStatus(vk.device, vk.fence);
    ODS_RES("Suspicious fence status: %s \n");
    result = vkResetFences(vk.device, 1, &vk.fence);
    ODS_RES("Suspicious fence reset: %s \n");
    
    memcpy(tree_data, tree_mapptr, tree_datasize);
    vkUnmapMemory(vk.device, tree_memory);
    
    
    u32 *sorted_values = (u32 *)calloc(worksize, sizeof(u32));
    for(u32 i = 0; i < worksize; i++)
        sorted_values[i] = sorted_keys[i].code;
    
    tree_entry *tree_check = (tree_entry *)calloc(2*worksize-1, sizeof(tree_entry));  // FREE after the validation
    tree_check[0].parent = -1;
    
    u32 n = worksize;
    for(u32 i = (n-1); i < (2*n-1); i++)
    {
        tree_check[i].left  = -1;
        tree_check[i].right = -1;
    }
    
    
    // last index == n-2, so processing n-1 elements
    for(u32 i = 0; i < n-1; i++)
    {
        s32 d = ((delta(sorted_values, n, i, i+1) - delta(sorted_values, n, i, i-1)) > 0) ? 1: -1;
        
        s32 delta_min = delta(sorted_values, n, i, i-d);
        //tree_check[i].delta_min = delta_min;
        
        s32 lmax = 2;
        while(delta(sorted_values, n, i, i+lmax*d) > delta_min)
            lmax *= 2;
        
        u32 l = 0;
        r32 m = 2.0f;
        s32 t = (s32)(lmax/m);
        for(; t >= 1; t = (s32)(lmax/m))
        {
            s32 del = delta(sorted_values, n, i, i+(t+l)*d);
            if(del > delta_min)
                l += t;
            m *= 2;
        }
        
        s32 j = i + l*d;
        
        s32 gamma = 0;
        if(i != j)
        {
            s32 delta_node = delta(sorted_values, n, i, j);
            //tree_check[i].delta_node = delta_node;
            
            s32 s = 0;
            r32 m = 2.0f;
            s32 t = (s32)ceil(((r32)l)/m);
            for(; t > 1; t = (s32)ceil(((r32)l)/m))
            {
                s32 del = delta(sorted_values, n, i, i+(s+t)*d);
                if(del > delta_node)
                    s += t;
                m *= 2.0f;
            }
            t = 1;
            if(delta(sorted_values, n, i, i+(s+t)*d) > delta_node)
                s += t;
            gamma = i + s*d + min(d, 0);
        }
        else
            gamma = min(i, j);
        
        if(min(i,j) == gamma)
            tree_check[i].left = gamma+(n-1);
        else
            tree_check[i].left = gamma;
        
        if(max(i,j) == (gamma+1))
            tree_check[i].right = gamma+1+(n-1);
        else
            tree_check[i].right = gamma+1;
        
        //tree_check[i].d = d;
#if 1
        //ODS("Closing in on the suspect \n");
        s32 left_index  = tree_check[i].left;
        s32 right_index = tree_check[i].right;
        if((left_index < 0) || (left_index >= (2*worksize-1)))
        {
            ODS("ALARM!!! %5d, left = %d \n", i, left_index);
            //exit(0);
        }
        if((right_index < 0) || (right_index >= (2*worksize-1)))
        {
            ODS("ALARM!!! %5d, right = %d \n", i, right_index);
            //exit(0);
        }
        
        tree_check[left_index].parent  = i;
        tree_check[right_index].parent = i;
#endif
#if 0
        if()
            ODS("%d %d \n", i, tree_check[i].left);
        ODS("%d %d \n", i, tree_check[i].right);
        tree_check[tree_check[i].left].parent  = i;
        tree_check[tree_check[i].right].parent = i;
#endif
    }
    
    correct = true;
    for(u32 i = 0; i < (2*worksize-1); i++)
    {
        if(
            /*
            (tree_data[i].d          != tree_check[i].d)          ||
            (tree_data[i].delta_min  != tree_check[i].delta_min)  ||
            (tree_data[i].delta_node != tree_check[i].delta_node) ||
            */
            (tree_data[i].parent     != tree_check[i].parent)     ||
            (tree_data[i].left       != tree_check[i].left)       ||
            (tree_data[i].right      != tree_check[i].right))
        {
            ODS("- Divergence at %s %d \n", (i < worksize) ? "node" : "leaf", i);
            correct = false;
        }
    }
    ODS("Tree construction verification: %s \n", correct ? "PASSED" : "FAILED");
    
    //ODS("Look at the tree here \n");
    
    
    // check to see if there's a lost leaf or if index assignment is incorrect
#if 0    
    
    u32 node_count = worksize-1;
    u32 last_node_index = node_count-1;
    u32 *leaf_counters = (u32 *)calloc(worksize, sizeof(u32));
    for(u32 i = 0; i < node_count; i++)
    {
        u32 left  = tree_data[i].left;
        u32 right = tree_data[i].right;
        
        if(left > last_node_index)
            leaf_counters[left-node_count]++;
        if(right > last_node_index)
            leaf_counters[right-node_count]++;
    }
    
    for(u32 i = 0; i < worksize; i++)
    {
        if(leaf_counters[i] != 1)
        {
            ODS("Leaf %d is suspicious \n", i);
        }
    }
    exit(0);
#endif
    
    // TO DO: there is no provision on some of the compute steps for thread counts not cleanly divisible by 32. DANGER!
    // I would expect the rabbit and the dragon to have some hiccups due to that.
    
    
    //exit(0);
    
    
    
    // --- Let's try vk-resource cleanup here.
    // step1-7 pipelines, clean them up
    CleanPipeline(step1_out);
    CleanPipeline(step2_out);
    CleanPipeline(step3_out);
    CleanPipeline(step4_out);
    CleanPipeline(step5_out);
    CleanPipeline(step6_out);
    CleanPipeline(step7_out);
    CleanPipeline(tree_out);
    
    
    
    // --- Before implementing BVH on GPU, do it on CPU.
    // And for that, you need to explore the tree first.
    // Which is real easy. An array of 15487 values, each walks up until the parent is -1.
    // Which is also real easy to do on GPU :P
    
    // Later, counting up all the depth values tells me how many there are on each level,
    //  and I can write nodes into lists of levels. Then I'll have an easy time calcing AABBs on CPU.
    
    // inputs: tree_check.
    
    VkDeviceMemory tree_depth_memory;
    VkBuffer tree_depth_buffer = CreateBuffer(worksize * sizeof(u32), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                              vk.device, gpu_memprops,
                                              &tree_depth_memory);
    
    
    in_struct *tree_depth_in = (in_struct *)calloc(1, sizeof(in_struct));
    tree_depth_in->shader_file = String("../code/tree_depth.spv");
    
    tree_depth_in->resource_count = 2;
    
    tree_depth_in->resources = (resource_record *)calloc(tree_depth_in->resource_count, sizeof(resource_record));
    tree_depth_in->resources[0].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    tree_depth_in->resources[0].buffer = tree_buffer;
    tree_depth_in->resources[0].memory = tree_memory;
    tree_depth_in->resources[1].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    tree_depth_in->resources[1].buffer = tree_depth_buffer;
    tree_depth_in->resources[1].memory = tree_depth_memory;
    
    tree_depth_in->pcr_size = 1;
    tree_depth_in->pcr_data = (u32 *)calloc(tree_depth_in->pcr_size, sizeof(u32));
    tree_depth_in->pcr_data[0] = worksize;
    
    out_struct *tree_depth_out = CreateComputePipeline(tree_depth_in);
    
    
    
    vkUpdateDescriptorSets(vk.device, tree_depth_out->descwrite_count, tree_depth_out->descwrites, 0, NULL);
    
    vkBeginCommandBuffer(vk.commandbuffer, &vk.commandbuffer_bi);
    vkCmdBindDescriptorSets(vk.commandbuffer, pipeline_bindpoint, tree_depth_out->pipe_layout, 0, 1, &tree_depth_out->pipe_dset, 0, NULL);
    vkCmdPushConstants(vk.commandbuffer, tree_depth_out->pipe_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, tree_depth_in->pcr_size * sizeof(u32), tree_depth_in->pcr_data);
    vkCmdBindPipeline(vk.commandbuffer, pipeline_bindpoint, tree_depth_out->pipe);
    vkCmdDispatch(vk.commandbuffer, blockcount, 1, 1);
    vkEndCommandBuffer(vk.commandbuffer);
    
    vkQueueSubmit(vk.queue, 1, &compute_si, vk.fence);
    vkWaitForFences(vk.device, 1, &vk.fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vk.device, 1, &vk.fence);
    
    
    
    u32 *tree_depth_array = (u32 *)calloc(worksize, sizeof(u32));
    void *tree_depth_mapptr;
    vkMapMemory(vk.device, tree_depth_memory, 0, VK_WHOLE_SIZE, 0, &tree_depth_mapptr);
    memcpy(tree_depth_array, tree_depth_mapptr, worksize * sizeof(u32));
    vkUnmapMemory(vk.device, tree_depth_memory);
    
    u32 depth_levels[32] = {};
    ODS("Depth values of nodes: \n");
    for(u32 i = 0; i < worksize; i++)
    {
        ODS("%5d %5d \n", i, tree_depth_array[i]);
        depth_levels[tree_depth_array[i]]++;
    }
    
    ODS("Depth distribution: \n");
    for(u32 i = 0; i < 32; i++)
    {
        ODS("%2d : %5d\n", i, depth_levels[i]);
    }
    ODS("\n");
    
    
    // --- trace node to check if the depth distribution is correct
    u32 node_start = 14345;
    s32 parent = 0;
    u32 depth = 0;
    u32 node = node_start;
#if 1
    while(tree_check[node].parent != -1)
    {
        ODS("%d -> ", node);
        node = tree_check[node].parent;
        depth++;
    }
    ODS("%d \n", node);
#endif
#if 1
    while(true)
    {
        if(tree_data[node].parent != -1)
        {
            depth++;
            node = tree_data[node].parent;
        }
        else
        {
            ODS("Node %5d has depth %d \n", node_start, depth);
            break;
        }
    }
#endif
    //ODS("0 \n");
    //ODS("Node %5d has depth %d \n", node_start, depth);
    
    // according to the depth levels, make an array of arrays
    u32 **depth_arrays = (u32 **)calloc(32, sizeof(u32 *));
    for(u32 i = 0; i < 32; i++)
    {
        if(depth_levels[i] > 0)
            depth_arrays[i] = (u32 *)calloc(depth_levels[i], sizeof(u32));
    }
    
    u32 depth_counters[32] = {};
    for(u32 i = 0; i < worksize-1; i++)
    {
        u32 current_depth_index = tree_depth_array[i];
        depth_arrays[current_depth_index][depth_counters[current_depth_index]++] = i;
    }
    
    
    
    
    
    // Calculate the BVH on CPU to ensure GPU computation results
    
    //AABB *bvh_cpu = (AABB *)calloc(2*worksize-1, sizeof(AABB));
    //AABB *bvh_gpu = (AABB *)calloc(2*worksize-1, sizeof(AABB));
    
    AABB_padded *bvh_cpu = (AABB_padded *)calloc(2*worksize-1, sizeof(AABB_padded));
    AABB_padded *bvh_gpu = (AABB_padded *)calloc(2*worksize-1, sizeof(AABB_padded));
    
    
    
    // - write leaf boxes (N)
    u32 ipt = 3;  // indexes per triangle
    u32 fpv = model.floats_per_vertex;
    // this indexing convention is shite :/
    u32 leaves_begin = worksize-1;
    u32 leaves_end   = 2*worksize-1;
    
    r32 v_min = 10000.0f;
    r32 vx_min = v_min, vy_min = v_min, vz_min = v_min;
    r32 v_max = -10000.0f;
    r32 vx_max = v_max, vy_max = v_max, vz_max = v_max;
    
    for(u32 i = leaves_begin; i < leaves_end; i++)
    {
        u32 index = i-(worksize-1);
        
        // indexes
        u32 i0 = model.indices[ipt*index];
        u32 i1 = model.indices[ipt*index+1];
        u32 i2 = model.indices[ipt*index+2];
        
        // vertexes
        r32 v0_x = model.vertices[fpv*i0];
        r32 v0_y = model.vertices[fpv*i0+1];
        r32 v0_z = model.vertices[fpv*i0+2];
        
        r32 v1_x = model.vertices[fpv*i1];
        r32 v1_y = model.vertices[fpv*i1+1];
        r32 v1_z = model.vertices[fpv*i1+2];
        
        r32 v2_x = model.vertices[fpv*i2];
        r32 v2_y = model.vertices[fpv*i2+1];
        r32 v2_z = model.vertices[fpv*i2+2];
        
        if(vx_max < v0_x) vx_max = v0_x;
        if(vy_max < v0_y) vy_max = v0_y;
        if(vz_max < v0_z) vz_max = v0_z;
        if(vx_max < v1_x) vx_max = v1_x;
        if(vy_max < v1_y) vy_max = v1_y;
        if(vz_max < v1_z) vz_max = v1_z;
        if(vx_max < v2_x) vx_max = v2_x;
        if(vy_max < v2_y) vy_max = v2_y;
        if(vz_max < v2_z) vz_max = v2_z;
        
        if(vx_min > v0_x) vx_min = v0_x;
        if(vy_min > v0_y) vy_min = v0_y;
        if(vz_min > v0_z) vz_min = v0_z;
        if(vx_min > v1_x) vx_min = v1_x;
        if(vy_min > v1_y) vy_min = v1_y;
        if(vz_min > v1_z) vz_min = v1_z;
        if(vx_min > v2_x) vx_min = v2_x;
        if(vy_min > v2_y) vy_min = v2_y;
        if(vz_min > v2_z) vz_min = v2_z;
        
        // AABB
        r32 lower_x = min(min(v0_x, v1_x), v2_x);
        r32 lower_y = min(min(v0_y, v1_y), v2_y);
        r32 lower_z = min(min(v0_z, v1_z), v2_z);
        
        r32 upper_x = max(max(v0_x, v1_x), v2_x);
        r32 upper_y = max(max(v0_y, v1_y), v2_y);
        r32 upper_z = max(max(v0_z, v1_z), v2_z);
        
        bvh_cpu[i].lower.x = lower_x;
        bvh_cpu[i].lower.y = lower_y;
        bvh_cpu[i].lower.z = lower_z;
        
        bvh_cpu[i].upper.x = upper_x;
        bvh_cpu[i].upper.y = upper_y;
        bvh_cpu[i].upper.z = upper_z;
        
        bvh_gpu[i].lower.x = lower_x;
        bvh_gpu[i].lower.y = lower_y;
        bvh_gpu[i].lower.z = lower_z;
        
        bvh_gpu[i].upper.x = upper_x;
        bvh_gpu[i].upper.y = upper_y;
        bvh_gpu[i].upper.z = upper_z;
    }
    // not to mention this ^ should be a slender function :'/
    // I think moving all the typedef structs outside(where I used to do them) will enable all sorts of good code compression.
    
    // - write node boxes
    // walk every level
    for(s32 i = 31; i >= 0; i--)
    {
        if(depth_levels[i] > 0)
        {
            ODS("Processing level %d \n", i);
            for(u32 j = 0; j < depth_levels[i]; j++)
            {
                u32 node = depth_arrays[i][j];
                u32 left  = tree_check[node].left;
                u32 right = tree_check[node].right;
                
#if 0            
                ODS("Box of left(%5d): \n", left);
                ODS("X: [ %+-7.3f | %+-7.3f ]\n", bvh_cpu[left].lower.x, bvh_cpu[left].upper.x);
                ODS("Y: [ %+-7.3f | %+-7.3f ]\n", bvh_cpu[left].lower.y, bvh_cpu[left].upper.y);
                ODS("Z: [ %+-7.3f | %+-7.3f ]\n", bvh_cpu[left].lower.z, bvh_cpu[left].upper.z);
                
                ODS("Box of right(%5d): \n", right);
                ODS("X: [ %+-7.3f | %+-7.3f ]\n", bvh_cpu[right].lower.x, bvh_cpu[right].upper.x);
                ODS("Y: [ %+-7.3f | %+-7.3f ]\n", bvh_cpu[right].lower.y, bvh_cpu[right].upper.y);
                ODS("Z: [ %+-7.3f | %+-7.3f ]\n", bvh_cpu[right].lower.z, bvh_cpu[right].upper.z);
#endif
                bvh_cpu[node].lower.x = min(bvh_cpu[left].lower.x, bvh_cpu[right].lower.x);
                bvh_cpu[node].lower.y = min(bvh_cpu[left].lower.y, bvh_cpu[right].lower.y);
                bvh_cpu[node].lower.z = min(bvh_cpu[left].lower.z, bvh_cpu[right].lower.z);
                
                bvh_cpu[node].upper.x = max(bvh_cpu[left].upper.x, bvh_cpu[right].upper.x);
                bvh_cpu[node].upper.y = max(bvh_cpu[left].upper.y, bvh_cpu[right].upper.y);
                bvh_cpu[node].upper.z = max(bvh_cpu[left].upper.z, bvh_cpu[right].upper.z);
#if 0                
                ODS("Resulting box(node %5d) \n", node);
                ODS("X: [ %+-7.3f | %+-7.3f ]\n", bvh_cpu[node].lower.x, bvh_cpu[node].upper.x);
                ODS("Y: [ %+-7.3f | %+-7.3f ]\n", bvh_cpu[node].lower.y, bvh_cpu[node].upper.y);
                ODS("Z: [ %+-7.3f | %+-7.3f ]\n", bvh_cpu[node].lower.z, bvh_cpu[node].upper.z);
                ODS("\n");
#endif
            }
            //exit(0);
        }
    }
    
    ODS("Global box: \n");
    ODS("X: [ %+-7.3f | %+-7.3f ]\n", vx_min, vx_max);
    ODS("Y: [ %+-7.3f | %+-7.3f ]\n", vy_min, vy_max);
    ODS("Z: [ %+-7.3f | %+-7.3f ]\n", vz_min, vz_max);
    ODS("\n");
    
#if 0
    ODS("BVH: \n");
    for(u32 i = 0; i < worksize-1; i++)
    {
        ODS("Node %5d: \n", i);
        ODS("X: [ %+-7.3f | %+-7.3f ]\n", bvh_cpu[i].lower.x, bvh_cpu[i].upper.x);
        ODS("Y: [ %+-7.3f | %+-7.3f ]\n", bvh_cpu[i].lower.y, bvh_cpu[i].upper.y);
        ODS("Z: [ %+-7.3f | %+-7.3f ]\n", bvh_cpu[i].lower.z, bvh_cpu[i].upper.z);
        ODS("\n");
    }
    ODS("\n");
#endif
    
    
    
    // --- walk the tree, calculate bounding boxes for every node
    
    // Inputs:
    // - atomic counters array
    // - binary tree data
    // - bvh operating space with leaf boxes prefilled
    
    VkDeviceMemory atomic_counters_memory;
    VkBuffer atomic_counters_buffer = CreateBuffer((worksize-1) * sizeof(u32), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                   vk.device, gpu_memprops,
                                                   &atomic_counters_memory);
    
    VkDeviceMemory bvh_memory;
    VkBuffer bvh_buffer = CreateBuffer((2*worksize-1) * sizeof(AABB_padded), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                       vk.device, gpu_memprops,
                                       &bvh_memory);
    
    void *bvh_mapptr;
    vkMapMemory(vk.device, bvh_memory, 0, VK_WHOLE_SIZE, 0, &bvh_mapptr);
    memcpy(bvh_mapptr, bvh_gpu, (2*worksize-1) * sizeof(AABB_padded));
    
    
    in_struct *bvh_in = (in_struct *)calloc(1, sizeof(in_struct));
    bvh_in->shader_file = String("../code/bvh.spv");
    
    bvh_in->resource_count = 3;
    
    bvh_in->resources = (resource_record *)calloc(bvh_in->resource_count, sizeof(resource_record));
    bvh_in->resources[0].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bvh_in->resources[0].buffer = atomic_counters_buffer;
    bvh_in->resources[0].memory = atomic_counters_memory;
    bvh_in->resources[1].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bvh_in->resources[1].buffer = tree_buffer;
    bvh_in->resources[1].memory = tree_memory;
    bvh_in->resources[2].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bvh_in->resources[2].buffer = bvh_buffer;
    bvh_in->resources[2].memory = bvh_memory;
    
    
    bvh_in->pcr_size = 1;
    bvh_in->pcr_data = (u32 *)calloc(bvh_in->pcr_size, sizeof(u32));
    bvh_in->pcr_data[0] = worksize;
    
    out_struct *bvh_out = CreateComputePipeline(bvh_in);
    
    
    
    vkUpdateDescriptorSets(vk.device, bvh_out->descwrite_count, bvh_out->descwrites, 0, NULL);
    
    vkBeginCommandBuffer(vk.commandbuffer, &vk.commandbuffer_bi);
    vkCmdBindDescriptorSets(vk.commandbuffer, pipeline_bindpoint, bvh_out->pipe_layout, 0, 1, &bvh_out->pipe_dset, 0, NULL);
    vkCmdPushConstants(vk.commandbuffer, bvh_out->pipe_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, bvh_in->pcr_size * sizeof(u32), bvh_in->pcr_data);
    vkCmdBindPipeline(vk.commandbuffer, pipeline_bindpoint, bvh_out->pipe);
    vkCmdDispatch(vk.commandbuffer, blockcount, 1, 1);
    vkEndCommandBuffer(vk.commandbuffer);
    
    vkQueueSubmit(vk.queue, 1, &compute_si, vk.fence);
    vkWaitForFences(vk.device, 1, &vk.fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vk.device, 1, &vk.fence);
    
    
#if 0
    s32 *atomic_counters_data = (s32 *)calloc(worksize-1, sizeof(s32));
    
    void *atomic_counters_mapptr;
    vkMapMemory(vk.device, atomic_counters_memory, 0, VK_WHOLE_SIZE, 0, &atomic_counters_mapptr);
    memcpy(atomic_counters_data, atomic_counters_mapptr, (worksize-1)*sizeof(u32));
    vkUnmapMemory(vk.device, atomic_counters_memory);
#endif
    
    
    memcpy(bvh_gpu, bvh_mapptr, (2*worksize-1)*sizeof(AABB_padded));
    
    
    // check bvh_cpu and bvh_gpu for equality
    correct = true;
    for(u32 i = 0; i < (worksize-1); i++)
    {
        if((bvh_cpu[i].lower.x != bvh_gpu[i].lower.x) ||
           (bvh_cpu[i].lower.y != bvh_gpu[i].lower.y) ||
           (bvh_cpu[i].lower.z != bvh_gpu[i].lower.z) ||
           (bvh_cpu[i].upper.x != bvh_gpu[i].upper.x) ||
           (bvh_cpu[i].upper.y != bvh_gpu[i].upper.y) ||
           (bvh_cpu[i].upper.z != bvh_gpu[i].upper.z))
            correct = false;
    }
    vkUnmapMemory(vk.device, bvh_memory);
    
    ODS("BVH verification: %s \n", correct ? "PASSED" : "FAILED");
    
    
    
    // fool with compaction later? I wonder though if it's not compact already.
    
    
    
    
    // What's left to do now...
    // - launch rays. From every pixel, or rather, from a camera a ray is shot at every pixel.
    //    It hits boxes, checks for collision and then looks at the children of the box.
    //    If 
    
    // inputs and outputs- I should say, resources:
    // - tree
    // - boxes
    // - index and vertex data
    // - image
    
    
    // experiment with ray-box here
    vertex3 origin;
    origin.x = 0.0f;
    origin.y = 0.0f;
    origin.z = -10.0f;
    
    vertex3 direction;
    direction.x = 0.0f;
    direction.y = 0.0f;
    direction.z = 1.0f;
    
    ray r;
    r.o.x = origin.x;
    r.o.y = origin.y;
    r.o.z = origin.z;
    r.d.x = direction.x;
    r.d.y = direction.y;
    r.d.z = direction.z;
    
    r32 t;
    bool intersection = RayBox(r, bvh_gpu[0], &t);
    
    r32 inter_x = r.o.x + t * r.d.x;
    r32 inter_y = r.o.y + t * r.d.y;
    r32 inter_z = r.o.z + t * r.d.z;
    
    if(intersection)
    {
        ODS("Intersection with the global aabb at (%7.4f,%7.4f,%7.4f ), t=%7.4f \n", inter_x,inter_y,inter_z, t);
    }
    else
    {
        ODS("No intersection with the global aabb \n");
    }
    
    
    
    
    
    in_struct_v2 *ray_in = (in_struct_v2 *)calloc(1, sizeof(in_struct_v2));
    ray_in->shader_file = String("../code/raytrace.spv");
    
    ray_in->resource_count = 4;
    
    ray_in->resources = (resource_record *)calloc(ray_in->resource_count, sizeof(resource_record));
    ray_in->resources[0].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    ray_in->resources[0].memory = tree_memory;
    ray_in->resources[0].buffer = tree_buffer;
    ray_in->resources[1].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    ray_in->resources[1].memory = bvh_memory;
    ray_in->resources[1].buffer = bvh_buffer;
    ray_in->resources[2].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    ray_in->resources[2].memory = model_index_memory;
    ray_in->resources[2].buffer = model_index_buffer;
    ray_in->resources[3].type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    ray_in->resources[3].memory = model_vertex_memory;
    ray_in->resources[3].buffer = model_vertex_buffer;
    
    ray_in->imageresource_count = 1;
    ray_in->imageresources = (imageresource_record *)calloc(ray_in->imageresource_count, sizeof(imageresource_record));
    ray_in->imageresources[0].type      = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    ray_in->imageresources[0].imageview = computed_imageview;
    
    ray_in->pcr_size = 1;
    ray_in->pcr_data = (u32 *)calloc(bvh_in->pcr_size, sizeof(u32));
    ray_in->pcr_data[0] = worksize-1;
    
    out_struct *ray_out = CreateComputePipeline_v2(ray_in);
    
    
    
    
    
    u32 xdim = ceil(r32(app.window_width)  / 32.0f);
    u32 ydim = ceil(r32(app.window_height) / 32.0f);
    
    vkUpdateDescriptorSets(vk.device, ray_out->descwrite_count, ray_out->descwrites, 0, NULL);
    
    vkBeginCommandBuffer(vk.commandbuffer, &vk.commandbuffer_bi);
    vkCmdBindDescriptorSets(vk.commandbuffer, pipeline_bindpoint, ray_out->pipe_layout, 0, 1, &ray_out->pipe_dset, 0, NULL);
    vkCmdBindPipeline(vk.commandbuffer, pipeline_bindpoint, ray_out->pipe);
    vkCmdDispatch(vk.commandbuffer, xdim, ydim, 1);
    vkEndCommandBuffer(vk.commandbuffer);
    
    vkQueueSubmit(vk.queue, 1, &compute_si, vk.fence);
    vkWaitForFences(vk.device, 1, &vk.fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vk.device, 1, &vk.fence);
    
    
    
    // ---
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
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
    vertex_binding.stride    = 5 * sizeof(float);
    vertex_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    VkVertexInputAttributeDescription vert_description = {};
    vert_description.location = 0;
    vert_description.binding  = vertex_binding.binding;
    vert_description.format   = VK_FORMAT_R32G32B32_SFLOAT;
    vert_description.offset   = 0;
    
    VkVertexInputAttributeDescription uv_description = {};
    uv_description.location = 1;
    uv_description.binding  = vertex_binding.binding;
    uv_description.format   = VK_FORMAT_R32G32_SFLOAT;
    uv_description.offset   = 3 * sizeof(float);
    
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
    
    //VkPipelineLayout pipelinelayout;
    vkCreatePipelineLayout(vk.device, &pipelinelayout_ci, NULL, &vk.pipelinelayout);
    
    
    
    VkAttachmentReference subpass_attachmentreference = {};
    subpass_attachmentreference.attachment = 0;
    subpass_attachmentreference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass = {};
    subpass.flags                   = 0;
    subpass.pipelineBindPoint       = vk.bindpoint_graphics;
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
    
    //VkRenderPass renderpass;
    vkCreateRenderPass(vk.device, &renderpass_ci, NULL, &vk.renderpass);
    
    
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
    graphics_pipeline_ci.layout              = vk.pipelinelayout;
    graphics_pipeline_ci.renderPass          = vk.renderpass;
    graphics_pipeline_ci.subpass             = 0;
    
    //VkPipeline graphics_pipeline;
    vkCreateGraphicsPipelines(vk.device, NULL, 1, &graphics_pipeline_ci, NULL, &vk.graphics_pipeline);
    
    
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
    
    //VkDescriptorSet descriptorset;
    vkAllocateDescriptorSets(vk.device, &descriptorset_ai, &vk.descriptorset);
    
    
    
    
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.image = computed_image;
    
    vkBeginCommandBuffer(vk.commandbuffer, &vk.commandbuffer_bi);
    vkCmdPipelineBarrier(vk.commandbuffer,
                         VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                         0,
                         0, NULL,
                         0, NULL,
                         1, &barrier);
    vkEndCommandBuffer(vk.commandbuffer);
    vkQueueSubmit(vk.queue, 1, &transit_si, vk.fence);
    
    vkWaitForFences(vk.device, 1, &vk.fence, VK_TRUE, UINT64_MAX);
    vkResetFences(vk.device, 1, &vk.fence);
    
    
    
    
    VkSampler sampler;
    CreateSampler(&sampler);
    
    VkDescriptorImageInfo computed_image_info = {};
    computed_image_info.sampler     = sampler;
    computed_image_info.imageView   = computed_imageview;
    computed_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    VkWriteDescriptorSet descriptor_write = {};
    descriptor_write.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.pNext            = NULL;
    descriptor_write.dstSet           = vk.descriptorset;
    descriptor_write.dstBinding       = 0;
    descriptor_write.dstArrayElement  = 0;
    descriptor_write.descriptorCount  = 1;
    descriptor_write.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_write.pImageInfo       = &computed_image_info;
    descriptor_write.pBufferInfo      = NULL;
    descriptor_write.pTexelBufferView = NULL;
    vkUpdateDescriptorSets(vk.device, 1, &descriptor_write, 0, NULL);
    
    
    
    //VkSemaphoreCreateInfo sem_ci = {};
    vk.sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vk.sem_ci.pNext = NULL;
    vk.sem_ci.flags = 0;
    
    //VkSemaphore semaphore_acquired;
    vkCreateSemaphore(vk.device, &vk.sem_ci, NULL, &vk.semaphore_acquired);
    
    
    
#if 0    
    u32 present_index = 0;
    result = vkAcquireNextImageKHR(vk.device, vk.swapchain, UINT64_MAX, semaphore_acquired, NULL, &present_index);
    ODS_RES("Acquisition result: %s\n");
#endif
    
    vk.framebuffers = (VkFramebuffer *)calloc(2, sizeof(VkFramebuffer));
    
    
#if 0
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
#endif
    
    
    
    
    struct vertex
    {
        r32 x, y, z, u, v;
    };
    vertex v0 = { -1.0f, -1.0f, 0.0f, 0.0f, 0.0f };
    vertex v1 = { -1.0f,  1.0f, 0.0f, 0.0f, 1.0f };
    vertex v2 = {  1.0f, -1.0f, 0.0f, 1.0f, 0.0f };
    vertex v3 = {  1.0f,  1.0f, 0.0f, 1.0f, 1.0f };
    vertex quad[] = { v0, v1, v2, v3 };
    
    VkDeviceMemory vertex_memory;
    //VkBuffer 
    rendering.vertex_buffer = CreateBuffer(4 * 5 * sizeof(float),  // 4 * 5 *
                                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                           vk.device,
                                           gpu_memprops,
                                           &vertex_memory);
    
    void *vertex_mapptr;
    vkMapMemory(vk.device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &vertex_mapptr);
    memcpy(vertex_mapptr, quad, 5 * 4 * sizeof(float));  // * 5 * 4
    vkUnmapMemory(vk.device, vertex_memory);
    
    
    u32 indexes[] = { 0, 1, 2, 1, 3, 2 };
    VkDeviceMemory index_memory;
    //VkBuffer
    rendering.index_buffer = CreateBuffer(6 * sizeof(u32),  // 6 *
                                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                          vk.device,
                                          gpu_memprops,
                                          &index_memory);
    
    void *index_mapptr;
    vkMapMemory(vk.device, index_memory, 0, VK_WHOLE_SIZE, 0, &index_mapptr);
    memcpy(index_mapptr, indexes, sizeof(u32) * sizeof(indexes));
    vkUnmapMemory(vk.device, index_memory);
    
    
#if 0    
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
    
    vkBeginCommandBuffer(vk.commandbuffer, &vk.commandbuffer_bi);
    vkCmdBindVertexBuffers(vk.commandbuffer, 0, 1, &vertex_buffer, &offset);
    vkCmdBindIndexBuffer(vk.commandbuffer, index_buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(vk.commandbuffer, bindpoint_graphics, vk.graphice_pipeline);
    vkCmdBindDescriptorSets(vk.commandbuffer, bindpoint_graphics, vk.pipelinelayout, 0, 1, &vk.descriptorset, 0, NULL);
    vkCmdBeginRenderPass(vk.commandbuffer, &renderpass_bi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdDrawIndexed(vk.commandbuffer, 6, 1, 0, 0, 0);
    vkCmdEndRenderPass(vk.commandbuffer);
    vkEndCommandBuffer(vk.commandbuffer);
    
    
    VkFence fence_rendered;
    vkCreateFence(vk.device, &fence_ci, NULL, &fence_rendered);
    
    VkPipelineStageFlags waitmask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    
    VkSubmitInfo si = {};
    si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.pNext                = NULL;
    si.pWaitDstStageMask    = &waitmask;
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &vk.commandbuffer;
    vkQueueSubmit(vk.queue, 1, &si, fence_rendered);
    result = vkWaitForFences(vk.device, 1, &fence_rendered, VK_TRUE, UINT64_MAX);
    ODS_RES("Render wait result: %s\n");
    vkResetFences(vk.device, 1, &vk.fence);
    
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
    ODS_RES("Present result: %s\n");
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
    
    free(revenum_buffer);
    
    return msg.wParam;
}