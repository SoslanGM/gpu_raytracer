#define MS_GRANULARITY 1

typedef enum { SECONDS, MILLISECONDS, MICROSECONDS, NANOSECONDS } TIME_RES;

bool ms_granular = false;
u64 timer_frequency = 0;
u64 time_appstart = 0;

// - timer controls: once each per app run
void TimerSetup()
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    timer_frequency = freq.QuadPart;
    
    ms_granular = (timeBeginPeriod(MS_GRANULARITY) == TIMERR_NOERROR);
}

void TimerShutdown()
{
    if(ms_granular)
        timeEndPeriod(MS_GRANULARITY);
}

void TimerAppstart()
{
    LARGE_INTEGER L;
    QueryPerformanceCounter(&L);
    time_appstart = L.QuadPart;
}
// ---



u64 TimerRead()
{
    LARGE_INTEGER L;
    QueryPerformanceCounter(&L);
    
    u64 time = L.QuadPart;
    return time;
}

// This is for periodic stuff like tracking frame times.
r64 TimerElapsedFrom(u64 prev, TIME_RES res)
{
    LARGE_INTEGER L;
    QueryPerformanceCounter(&L);
    
    u64 mult = 0;
    if(res == SECONDS)
        mult = 1;           // s
    else if(res == MILLISECONDS)
        mult = 1000;        // ms
    else if(res == MICROSECONDS)
        mult = 1000000;     // us
    else if(res == NANOSECONDS) 
        mult = 1000000000;  // ns
    
    u64 curr = L.QuadPart;
    u64 diff = curr - prev;
    r64 result = (r64)(diff * mult) / (r64)timer_frequency;
    
    return result;
}


// - a bit weldy in terms of resolution and required format(microseconds)
string *TimerString(r64 t_us)
{
    r64 us_to_ms = 1000.0;
    r64 ms_to_s  = 1000.0;
    r64 s_to_m   = 60.0;
    r64 m_to_h   = 60.0;
    
    u32 m  = (u64)(t_us / (us_to_ms * ms_to_s * s_to_m));
    u32 s  = (u32)(t_us / (us_to_ms * ms_to_s)) % 60;
    u32 ms = (u32)((u32)(t_us / us_to_ms) % (u32)us_to_ms);
    
    char *format = "%02dm:%02ds.%03dms";
    u32 size = 100;
    char *buf = (char *)malloc(size);
    snprintf(buf, size, format, m, s, ms);
    
    return String(buf);
}
// TO DO: add resolution, choose a format based on that
// Ace skills would be to have resolution contraints on both sides.
// But then you could also have some sort of configuration that you set at the beginning of the program...
//  for the overloads that I usually weld a value into; instead, they could be abiding by that configuration.
string *TimerString_v2(r64 t, TIME_RES res)
{
    char *format_m  = "%02dm";
    char *format_s  = "%02dm:%02ds";
    char *format_ms = "%02dm:%02ds.%03dms";
    char *format_us = "%02dm:%02ds.%03dms.%03dus";
    char *format_ns = "%02dm:%02ds.%03dms.%03dus.%03ns";
    
#if 0    
    if(res == SECONDS)
    {
        u32 req_len = snprintf(NULL, 0, format_s, m, s);
        
    }
    else if(res == MILLISECONDS)
    {
        u32 req_len = snprintf(NULL, 0, format_ms, m, s, ms);
        
    }
    else if(res == MICROSECONDS)
    {
        u32 req_len = snprintf(NULL, 0, format_us, m, s, ms, us);
        
    }
    else if(res == NANOSECONDS)
    {
        u32 req_len = snprintf(NULL, 0, format_ns, m, s, ms, us, ns);
        
    }
#endif
    
    return NULL;
}