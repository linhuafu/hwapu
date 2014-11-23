#ifndef __INFO_PRINT_H__
#define __INFO_PRINT_H__


#if DEBUG_PRINT
#define info_debug(fmt,args...) printf("[%s] %s %d: "fmt, __FILE__,__FUNCTION__,__LINE__,##args)
#else
#define info_debug(fmt,args...) do{}while(0)
#endif


#if ERROR_PRINT

#define info_err(fmt,args...) printf("ERROR![%s] %s %d: "fmt, __FILE__,__FUNCTION__, __LINE__,##args)
#else
#define info_err(fmt,args...) do{}while(0)
#endif


#if NORMAL_PRINT
#define info_normal(fmt,args...) printf(fmt,##args)
#else
#define info_normal(fmt,args...) do{}while(0)
#endif



#endif

