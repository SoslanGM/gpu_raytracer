
char *OpenFile_v0(char *filename, int *size, bool removeslashr, bool debugprint)
{
    FILE *f = fopen(filename, "rb");
    if(!f)
    {
        printf("Couldn't open %s for reading\n", filename);
        exit(0);
    }
    
    fseek(f, 0, SEEK_END);
    int length = ftell(f);
    rewind(f);
    
    if(debugprint)
        printf("> OpenFile: ftell says length = %d\n", length);
    
    int alloc = length + 1;
    char *buffer = (char *)malloc(sizeof(char) * alloc);
    fread(buffer, length, 1, f);
    buffer[length] = terminator;
    
    fclose(f);
    
    if(removeslashr)
    {
        int lengthsansr = 0;
        // count length without /r
        char *reader = buffer;
        while(reader[0] != terminator)
        {
            if(reader[0] != carriageret)
                lengthsansr++;
            reader++;
        }
        
        int allocsansr = lengthsansr + 1;
        char *buffersansr = (char *)malloc(sizeof(char) * allocsansr);
        
        reader = buffer;
        char *writer = buffersansr;
        while(reader[0] != terminator)
        {
            if(reader[0] != carriageret)
                *writer++ = *reader;
            reader++;
        }
        buffersansr[lengthsansr] = terminator;
        
        free(buffer);
        
        if(size != NULL)
            *size = lengthsansr;
        return buffersansr;
    }
    
    if(size != NULL)
        *size = length;
    return buffer;
}
char *OpenFile_v0(char *filename, int *size)
{
    bool killslashrburnitdown = true;
    bool debugprint = false;
    
    return OpenFile_v0(filename, size, killslashrburnitdown, debugprint);
}
string *OpenFile(char *filename, int *size, bool removeslashr, bool debugprint)
{
    string *result = (string *)malloc(sizeof(string));
    
    result->ptr = OpenFile_v0(filename, size, removeslashr, debugprint);
    result->length = *size;
    
    return result;
}
string *OpenFile(char *filename, int *size)
{
    bool removeslashr = true;
    bool debugprint = false;
    
    string *result = (string *)malloc(sizeof(string));
    
    result->ptr = OpenFile_v0(filename, size, removeslashr, debugprint);
    result->length = *size;
    
    return result;
}
string *OpenFile(char *filename)
{
    bool removeslashr = true;
    bool debugprint = false;
    
    string *result = (string *)malloc(sizeof(string));
    
    int size = 0;
    result->ptr = OpenFile_v0(filename, &size, removeslashr, debugprint);
    result->length = size;
    
    return result;
}