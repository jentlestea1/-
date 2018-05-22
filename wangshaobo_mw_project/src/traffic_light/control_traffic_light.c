/*
 *control_traffic_light.c,定义操作交通灯数据结构的各种操作结构。
 */
#include "compile_type.h"
#include "control_traffic_light.h"
#include "unistd.h"
#include "handle_event.h"
#include "route_map.h"
#include "address_map.h"
#ifdef __GCC_C99
#include <pthread.h>
#elif __SPARC_GCC_MMU
#include <fsu_pthread.h>
#endif
#include "stdio.h"
#include "string.h"

static traffic_light_repos* p_traffic_light_repos_array[TRAFFIC_REPOS_ARRAY_MAX_LEN];
UINT config_traffic_repos(char* bus_type,char* bus_lid){
    int i=0;
    printf("正在为bus_type:%s,bus_lid:%s的总线创建交通灯库...\n",bus_type,bus_lid);
    for(;i<TRAFFIC_REPOS_ARRAY_MAX_LEN;i++){
        if(p_traffic_light_repos_array[i]==NULL){
            void* p_tmp=create_traffic_repos(bus_type,bus_lid);
            p_traffic_light_repos_array[i]=p_tmp;
            break;
        }
    }
    if(i>=TRAFFIC_REPOS_ARRAY_MAX_LEN||i<0)i=-1;
    return i;
}
void* get_traffic_repos_node(UINT traffic_repos_id){
    return p_traffic_light_repos_array[traffic_repos_id];
}
void config_traffic(){
    UINT form_num=get_form_num();
    int i=0;
    for(;i<form_num;i++){
        void* p_form_item=get_forms_item(i);
        char* bus_type=get_form_bus_type(p_form_item);
        char* bus_lid=get_form_bus_lid(p_form_item);
        config_traffic_repos(bus_type,bus_lid);
    }
}
void* traffic_repos_scan_pthread_func(void* argc){
    int i=0;
    for(;i<TRAFFIC_REPOS_ARRAY_MAX_LEN;i++){
        if(p_traffic_light_repos_array[i]!=NULL){
            while(true){
                traffic_repos_scan_func(i);
#ifdef __GCC_C99
        usleep(10000);
#elif __SPARC_GCC_MMU
        sleep(1);
#endif
            }
        }
    }
}

void  set_traffic_light(UINT traffic_repos_id,UINT light_pos,TRAFFIC_STATUS traffic_status,IS_LOADED is_loaded,IS_BACK is_back){
    if(traffic_repos_id<0||traffic_repos_id>=TRAFFIC_REPOS_ARRAY_MAX_LEN||p_traffic_light_repos_array[traffic_repos_id]==NULL)return;  
    traffic_light_repos* p_repos_tmp=(traffic_light_repos*)p_traffic_light_repos_array[traffic_repos_id];
    int i=0;
    for(;i<p_repos_tmp->list_len;i++){
        traffic_light* p_list_tmp=p_repos_tmp->p_traffic_light_list[i];
        traffic_light* p=p_list_tmp+light_pos;
        if(traffic_status!=NOT_SET){
            p->traffic_status=traffic_status;
        }
        if(is_loaded!=NOT_SET)
            p->is_loaded=is_loaded;
        if(is_back!=NOT_SET)
            p->is_back=is_back;
    }
}

