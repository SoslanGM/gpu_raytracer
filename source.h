
// console - 0
// window  - 1
#define MODE 1

#if MODE == 1
char ods_buf[2048];
#define ODS(...) \
_snprintf_s(ods_buf, sizeof(ods_buf), __VA_ARGS__); \
OutputDebugStringA(ods_buf);
#elif MODE == 0
char ods_buf[2048];
#define ODS(...) \
_snprintf_s(ods_buf, sizeof(ods_buf), __VA_ARGS__); \
printf(ods_buf);
#endif

#define ODS_RES(message) ODS(message, RevEnum(vk_enums.result_enum, result));



void PadDebugOutput(u32 padsize)
{
    for(u32 i = 0; i < padsize; i++)
        OutputDebugStringA("\n");
}

void Assert(VkResult result, char *msg)
{
    if(result != VK_SUCCESS)
    {
        ODS("Result code: %d \n", result);
        ODS("ASSERT: %s \n", msg);
        
        int *base = 0;
        *base = 1;
    }
}
void Assert(bool flag, char *msg = "")
{
    if(!flag)
    {
        ODS("ASSERT: %s \n", msg);
        
        int *base = 0;
        *base = 1;
    }
}


typedef struct
{
    u32 length;
    char *ptr;
} string;


struct
{
    string *result_enum;
    string *format_enum;
    string *colorspace_enum;
    string *presentmode_enum;
    string *imagelayout_enum;
    string *pipeflags_enum;
} vk_enums;


struct
{
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    VkPhysicalDevice gpu;
    VkSurfaceKHR surface;
    VkDevice device;
    u32 queue_family_index;
    VkQueue queue;
    
    VkPhysicalDeviceMemoryProperties gpu_memprops;
    
    VkPipeline compipe;
    VkPipelineLayout pipelayout;
    VkDescriptorSet dsl;
    VkDeviceMemory memory;
    
    VkCommandBuffer cbuffer;
} vk;

