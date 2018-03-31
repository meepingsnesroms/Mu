#include <PalmOS.h>
#include <PalmCompatiblity.h>
#include <stdint.h>
#include <ugui.h>

uint8_t* framebuffer;


void uguiDrawPixel(UG_S16,UG_S16,UG_COLOR){
   
}

void testerInit(){
   framebuffer = *((uint8_t*)0xFFFFFA00);//lssa
   KeySetMask(~(keyBitPageUp | keyBitPageDown |
                keyBitHard1  | keyBitHard2 |
                keyBitHard3  | keyBitHard4 ));
   
}



Bool testerFrameLoop(){
   
}

void testerExit(){
   
}

DWord PilotMain(Word cmd, Ptr cmdBPB, Word launchFlags){
   Bool running = true;
   testerInit()
   
   while(running){
      running = testerFrameLoop();
      usleep(333333);//30 fps
   }
   
   testerExit();
   return(0);
}