void traffic_repos_scan_func(UINT traffic_repos_id){
    /*
     * 刷新，10ms间隔
     */
    if(traffic_repos_id<0||traffic_repos_id>=TRAFFIC_REPOS_ARRAY_MAX_LEN||p_traffic_light_repos_array[traffic_repos_id]==NULL)return;
    traffic_light_repos* p_repos_tmp=(traffic_light_repos*)p_traffic_light_repos_array[traffic_repos_id];
        if(p_repos_tmp->is_traffic_enable==TRAFFIC_ENABLE){
            int i=0;
            for(;i<p_repos_tmp->list_len;i++){
                int j=0;
                traffic_light* p_light_tmp=p_repos_tmp->p_traffic_light_list[i];
                void* p_route_node=get_route_node();
                for(;j<p_repos_tmp->dev_num[i];j++){
                        if(p_light_tmp->is_loaded==LOADED){
                            if(p_light_tmp->traffic_status==UNCHECKED){
                                p_light_tmp->traffic_status=GREEN;
                            }
                            else if(p_light_tmp->traffic_status==GREEN){
                                p_light_tmp->traffic_status=YELLOW;
                                p_repos_tmp->RT_section_priority[i]=PRIORITY_HIGH;
                            }
                            else if(p_light_tmp->traffic_status==YELLOW){
                                p_light_tmp->traffic_status=RED;
                            }
                        }
                        char* dev_lid=p_light_tmp->dev_lid;
                        get_dev_route_map(dev_lid,&p_route_node);
                        char* bus_type=get_route_bus_type(p_route_node);
                        char* bus_lid=get_route_bus_lid(p_route_node);
                        char* RT_lid=get_route_RT_lid(p_route_node);
                        UINT dev_read_block_size_tmp=get_dev_trans_attr(bus_type,bus_lid,RT_lid,dev_lid,RECEIVE_BLOCK_FLAG);
                        UINT dev_read_buffer_size;
                        if(!is_read_region_empty(bus_type,bus_lid,RT_lid,dev_lid)){
                            p_light_tmp->traffic_status=GREEN;
                            p_light_tmp->is_loaded=LOADED;
                            if(dev_read_buffer_size=get_read_region_size(bus_type,bus_lid,RT_lid,dev_lid)>=READ_REGION_MAX_SIZE/2){
                                p_repos_tmp->RT_section_priority[j]=PRIORITY_HIGH;
                                if(dev_read_buffer_size%dev_read_block_size_tmp!=0){
                                    throw_event(0,RT_lid,EVT_APP_DATA_SIZE_ERR);
                                }
                            }
                            else{
                                p_repos_tmp->RT_section_priority[j]=PRIORITY_LOW;
                            }

                        }
                        else{
                            p_light_tmp->traffic_status=UNCHECKED;
                        }
                        if(!is_write_region_empty(bus_type,bus_lid,RT_lid,dev_lid)){
                            p_light_tmp->is_back=BACK;
                        }
                        else{
                            p_light_tmp->is_back=NOT_BACK;
                        }
                        p_light_tmp++;
                }
                free_route_node(&p_route_node);
            }
        }
}

void set_traffic_repos_disable(UINT traffic_repos_id){
  if(traffic_repos_id<0||traffic_repos_id>=TRAFFIC_REPOS_ARRAY_MAX_LEN||p_traffic_light_repos_array[traffic_repos_id]==NULL)return;
    traffic_light_repos* p_repos_tmp=(traffic_light_repos*)p_traffic_light_repos_array[traffic_repos_id];
    p_repos_tmp->is_traffic_enable=TRAFFIC_DISABLE;
}

void set_traffic_repos_enable(UINT traffic_repos_id){
  if(traffic_repos_id<0||traffic_repos_id>=TRAFFIC_REPOS_ARRAY_MAX_LEN||p_traffic_light_repos_array[traffic_repos_id]==NULL)return;
    traffic_light_repos* p_repos_tmp=(traffic_light_repos*)p_traffic_light_repos_array[traffic_repos_id];
    p_repos_tmp->is_traffic_enable=TRAFFIC_ENABLE;
}

void set_traffic_light_list_enable(UINT traffic_repos_id,UINT light_pos){
    if(traffic_repos_id<0||traffic_repos_id>=TRAFFIC_REPOS_ARRAY_MAX_LEN||p_traffic_light_repos_array[traffic_repos_id]==NULL)return;
    traffic_light_repos* p_repos_tmp=(traffic_light_repos*)p_traffic_light_repos_array[traffic_repos_id]; 
    if(light_pos<0||light_pos>=TRAFFIC_LIST_MAX_LEN)return;
    p_repos_tmp->is_RT_section_enable[light_pos]=true;
}

void set_traffic_light_list_disable(UINT traffic_repos_id,UINT light_pos){
    if(traffic_repos_id<0||traffic_repos_id>=TRAFFIC_REPOS_ARRAY_MAX_LEN||p_traffic_light_repos_array[traffic_repos_id]==NULL)return;
    traffic_light_repos* p_repos_tmp=(traffic_light_repos*)p_traffic_light_repos_array[traffic_repos_id]; 
    if(light_pos<0||light_pos>=TRAFFIC_LIST_MAX_LEN)return;
    p_repos_tmp->is_RT_section_enable[light_pos]=false;
    
}

void reset_traffic_repos_light(UINT traffic_repos_id,UINT light_pos){
    if(traffic_repos_id<0||traffic_repos_id>=TRAFFIC_REPOS_ARRAY_MAX_LEN||p_traffic_light_repos_array[traffic_repos_id]==NULL)return;
    traffic_light_repos* p_repos_tmp=(traffic_light_repos*)p_traffic_light_repos_array[traffic_repos_id];
    if(light_pos<0||light_pos>=TRAFFIC_LIST_MAX_LEN)return;
    traffic_light* p_light_tmp=p_repos_tmp->p_traffic_light_list[light_pos];
    if(p_light_tmp==NULL)return;

    p_repos_tmp->RT_section_priority[light_pos]=PRIORITY_LOW;
    p_light_tmp->traffic_status=UNCHECKED;
    p_light_tmp->is_loaded=NOT_LOADED;
}

