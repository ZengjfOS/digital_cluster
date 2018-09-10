/**
  Copyright (c) 2009 Freescale Semiconductor
* 
* No part of this document must be reproduced in any form - including copied,
* transcribed, printed or by any electronic means - without specific written
* permission from Freescale Semiconductor.
*
* Auth: D.Bogavac 2011-11-03
*
*/

/* GPU includes */
#include "VG/openvg.h"
#include "VG/vgu.h"
#include "EGL/egl.h"
//#include "EGL/eglfsl.h"

/*Util includes */
#include "PVGLoader.h"
//#include "common.h"
//#include "Graphics.h"

/* resources includes */
#include "PointerL.h"
#include "CPshort.h"
#include "SPH.h"

/* defines */
#define ACCMAX 		2.5f // max acceleration angle increment


/* Variables */
//EGLDisplay			meterRSdsp;
//EGLConfig			meterRScfg;
//EGLContext			meterRSctx;
//EGLSurface			meterRSsurf[2];
//uint32_t 			meterRSBuffer[2];
uint8_t BfFlag = 0, TestCounter = 0;
VGfloat angle1_latchV = 0, angle2_latch = 0, PosAngle = 0;
VGfloat Rangle1_latchV = 0, Rangle2_latch = 0, RPosAngle = 0;
	
uint8_t DrawHighLights = 1;
VGfloat DHL = -60;
uint8_t DHLBufferFlag = 0;

PVG_Element_t PointerL, CPshort, SPH;
float SPEEDMAX = 2.0f;  // degrees of rotation / frame ex. 12deg/frame x 60 = 720deg/s.

VGfloat SpeedCurrentPosV = -60.0f;
VGfloat RPMCurrentPosV = -60.0f;
VGfloat SpeedCurrentSpeedV = 0.0f;
VGfloat RPMCurrentSpeedV = 0.0f;
VGfloat angleLatchV[6]={0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};
int BufferFlag;
uint32_t VGPerformance[6];
uint8_t TransformFrames = 0;
uint8_t TransformFrames2 = 0;

VGImage Center_r;
VGImage Blank, BlankS, BlankH;
VGfloat ManualSpeed = 0;
VGfloat SetSpeed = -60.0f;
VGfloat SpeedInc = 0;

VGfloat NeedlePhysicsV(VGfloat CurrentPosition, VGfloat CurrentSpeed, VGfloat TargetPosition, uint8_t user);

extern const char n_center_r_Bitmap[];
extern const char blank_Bitmap[];
//extern Graphics_Object_t frameBuffer;
/*
void meterRSSwapBuffer(uint8_t layer)
{
	uint32_t temp;
	VGPerformance[4] = PIT.CH[2].CVAL.R;
	//vgFinish(); // eglSwapBuffers are expected to have an implicit vgFinish operation.
	//frameBuffer.address = meterRSBuffer[BfFlag];
	//Display_InitLayer(layer,&frameBuffer,192,74);
	DCU_LayerAddress(1) = meterRSBuffer[BfFlag];
	DCU_LayerEnable(1);
	BufferFlag^=1;
	BfFlag^=1;
	
	eglMakeCurrent(meterRSdsp, meterRSsurf[BfFlag], meterRSsurf[BfFlag], meterRSctx);
	temp = (VGPerformance[0] - PIT.CH[2].CVAL.R) / 124;
	if (VGPerformance[1] < temp) VGPerformance[1] = temp;
	if (VGPerformance[2] > temp) VGPerformance[2] = temp;
	VGPerformance[3] = temp;
	VGPerformance[5] = (VGPerformance[0] - VGPerformance[4]) / 124; 		
}
*/

