
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

// retired for now, since if makes freeing a temporary string harder.
//#define ODS_RES(message) ODS(message, RevEnum(vk_enums.result_enum, result));



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
string *String(u32 size)
{
    string *result = (string *)calloc(1, sizeof(string));
    result->length = size;
    result->ptr = (char *)calloc(size, sizeof(char));
    
    return result;
}
string *String(char *str)
{
    string *result = (string *)calloc(1, sizeof(string));
    result->length = strlen(str);
    result->ptr = (char *)calloc(result->length+1, sizeof(char));
    result->ptr[result->length] = terminator;
    strncpy(result->ptr, str, result->length);
    
    return result;
}

typedef struct
{
    u32 count;
    string **lines;
} text;
/*
    lines[0] = line0;
    lines[1] = line1;
    
    line0 = String(15);  <- pass size?
    // - or -
string *line0 = (string *)calloc(1, sizeof(string));
    line0.length = 15;
    line0.ptr = (char *)calloc(line0.length, sizeof(char));
    
strncpy(line0.ptr, "boiii", line0.length);

    line1 = String();
    line1.length = 30;
    line1.ptr = "This is right!";
    */

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
    
    VkExtent2D screenextent;
    
    VkFormat format;
    VkComponentMapping components;
    VkImageSubresourceRange color_sr;
    VkColorSpaceKHR color_space;
    VkPresentModeKHR present_mode;
    
    VkSwapchainKHR swapchain;
    VkImage *swapchain_images;
    VkImageView *swapchain_imageviews;
    VkFramebuffer *framebuffers;
    
    VkPhysicalDeviceMemoryProperties gpu_memprops;
    
    VkPipeline compipe;
    VkPipelineLayout pipelayout;
    VkDescriptorSet dsl;
    VkDeviceMemory memory;
    
    VkCommandBuffer cbuffer;
} vk;

struct
{
    u32 image_count = 2;
    
    
} data;

struct
{
    HINSTANCE instance;
    HWND window;
    u32 window_width;
    u32 window_height;
} app;

