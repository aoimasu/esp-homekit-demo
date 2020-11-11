// (c) 2018-2020 HomeAccessoryKid
// see udplogger.h for instructions
#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <espressif/esp_system.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>
#include <string.h>
#include <lwip/sockets.h>
#include <lwip/raw.h>
#include <udplogger.h>

SemaphoreHandle_t xUDPlogSemaphore = NULL;
_WriteFunction    *old_stdout_write;
char udplogstring[UDPLOGSTRING_SIZE]={0};
int  udplogmembers=0,udploglSocket,udplogstring_len=0;
struct sockaddr_in udplogsClntAddr;

void udplog_send(void *pvParameters){
    int oldtime=0,timeout=1,n,holdoff=20*HOLDOFF; //should represent 2 seconds after trigger
    unsigned int length;
    struct sockaddr_in sLocalAddr;
    char buffer[2];

    while (sdk_wifi_station_get_connect_status() != STATION_GOT_IP) vTaskDelay(20); //Check if we have an IP every 200ms

    udploglSocket = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    memset((char *)&sLocalAddr, 0, sizeof(sLocalAddr));
    memset((char *)&udplogsClntAddr,  0, sizeof(udplogsClntAddr));
    /*Destination*/
    udplogsClntAddr.sin_family = AF_INET;
    udplogsClntAddr.sin_len = sizeof(udplogsClntAddr);
    udplogsClntAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST); // inet_addr("255.255.255.255");  or htonl(INADDR_BROADCAST);
    udplogsClntAddr.sin_port =htons(45677);
    /*Source*/
    sLocalAddr.sin_family = AF_INET;
    sLocalAddr.sin_len = sizeof(sLocalAddr);
    sLocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sLocalAddr.sin_port =htons(45676);
    lwip_bind(udploglSocket, (struct sockaddr *)&sLocalAddr, sizeof(sLocalAddr));

    lwip_sendto(udploglSocket, INIT, strlen(INIT), 0, (struct sockaddr *)&udplogsClntAddr, sizeof(udplogsClntAddr));
    fd_set   rset;
    FD_ZERO(&rset); // clear the descriptor set
    struct timeval tv = { 0, 10000 }; /* 10 millisecond cycle time for the while(1) loop */

    while (1) {
        FD_SET(udploglSocket,&rset);
        select(udploglSocket+1, &rset, NULL, NULL, &tv); //is a delay of 10ms
        if (FD_ISSET(udploglSocket, &rset)) {
            length = sizeof(udplogsClntAddr);
            n = recvfrom(udploglSocket, (char *)buffer, 2, 0, ( struct sockaddr *) &udplogsClntAddr, &length);
            if (n==1) {udplogmembers=1; holdoff=0; timeout=(int)buffer[0];oldtime=sdk_system_get_time()/1000;}
        }
        if ((holdoff<=0 && udplogstring_len) || (holdoff<HOLDOFF && udplogstring_len>UDPLOGSTRING_SIZE*3/4)) {
            if ((sdk_system_get_time()/1000-oldtime)>timeout*1000) udplogmembers=0;
            if( xSemaphoreTake( xUDPlogSemaphore, ( TickType_t ) 1 ) == pdTRUE ) {
                UDPLOGFLUSHNOFIT(UDPLOGSTRING_SIZE);
                xSemaphoreGive( xUDPlogSemaphore );
                holdoff=HOLDOFF;
            }
        }
        if (holdoff<=0) holdoff=HOLDOFF;
        holdoff--;
    }
}

#ifdef  UDPLOG_PRINTF_TO_UDP
#pragma message "UDPLOG_PRINTF_TO_UDP activated"
 static int new_stdout_write(struct _reent *r, int fd, const void *ptr, size_t len) {
    #ifdef  UDPLOG_PRINTF_ALSO_SERIAL
     old_stdout_write(NULL,0,ptr,len);
    #endif  //ifdef UDPLOG_PRINTF_ALSO_SERIAL
    if (xSemaphoreTake( xUDPlogSemaphore, ( TickType_t ) 1 ) == pdTRUE) {
        UDPLOGFLUSHNOFIT(len);
        if (len>UDPLOGSTRING_SIZE) { //normally never used since printf only sends in chunks of 128 bytes at a time
            if (udplogmembers) lwip_sendto(udploglSocket, ptr, len,
                                             0, (struct sockaddr *)&udplogsClntAddr,sizeof(udplogsClntAddr));
        } else {
            memcpy(udplogstring+udplogstring_len,ptr,len); udplogstring_len+=len;
        }
        xSemaphoreGive( xUDPlogSemaphore );
    }
    return len;
 }
#endif  //ifdef UDPLOG_PRINTF_TO_UDP

void udplog_init(int prio) {
    old_stdout_write=get_write_stdout();
    #ifdef  UDPLOG_PRINTF_TO_UDP
     set_write_stdout(new_stdout_write);
    #endif  //ifdef UDPLOG_PRINTF_TO_UDP
    
    xUDPlogSemaphore   = xSemaphoreCreateMutex();
    xTaskCreate(udplog_send, "udplog", 320, NULL, prio, NULL);
}