void meterRS_init(void){

float color[4]={0,0,0,1.0};		
vgSetfv(VG_CLEAR_COLOR, 4, color);				
				

	Center_r = vgCreateImage(VG_sRGBA_8888,62,62,VG_IMAGE_QUALITY_BETTER);
	Blank = vgCreateImage(VG_sRGBA_8888,80,230,VG_IMAGE_QUALITY_FASTER);
	BlankS = vgCreateImage(VG_sRGBA_8888,16,60,VG_IMAGE_QUALITY_FASTER);
	BlankH = vgCreateImage(VG_sRGBA_8888,6,16,VG_IMAGE_QUALITY_FASTER);
	
	vgImageSubData(Center_r, n_center_r_Bitmap,62*4, VG_sARGB_8888, 0,0,62,62);
	vgImageSubData(Blank, blank_Bitmap,80*4, VG_sABGR_8888, 0,0,80,230);
	vgImageSubData(BlankS, blank_Bitmap,16*4, VG_sABGR_8888, 0,0,16,60);
	vgImageSubData(BlankH, blank_Bitmap,6*4, VG_sABGR_8888, 0,0,6,16);
	
	//meterRSsurf[2] = eglCreatePbufferFromClientBuffer(meterRSdsp, EGL_OPENVG_IMAGE, (EGLClientBuffer)Blank, meterRScfg, surfaceAttrib);
	
	PVG_Init();
	PointerL = PVG_Load(&PointerL_PVG);
	CPshort = PVG_Load(&CPshort_PVG);
	SPH = PVG_Load(&SPH_PVG);
	VGPerformance[2] = 0xFFFFFFFF; // this var is used to latch the fastest frame. So we init it here to something large
}

void meterRS_drawSpeed(VGfloat SpeedValue, VGfloat scale1, uint8_t mode){

	//static VGfloat angle1_latchV = 0, angle2_latch = 0, PosAngle = 0;
	
// Calculate the next position using a PD regulator that is not working properly yet 
	SPEEDMAX = 1.0f;
	SpeedCurrentSpeedV = NeedlePhysicsV(SpeedCurrentPosV, SpeedCurrentSpeedV, SpeedValue, 0);	
	if (SpeedCurrentPosV > SpeedValue) SpeedCurrentPosV -= SpeedCurrentSpeedV; 
	else SpeedCurrentPosV += SpeedCurrentSpeedV;
	PosAngle = SpeedCurrentPosV;
	//PosAngle = -160.0;
	
// Setup matrix etc...
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
	vgSeti(VG_IMAGE_MODE, VG_DRAW_IMAGE_NORMAL);
	vgSeti(VG_IMAGE_QUALITY, VG_IMAGE_QUALITY_BETTER);
	vgSeti(VG_BLEND_MODE, VG_BLEND_SRC);
	vgLoadIdentity();

/*	if (TransformFrames) //erase the remaining small pointer if it exists 
	{
	// draw the blank bitmap and thereby clear the needle.
		vgTranslate(208.0f,103.0f);
		vgRotate(angleLatchV[BufferFlag + 3]);
		//vgScale(scale1, scale1);
		vgTranslate(-8.0f, -187.0f);
		vgDrawImage(BlankS);
		TransformFrames--;
	}
*/
// clear center area and but only for big pointer and if center function is RPM 
	if (mode < 5) 
	{
		vgClear(0,0,448,353);
	}

// in case we just switched mode to small pointer, we need to erase the remaining RPM pointer 
	else if (TransformFrames2) 
	{
		vgClear(0,0,448,353);
		TransformFrames2--;
	}
/*
//draw the blank bitmap and thereby clear the needle.	
	vgTranslate(208.0f,103.0f);
	vgRotate(angleLatchV[BufferFlag]);
	angleLatchV[BufferFlag] = SpeedCurrentPosV;
	vgTranslate(-8.0f, -187.0f);
	vgDrawImage(BlankS);
	
// we need to erase the RPM pointer while in transform transition. e.g mode 3 and while it's big enough
// to be outside of the above vgClear region.
// TransformFrames is set = 2 after we reach the transform end, and switch to mode 0. To erase the 2 last frames of approaching pointer 	
	if (((mode == 3) && (scale1 > 0.65f)) || (TransformFrames)) 
	{
		vgLoadIdentity();
		vgTranslate(208.0f,103.0f);
		vgRotate(angleLatchV[BufferFlag + 3]);
		//angleLatchV[BufferFlag] = SpeedCurrentPosV;
		vgTranslate(-8.0f, -187.0f);
		vgDrawImage(BlankS);
		if (TransformFrames) TransformFrames--;
	}
*/	
// we need to erase the Speed pointer while in transform transition. e.g mode 0 and while it's big enough
// to be outside of the above vgClear region	
/*	if ((mode == 0) && (scale1 < 1.0f))
	{
		vgLoadIdentity();
		vgTranslate(208.0f,103.0f);
		vgRotate(angleLatchV[BufferFlag]);
		vgTranslate(-8.0f, -187.0f);
		vgDrawImage(BlankS);
	}
*/	
// Draw the center cap 
	if (mode == 0)
	{
		vgLoadIdentity();
		vgTranslate(208.0f,103.0f);
		vgScale(scale1, scale1);	
		vgTranslate(-31.0f, -31.0f);
		vgDrawImage(Center_r);
	}
// set the path matrix and blend mode
	vgSeti(VG_BLEND_MODE, VG_BLEND_SRC_OVER);
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
// draw the vector part of the needle	
	vgLoadIdentity();
	vgTranslate(208.0f, 103.0f);
	vgRotate(PosAngle);
	if (mode == 0) vgScale(scale1, scale1);
	vgTranslate(0, -97.0);
	if (mode) PVG_Draw(CPshort,0,0); // draw the short pointer  
	else PVG_Draw(PointerL,0,0); //	Draw the long pointer
	
// in case highligt ticks are on, draw or clear 1 tick	
	if (DrawHighLights)
	{
		if (PosAngle < (DHL - 1.8f))
		{
			
			vgLoadIdentity();
			vgTranslate(208.0f, 103.0f);
			vgRotate(DHL - 2.0f);
			//vgScale(scale1, scale1);
			vgTranslate(-208, -103.0);
			PVG_Draw(SPH,0,0); // draw the tick
			//PVG_Draw(PointerL,0,0);
			 
			if ((DHLBufferFlag & 0xF) == 2) 
			{
				DHL -= 2.0f;
				DHLBufferFlag &= 0xF0;
			}
			else DHLBufferFlag += 1;
		}
		else if (PosAngle > (DHL))
		{
		// Setup matrix etc...
			vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
			vgSeti(VG_IMAGE_MODE, VG_DRAW_IMAGE_NORMAL);
			vgSeti(VG_IMAGE_QUALITY, VG_IMAGE_QUALITY_BETTER);
			vgSeti(VG_BLEND_MODE, VG_BLEND_SRC);
			vgLoadIdentity();
			vgTranslate(208.0f,103.0f);
			vgRotate(DHL);
			vgTranslate(-3.0f, -204.0f);
			vgDrawImage(BlankH);
			if ((DHLBufferFlag & 0xF0) == 0x20) 
			{
				DHL += 2.0f;
				DHLBufferFlag &= 0xF;
			}
			else DHLBufferFlag += 0x10;

		}
		
	}
}

