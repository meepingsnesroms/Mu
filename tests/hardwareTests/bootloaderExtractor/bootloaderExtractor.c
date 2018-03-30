#include <PalmOS.h>
//#include <PalmCompatibility.h>
//#include <System/DLServer.h>


static Boolean StartApplication(void);
static void    EventLoop(void);


static Boolean MyHandleEvent(EventType* event){
   
   return false;
}


/////////////////////////
// EventLoop
/////////////////////////

static void EventLoop(void){
  EventType event;
  Word      error;

  do{
     EvtGetEvent(&event, 1);
     
     if(!SysHandleEvent(&event)){
        if(!MenuHandleEvent(0, &event, &error)){
           if(!MyHandleEvent(&event)){
              FrmDispatchEvent(&event);
           }
        }
     }

  }
  while(event.eType != appStopEvent);
}

/************************************************************/
/** global init and cleanup                                 */
static Boolean StartApplication(void) {

  /* Mask hardware keys */
  KeySetMask(~(keyBitPageUp | keyBitPageDown | 
	       keyBitHard1  | keyBitHard2 | 
	       keyBitHard3  | keyBitHard4 ));

  SysRandom(TimGetTicks());

  return true;
}

static void StopApplication(void){

   
}

/////////////////////////
// PilotMain
/////////////////////////

DWord PilotMain(Word cmd, Ptr cmdBPB, Word launchFlags){
  if (cmd == sysAppLaunchCmdNormalLaunch){
    if(StartApplication()){
      EventLoop();
      StopApplication();
    }
  }
  return(0);
}

