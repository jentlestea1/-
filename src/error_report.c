#include "error_report.h"
#include "config_info_collect.h"
#include "driver_match.h"
#include <stdio.h>

void report_error(const char* file, const char* func, const char* msg)
{
    const char* device_context = get_device_context();
    const char* op_context = get_op_context();

#if DEBUG
    if (op_context != NULL && device_context != NULL){
        printf("<Error context=\"%s@%s\" position=\"%s@%s\">\n", 
                               op_context, device_context, func, file);
    }else if(device_context != NULL){
        printf("<Error context=\"%s\" position=\"%s@%s\">\n", 
                               device_context, func, file);
    }else{
        printf("<Error position=\"%s@%s\">\n", func, file);
    }
    printf("\t<Message>%s</Message>\n", msg);
    printf("</Error>\n");
#endif
}


int check_null(const char* file, const char* func, const char* para, const void* val)
{
   char msg[1024];

   if (val == NULL){
#if DEBUG
       sprintf(msg, "'%s' is NULL", para);
       report_error(file, func, msg);
#endif
       return FAILURE;
   } 

   return SUCCESS;
}

