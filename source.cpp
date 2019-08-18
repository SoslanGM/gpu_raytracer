
// SSSTANDARD
#include <stdio.h>
#include <stdlib.h>
#include <shlwapi.h>

// VULKANSSS
#define VK_USE_PLATFORM_WIN32_KHR
#define VK_NO_PROTOTYPES
#include "vulkan/vulkan.h"
#include "vulkan/vk_platform.h"

// PRECIOUSSS
#include "M:/types.h"
#include "source.h"
#include "vk_extract.h"
#include "M:/special_symbols.h"
#include "file_io.h"
#include "M:/tokenizer.cpp"
#include "vk_prepare.h"
//#include "vk_setup.h"

string *FindVulkanSDK()
{
    u32 MAX_LEN = 260;  // as per MSDN
    
    string *result = (string *)calloc(1, sizeof(string));
    result->length = MAX_LEN;
    result->ptr = (char *)calloc(MAX_LEN, sizeof(char));
    GetEnvironmentVariable("vulkan_sdk", result->ptr, MAX_LEN);
    
    return result;
}

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
    surface_ci.hinstance = NULL;
    surface_ci.hwnd      = NULL;
    
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


void GetComputePipeline()
{
    VkPipelineShaderStageCreateInfo stage_ci = {};
    stage_ci.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_ci.pNext               = NULL;
    stage_ci.flags               = 0;
    stage_ci.stage               = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_ci.module              = GetShaderModule("../code/shader_comp.spv");
    stage_ci.pName               = "main";
    stage_ci.pSpecializationInfo = NULL;
    
    
    VkDescriptorSetLayoutBinding binding = {};
    binding.binding            = 0;
    binding.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.descriptorCount    = 1;
    binding.stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
    binding.pImmutableSamplers = NULL;
    
    VkDescriptorSetLayoutCreateInfo dsl_ci = {};
    dsl_ci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsl_ci.pNext        = NULL;
    dsl_ci.flags        = 0;
    dsl_ci.bindingCount = 1;     // 1
    dsl_ci.pBindings    = &binding;  // binding
    VkDescriptorSetLayout dslayout;
    vkCreateDescriptorSetLayout(vk.device, &dsl_ci, NULL, &dslayout);
    
    VkDescriptorPoolSize dspool_size = {};
    dspool_size.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
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

#if 0
void RecordCommandBuffer(VkCommandBuffer cb)
{
    VkCommandBufferBeginInfo cb_bi = {};
    cb_bi.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cb_bi.pNext            = NULL;
    cb_bi.flags            = 0;
    cb_bi.pInheritanceInfo = NULL;
    
    vkBeginCommandBuffer(cb, &cb_bi);
    
    vkEndCommandBuffer(cb);
}
#endif

void main()
{
    string *vk_sdk_path = FindVulkanSDK();
    printf("Here's a Vulkan SDK: %.*s\n", vk_sdk_path->length, vk_sdk_path->ptr);
    LoadVkEnums(vk_sdk_path);
    
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
    printf("Here's a Vulkan instance: 0x%p\n", &vk.instance);
    Vulkan_LoadExtensionFunctions(vk.instance);
    
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
    printf("Here's a Vulkan device:   0x%p\n", &vk.device);
    
    GetComputePipeline();
    
    
    // prepare data
    CheckDeviceMemoryProperties();
    
#if 0
    u32 workgroup_size = 32;
    u32 int_byte_size = 4;
    u32 buffer_byte_size = workgroup_size * workgroup_size * int_byte_size;
#endif
    u32 buffer_byte_size = 32 * 4;
    VkBuffer buffer = CreateBuffer(buffer_byte_size,
                                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                   vk.device, vk.gpu_memprops, &vk.memory);
    
    // prepare command buffer
    VkCommandPool cpool; CreateCommandPool(&cpool);
    AllocateCommandBuffer(cpool, &vk.cbuffer);
    
    
    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = buffer;
    buffer_info.offset = 0;
    buffer_info.range  = VK_WHOLE_SIZE;
    
    VkWriteDescriptorSet write = {};
    write.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext            = NULL;
    write.dstSet           = vk.dsl;
    write.dstBinding       = 0;
    write.dstArrayElement  = 0;
    write.descriptorCount  = 1;
    write.descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.pImageInfo       = NULL;
    write.pBufferInfo      = &buffer_info;
    write.pTexelBufferView = NULL;
    vkUpdateDescriptorSets(vk.device, 1, &write, 0, NULL);
    
    VkCommandBufferBeginInfo cb_bi = {};
    cb_bi.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cb_bi.pNext            = NULL;
    cb_bi.flags            = 0;
    cb_bi.pInheritanceInfo = NULL;
    
    vkBeginCommandBuffer(vk.cbuffer, &cb_bi);
    vkCmdBindDescriptorSets(vk.cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vk.pipelayout, 0, 1, &vk.dsl, 0, NULL);
    vkCmdBindPipeline(vk.cbuffer, VK_PIPELINE_BIND_POINT_COMPUTE, vk.compipe);
    vkCmdDispatch(vk.cbuffer, 1, 1, 1);
    vkEndCommandBuffer(vk.cbuffer);
    
    
    // execute queue
    VkFenceCreateInfo fence_ci = {};
    fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_ci.pNext = NULL;
    fence_ci.flags = 0;
    VkFence fence;
    vkCreateFence(vk.device, &fence_ci, NULL, &fence);
    
    
    VkPipelineStageFlags flag = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    
    VkSubmitInfo si = {};
    si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.pNext                = NULL;
    si.waitSemaphoreCount   = 0;
    si.pWaitSemaphores      = NULL;
    si.pWaitDstStageMask    = &flag;
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &vk.cbuffer;
    si.signalSemaphoreCount = 0;
    si.pSignalSemaphores    = NULL;
    vkQueueSubmit(vk.queue, 1, &si, fence);
    vkWaitForFences(vk.device, 1, &fence, VK_TRUE, UINT64_MAX);
    
    // read results
    // - map the buffer to a piece of memory
    void *data;
    vkMapMemory(vk.device, vk.memory, 0, buffer_byte_size, 0, &data);
    u32 *values = (u32 *)data;
    // - read the memory
#if 0
    for(u32 i = 0; i < workgroup_size * 4; i++)
    {
        for(u32 j = 0; j < workgroup_size; j++)
            printf("%3d ", values[i]);
        printf("\n");
    }
#endif
    
#if 1
    for(u32 i = 0; i < 32; i++)
    {
        printf("%3d", values[i]);
        if((i+1) % 4 == 0)
            printf("\n");
    }
#endif
}