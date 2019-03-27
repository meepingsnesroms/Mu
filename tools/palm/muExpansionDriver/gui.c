/*dont include this anywhere else*/
#include "MuExpDriverRsc.h"

#include "sdkPatch/PalmOSPatched.h"
#include <stdint.h>
#include <stdlib.h>

#include "debug.h"
#include "config.h"
#include "palmOsPriv.h"
#include "palmGlobalDefines.h"


/*global varibles are OK here because the GUI is only run when the app is fully loaded*/
static uint32_t* guiConfigFile;


static void* getObjectPtr(UInt16 objectId){
   FormType* form = FrmGetActiveForm();
   return FrmGetObjectPtr(form, FrmGetObjectIndex(form, objectId));
}

static Boolean appHandleEvent(EventPtr eventP){
   switch(eventP->eType){
      case frmLoadEvent:
         FrmSetActiveForm(FrmInitForm(eventP->data.frmLoad.formID));
         
         /*init form controls*/
         switch(eventP->data.frmLoad.formID){
            case GUI_FORM_MAIN_WINDOW:{
                  MemHandle cpuSpeedTextHandle = MemHandleNew(4);
                  MemPtr cpuSpeedText;
               
                  cpuSpeedText = MemHandleLock(cpuSpeedTextHandle);
                  StrPrintF(cpuSpeedText, "%ld", guiConfigFile[BOOT_CPU_SPEED]);
                  MemHandleUnlock(cpuSpeedTextHandle);
                  
                  CtlSetValue(getObjectPtr(GUI_CHECKBOX_DRIVER_ENABLED), guiConfigFile[DRIVER_ENABLED]);
                  CtlSetValue(getObjectPtr(GUI_CHECKBOX_PATCH_INCONSISTENT_APIS), guiConfigFile[PATCH_INCONSISTENT_APIS]);
                  FldSetTextHandle(getObjectPtr(GUI_FIELD_CPU_SPEED), cpuSpeedTextHandle);
                  if(guiConfigFile[SAFE_MODE])
                     FrmShowObject(FrmGetActiveForm(), GUI_LABEL_SAFE_MODE);
                  else
                     FrmHideObject(FrmGetActiveForm(), GUI_LABEL_SAFE_MODE);
               }
               break;
               
            default:
               break;
         }
         return true;
         
      case frmOpenEvent:
         FrmDrawForm(FrmGetActiveForm());
         return true;
         
      case ctlSelectEvent:
         switch(eventP->data.ctlExit.controlID){
            case GUI_CHECKBOX_DRIVER_ENABLED:
               guiConfigFile[DRIVER_ENABLED] = CtlGetValue(getObjectPtr(GUI_CHECKBOX_DRIVER_ENABLED));
               return true;
               
            case GUI_CHECKBOX_PATCH_INCONSISTENT_APIS:
               guiConfigFile[PATCH_INCONSISTENT_APIS] = CtlGetValue(getObjectPtr(GUI_CHECKBOX_PATCH_INCONSISTENT_APIS));
               return true;
               
            case GUI_BUTTON_SAVE_AND_REBOOT:
               /*clear safe mode flag if its set and test the new config*/
               guiConfigFile[SAFE_MODE] = false;
               writeConfigFile(guiConfigFile);
               debugLog("Attempting reboot!\n");
               SysReset();
               return true;
               
            default:
               return false;
         }
         
      case fldChangedEvent:
         switch(eventP->data.fldChanged.fieldID){
            case GUI_FIELD_CPU_SPEED:{
                  uint16_t cpuSpeed;
                  Char* fieldText = FldGetTextPtr(getObjectPtr(GUI_FIELD_CPU_SPEED));
                  
                  if(fieldText && fieldText[0] != '\0')
                     cpuSpeed = clamp(10, atoi(fieldText), 999);/*make sure the CPU is at least on and not infinitely fast*/
                  else
                     cpuSpeed = 100;

                  guiConfigFile[BOOT_CPU_SPEED] = cpuSpeed;
                  return true;
            }
               
            default:
               return false;
         }
         
      default:
         return false;
   }
}

void showGui(uint32_t* configFile){
   EventType event;
   
   debugLog("Attempting to load GUI.\n");
   
   /*popup warning dialog*/
   if(!configFile[USER_WARNING_GIVEN]){
      /*continue if user pressed "Emulator"*/
      if(!FrmAlert(GUI_ALERT_USER_WARNING)){
         configFile[USER_WARNING_GIVEN] = true;
         configFile[DRIVER_ENABLED] = true;
      }
   }
   
   /*give GUI pointer to config file*/
   guiConfigFile = configFile;
   
   /*set starting window*/
   FrmGotoForm(GUI_FORM_MAIN_WINDOW);
   
   do{
      EvtGetEvent(&event, evtWaitForever);
      
      if(!appHandleEvent(&event))
         if(!SysHandleEvent(&event))
               FrmDispatchEvent(&event);
   }
   while(event.eType != appStopEvent);
}
