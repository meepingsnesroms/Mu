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
            case GUI_FORM_MAIN_WINDOW:
               CtlSetValue(getObjectPtr(GUI_CHECKBOX_DRIVER_ENABLED), guiConfigFile[DRIVER_ENABLED]);
               break;
               
            default:
               break;
         }
         return true;
         
      case frmOpenEvent:
         FrmDrawForm(FrmGetActiveForm());
         return true;
         
      case ctlExitEvent:
         switch(eventP->data.ctlExit.controlID){
            case GUI_CHECKBOX_DRIVER_ENABLED:
               guiConfigFile[DRIVER_ENABLED] = CtlGetValue(getObjectPtr(GUI_CHECKBOX_DRIVER_ENABLED));
               return true;
               
            case GUI_CHECKBOX_PATCH_INCONSISTENT_APIS:
               /*havent found any APIs that differ between OS 4 and 5 other than not existing at all*/
               return true;
               
            case GUI_BUTTON_SAVE_AND_REBOOT:
               writeConfigFile(guiConfigFile);
               SysReset();/*this is not the same as a full reset!*/
               stopExecution();
               
            default:
               return false;
         }
         
      case fldChangedEvent:
         switch(eventP->data.fldChanged.fieldID){
            case GUI_FIELD_CPU_SPEED:{
               uint16_t cpuSpeed = atoi(FldGetTextPtr(getObjectPtr(GUI_FIELD_CPU_SPEED)));
               
               /*make sure the CPU is at least on and not infinitely fast*/
               cpuSpeed = clamp(10, cpuSpeed, 999);
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
   Err unused;
   FormType* currentWindow = NULL;
   
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
            if(!MenuHandleEvent(0, &event, &unused))
               FrmDispatchEvent(&event);
   }
   while(event.eType != appStopEvent);
}