UINT get_dev_light_pos(UINT traffic_repos_id,char* dev_lid){
    if(traffic_repos_id<0||traffic_repos_id>=TRAFFIC_REPOS_ARRAY_MAX_LEN||p_traffic_light_repos_array[traffic_repos_id]==NULL)return -1;  
    traffic_light_repos* p_repos_tmp=(traffic_light_repos*)p_traffic_light_repos_array[traffic_repos_id];
    int i=0;
    UINT light_pos=-1;
    for(;i<p_repos_tmp->list_len;i++){
        traffic_light* p_list_tmp=p_repos_tmp->p_traffic_light_list[i];
        int j=0;
        for(;j<p_repos_tmp->dev_num[i];j++){
            traffic_light* p=p_list_tmp+j;
            if(strcmp(p->dev_lid,dev_lid)==0){
                light_pos=i;
                break;
            }
        }
        if(light_pos!=-1)break;
    }
    return light_pos;
}

UINT get_light_pos(UINT traffic_repos_id,char* RT_lid){
    if(traffic_repos_id<0||traffic_repos_id>=TRAFFIC_REPOS_ARRAY_MAX_LEN||p_traffic_light_repos_array[traffic_repos_id]==NULL)return -1;  
    traffic_light_repos* p_repos_tmp=(traffic_light_repos*)p_traffic_light_repos_array[traffic_repos_id]; 
    int i=0;
    UINT light_pos=-1;
    for(;i<p_repos_tmp->list_len;i++){
        char* RT_lid_tmp=p_repos_tmp->RT_lid_array[i];
        if(strcmp(RT_lid,RT_lid_tmp)==0)light_pos=i;
    }
    return light_pos;
}

bool is_RT_section_readable(UINT traffic_repos_id,UINT light_pos){
    if(traffic_repos_id<0||traffic_repos_id>=TRAFFIC_REPOS_ARRAY_MAX_LEN||p_traffic_light_repos_array[traffic_repos_id]==NULL)return false;
    traffic_light_repos* p_repos_tmp=(traffic_light_repos*)p_traffic_light_repos_array[traffic_repos_id]; 
    if(p_repos_tmp->is_traffic_enable==false)return false;
    if(light_pos<0||light_pos>=TRAFFIC_LIST_MAX_LEN)return false;
    return p_repos_tmp->is_RT_section_enable[light_pos];
}

bool is_RT_section_writeable(UINT traffic_repos_id,UINT light_pos){        
    if(traffic_repos_id<0||traffic_repos_id>=TRAFFIC_REPOS_ARRAY_MAX_LEN||p_traffic_light_repos_array[traffic_repos_id]==NULL)return false;
    traffic_light_repos* p_repos_tmp=(traffic_light_repos*)p_traffic_light_repos_array[traffic_repos_id]; 
    if(p_repos_tmp->is_traffic_enable==false)return false;
    if(light_pos<0||light_pos>=TRAFFIC_LIST_MAX_LEN)return false;
    return p_repos_tmp->is_RT_section_enable[light_pos];
}

bool is_dev_readable(UINT traffic_repos_id,UINT light_pos,UINT dev_pos){
    if(is_RT_section_readable(traffic_repos_id,light_pos)==false)return false;
    if(traffic_repos_id<0||traffic_repos_id>=TRAFFIC_REPOS_ARRAY_MAX_LEN||p_traffic_light_repos_array[traffic_repos_id]==NULL)return false;
    traffic_light_repos* p_repos_tmp=(traffic_light_repos*)p_traffic_light_repos_array[traffic_repos_id];
    if(light_pos<0||light_pos>=TRAFFIC_LIST_MAX_LEN)return false;
    traffic_light* p_light_tmp=p_repos_tmp->p_traffic_light_list[light_pos];
    if(p_light_tmp==NULL)return false;
    if((p_light_tmp+dev_pos)->traffic_status==GREEN||(p_light_tmp+dev_pos)->traffic_status==YELLOW||(p_light_tmp+dev_pos)->traffic_status==RED){
        return true;//红灯先保留
    }
    else return false;
}