void meterRS_drawRPM(VGfloat RPMValue, VGfloat scale1, uint8_t mode){

	//static VGfloat Rangle1_latchV = 0, Rangle2_latch = 0, RPosAngle = 0;
	
// Calculate the next position using a PD regulator that is not working properly yet 
	SPEEDMAX = 10.0f;
	RPMCurrentSpeedV = NeedlePhysicsV(RPMCurrentPosV, RPMCurrentSpeedV, RPMValue, 1);	
	if (RPMCurrentPosV > RPMValue) RPMCurrentPosV -= RPMCurrentSpeedV; 
	else RPMCurrentPosV += RPMCurrentSpeedV;
	RPosAngle = RPMCurrentPosV;
	if (RPMCurrentPosV == RPMValue) RPMCurrentSpeedV = 0; 

	
// Setup matrix etc...
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
	vgSeti(VG_IMAGE_MODE, VG_DRAW_IMAGE_NORMAL);
	vgSeti(VG_IMAGE_QUALITY, VG_IMAGE_QUALITY_BETTER);
	vgSeti(VG_BLEND_MODE, VG_BLEND_SRC);
	vgLoadIdentity();
	angleLatchV[BufferFlag + 3] = RPMCurrentPosV;
// Draw the center cap 
	if (mode == 0)
	{
		vgLoadIdentity();
		vgTranslate(208.0f,103.0f);
		vgScale(scale1, scale1);	
		vgTranslate(-31.0f, -31.0f);
		vgDrawImage(Center_r);
	}
// set the path matrix and blend mode
	vgSeti(VG_BLEND_MODE, VG_BLEND_SRC_OVER);
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
// draw the vector part of the needle	
	vgLoadIdentity();
	vgTranslate(208.0f, 103.0f);
	vgRotate(RPosAngle);
	vgScale(scale1, scale1);
	vgTranslate(0, -97.0);
	if (mode) PVG_Draw(CPshort,0,0); // draw the short pointer  
	else PVG_Draw(PointerL,0,0); //	Draw the long pointer

}

