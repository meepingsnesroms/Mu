#include <stdio.h>
#include <string.h>
#include <3ds.h>


int main(int argc, char* argv[]){
   gfxInitDefault();
   consoleInit(GFX_TOP, NULL);
   
   /*
   printf("3dsIrForwarder Starting...");
   printf("Getting Encryption Certificates...");
   printf("Opening WIFI Connection...");
   printf("WIFI Connection Opened...");
   printf("Opening IR Connection...");
   printf("IR Connection Opened...");
   printf("Init Success!");
   */

   //Loop as long as the status is not exit
   while(aptMainLoop()){
      //Scan hid shared memory for input events
      hidScanInput();
      
      

      //Flush + swap framebuffers and wait for VBlank
      gfxFlushBuffers();
      gfxSwapBuffers();
      gspWaitForVBlank();
   }

   
	gfxExit();
   return 0;
}
