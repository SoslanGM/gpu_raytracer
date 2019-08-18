#define DECLARE_FUNCTION_POINTER(Func) \
PFN_##Func Func = NULL;

#define LOAD_FUNCTION_POINTER_AND_ASSERT(Func) \
Func = (PFN_##Func) GetProcAddress (vulkanDLL, "" #Func ""); \
Assert(Func, "Unable to load function " #Func);

#define LOAD_EXTENSION_FUNCTION_POINTER_AND_ASSERT(Func, Inst) \
*(void **)&##Func = vkGetInstanceProcAddr(Inst, "" #Func); \
Assert(Func, "Unable to load extension function " #Func);


#define X(Func) DECLARE_FUNCTION_POINTER(Func)
#include "list_of_functions.h"
LIST_OF_FUNCTIONS
#include "list_of_ext_functions.h"
LIST_OF_EXT_FUNCTIONS
#undef X

// extraction of functions from the library and filling of function pointers
#define X(Func) LOAD_FUNCTION_POINTER_AND_ASSERT(Func)
void Vulkan_LoadInstanceFunctions()
{
    HMODULE vulkanDLL = LoadLibrary("vulkan-1.dll");
    Assert(vulkanDLL, "! Can't load Vulkan.\n");
    
    LIST_OF_FUNCTIONS
}
#undef LIST_OF_FUNCTIONS
#undef X

// loading of extension functions and filling of their pointers
#define X(Func) LOAD_EXTENSION_FUNCTION_POINTER_AND_ASSERT(Func, instance)
void Vulkan_LoadExtensionFunctions(VkInstance instance)
{
    LIST_OF_EXT_FUNCTIONS
}
#undef LIST_OF_EXT_FUNCTIONS
#undef X