void meterRS_drawSpeedRPM(VGfloat SpeedValue, VGfloat scale1, uint8_t mode, VGfloat RPMValue, VGfloat scale2, uint8_t mode2){

	//static VGfloat Rangle1_latchV = 0, Rangle2_latch = 0, RPosAngle = 0;
	//static VGfloat angle1_latchV = 0, angle2_latch = 0, PosAngle = 0;
	
// Calculate the next position using a PD regulator that is not working properly yet
	if (SpeedCurrentPosV != SpeedValue)
	{
		SPEEDMAX = 1.0f;
		SpeedCurrentSpeedV = NeedlePhysicsV(SpeedCurrentPosV, SpeedCurrentSpeedV, SpeedValue, 0);	
		if (SpeedCurrentPosV > SpeedValue) SpeedCurrentPosV -= SpeedCurrentSpeedV; 
		else SpeedCurrentPosV += SpeedCurrentSpeedV;
	}
	PosAngle = SpeedCurrentPosV;
	
// Calculate the next position using a PD regulator that is not working properly yet 
	if (RPMCurrentPosV != RPMValue)
	{
		SPEEDMAX = 12.0f;
		RPMCurrentSpeedV = NeedlePhysicsV(RPMCurrentPosV, RPMCurrentSpeedV, RPMValue,0);	
		if (RPMCurrentPosV > RPMValue) RPMCurrentPosV -= RPMCurrentSpeedV; 
		else RPMCurrentPosV += RPMCurrentSpeedV;
	}
	RPosAngle = RPMCurrentPosV;

	
// Setup matrix etc...
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
	vgSeti(VG_IMAGE_MODE, VG_DRAW_IMAGE_NORMAL);
	vgSeti(VG_IMAGE_QUALITY, VG_IMAGE_QUALITY_BETTER);
	vgSeti(VG_BLEND_MODE, VG_BLEND_SRC);
/*	vgLoadIdentity();
	
// draw the blank bitmap and thereby clear the needle.
	vgTranslate(208.0f,103.0f);
	vgRotate(angleLatchV[BufferFlag + 3]);
	angleLatchV[BufferFlag + 3] = RPMCurrentPosV;
// the RPM part	
	if (mode2) //e.g. are we drawing the small speed pointer or the big one 
	{
		vgScale(scale2, scale2);
		vgTranslate(-40.0f, -192.0f);
		vgDrawImage(BlankS);
	}
	else {
		vgScale(scale2, scale2);
		vgTranslate(-40.0f, -192.0f);
		vgDrawImage(Blank);
	}
// the Speed part
	vgLoadIdentity();
	vgTranslate(208.0f,103.0f);
	vgRotate(angleLatchV[BufferFlag]);
	angleLatchV[BufferFlag] = SpeedCurrentPosV;
	vgScale(scale1, scale1);
	vgTranslate(-40.0f, -187.0f);
	vgDrawImage(BlankS);
*/
	vgClear(0,0,416,308);
// Draw the center cap 
	if (mode2 == 0)
	{
		vgLoadIdentity();
		vgTranslate(208.0f,103.0f);
		vgScale(scale2, scale2);	
		vgTranslate(-31.0f, -31.0f);
		vgDrawImage(Center_r);
	}
// set the path matrix and blend mode
	vgSeti(VG_BLEND_MODE, VG_BLEND_SRC_OVER);
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
// draw the vector part of the needle	
	vgLoadIdentity();
	vgTranslate(208.0f, 103.0f);
	vgRotate(RPosAngle);
	vgScale(scale2, scale2);
	vgTranslate(0, -97.0);
	if (mode2) PVG_Draw(CPshort,0,0); // draw the short pointer  
	else PVG_Draw(PointerL,0,0); //	Draw the long pointer

	vgLoadIdentity();
	vgTranslate(208.0f, 103.0f);
	vgRotate(PosAngle);
	vgScale(scale1, scale1);
	vgTranslate(0, -97.0);
	if (mode) PVG_Draw(CPshort,0,0); // draw the short pointer  
	else PVG_Draw(PointerL,0,0); //	Draw the long pointer

}



