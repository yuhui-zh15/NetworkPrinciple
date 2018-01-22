#include "sysinclude.h"

/*
    - 参数pData：指向要发送的RIP分组内容的指针。其指向的数据应该为网络序，是从RIP报头开始的。
    - 参数len：要发送的RIP分组的长度 。
    - 参数dstPort：要发送的RIP分组的目的端口 
    - 参数iNo：发送该分组的接口号 
*/
extern void rip_sendIpPkt(unsigned char *pData, UINT16 len,unsigned short dstPort,UINT8 iNo);

/*
    系统以单向链表存储RIP路由表，学生需要利用此表存储RIP路由，供客户端软件检查。该全局变量为系统中RIP路由表链表的头指针， 其中，stud_rip_route_node结构定义如下：
    typedef struct stud_rip_route_node { 
        unsigned int dest;     
        unsigned int mask;    
        unsigned int nexthop; 
        unsigned int metric; 
        unsigned int if_no;
        struct stud_rip_route_node *next; 
    }; 
*/
extern struct stud_rip_route_node *g_rip_route_table;

// RIP使用UDP的520端口进行路由信息交互
#define PORT 520

// RIP表头
struct rip_header {
    unsigned char command;
    unsigned char version;
    unsigned short mustbezero;
};

// RIP表项
struct rip_entry {
    unsigned short addressid;
    unsigned short routetag;
    unsigned int dest;
    unsigned int mask;
    unsigned int nexthop;
    unsigned int metric;
};

// RIP分组
struct rip_packet {
    rip_header ripheader;
    rip_entry ripentry[25];
};

/*
    - 参数pBuffer：指向接收到的RIP分组内容的指针。其指向的数据为网络序，是从RIP报头开始的。
    - 参数bufferSize：接收到的RIP分组的长度。
    - 参数iNo：接收该分组的接口号。
    - 参数srcAdd：接收到的分组的源IP地址。
    - 返回值：成功为0，失败为-1。
*/
int stud_rip_packet_recv(char *pBuffer, int bufferSize, UINT8 iNo, UINT32 srcAdd) {   
    // 对RIP分组进行合法性检查，若分组存在错误，则调用ip_DiscardPkt()函数，并在type参数中传入错误编号
    unsigned char command = pBuffer[0];
    unsigned char version = pBuffer[1];
    if (command != 1 && command != 2) {
        ip_DiscardPkt(pBuffer, STUD_RIP_TEST_COMMAND_ERROR);
        return -1;
    }
    if (version != 2) {
        ip_DiscardPkt(pBuffer, STUD_RIP_TEST_VERSION_ERROR);
        return -1;
    }
    // 对于正确的分组，则根据分组的command域，判断分组类型，即Request或Response分组类型
    // 对于Request分组，应该将根据本地的路由表信息组成Response分组，并通过rip_sendIpPkt()函数发送出去。
    if (command == 1) { //request,发送回当前表项
        rip_packet rippacket;
        stud_rip_route_node* pointer = g_rip_route_table;
        while (pointer != NULL) {
            memset(&rippacket, 0, sizeof(rip_packet));
            rippacket.ripheader.command = 2;
            rippacket.ripheader.version = 2;
            int cnt = 0;
            while (pointer != NULL && cnt < 25) { //每个rip_packet最大25个rip_entry
                // 由于实现水平分裂算法，封装Response分组时应该检查该Request分组的来源接口，Response分组中的路由信息不包括来自该来源接口的路由
                if (pointer->if_no != iNo) {
                    rippacket.ripentry[cnt].addressid = htons(2);
                    rippacket.ripentry[cnt].routetag = 0;
                    rippacket.ripentry[cnt].dest = htonl(pointer->dest);
                    rippacket.ripentry[cnt].mask = htonl(pointer->mask);
                    rippacket.ripentry[cnt].nexthop = htonl(pointer->nexthop);
                    rippacket.ripentry[cnt].metric = htonl(pointer->metric);
                    ++cnt;
                }
                pointer = pointer->next;
            }
            UINT16 len = 4 + cnt * sizeof(rip_entry);
            rip_sendIpPkt((unsigned char*)(&rippacket), len, PORT, iNo);
        }
    }
    // 对于Response分组，应该提取出该分组中携带的所有路由表项，针对每个Response分组的路由表项，进行不同操作
    else if (command == 2) {
        rip_packet rippacket = *(rip_packet*)pBuffer;
        int cnt = (bufferSize - 4) / sizeof(rip_entry);
        for (int i = 0; i < cnt; ++i) {
            rippacket.ripentry[i].dest = ntohl(rippacket.ripentry[i].dest);
            rippacket.ripentry[i].mask = ntohl(rippacket.ripentry[i].mask);
            rippacket.ripentry[i].nexthop = ntohl(rippacket.ripentry[i].nexthop);
            rippacket.ripentry[i].metric = ntohl(rippacket.ripentry[i].metric);
            // 根据IP Address与Mask搜索表项
            stud_rip_route_node* pointer = g_rip_route_table;
            while (pointer != NULL) {
                if (pointer->dest == rippacket.ripentry[i].dest && pointer->mask == rippacket.ripentry[i].mask) break;
                pointer = pointer->next;
            }
            // 若该Response路由表项为新的表项，即该Response路由表项中的Ip Address与所有本地路由表项的IP Address都不相同，则将其Metric值加1之后添加到本地路由表中
            // 如果Metric加1之后等于16，则表明该Response路由表项已经失效，此时无需添加该表项
            if (pointer == NULL) { 
                if (rippacket.ripentry[i].metric + 1 < 16) {
                    stud_rip_route_node* newnode = new stud_rip_route_node;
                    newnode->dest = rippacket.ripentry[i].dest;
                    newnode->mask = rippacket.ripentry[i].mask;
                    newnode->nexthop = srcAdd; //nexthop为srcAdd(接收到的分组的源IP地址)
                    newnode->metric = rippacket.ripentry[i].metric + 1;
                    newnode->if_no = iNo;
                    newnode->next = g_rip_route_table;
                    g_rip_route_table = newnode;
                }
            }
            // 若该Response路由表项已经在本地路由表中存在，且两者的Next Hop字段相同，则将其Metric值加1后，更新到本地路由表项的Metric字段
            // 如果Metric加1之后等于16，应置该本地路由表项为无效
            else if (pointer->nexthop == srcAdd) {
                pointer->metric = (rippacket.ripentry[i].metric + 1 < 16)? rippacket.ripentry[i].metric + 1: 16; 
                pointer->if_no = iNo;
            }
            // 若该Response路由表项已经在本地路由表中存在，且两者的Next Hop字段不同，则只有当Response路由表项中的Metric值小于本地路由表项中的Metric值时，才将其Metric值加1后，更新到本地路由表项的Metric字段，并更新Next Hop字段。
            // 如果Metric加1之后等于16，应置该本地路由表项为无效
            else {
                if (pointer->metric > rippacket.ripentry[i].metric) { //存在且nexthop与srcAdd不等,比较后更新
                    pointer->metric = (rippacket.ripentry[i].metric + 1 < 16) ? rippacket.ripentry[i].metric + 1: 16;
                    pointer->nexthop = srcAdd;
                    pointer->if_no = iNo;
                }
            }
        }
    }
    return 0;
}


