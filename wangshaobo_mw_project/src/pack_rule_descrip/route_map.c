#include "route_map.h"
#include "stdlib.h"
#include "string.h"

//将堆换成栈
//void* get_route_node(){
//   void* p_route_node=(void*)(route*)malloc(sizeof(route));
//    return p_route_node;
//}

void get_RT_route_map(char* RT_lid,/*void** pp_route*/route *r){
    if(strcmp(RT_lid,"")==0)return;
    UINT irdirect_pos=find_irdirect_RT_pos(RT_lid);
    //*pp_route=get_route_node();
    //strcpy(((route*)(*pp_route))->bus_type,get_irdirect_bus_type(irdirect_pos));
    //strcpy(((route*)(*pp_route))->bus_lid,get_irdirect_bus_lid(irdirect_pos));
    //strcpy(((route*)(*pp_route))->RT_lid,get_irdirect_RT_lid(irdirect_pos));
    r->bus_type=get_irdirect_bus_type(irdirect_pos);
    r->bus_lid=get_irdirect_bus_lid(irdirect_pos);
    r->RT_lid=get_irdirect_RT_lid(irdirect_pos);
}

void get_dev_route_map(char* dev_lid,/*void** pp_route*/route *r){
    if(strcmp(dev_lid,"")==0)return;
    UINT irdirect_pos=find_irdirect_dev_pos(dev_lid);
    //*pp_route=get_route_node();
    //strcpy(((route*)(*pp_route))->bus_type,get_irdirect_bus_type(irdirect_pos));
    //strcpy(((route*)(*pp_route))->bus_lid,get_irdirect_bus_lid(irdirect_pos));
    //strcpy(((route*)(*pp_route))->RT_lid,get_irdirect_RT_lid(irdirect_pos));
    r->bus_type=get_irdirect_bus_type(irdirect_pos);
    r->bus_lid=get_irdirect_bus_lid(irdirect_pos);
    r->RT_lid=get_irdirect_RT_lid(irdirect_pos);
}

//void free_route_node(void** pp_route){
//    route* p_route_tmp=(route*)(*pp_route);
//    free(p_route_tmp);
//    *pp_route=NULL;
//}

char* get_route_bus_type(route r){
    return r.bus_type;
}

char* get_route_bus_lid(route r){

    return r.bus_lid;
}

char* get_route_RT_lid(route r){
    return r.RT_lid;
    
}

