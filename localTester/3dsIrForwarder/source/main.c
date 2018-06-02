#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <3ds.h>

#include "relay.h"


void safeUSleep(unsigned long long uSeconds){
   while(aptMainLoop() && uSeconds > 0){
      usleep(1);
      uSeconds--;
   }
}

void safeSleep(unsigned long long seconds){
   safeUSleep(seconds * 1000000);
}

int main(int argc, char* argv[]){
   bool hasConnected = false;
   uint32_t error;
   
   gfxInitDefault();
   consoleInit(GFX_TOP, NULL);
   
   printf("3dsIrForwarder Starting...");
   error = relayInit(9600/*baud*/);
   if(error == RELAY_ERROR_NONE){
      uint32_t currentIp = relayGetMyIp();
      
      printf("Init Success!\n");
      printf("My IP Address is:%d.%d.%d.%d\n", currentIp >> 24, currentIp >> 16 & 0xFF, currentIp >> 8 & 0xFF, currentIp & 0xFF);
   }
   
   //printf("Getting Encryption Certificates...\n");

   //loop as long as the status is not exit
   while(aptMainLoop()){
      u32 keysPressedLastFrame;
      
      //scan hid shared memory for input events
      hidScanInput();
      keysPressedLastFrame = hidKeysDown();
      
      //exit program
      /*
      if(keysPressedLastFrame & KEY_START)
         break;
      */
      
      if(!hasConnected){
         uint32_t error = relayAttemptConnection();
         if(error == RELAY_ERROR_NONE){
            hasConnected = true;
            printf("WIFI Connection Opened.\n");
         }
         else if(error == RELAY_ERROR_INVALID_PARAM){
            printf("Invalid Connection Attempted.\n");
         }
      }
      

      //flush + swap framebuffers and wait for VBlank
      gfxFlushBuffers();
      gfxSwapBuffers();
      gspWaitForVBlank();
   }

   relayExit();
	gfxExit();
   return 0;
}
