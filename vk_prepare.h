
char *NumToString(int number, bool debugprint)
{
    int sign = (number < 0) ? -1 : 1;
    if(debugprint)
        printf("Is this %d value positive? %s\n", number, number >= 1 ? "YES" : "NO");
    
    number *= sign;
    int copynum = number;
    int length = 0;
    while(copynum >= 1)
    {
        copynum /= 10;
        length++;
    }
    
    u32 signlength;
    if(sign == 1)
        signlength = 0;
    else
        signlength = 1;
    
    if(length == 0)
        length = 1;
    char *str = (char *)malloc(sizeof(char) * (signlength + length + 1));
    str[length] = terminator;
    for(int i = 0; i < length; i++)
    {
        if(debugprint)
        {
            printf(" --- \n");
            printf("number: %d\n", number);
            printf("number %% 10: %d\n", number % 10);
            printf("symbol: %c\n", (number % 10) + '0');
            printf(" --- \n");
        }
        str[i] = number % 10 + '0';
        number /= 10;
    }
    
    // reverse value
    char *result = (char *)malloc(sizeof(char) * (signlength + length + 1));
    if(sign)
        result[0] = '-';
    
    for(int i = signlength, j = length-1; i < length+signlength; i++, j--)
        result[i] = str[j];
    
    free(str);
    
    result[length+signlength] = terminator;
    
    return result;
}

char *FindLine(char *buffer, int *length, char *searchterm)
{
    char *prelimstart = strstr(buffer, searchterm);
    
    if(prelimstart == NULL)
        return NULL;
    
    char *start;
    char *end;
    
    start = prelimstart;
    while(start != buffer)
    {
        start--;
        if(*start == newline)
        {
            start++;
            break;
        }
    }
    
    end = prelimstart;
    while(*end != terminator)
    {
        end++;
        if(*end == newline)
            break;
    }
    
    *length = (int)(end - start);
    return start;
}

string *RetrieveEnum(char *filename, char *enum_name)
{
    string *buffer = OpenFile(filename);
    
    char *enum_prefix = "typedef enum ";
    char *enum_searchterm = (char *)calloc(strlen(enum_prefix) + strlen(enum_name) + 1, sizeof(char));
    strncpy(enum_searchterm, enum_prefix, strlen(enum_prefix));
    strncat(enum_searchterm, enum_name,   strlen(enum_name));
    
    char *pos = strstr(buffer->ptr, enum_searchterm);
    free(enum_searchterm);
    if(pos == NULL)
    {
        ODS("Wrong enum name!\n");
        ODS("You were searching for this: %s\n", enum_name);
        exit(0);
    }
    
    free(buffer);
    
    char *begin = strchr(pos,   braceopen)  + 2;  // scroll forward a brace and a newline
    char *end   = strchr(begin, braceclose) - 1;  // scroll back a newline
    
    int length = (int)(end - begin);
    
    char *enumbuffer = (char *)malloc(sizeof(char) * (length + 1));
    strncpy(enumbuffer, begin, length);
    
    string *result = (string *)malloc(sizeof(string));
    result->ptr   = enumbuffer;
    result->length = length;
    
    return result;
}
char *RetrieveVkEnumString(string *vkenumbuffer, u32 enumvalue, bool debugprint)
{
    bool deb = false;
    
    char *valuestring = NumToString(enumvalue, debugprint);
    if(debugprint)
        printf("value: %s\n", valuestring);
    
    int length = 0;
    char *enumstring = FindLine(vkenumbuffer->ptr, &length, valuestring);
    
    free(valuestring);
    
    Tokenizer.At = enumstring;
    Token token = GetToken();
    
    if(token.length < 0)
    {
        ODS("Strange token length bug\n");
    }
    
    if(deb)
    {
        printf("token text:   %.*s\n", token.length, token.text);
        printf("token length: %d\n", token.length);
    }
    
    char *result = (char *)malloc(sizeof(char) * (token.length + 1));
    strncpy(result, token.text, token.length);
    result[token.length] = terminator;
    
    if(deb)
        printf("string length: %d\n", length);
    
    return result;
}
char *RevEnum(string *enum_buffer, s32 value, bool debugprint)
{
    return RetrieveVkEnumString(enum_buffer, value, debugprint);
}
char *RevEnum(string *enum_buffer, s32 value)
{
    bool debugprint = false;
    return RetrieveVkEnumString(enum_buffer, value, debugprint);
}



string *FindVulkanSDK()
{
    u32 MAX_LEN = 260;
    
    string *result = (string *)calloc(1, sizeof(string));
    result->length = MAX_LEN;
    result->ptr = (char *)calloc(MAX_LEN, sizeof(char));
    GetEnvironmentVariable("vulkan_sdk", result->ptr, MAX_LEN);
    
    return result;
}

