#include "sysinclude.h"
#include <iostream>
#include <deque>
using namespace std;

#define WINDOW_SIZE_STOP_WAIT 1 // 停等协议窗口大小
#define WINDOW_SIZE_BACK_N_FRAME 4 // 回退N帧协议窗口大小

extern void SendFRAMEPacket(unsigned char* pData, unsigned int len); // 系统发送包函数

typedef enum {data, ack, nak} frame_kind;
typedef struct frame_head {
    frame_kind kind;
    unsigned int seq;
    unsigned int ack;
    unsigned char data[100];
};
typedef struct frame {
    frame_head head;
    unsigned int size;
};
typedef struct buffer {
    frame frm;
    int size;
};
/*
buffer 结构：
buffer+0    frame_kind  帧类型
buffer+4    seq         发送帧序号
buffer+8    ack         确认帧序号
buffer+12   data[100]   帧数据
buffer+112  size        帧数据大小
buffer+116  size        帧大小
*/

/*
* 停等协议测试函数
*/
deque<buffer> stop_and_wait_deque; 
int stop_and_wait_window_size = 0;
int stud_slide_window_stop_and_wait(char *pBuffer, int bufferSize, UINT8 messageType) {
    // 若类型为发送：构造帧，加入队列，若当前发送窗口未满，发送该帧
    if (messageType == MSG_TYPE_SEND) {
        buffer buf;
        buf.frm = *(frame*)pBuffer;
        buf.size = bufferSize;
        stop_and_wait_deque.push_back(buf);
        if (stop_and_wait_window_size < WINDOW_SIZE_STOP_WAIT) {
            stop_and_wait_window_size++;
            SendFRAMEPacket((unsigned char*)&buf.frm, buf.size);
        }
    }
    // 若类型为接收：若队列头序号等于确认号，弹出该元素，如果有等待发送的帧，继续发送，如果没有，移动窗口下界
    else if (messageType == MSG_TYPE_RECEIVE) {
        unsigned int ack = ntohl(((frame*)pBuffer)->head.ack);
        if (!stop_and_wait_deque.empty()) {
            buffer buf = stop_and_wait_deque.front();
            printf("<DEBUG STOP_AND_WAIT RECEIVE>: buf.frm.head.seq = %d, ack = %d\n", ntohl(buf.frm.head.seq), ack);
            if (ntohl(buf.frm.head.seq) == ack) { 
                stop_and_wait_deque.pop_front();
                if (!stop_and_wait_deque.empty()) {
                    buf = stop_and_wait_deque.front();
                    SendFRAMEPacket((unsigned char*)&buf.frm, buf.size);
                }
                else {
                    stop_and_wait_window_size--;
                }
            }
        } 
    }
    // 若类型为超时：如果队列头序号等于超时号，重新发送该帧
    else if (messageType == MSG_TYPE_TIMEOUT) {
        unsigned int seq = *(unsigned int*)pBuffer;
        buffer buf = stop_and_wait_deque.front();
        printf("<DEBUG STOP_AND_WAIT TIMEOUT>: buf.frm.head.seq = %d, seq = %d\n", ntohl(buf.frm.head.seq), seq);
        if (ntohl(buf.frm.head.seq) == seq) {
            SendFRAMEPacket((unsigned char*)&buf.frm, buf.size);
        }
    }
    return 0;
}

/*
* 回退n帧测试函数
*/
deque<buffer> back_n_frame_deque;
int back_n_frame_window_size = 0;
int stud_slide_window_back_n_frame(char *pBuffer, int bufferSize, UINT8 messageType) {
    // 若类型为发送：构造帧，加入队列，若当前发送窗口未满，发送该帧
    if (messageType == MSG_TYPE_SEND) {
        buffer buf;
        buf.frm = *(frame*)pBuffer;
        buf.size = bufferSize;
        back_n_frame_deque.push_back(buf);
        if (back_n_frame_window_size < WINDOW_SIZE_BACK_N_FRAME) {
            back_n_frame_window_size++;
            SendFRAMEPacket((unsigned char*)&buf.frm, buf.size);
        }
    }
    // 若类型为接收：在队列中找到序号等于确认号的帧，弹出之前所有元素，移动窗口下界，如果有等待发送的帧，继续发送至窗口已满，移动窗口上界
    else if (messageType == MSG_TYPE_RECEIVE) {
        unsigned int ack = ntohl(((frame*)pBuffer)->head.ack);
        int i, j;
        for (i = 0; i < back_n_frame_deque.size() && i < back_n_frame_window_size; i++) {
            buffer buf = back_n_frame_deque[i];
            printf("<DEBUG BACK_N_FRAME RECEIVE>: buf.frm.head.seq = %d, ack = %d\n", ntohl(buf.frm.head.seq), ack);
            if (ntohl(buf.frm.head.seq) == ack) break;
        }
        if (i < back_n_frame_deque.size() && i < back_n_frame_window_size) {
            for (j = 0; j <= i; j++) {
                back_n_frame_deque.pop_front();
                back_n_frame_window_size--;
            }
        }
        for (i = back_n_frame_window_size; i < back_n_frame_deque.size(); i++) {
            if (back_n_frame_window_size < WINDOW_SIZE_BACK_N_FRAME) {
                buffer buf = back_n_frame_deque[i];
                back_n_frame_window_size++;
                SendFRAMEPacket((unsigned char*)&buf.frm, buf.size);
            }
            else {
                break;
            }
        }
    }
    // 若类型为超时：在队列中找到序号等于超时号的帧，重发窗口内所有元素<见问题与解决方案>
    else if (messageType == MSG_TYPE_TIMEOUT) {
        unsigned int seq = *(unsigned int*)pBuffer;
        int i, j;
        for (i = 0; i < back_n_frame_deque.size() && i < back_n_frame_window_size; i++) {
            buffer buf = back_n_frame_deque[i];
            printf("<DEBUG BACK_N_FRAME TIMEOUT>: buf.frm.head.seq = %d, seq = %d\n", ntohl(buf.frm.head.seq), seq);
            if (ntohl(buf.frm.head.seq) == seq) break;
        }
        if (i < back_n_frame_deque.size() && i < back_n_frame_window_size) {
            for (j = 0; j < back_n_frame_deque.size() && j < back_n_frame_window_size; j++) {
                buffer buf = back_n_frame_deque[j];
                SendFRAMEPacket((unsigned char*)&buf.frm, buf.size);
            }
        }
    }
    return 0;
}

/*
* 选择性重传测试函数
*/
int stud_slide_window_choice_frame_resend(char *pBuffer, int bufferSize, UINT8 messageType) {
    return 0;
}
