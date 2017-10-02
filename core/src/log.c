#include "core.h"

void
log_d_impl(const char *format, ...)
{
    va_list list;
    va_start(list, format);
    
    printf("%s", "[DEBUG] ");
    vprintf(format, list);
    
    va_end(list);
    
    putc('\n', stdout);
}

void
log_w_impl(const char *format, ...)
{
    va_list list;
    va_start(list, format);
    
    printf("%s", "[WARN] ");
    vprintf(format, list);
    
    va_end(list);
    
    putc('\n', stdout);
}

void
log_e_impl(const char *format, ...)
{
    va_list list;
    va_start(list, format);
    
    printf("%s", "[ERROR] ");
    vprintf(format, list);
    
    va_end(list);
    
    putc('\n', stdout);
}