/*
    - 参数destAdd：路由超时消息中路由的目标地址。
    - 参数mask：路由超时消息中路由的掩码。
    - 参数msgType：消息类型，包括以下两种定义： 
        #define RIP_MSG_SEND_ROUTE 
        #define RIP_MSG_DELE_ROUTE 
*/
void stud_rip_route_timeout(UINT32 destAdd, UINT32 mask, unsigned char msgType)
{
    // RIP协议每隔30秒，重新广播一次路由信息，系统调用该函数并置msgType为RIP_MSG_SEND_ROUTE来进行路由信息广播。该函数应该在每个接口上分别广播自己的RIP路由信息，即通过rip_sendIpPkt函数发送RIP Response分组。由于实现水平分割，分组中的路由信息不包括来自该接口的路由信息
    if (msgType == RIP_MSG_SEND_ROUTE) {
        rip_packet rippacket;
        // 接口号iNo为1
        stud_rip_route_node* pointer = g_rip_route_table;
        while (pointer != NULL) {
            memset(&rippacket, 0, sizeof(rip_packet));
            rippacket.ripheader.command = 2;
            rippacket.ripheader.version = 2;
            int cnt = 0;
            while (pointer != NULL && cnt < 25) {
                if (pointer->if_no != 1) {
                    rippacket.ripentry[cnt].addressid = htons(2);
                    rippacket.ripentry[cnt].routetag = 0;
                    rippacket.ripentry[cnt].dest = htonl(pointer->dest);
                    rippacket.ripentry[cnt].mask = htonl(pointer->mask);
                    rippacket.ripentry[cnt].nexthop = htonl(pointer->nexthop);
                    rippacket.ripentry[cnt].metric = htonl(pointer->metric);
                    ++cnt;
                }
                pointer = pointer->next;
            }
            UINT16 len = 4 + cnt * sizeof(rip_entry);
            rip_sendIpPkt((unsigned char*)(&rippacket), len, PORT, 1);
        }
        // 接口号iNo为2
        pointer = g_rip_route_table;
        while (pointer != NULL) {
            memset(&rippacket, 0, sizeof(rip_packet));
            rippacket.ripheader.command = 2;
            rippacket.ripheader.version = 2;
            int cnt = 0;
            while (pointer != NULL && cnt < 25) {
                if (pointer->if_no != 2) {
                    rippacket.ripentry[cnt].addressid = htons(2);
                    rippacket.ripentry[cnt].routetag = 0;
                    rippacket.ripentry[cnt].dest = htonl(pointer->dest);
                    rippacket.ripentry[cnt].mask = htonl(pointer->mask);
                    rippacket.ripentry[cnt].nexthop = htonl(pointer->nexthop);
                    rippacket.ripentry[cnt].metric = htonl(pointer->metric);
                    ++cnt;
                }
                pointer = pointer->next;
            }
            UINT16 len = 4 + cnt * sizeof(rip_entry);
            rip_sendIpPkt((unsigned char*)(&rippacket), len, PORT, 2);
        }
    }
    // RIP协议每个路由表项都有相关的路由超时计时器，当路由超时计时器过期时，该路径就标记为失效的，但仍保存在路由表中，直到路由清空计时器过期才被清掉。当超时定时器被触发时，系统会调用该函数并置msgType为RIP_MSG_DELE_ROUTE，并通过destAdd和mask参数传入超时的路由项。该函数应该置本地路由的对应项为无效，即metric值置为16
    else if (msgType == RIP_MSG_DELE_ROUTE) {
        stud_rip_route_node* pointer = g_rip_route_table;
        // 根据IP Address与Mask搜索表项
        while (pointer != NULL) {
            if (pointer->dest == destAdd && pointer->mask == mask) break;
            pointer = pointer->next;
        }
        if (pointer != NULL) {
            pointer->metric = 16;
        }
    }
}
