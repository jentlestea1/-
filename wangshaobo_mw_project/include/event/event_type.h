#ifndef EVENT_TYPE_H_
#define EVENT_TYPE_H_

#define EVT_RT_RECV_DATA 10
#define EVT_RT_RECV_DATA_ERR 20
#define EVT_1553_RECV_COMMAND 30
#define EVT_1553_RECV_COMMAND_ERR 40
#define EVT_1553_RETURN_DATA 50
#define EVT_1553_RETURN_DATA_ERR 60
#define EVT_WRITE_OVERFLOW 70
#define EVT_WAIT_OVER_TIME 80
#define EVT_RED_LIGHT_READ_ERR 90
#define EVT_APP_READ_BLOCK_OVERFLOW 100
#define EVT_APP_WRITE_ERR 101
#define EVT_APP_WRITE_BLOCK_OVERFLOW 110
#define EVT_RT_SECTION_WRITE_ERR 120
#define EVT_COMMAND_OVER_TIME 130
#define EVT_APP_DATA_SIZE_ERR 140
#define EVT_COUNT_ERR 150
#define EVT_CREATE_THREAD_ERR 160
#define EVT_VI_SIGNAL_ERR 170
#define EVT_VI_WAIT_ERR 170
#define EVT_VCAN_RECV_SIZE_FRAME_ERR 180
#define EVT_VCAN_RECV_BUF_OVERFLOW 190
#define EVT_VCAN_IRQ_OVERRUN 200
#define EVT_VCAN_RECV_PACKAGE_ERR 210
#define EVT_VCAN_RECV_BUF_LIMIT_ERR 220
#define EVT_VCAN_SEND_PACKAGE_ERR 230
#define EVT_HASH_TYPE_ERR 240
#define EVT_HASH_KEY_ERR 250
#define EVT_GET_SYNC_FLAG_ERR 260
#define EVT_WRITE_SYNC_FLAG_ERR 260
#define EVT_SYNC_FLAG_WRITE_ERR 270
#define EVT_DATA_NODE_GET_TIME_ERR 280
#define EVT_INVALID_SERIAL_FRAME 290

//RT端
#define RT_EVT_INITPORT_ERR 1000
#define RT_EVT_HANDLE_PACKAGE_ERR 1010
#define RT_EVT_RECV_PACKAGE_ERR 1020
#define RT_EVT_INVALID_SERIAL_FRAME 1030

#endif