/**
* \brief	Needle movement physics.  
* \author	DB, r54930
* \param	Current Speed, speed in this case is the same as angle increments / frame. 
* \param	CurrentPosition, angle relative to 6 o'clock. Positive angle moves CCW.
* \param	TargetPosition,  angle relative to 6 o'clock. Positive angle moves CCW.
* \return	Speed, that is the angle increment for the current frame. return value is direction agnostic
* \todo		Need to be tested properly, only a simple quick and dirty test has been performed
*/
VGfloat NeedlePhysicsV(VGfloat CurrentPosition, VGfloat CurrentSpeed, VGfloat TargetPosition, uint8_t user)
{
	float delta;
	static unsigned char NPstate[2] = 
	{
		0,0
	};
	
	 
	static float PPH = 1.2f; 
	
	if (user) PPH = 1.3f;
	else PPH = 1.1f; 
	// Calculate the delta between current position and the target position
	if(CurrentPosition < TargetPosition)
		delta = TargetPosition - CurrentPosition;
	else 
		delta = CurrentPosition - TargetPosition;
	
	
	if (NPstate[user] == 0) //no ACC or DECC or SPEEDMAX
	{
			//If the speed is = 0, we need a seed value; 
		if ((delta) && (CurrentSpeed < 0.1f)) CurrentSpeed = 0.1f;
			// if the step is larger than the max speed return max speed
		if ((CurrentSpeed * PPH) > SPEEDMAX) 
		{
			NPstate[user] = 3; //we have reached max speed
			return SPEEDMAX;
		}
			// if the step is larger than the delta return delta
		else if ((CurrentSpeed * PPH) > delta)  
		{
			NPstate[user] = 0; //we have reached the target value
			return delta;
		}
		
		else 
		{
			NPstate[user] = 1; //we are accelerating
			return (CurrentSpeed * PPH);
		}
	}
	else if (NPstate[user] == 1) //ACC state
	{
		if (CurrentSpeed < 0.1f) CurrentSpeed = 0.1f; //min guard in case a previous delta was very low. 
		if ((CurrentSpeed * PPH) > delta)  
		{
			NPstate[user] = 0; //we have reached the target value
			return delta;
		}
		else if (delta < (CurrentSpeed * 3))
		{
			NPstate[user] = 2; //we are deccelerating
			if (delta < (CurrentSpeed / PPH))
			{
				NPstate[user] = 0; //we have reached the target value
				return delta;
			}
			else return (CurrentSpeed / PPH);
		}
		else if ((CurrentSpeed * PPH) > SPEEDMAX) 
		{
			NPstate[user] = 3; //we have reached max speed
			return SPEEDMAX;
		}
		else return (CurrentSpeed * PPH);
	}
	else if (NPstate[user] == 2) //DECC state
	{
		if (CurrentSpeed < 0.1f) CurrentSpeed = 0.1f; //min guard in case a previous delta was very low. 
		if ((CurrentSpeed / PPH) > delta)  
		{
			NPstate[user] = 0; //we have reached the target value
			return delta;
		}
		else if (delta > (CurrentSpeed * 3))
		{
			NPstate[user] = 1; //we are accelerating
			if (delta < (CurrentSpeed * PPH)) 
			{
				NPstate[user] = 0; //we have reached the target value
				return delta;
			}
			else return (CurrentSpeed * PPH);
		}
		else return (CurrentSpeed / PPH);
	}
	else if (NPstate[user] == 3) //MAX speed
	{
		if (delta < (CurrentSpeed * 3))
		{
			NPstate[user] = 2; //we are deccelerating
			if (delta < (CurrentSpeed / PPH)) 
			{
				NPstate[user] = 0; //we have reached the target value
				return delta;
			}
			else return (CurrentSpeed / PPH);
		}
		else return SPEEDMAX;
	}

}