void LoadVkEnums()
{
    string *vk_sdk_path = FindVulkanSDK();
    string *header_end = String("/Include/vulkan/vulkan_core.h");
    
    char *header_path = (char *)calloc(1024, sizeof(char));
    strncpy(header_path, vk_sdk_path->ptr, vk_sdk_path->length);
    strncat(header_path, header_end->ptr, header_end->length);
    header_path[vk_sdk_path->length+header_end->length] = terminator;
    
    vk_enums.result_enum      = RetrieveEnum(header_path, "VkResult");
    vk_enums.format_enum      = RetrieveEnum(header_path, "VkFormat");
    vk_enums.colorspace_enum  = RetrieveEnum(header_path, "VkColorSpaceKHR");
    vk_enums.presentmode_enum = RetrieveEnum(header_path, "VkPresentModeKHR");
    vk_enums.imagelayout_enum = RetrieveEnum(header_path, "VkImageLayout");
    vk_enums.pipeflags_enum   = RetrieveEnum(header_path, "VkPipelineStageFlagBits");
    
    free(header_path);
    free(header_end->ptr);  // Lol, string needs a special free function :D
    free(header_end);
    free(vk_sdk_path->ptr);
    free(vk_sdk_path);
}



void EnumerateGlobalExtensions()
{
    u32 global_propscount;
    result = vkEnumerateInstanceExtensionProperties(NULL, &global_propscount, NULL);
    
    char *rev = RevEnum(vk_enums.result_enum, result);
    ODS("Instance extension props(count): %s\n", rev);
    free(rev);
    
    VkExtensionProperties *global_props = (VkExtensionProperties *)malloc(sizeof(VkExtensionProperties) * global_propscount);
    result = vkEnumerateInstanceExtensionProperties(NULL, &global_propscount, global_props);
    
    char *rev1 = RevEnum(vk_enums.result_enum, result);
    ODS("Instance extension props(fill):  %s\n", rev1);
    free(rev1);
    
    
    ODS("\n> Instance-wide extensions: %d\n", global_propscount);
    for(u32 i = 0; i < global_propscount; i++)
    {
        u32 version = global_props[i].specVersion;
        u32 major = VK_VERSION_MAJOR(version);
        u32 minor = VK_VERSION_MINOR(version);
        u32 patch = VK_VERSION_PATCH(version);
        ODS("%2d - %-40s | %d.%d.%d\n", i, global_props[i].extensionName, major, minor, patch);
    }
    ODS("\n");
    
    free(global_props);
}

void EnumerateLayerExtensions()
{
    u32 layer_count = 0;
    result = vkEnumerateInstanceLayerProperties(&layer_count, NULL);
    
    char *rev = RevEnum(vk_enums.result_enum, result);
    ODS("Instance layer props(count): %s\n", rev);
    free(rev);
    
    VkLayerProperties *layer_props = (VkLayerProperties *)malloc(sizeof(VkLayerProperties) * layer_count);
    result = vkEnumerateInstanceLayerProperties(&layer_count, layer_props);
    
    char *rev1 = RevEnum(vk_enums.result_enum, result);
    ODS("Instance layer props(fill):  %s\n", rev1);
    free(rev1);
    
    ODS("\n> Instance layers: %d\n", layer_count);
    for(u32 i = 0; i < layer_count; i++)
    {
        u32 spec_version = layer_props[i].specVersion;
        u32 spec_major = VK_VERSION_MAJOR(spec_version);
        u32 spec_minor = VK_VERSION_MINOR(spec_version);
        u32 spec_patch = VK_VERSION_PATCH(spec_version);
        
        u32 impl_version = layer_props[i].implementationVersion;
        u32 impl_major = VK_VERSION_MAJOR(impl_version);
        u32 impl_minor = VK_VERSION_MINOR(impl_version);
        u32 impl_patch = VK_VERSION_PATCH(impl_version);
        
        ODS("%2d - %-40s | %d.%d.%-3d | %d.%d.%d | %.50s\n",
            i,
            layer_props[i].layerName,
            spec_major, spec_minor, spec_patch,
            impl_major, impl_minor, impl_patch,
            layer_props[i].description);
        
        u32 ext_count = 0;
        vkEnumerateInstanceExtensionProperties(layer_props[i].layerName, &ext_count, NULL);
        if(ext_count)
        {
            ODS(">> its extensions: %d\n", ext_count);
            
            VkExtensionProperties *ext_props = (VkExtensionProperties *)malloc(sizeof(VkExtensionProperties) * ext_count);
            vkEnumerateInstanceExtensionProperties(layer_props[i].layerName, &ext_count, ext_props);
            
            for(u32 i = 0; i < ext_count; i++)
            {
                u32 version = ext_props[i].specVersion;
                u32 major = VK_VERSION_MAJOR(version);
                u32 minor = VK_VERSION_MINOR(version);
                u32 patch = VK_VERSION_PATCH(version);
                ODS(" - %2d - %-37s | %d.%d.%d\n", i, ext_props[i].extensionName, major, minor, patch);
            }
            
            free(ext_props);
        }
    }
    ODS("\n");
    
    free(layer_props);
}
