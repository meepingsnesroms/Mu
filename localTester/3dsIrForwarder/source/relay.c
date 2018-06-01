#include <3ds.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000


bool relayInited = false;
static u32* irBuffer = NULL;
static u32* socketBuffer = NULL;
static s32  sock = -1;
static struct sockaddr_in client;
static struct sockaddr_in server;


static void failCleanup(){
   if(irBuffer)
      free(irBuffer);
   if(socketBuffer)
      free(socketBuffer);
}

bool relayInit(){
   if(!relayInited){
      int ret;
      
      socketBuffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
      irBuffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
      if(!irBuffer || !socketBuffer){
         failCleanup();
         return false;
      }
      
      ret = iruInit(irBuffer, SOC_BUFFERSIZE);
      if(ret != RL_SUCCESS){
         failCleanup();
         return false;
      }
      
      ret = socInit(socketBuffer, SOC_BUFFERSIZE)
      if(ret != RL_SUCCESS){
         failCleanup();
         return false;
      }
      
      sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
      csock = accept(sock, (struct sockaddr *) &client, &clientlen);
      
      relayInited = true;
   }
   
   return true;//already inited return success
}

void relayExit(){
   if(relayInited){
      iruExit();
      socExit();
      free(irBuffer);
      free(socketBuffer);
      
      relayInited = false;
   }
}

void relayRun
