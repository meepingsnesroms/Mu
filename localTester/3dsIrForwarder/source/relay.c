#include <3ds.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "main.h"
#include "relay.h"


#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000


bool                      relayInited = false;
bool                      irInited;
bool                      socketInited;
static u32*               irBuffer;
static u32*               socketBuffer;
static s32                sock;
static s32                csock;
static struct sockaddr_in client;
static struct sockaddr_in server;
static u32                clientLen = sizeof(client);

static const char loginQuery[] = "I am a Palm OS command transmitter.";
static const char loginAnswer[] = "I want to login to a Palm OS Device!";


static inline int8_t makeCodeFromBaud(uint32_t baud){
   switch(baud){
      case 115200:
         return 3;
      case 96000:
         return 4;
      case 72000:
         return 5;
      case 57600:
         return 14;
      case 48000:
         return 6;
      case 38400:
         return 15;
      case 36000:
         return 7;
      case 24000:
         return 8;
      case 19200:
         return 16;
      case 18000:
         return 9;
      case 12000:
         return 10;
      case 9600:
         return 11;
      case 7200:
         return 17;
      case 6000:
         return 12;
      case 4800:
         return 18;
      case 3000:
         return 13;
   }
   
   return -1;
}

static void cleanup(){
   if(csock >= 0)
      close(csock);
   if(sock >= 0)
      close(sock);
   if(irInited)
      iruExit();
   if(socketInited)
      socExit();
   if(irBuffer)
      free(irBuffer);
   if(socketBuffer)
      free(socketBuffer);
}

uint32_t relayInit(uint32_t irBaud){
   if(!relayInited){
      int8_t irBaudCode = makeCodeFromBaud(irBaud);
      Result systemCallValue;
      int ret;
      
      if(irBaudCode == -1)
         return RELAY_ERROR_INVALID_PARAM;
      
      irInited = false;
      socketInited = false;
      
      socketBuffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
      irBuffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
      if(!irBuffer || !socketBuffer){
         cleanup();
         return RELAY_ERROR_OUT_OF_MEMORY;
      }
      
      systemCallValue = iruInit(irBuffer, SOC_BUFFERSIZE);
      if(systemCallValue != RL_SUCCESS){
         cleanup();
         return RELAY_ERROR_IR_DRIVER;
      }
      irInited = true;
      
      systemCallValue = IRU_SetBitRate((uint8_t)irBaudCode);
      if(systemCallValue != RL_SUCCESS){
         cleanup();
         return RELAY_ERROR_IR_DRIVER;
      }
      
      systemCallValue = socInit(socketBuffer, SOC_BUFFERSIZE);
      if(systemCallValue != RL_SUCCESS){
         cleanup();
         return RELAY_ERROR_WIFI_DRIVER;
      }
      socketInited = true;
      
      sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
      if(sock < 0){
         cleanup();
         return RELAY_ERROR_CANT_OPEN_SOCKET;
      }
      
      memset(&server, 0, sizeof(server));
      memset(&client, 0, sizeof(client));
      
      server.sin_family = AF_INET;
      server.sin_port = htons(80);
      server.sin_addr.s_addr = gethostid();
      
      ret = bind(sock, (struct sockaddr*)&server, sizeof(server));
      if(ret != 0){
         cleanup();
         return RELAY_ERROR_CANT_BIND_SOCKET;
      }
      
      fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
      
      ret = listen(sock, 5);
      if(ret != 0){
         cleanup();
         return RELAY_ERROR_CANT_LISTEN_SOCKET;
      }
      
      relayInited = true;
   }
   
   return RELAY_ERROR_NONE;//already inited returns success
}

void relayExit(){
   if(relayInited)
      cleanup();
}

uint32_t relayAttemptConnection(){
   if(relayInited){
      uint8_t commandBuffer[sizeof(loginAnswer)] = {0};
      uint32_t bytes;
      
      if(csock)
         close(csock);
      
      csock = accept(sock, (struct sockaddr*)&client, &clientLen);
      if(csock < 0)
         return RELAY_ERROR_NO_CLIENTS;
      
      //check if this is an internet scanner bot or a login
      send(csock, loginQuery, strlen(loginQuery), 0);
      
      //check if responce matches
      safeSleep(3);
      bytes = recv(csock, commandBuffer, sizeof(loginAnswer), 0);
      if(bytes != sizeof(loginAnswer)){
         cleanup();
         return RELAY_ERROR_TIMEOUT;
      }
      
      if(memcmp(commandBuffer, loginAnswer, sizeof(loginAnswer)) != 0){
         //responce is wrong, this is a scanner, DDOS(very unlikely) or clueless web user(most likely, they tried to open the forwarder as a web page)
         close(csock);
         return RELAY_ERROR_INVALID_PARAM;
      }
      
      return RELAY_ERROR_NONE;
   }

   return RELAY_ERROR_NOT_INITED;
}

uint32_t relayGetMyIp(){
   return gethostid();
}

void relayRun(){
   //simply copys all data from WIFI to IR and IR to WIFI
   uint8_t swapBuffer[1024];
   uint32_t bytes;
   
   //read from WIFI
   bytes = recv(csock, swapBuffer, sizeof(swapBuffer), 0);
   //forward to IR
   iruSendData(swapBuffer, bytes, false/*wait*/);
   
   //read from IR
   iruRecvData(swapBuffer, sizeof(swapBuffer), 0/*flag*/, &bytes, false/*wait*/);
   //forward to WIFI
   send(csock, swapBuffer, bytes, 0);
}
