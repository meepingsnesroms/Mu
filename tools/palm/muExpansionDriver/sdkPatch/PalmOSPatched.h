/******************************************************************************
 *
 * Copyright (c) 1994-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 *
 * File: PalmOS.h
 *
 * Release: Palm OS SDK 4.0 (63220)
 *
 * Description:
 *		Public includes for the whole core OS, generally apps/panels/libraries
 *    only need to include this.
 *
 * History:
 *   	6/ 6/95  RM - Created by Ron Marianetti
 *		2/24/97  RF - Changed to handle c++ versions.
 *   	4/24/97  SL - Changes for PalmOS 2.0 SDK
 *   	5/19/97  SL - Now includes <types.h> only in MWERKS environments
 *    7/14/99  bob - Created from Pilot.h
 *
 *****************************************************************************/

#ifndef	__PALMOS_H__ 
#define	__PALMOS_H__ 

#include <PalmTypes.h>
#include <SystemPublic.h>
#include <UIPublic.h>

#include "CoreTrapsPatch.h"
#include "PceNativeCall.h"
#include "ByteOrderUtils.h"
#include "PalmChars.h"
#include "PenInputMgr.h"

#endif	// __PALMOS_H__
