#include <PalmOS.h>
#include <PalmCompatiblity.h>
#include <stdint.h>
#include <ugui.h>

/* register for direct video hardware access on OS 3.1 */
#define LSSA  *((         void **)0xFFFFFA00)
#define VPW   *((unsigned char  *)0xFFFFFA05)
#define LYMAX *((unsigned short *)0xFFFFFA0A)
#define LCXP  *((unsigned short *)0xFFFFFA18)
#define LCYP  *((unsigned short *)0xFFFFFA1A)
#define LCWCH *((unsigned short *)0xFFFFFA1C)
#define LBLCK *((unsigned char  *)0xFFFFFA1F)
#define PICF  *((unsigned char  *)0xFFFFFA20)
#define LPXCD *((unsigned char  *)0xFFFFFA25)
#define CKCON *((unsigned char  *)0xFFFFFA27)
#define LLBAR *((unsigned char  *)0xFFFFFA29)
#define LPOSR *((unsigned char  *)0xFFFFFA2D)
#define FRCM  *((unsigned char  *)0xFFFFFA31)
#define LGPMR *((unsigned short *)0xFFFFFA32)

#define SCREEN_WIDTH  160
#define SCREEN_HEIGHT 160

UG_GUI   uguiStruct;
uint8_t* framebuffer;
uint8_t* oldFramebuffer;
uint8_t  oldLpxcd;
uint8_t  oldPicf;
uint8_t  oldVpw;
uint8_t  oldLlbar;

void uguiDrawPixel(UG_S16 x, UG_S16 y, UG_COLOR color){
   //using 1bit grayscale
   int pixel = x + y * SCREEN_WIDTH;
   int byte = pixel / 8;
   int bit = pixel % 8;
   if(color){
      framebuffer[byte] |= 1 << (7 - bit);
   }
   else{
      framebuffer[byte] &= ~(1 << (7 - bit));
   }
}

void testerInit(){
   KeySetMask(~(keyBitPageUp | keyBitPageDown |
                keyBitHard1  | keyBitHard2 |
                keyBitHard3  | keyBitHard4 ));
   
   framebuffer = MemPtrNew(SCREEN_WIDTH * SCREEN_HEIGHT / 8);
   
   /* save old video regs */
   oldFramebuffer = LSSA;
   oldLpxcd = LPXCD;
   oldPicf = PICF;
   oldVpw = VPW;
   oldLlbar = LLBAR;
   
   /* set to full refresh */
   LPXCD = 0;
   
   /* display off */
   CKCON &= ~0x80;
   
   /* virtual page width now 20 bytes (160 greyscale pixels) */
   VPW    = 10;
   PICF  &= ~0x03;  /* switch to black and white mode */
   LLBAR  = 10;     /* line buffer now 20 bytes */
   
   /* register to control grayscale pixel oscillations */
   FRCM = 0xB9;//not listed in Dragonball VZ datasheet
   
   LSSA = framebuffer;
   
   /* let the LCD get to a 2 new frames (40ms delay) */
   SysTaskDelay(4);
   
   /* switch LCD back on */
   CKCON |= 0x80;
   
   UG_Init(&uguiStruct, uguiDrawPixel, 160, 160);
   
}

void testerExit(){
   LSSA = oldFramebuffer;
   LPXCD = oldLpxcd;
   PICF = oldPicf;
   VPW= oldVpw;
   LLBAR = oldLlbar;
}

Bool testerFrameLoop(){
   uint16_t buttons = KeyCurrentState();
   
   if(buttons & keyBitHard3){
      //back button
      return false;
   }
   
   UG_PutString(0, 0, "TestSuite running!");
   
   return true;
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