bool is_red_light(UINT traffic_repos_id,UINT light_pos,char* dev_lid){
    if(traffic_repos_id<0||traffic_repos_id>=TRAFFIC_REPOS_ARRAY_MAX_LEN||p_traffic_light_repos_array[traffic_repos_id]==NULL)return false;
    traffic_light_repos* p_repos_tmp=(traffic_light_repos*)p_traffic_light_repos_array[traffic_repos_id];
    if(light_pos<0||light_pos>=TRAFFIC_LIST_MAX_LEN)return false;
    traffic_light* p_light_tmp=p_repos_tmp->p_traffic_light_list[light_pos];
    if(p_light_tmp==NULL)return false;
    int i=0;
    for(;i<p_repos_tmp->dev_num[light_pos];i++){
        if(strcmp(dev_lid,(p_light_tmp+i)->dev_lid)==0){
            if((p_light_tmp+i)->traffic_status==RED){
                return true;
            }   
        }
    }
    return false;
    
}

UINT get_scan_pos(UINT traffic_repos_id){ 
    UINT next_pos=-1; if(traffic_repos_id<0||traffic_repos_id>=TRAFFIC_REPOS_ARRAY_MAX_LEN||p_traffic_light_repos_array[traffic_repos_id]==NULL)return -1;
    traffic_light_repos* p_repos_tmp=(traffic_light_repos*)p_traffic_light_repos_array[traffic_repos_id];
    UINT pos_tmp=p_repos_tmp->scan_pos;
    int i=pos_tmp==-1?0:pos_tmp;
    UINT list_len_tmp=p_repos_tmp->list_len;
    int j=(i+1)%list_len_tmp;
    UINT count_tmp=list_len_tmp;
    while(count_tmp--){
        UINT light_pos_tmp=j;
        if(p_repos_tmp->RT_section_priority[light_pos_tmp]==PRIORITY_HIGH){
            traffic_light* p_light_tmp=p_repos_tmp->p_traffic_light_list[light_pos_tmp];
            UINT dev_num_tmp=p_repos_tmp->dev_num[light_pos_tmp];
            UINT k=0;
            for(;k<dev_num_tmp;k++){
                if(is_dev_readable(traffic_repos_id,light_pos_tmp,k)){
                    next_pos=light_pos_tmp;
                    break;
                }
            }
        }
        if(next_pos!=-1)break;
        j=(j+1)%list_len_tmp;
    }
    count_tmp=list_len_tmp;
    j=(i+1)%list_len_tmp;
    if(next_pos==-1){
        while(count_tmp--){
            UINT light_pos_tmp=j;
            if(p_repos_tmp->RT_section_priority[light_pos_tmp]==PRIORITY_LOW){
                    traffic_light* p_light_tmp=p_repos_tmp->p_traffic_light_list[light_pos_tmp];
                    UINT dev_num_tmp=p_repos_tmp->dev_num[light_pos_tmp];
                    UINT k=0;
                    for(;k<dev_num_tmp;k++){
                        if(is_dev_readable(traffic_repos_id,light_pos_tmp,k)){
                        next_pos=light_pos_tmp;
                        break;
                    }
                }
            }
        if(next_pos!=-1)break;
        j=(j+1)%list_len_tmp;
        }   
    }
    p_repos_tmp->scan_pos=next_pos;
    return pos_tmp;
}

char* get_RT_section_RT_lid(UINT traffic_repos_id,UINT light_pos){
    char* RT_lid="";
    if(traffic_repos_id<0||traffic_repos_id>=TRAFFIC_REPOS_ARRAY_MAX_LEN||p_traffic_light_repos_array[traffic_repos_id]==NULL)return RT_lid;
    traffic_light_repos* p_repos_tmp=(traffic_light_repos*)p_traffic_light_repos_array[traffic_repos_id];
    if(light_pos<0||light_pos>=TRAFFIC_LIST_MAX_LEN)return RT_lid;
    RT_lid=p_repos_tmp->RT_lid_array[light_pos];
    return RT_lid;
}

void create_traffic_repos_scan_unit(void){
    pthread_t tid;
#ifdef __GCC_C99
    int err=pthread_create(&tid,NULL,traffic_repos_scan_pthread_func,NULL);
#elif __SPARC_GCC_MMU
    int err=pthread_create(&tid,NULL,(pthread_func_t)traffic_repos_scan_pthread_func,NULL);
#endif
    if(err!=0)printf("创建交通等库扫描线程失败...\n");
    else printf("成功创建交通灯库扫描线程,本扫描进程每50ms扫描一次所有的交通灯库...\n");
}


