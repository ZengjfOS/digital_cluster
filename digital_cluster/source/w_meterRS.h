/**
  Copyright (c) 2010 Freescale Semiconductor
  
  \file       w_meters.h
  \author     Freescale Semiconductor
  \author     DB, r54930
  \version	  1.0
  \date  	  $Date: 2011-11-03  
  
  * History:  03/Nov/2011 - Initial Version

* Copyright (c) 2010, Freescale, Inc.  All rights reserved.
*
*
* No part of this document must be reproduced in any form - including copied,
* transcribed, printed or by any electronic means - without specific written
* permission from Freescale Semiconductor.
*
   
*/


#ifndef	_W_METERRS_H_
#define	_W_METERRS_H_

#include "VG/openvg.h"

void meterRS_init(void);
void meterRSSwapBuffer(uint8_t layer);
void meterRS_drawSpeed(VGfloat SpeedValue, VGfloat scale1, uint8_t mode);
void meterRS_drawRPM(VGfloat RPMValue, VGfloat scale1, uint8_t mode);
extern VGfloat SpeedCurrentPosV;
extern VGfloat RPMCurrentPosV;
extern uint8_t TransformFrames;
extern uint8_t TransformFrames2;
extern uint32_t VGPerformance[4];
#endif
