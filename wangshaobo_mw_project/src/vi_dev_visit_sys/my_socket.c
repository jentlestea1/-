#include "my_socket.h"
#include<stdio.h>  
#include<stdlib.h>  
#include<string.h>  
#include<errno.h>  
#include<sys/types.h>  
#include<sys/socket.h>  
#include<netinet/in.h>  
#include<unistd.h>
#include "control_package.h"
#include "handle_event.h"
#include "config_1553.h"
#include "my_RT_socket.h"
#include<pthread.h>

void create_bus_socket_client(UINT config_id,UINT RT_config_id,UINT port){  
    usleep(500000);
    int    sockfd, n,rec_len;  
    struct sockaddr_in    servaddr;
    void* p_config_node_tmp=get_config_node(config_id);
    UINT traffic_repos_id=get_config_node_traffic_repos_id(p_config_node_tmp);
    while(1){
    if(!get_buffer_is_avail(config_id,RT_config_id))continue;
    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){  
    printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);  
    exit(0);  
    }  
    memset(&servaddr, 0, sizeof(servaddr));  
    servaddr.sin_family = AF_INET;  
    servaddr.sin_port = htons(port);  
    if( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){  
    printf("bus--connect error: %s(errno: %d)\n",strerror(errno),errno);  
    exit(0);  
    }
    if(get_buffer_is_avail(config_id,RT_config_id)){
        set_buffer_unavail(config_id,RT_config_id);
            if(send(sockfd,read_buf_1553[config_id],strlen(read_buf_1553[config_id]),0) == -1)  
                perror("send error");
                memset(read_buf_1553[config_id],0,BUF_MAX_LEN);
    }
    close(sockfd);  
    }
    close(sockfd);  
    exit(0);  
}

void create_bus_socket_server(UINT config_id,UINT port)  //原port+1用来接受传回的数据
    {  
        void* p_config_node_tmp=get_config_node(config_id);
        UINT traffic_repos_id=get_config_node_traffic_repos_id(p_config_node_tmp);
        int    socket_fd, connect_fd;  
        struct sockaddr_in     servaddr;  
        char    recv_buffer[4096];  
        int     recv_len;  
        if( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){  
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);  
        exit(0);  
        }  
        memset(&servaddr, 0, sizeof(servaddr));  
        servaddr.sin_family = AF_INET;  
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);//IP地址设置成INADDR_ANY,让系统自动获取本机的IP地址。  
        servaddr.sin_port = htons(port+1);//
        if( bind(socket_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){  
        printf("bus--bind socket error: %s(errno: %d)\n",strerror(errno),errno);  
        exit(0);  
        }  
        if( listen(socket_fd, 10) == -1){  
        printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);  
        exit(0);  
        }  
        while(1){  
    //阻塞直到有客户端连接，不然多浪费CPU资源。  
            if( (connect_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL)) == -1){  
            printf("accept socket error: %s(errno: %d)",strerror(errno),errno);  
            continue;  
        }  
        recv_len = recv(connect_fd, recv_buffer,MAXLINE, 0);  
        recv_buffer[recv_len] = '\0';
        if(recv_len!=0){
                ctrl_unpack_package_to_1553(traffic_repos_id,port,recv_buffer,recv_len);
        }  
        close(connect_fd);  
    }  
        close(socket_fd);  
}

void* scan_1553_RT_section_pthread_func(void* p_scan_config){
    scan_config* p_scan_config_tmp=(scan_config*)p_scan_config;
    UINT config_id=p_scan_config_tmp->config_id;
    unsigned char* buffer=read_buf_1553[config_id];
    UINT size=0;
    void* p_config_node_tmp=get_config_node(config_id);
    UINT repos_pos_tmp=get_config_node_traffic_repos_id(p_config_node_tmp);
    UINT light_pos_tmp;
    while(true){
        ctrl_pack_package_to_1553(repos_pos_tmp,buffer,&size,&light_pos_tmp);
        if(size!=0){
            set_buffer_avail(config_id,light_pos_tmp);
        }
        usleep(5000);
    }
}

void* bus_socket_pthread_func(void* p_socket_config){
   socket_config* p_socket_config_tmp=(socket_config*)p_socket_config;
   UINT config_id_tmp=p_socket_config_tmp->config_id;
   UINT RT_config_id_tmp=p_socket_config_tmp->RT_config_id;
   UINT port_tmp=p_socket_config_tmp->port;
   create_bus_socket_client(config_id_tmp,RT_config_id_tmp,port_tmp);
}

void* bus_ret_socket_pthread_func(void* p_socket_config){
   socket_config* p_socket_config_tmp=(socket_config*)p_socket_config;
   UINT config_id_tmp=p_socket_config_tmp->config_id;
   UINT port_tmp=p_socket_config_tmp->port;
   create_bus_socket_server(config_id_tmp,port_tmp);
}

void create_scan_1553_RT_section_unit(void* p_scan_config){
    pthread_t tid;
    int err=pthread_create(&tid,NULL,scan_1553_RT_section_pthread_func,p_scan_config);
    if(err!=0) printf("创建RT section描线程失败...\n");
    else printf("成功创建RT section扫描线程，该扫描线程每20ms打包一次数据...\n");
}

void* RT_socket_pthread_func(void* p_socket_config){
    socket_config* p_socket_config_tmp=(socket_config*)p_socket_config;
    UINT port_tmp=p_socket_config_tmp->port;
    pthread_t tid;
    void* p_RT_con=get_one_port_con();
    set_RT_port(p_RT_con,port_tmp);
    int err=pthread_create(&tid,NULL,create_RT_socket_server,p_RT_con);
    if(err!=0)printf("RT 收端创建线程失败...\n");
}

void* RT_ret_socket_pthread_func(void* p_socket_config){
    socket_config* p_socket_config_tmp=(socket_config*)p_socket_config;
    UINT port_tmp=p_socket_config_tmp->port;
    if(port_tmp!=9000)return NULL;//9000测试用
    pthread_t tid;
    void* p_RT_con=get_one_port_con();
    set_RT_port(p_RT_con,port_tmp);
    int err=pthread_create(&tid,NULL,create_RT_ret_socket_client,p_RT_con);
    if(err!=0)printf("RT 发端创建线程失败...\n");

}

void create_1553_bus_unit(void* p_socket_config){
    pthread_t tid1;
    pthread_t tid2;
    pthread_t tid3;
    pthread_t tid4;
    UINT err1,err2,err3,err4=0;
    err1=pthread_create(&tid1,NULL,bus_ret_socket_pthread_func,p_socket_config);
    err2=pthread_create(&tid2,NULL,RT_socket_pthread_func,p_socket_config);
    err3=pthread_create(&tid4,NULL,RT_ret_socket_pthread_func,p_socket_config);
    err4=pthread_create(&tid3,NULL,bus_socket_pthread_func,p_socket_config);
    if(err1!=0||err2!=0||err3!=0||err4!=0)printf("创建1553模拟端口失败...\n");
    else printf("成功创建了一个1553模拟端口...\n");
}

bool get_buffer_is_avail(UINT config_id,UINT RT_config_id){
    return is_read_buf_1553_avail[config_id][RT_config_id];
}

bool set_buffer_unavail(UINT config_id,UINT RT_config_id){
    is_read_buf_1553_avail[config_id][RT_config_id]=false;
}

bool set_buffer_avail(UINT config_id,UINT RT_config_id){
    is_read_buf_1553_avail[config_id][RT_config_id]=true;
}
