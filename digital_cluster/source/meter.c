/**
  Copyright (c) 2009 Freescale Semiconductor
* 
* No part of this document must be reproduced in any form - including copied,
* transcribed, printed or by any electronic means - without specific written
* permission from Freescale Semiconductor.
*
* Auth: D.Bogavac 2009-10-30
*
* OpenVG meeter drawing example.
* 2009.Oct.30, V0.1. Initial version
*/

/* GPU includes */
#include "VG/openvg.h"
#include "VG/vgu.h"
#include "EGL/egl.h"

/* resources includes */
#include "meter.h"
#include "meterResources.h"
#include "FB_utils.h"

#define ACCMAX 		6.0f // max acceleration angle increment
#define SPEEDMAX	12.0f // max speed in degrees/frame. 
#define SPEED_SIGN 1
/* function prototypes */
VGfloat NeedlePhysics(VGfloat CurrentPosition, VGfloat CurrentSpeed, VGfloat TargetPosition);
void SpeedSignBlink(xOffset, yOffset);


extern frames;

const VGfloat ACCSETPOINT[12] = 
{
	0.1f,1.0f,3.0f,6.0f,10.0f,15.0f,21.0f,28.0f,36.0f,45.0f,55.0f,66.0f
};

/* Small needle dirty rectangle coordinate array. Position angle table with start at 0 degrees, in 5 deg increments 
** Format x,y,w,h. First enty for 0 - 5 deg CW rotation, second for 5-10 deg and so on... 
** Values are offset from center of 440x400 meeter object rectangle. (center is at 220,181pixles from bottom left)
** Values are for 45deg rotation from 40 - 85km/h on speed meeter. All other positions are calculated. */     

const int snDRCA[36] = { 
-185,-22,99,59,
-185,-14,102,66,
-185,-6,107,72,
-183,2,110,78,
-173,10,104,84,
-169,18,106,89,
-164,27,108,92,
-157,34,109,96,
-149,42,109,98
};

/* same as snDRCA but for the big pointer. In additon at the last coordinate entry, we also have the worst case coordinates and size / 45 degree sector */

const int bnDRCA[40] = { 
-179,-27,209,63,
-179,-29,212,80,
-177,-31,214,97,
-175,-33,214,113,
-171,-35,216,128,
-165,-36,214,142,
-151,-38,218,166,
-142,-39,197,177,
-132,-39,190,186,
-179,-39,237,186
};

volatile VGint SRcoords[8]={0,0,0,0,0,0,0,0};

//VGfloat RPMValue = 0.0f;
//VGfloat SpeedValue = 0.0f;
volatile VGfloat SpeedCurrentPos = 0.0f;
volatile VGfloat RPMCurrentPos = 0.0f;
volatile VGfloat SpeedCurrentSpeed = 0.0f;
volatile VGfloat RPMCurrentSpeed = 0.0f;
volatile VGfloat angleLatch[6]={0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};

VGubyte RectSegm[5] = {
VG_MOVE_TO_ABS, 
VG_LINE_TO_ABS,
VG_LINE_TO_ABS,
VG_LINE_TO_ABS,
VG_CLOSE_PATH};

VGint RectPoints[8] = {
0.0f,0.0f,
0.0f,10.0f,
10.0f,10.0f,
10.0f,0.0f};


static VGImage imgSpeed_base;
static VGImage imgRPM_base;
static VGImage imgSpeed_sign;
static VGImage sp0;
static VGImage sp1;
static VGImage sp2;
static VGImage sp3;
static VGImage sp4;
static VGImage sp5;
static VGImage sp6;
static VGImage sp7;
static VGImage sp8;
static VGImage sp9;
static VGImage sp10;
static VGImage sp11;
static VGImage sp12;

static VGImage bp0;
static VGImage bp1;
static VGImage bp2;
static VGImage bp3;
static VGImage bp4;
static VGImage bp5;
static VGImage bp6;
static VGImage bp7;
static VGImage bp8;
static VGImage bp9;
static VGImage bp10;
static VGImage bp11;
static VGImage bp12;
static VGImage sps;
static VGImage spsd;
static VGImage PBimage1;
//static VGImage PBimage2;
//static VGImage PBimage3;

VGPath pathRect;
VGPaint paintFill;
VGPaint paintStroke;

void RectInit(){

		RectPoints[0] = 0; //SRcoords[0]; //start x
		RectPoints[1] = 0; //SRcoords[1]; //start y
		RectPoints[2] = 60; //SRcoords[0]; //next point x
		RectPoints[3] = 0; //SRcoords[1] + SRcoords[3]; //next point y
		RectPoints[4] = 60; //SRcoords[0] + SRcoords[2]; //x
		RectPoints[5] = 60; //SRcoords[1] + SRcoords[3];//y
		RectPoints[6] = 0; //SRcoords[0] + SRcoords[2]; //x
		RectPoints[7] = 60; //SRcoords[1]; //y

	pathRect= vgCreatePath(VG_PATH_FORMAT_STANDARD,
	VG_PATH_DATATYPE_S_32,
	1.0f, 0.0f, 5, 4,
	(unsigned int)(VG_PATH_CAPABILITY_APPEND_TO));
	paintFill = vgCreatePaint();
	paintStroke = vgCreatePaint();
	vgSetParameteri(paintFill, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
	vgSetColor(paintFill, 0xFF0000FF);
	vgSetParameteri(paintStroke, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
	vgSetColor(paintStroke, 0x00FF00FF);
	vgAppendPathData(pathRect, 5, RectSegm, RectPoints);
	//vgRemovePathCapabilities(pathRect, VG_PATH_CAPABILITY_APPEND_TO);
	
}
void DrawRectangle(int x, int y, int color){
	static VGfloat angle = 0;
 
		vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
		vgLoadIdentity();
		vgClear(x-55,y-55,110,110);
		vgTranslate(x,y);
		vgRotate(angle);
		vgTranslate(-30,-30);
		angle += 0.1f;			
		//vgModifyPathCoords(pathRect, 0, 5, RectPoints);
				
		vgSetPaint(paintStroke,VG_STROKE_PATH);
		if (color == 0) vgSetColor(paintStroke, 0x00FF00FF);
		else if (color == 1) vgSetColor(paintStroke, 0xFF0000FF);
		else if (color == 2) vgSetColor(paintStroke, 0xFF0080FF);
		else vgSetColor(paintStroke, 0x0048FFFF);
		vgSetf(VG_STROKE_LINE_WIDTH,5.0f);
		//printf("X0,%d Y0,%d x1,%d y1 %d	x1,%d y1 %d	x1,%d y1 %d	\n", RectPoints[0],RectPoints[1],RectPoints[2],RectPoints[3],RectPoints[4],RectPoints[5],RectPoints[6],RectPoints[7]);	
		vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_BETTER);
		vgDrawPath(pathRect,VG_STROKE_PATH);
	}

InitMeeter(){
VGErrorCode err;


	imgRPM_base = vgCreateImage(VG_sARGB_8888,440,400,VG_IMAGE_QUALITY_BETTER);	
	imgSpeed_base = vgCreateImage(VG_sARGB_8888,440,400,VG_IMAGE_QUALITY_BETTER);
	imgSpeed_sign = vgCreateImage(VG_sARGB_8888,130,130,VG_IMAGE_QUALITY_BETTER);
	err = vgGetError();
	if( err != VG_NO_ERROR)
	{
		printf("VG error 1 %x\n",err);
	}

	bp0 = vgCreateImage(VG_sARGB_8888,205,45,VG_IMAGE_QUALITY_BETTER);
	bp1 = vgCreateImage(VG_sARGB_8888,205,45,VG_IMAGE_QUALITY_BETTER);
	bp2 = vgCreateImage(VG_sARGB_8888,205,45,VG_IMAGE_QUALITY_BETTER);
	bp3 = vgCreateImage(VG_sARGB_8888,205,45,VG_IMAGE_QUALITY_BETTER);
	bp4 = vgCreateImage(VG_sARGB_8888,205,45,VG_IMAGE_QUALITY_BETTER);
	bp5 = vgCreateImage(VG_sARGB_8888,205,45,VG_IMAGE_QUALITY_BETTER);
	bp6 = vgCreateImage(VG_sARGB_8888,205,45,VG_IMAGE_QUALITY_BETTER);
	bp7 = vgCreateImage(VG_sARGB_8888,205,45,VG_IMAGE_QUALITY_BETTER);
	bp8 = vgCreateImage(VG_sARGB_8888,205,45,VG_IMAGE_QUALITY_BETTER);
	bp9 = vgCreateImage(VG_sARGB_8888,205,45,VG_IMAGE_QUALITY_BETTER);
	bp10 = vgCreateImage(VG_sARGB_8888,205,45,VG_IMAGE_QUALITY_BETTER);
	bp11 = vgCreateImage(VG_sARGB_8888,205,45,VG_IMAGE_QUALITY_BETTER);
	bp12 = vgCreateImage(VG_sARGB_8888,205,45,VG_IMAGE_QUALITY_BETTER);
	err = vgGetError();
	if( err != VG_NO_ERROR)
	{
		printf("VG error 2%x\n",err);
	}


	sp0 = vgCreateImage(VG_sARGB_8888,84,44,VG_IMAGE_QUALITY_BETTER);
	sp1 = vgCreateImage(VG_sARGB_8888,84,44,VG_IMAGE_QUALITY_BETTER);
	sp2 = vgCreateImage(VG_sARGB_8888,84,44,VG_IMAGE_QUALITY_BETTER);
	sp3 = vgCreateImage(VG_sARGB_8888,84,44,VG_IMAGE_QUALITY_BETTER);
	sp4 = vgCreateImage(VG_sARGB_8888,84,44,VG_IMAGE_QUALITY_BETTER);
	sp5 = vgCreateImage(VG_sARGB_8888,84,44,VG_IMAGE_QUALITY_BETTER);
	sp6 = vgCreateImage(VG_sARGB_8888,84,44,VG_IMAGE_QUALITY_BETTER);
	sp7 = vgCreateImage(VG_sARGB_8888,84,44,VG_IMAGE_QUALITY_BETTER);
	sp8 = vgCreateImage(VG_sARGB_8888,84,44,VG_IMAGE_QUALITY_BETTER);
	sp9 = vgCreateImage(VG_sARGB_8888,84,44,VG_IMAGE_QUALITY_BETTER);
	sp10 = vgCreateImage(VG_sARGB_8888,84,44,VG_IMAGE_QUALITY_BETTER);
	sp11 = vgCreateImage(VG_sARGB_8888,84,44,VG_IMAGE_QUALITY_BETTER);
	sp12 = vgCreateImage(VG_sARGB_8888,84,44,VG_IMAGE_QUALITY_BETTER);
	sps = vgCreateImage(VG_sARGB_8888,84,44,VG_IMAGE_QUALITY_BETTER);
	spsd = vgCreateImage(VG_sARGB_8888,84,44,VG_IMAGE_QUALITY_BETTER);

	err = vgGetError();
	if( err != VG_NO_ERROR)
	{
		printf("VG error 3%x\n",err);
	}

	//PBimage1 = vgCreateImage(VG_sARGB_8888,64,64,VG_IMAGE_QUALITY_BETTER);
	//PBimage2 = vgCreateImage(VG_sARGB_8888,440,400,VG_IMAGE_QUALITY_BETTER);
	//PBimage3 = vgCreateImage(VG_sARGB_8888,440,400,VG_IMAGE_QUALITY_BETTER);

	err = vgGetError();
	if( err != VG_NO_ERROR)
	{
		printf("VG error 4 %x\n",err);
	}

	vgImageSubData(imgRPM_base, v2_images_Bitmap1,440*4, VG_sARGB_8888, 0,0,440,400);
	vgImageSubData(imgSpeed_base, v2_images_Bitmap0,440*4, VG_sARGB_8888, 0,0,440,400);
	vgImageSubData(imgSpeed_sign, Speed_sign_Bitmap0,130*4, VG_sARGB_8888, 0,0,130,130);
	err = vgGetError();
	if( err != VG_NO_ERROR)
	{
		printf("VG error 5 %x\n",err);
	}
	
	vgImageSubData(bp0, v2_b_pointer_Bitmap0,205*4, VG_sARGB_8888, 0,0,205,45);
	vgImageSubData(bp1, v2_b_pointer_Bitmap1,205*4, VG_sARGB_8888, 0,0,205,45);
	vgImageSubData(bp2, v2_b_pointer_Bitmap2,205*4, VG_sARGB_8888, 0,0,205,45);
	vgImageSubData(bp3, v2_b_pointer_Bitmap3,205*4, VG_sARGB_8888, 0,0,205,45);
	vgImageSubData(bp4, v2_b_pointer_Bitmap4,205*4, VG_sARGB_8888, 0,0,205,45);
	vgImageSubData(bp5, v2_b_pointer_Bitmap5,205*4, VG_sARGB_8888, 0,0,205,45);
	vgImageSubData(bp6, v2_b_pointer_Bitmap6,205*4, VG_sARGB_8888, 0,0,205,45);
	vgImageSubData(bp7, v2_b_pointer_Bitmap7,205*4, VG_sARGB_8888, 0,0,205,45);
	vgImageSubData(bp8, v2_b_pointer_Bitmap8,205*4, VG_sARGB_8888, 0,0,205,45);
	vgImageSubData(bp9, v2_b_pointer_Bitmap9,205*4, VG_sARGB_8888, 0,0,205,45);
	vgImageSubData(bp10, v2_b_pointer_Bitmap10,205*4, VG_sARGB_8888, 0,0,205,45);
	vgImageSubData(bp11, v2_b_pointer_Bitmap11,205*4, VG_sARGB_8888, 0,0,205,45);
	vgImageSubData(bp12, v2_b_pointer_Bitmap12,205*4, VG_sARGB_8888, 0,0,205,45);

	vgImageSubData(sp0, v2_s_pointer_Bitmap0,84*4, VG_sARGB_8888, 0,0,84,44);
	vgImageSubData(sp1, v2_s_pointer_Bitmap1,84*4, VG_sARGB_8888, 0,0,84,44);
	vgImageSubData(sp2, v2_s_pointer_Bitmap2,84*4, VG_sARGB_8888, 0,0,84,44);
	vgImageSubData(sp3, v2_s_pointer_Bitmap3,84*4, VG_sARGB_8888, 0,0,84,44);
	vgImageSubData(sp4, v2_s_pointer_Bitmap4,84*4, VG_sARGB_8888, 0,0,84,44);
	vgImageSubData(sp5, v2_s_pointer_Bitmap5,84*4, VG_sARGB_8888, 0,0,84,44);
	vgImageSubData(sp6, v2_s_pointer_Bitmap6,84*4, VG_sARGB_8888, 0,0,84,44);
	vgImageSubData(sp7, v2_s_pointer_Bitmap7,84*4, VG_sARGB_8888, 0,0,84,44);
	vgImageSubData(sp8, v2_s_pointer_Bitmap8,84*4, VG_sARGB_8888, 0,0,84,44);
	vgImageSubData(sp9, v2_s_pointer_Bitmap9,84*4, VG_sARGB_8888, 0,0,84,44);
	vgImageSubData(sp10, v2_s_pointer_Bitmap10,84*4, VG_sARGB_8888, 0,0,84,44);
	vgImageSubData(sp11, v2_s_pointer_Bitmap11,84*4, VG_sARGB_8888, 0,0,84,44);
	vgImageSubData(sp12, v2_s_pointer_Bitmap12,84*4, VG_sARGB_8888, 0,0,84,44);
	vgImageSubData(sps, shadows_Bitmap0,84*4, VG_sARGB_8888, 0,0,84,44);
	vgImageSubData(spsd, shadows_Bitmap0,84*4, VG_sARGB_8888, 0,0,84,44);
	err = vgGetError();
	if( err != VG_NO_ERROR)
	{
		printf("VG error 6 %x\n",err);
	}


}

RotateMeterBase(int xOffset, int yOffset, VGfloat scale)
{

	static VGfloat rotation;

	rotation += 0.2f;
	vgLoadIdentity();
	vgTranslate(xOffset + 220,yOffset + 200);
	vgScale(scale,scale);
	vgRotate(rotation);
	vgTranslate(-220,-200);
	vgDrawImage(imgSpeed_base);
}

RenderMeeterFirst(int xOffset, int yOffset, int panel, int needle, VGfloat scale){

	float clearColor[4] = {0,0,0,0};
	//vgTranslate(-4,-4);
	//vgDrawImage(sps);
	vgClear(xOffset,yOffset,xOffset + 440,yOffset + 400);
	
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
	vgSeti(VG_IMAGE_MODE, VG_DRAW_IMAGE_NORMAL);
	vgSeti(VG_IMAGE_QUALITY, VG_IMAGE_QUALITY_BETTER);
vgSeti(VG_BLEND_MODE, VG_BLEND_SRC);
	vgLoadIdentity();

	vgTranslate(xOffset + 220,yOffset + 200);
	vgScale(scale,scale);
	vgTranslate(-220,-199);
	if (panel == RPM_PANEL) {
		//vgDrawImage(imgRPM_base);
		vgSetPixels(xOffset, yOffset, imgRPM_base, 0, 
		0, 440, 400);
		
	}
	else {
		vgDrawImage(imgSpeed_base);
//		vgTranslate(220,199);
//		vgScale(scale * 0.8f,scale * 0.8f);
//		vgTranslate(-65,-85);
//		vgDrawImage(imgSpeed_sign);
	}
}

/*
** Function sets the dirty rectangle region based on the current pointern angle
**
*/
void SetDR(int xOffset, int yOffset, int dr, int object, int panel, VGfloat angle){

	int coords = 0;
	if (panel == RPM_PANEL) coords = 1;
	else coords = 0;
		
	if (dr == 0) vgSeti(VG_SCISSORING, VG_FALSE);
	else {
			vgSeti(VG_SCISSORING, VG_TRUE);
			vgSetiv(VG_SCISSOR_RECTS, 4, SRcoords);
			if (object == SMALL_NEEDLE){
				if ((angle >= 0.0f) && (angle <= 40.0f)){
					SRcoords[0]=snDRCA[((uint8_t)(angle/5)*4)] + xOffset + 220;
					SRcoords[1]= 181 - snDRCA[(((uint8_t)(angle/5)*4) + 1)] - snDRCA[(((uint8_t)(angle/5)*4) + 3)]+ yOffset;
					SRcoords[2]= snDRCA[(((uint8_t)(angle/5)*4) + 2)];
					SRcoords[3]= snDRCA[(((uint8_t)(angle/5)*4) + 3)];
				}
				else if ((angle <= 0.0f) && (angle >= -45.0f)){
					angle *= -1;
					SRcoords[0]=snDRCA[((uint8_t)(angle/5)*4)] + xOffset + 220;
					SRcoords[1]= 181 + snDRCA[(((uint8_t)(angle/5)*4) + 1)] + yOffset;
					SRcoords[2]= snDRCA[(((uint8_t)(angle/5)*4) + 2)];
					SRcoords[3]= snDRCA[(((uint8_t)(angle/5)*4) + 3)];
				}
				else if ((angle <= -45.0f) && (angle >= -90.0f)){
					angle += 90.0f;
					SRcoords[0]= xOffset + 220 - snDRCA[(((uint8_t)(angle/5)*4) + 1)] - 
					snDRCA[(((uint8_t)(angle/5)*4) + 3)];
					SRcoords[1]= yOffset + 181 + ((snDRCA[(((uint8_t)(angle/5)*4))] + 
					snDRCA[(((uint8_t)(angle/5)*4) + 2)]) * -1.0f);
					SRcoords[2]= snDRCA[(((uint8_t)(angle/5)*4) + 3)];
					SRcoords[3]= snDRCA[(((uint8_t)(angle/5)*4) + 2)];
				}
				else if ((angle <= -90.0f) && (angle >= -135.0f)){
					angle += 90.0f;
					angle *= -1.0f;
					SRcoords[0]= xOffset +220 + snDRCA[(((uint8_t)(angle/5)*4) + 1)];
					SRcoords[1]= yOffset + 181 + ((snDRCA[(((uint8_t)(angle/5)*4))] + 
					snDRCA[(((uint8_t)(angle/5)*4) + 2)]) * -1.0f);
					SRcoords[2]= snDRCA[(((uint8_t)(angle/5)*4) + 3)];
					SRcoords[3]= snDRCA[(((uint8_t)(angle/5)*4) + 2)];
				}
				else if ((angle <= -135.0f) && (angle >= -180.0f)){
					angle += 180.0f;
					SRcoords[0]= xOffset + 220 + ((snDRCA[(((uint8_t)(angle/5)*4))]+ 
					snDRCA[(((uint8_t)(angle/5)*4) + 2)]) * -1.0f);
					SRcoords[1]= 181 + snDRCA[(((uint8_t)(angle/5)*4) + 1)] + yOffset;
					SRcoords[2]= snDRCA[(((uint8_t)(angle/5)*4) + 2)];
					SRcoords[3]= snDRCA[(((uint8_t)(angle/5)*4) + 3)];
					//printf("angle %f angle/5 %f\n",angle,(angle/5));
				}
				else if ((angle <=-180.0f) && (angle >= -220.0f)){
					angle *= -1.0f;
					angle -=180.0f;
					SRcoords[0]= xOffset + 220 - snDRCA[(((uint8_t)(angle/5)*4))] - 
					snDRCA[(((uint8_t)(angle/5)*4) + 2)];
					SRcoords[1]= 181 - snDRCA[(((uint8_t)(angle/5)*4) + 1)] - snDRCA[(((uint8_t)(angle/5)*4) + 3)]+ yOffset;
					SRcoords[2]= snDRCA[(((uint8_t)(angle/5)*4) + 2)];
					SRcoords[3]= snDRCA[(((uint8_t)(angle/5)*4) + 3)];
					if (frames == 29) printf("angle %f SRcoord[1] %d\n",angle,SRcoords[1]);
				}
				vgSeti(VG_SCISSORING, VG_TRUE);
				//printf("x,%ld, y, %ld, w, %ld, h %ld, angle %lf deg \n",SRcoords[0], SRcoords[1], SRcoords[2], SRcoords[3], angle);
			} //if (SMALL_NEEDLE.....
			else if (object == BIG_NEEDLE){
				if ((angle >= 0.0f) && (angle <= 40.0f)){
					SRcoords[0]=bnDRCA[((uint8_t)(angle/5)*4)] + xOffset + 220;
					SRcoords[1]= 181 - bnDRCA[(((uint8_t)(angle/5)*4) + 1)] - bnDRCA[(((uint8_t)(angle/5)*4) + 3)]+ yOffset;
					SRcoords[2]= bnDRCA[(((uint8_t)(angle/5)*4) + 2)];
					SRcoords[3]= bnDRCA[(((uint8_t)(angle/5)*4) + 3)];
				}
				else if ((angle <= 0.0f) && (angle >= -45.0f)){
					angle *= -1;
					SRcoords[0]=bnDRCA[((uint8_t)(angle/5)*4)] + xOffset + 220;
					SRcoords[1]= 181 + bnDRCA[(((uint8_t)(angle/5)*4) + 1)] + yOffset;
					SRcoords[2]= bnDRCA[(((uint8_t)(angle/5)*4) + 2)];
					SRcoords[3]= bnDRCA[(((uint8_t)(angle/5)*4) + 3)];
				}
				else if ((angle <= -45.0f) && (angle >= -90.0f)){
					angle += 90.0f;
					SRcoords[0]= xOffset + 220 - bnDRCA[(((uint8_t)(angle/5)*4) + 1)] - 
					bnDRCA[(((uint8_t)(angle/5)*4) + 3)];
					SRcoords[1]= yOffset + 181 + ((bnDRCA[(((uint8_t)(angle/5)*4))] + 
					bnDRCA[(((uint8_t)(angle/5)*4) + 2)]) * -1.0f);
					SRcoords[2]= bnDRCA[(((uint8_t)(angle/5)*4) + 3)];
					SRcoords[3]= bnDRCA[(((uint8_t)(angle/5)*4) + 2)];
				}
				else if ((angle <= -90.0f) && (angle >= -135.0f)){
					angle += 90.0f;
					angle *= -1.0f;
					SRcoords[0]= xOffset +220 + bnDRCA[(((uint8_t)(angle/5)*4) + 1)];
					SRcoords[1]= yOffset + 181 + ((bnDRCA[(((uint8_t)(angle/5)*4))] + 
					bnDRCA[(((uint8_t)(angle/5)*4) + 2)]) * -1.0f);
					SRcoords[2]= bnDRCA[(((uint8_t)(angle/5)*4) + 3)];
					SRcoords[3]= bnDRCA[(((uint8_t)(angle/5)*4) + 2)];
				}
				else if ((angle <= -135.0f) && (angle >= -180.0f)){
					angle += 180.0f;
					SRcoords[0]= xOffset + 220 + ((bnDRCA[(((uint8_t)(angle/5)*4))]+ 
					bnDRCA[(((uint8_t)(angle/5)*4) + 2)]) * -1.0f);
					SRcoords[1]= 181 + bnDRCA[(((uint8_t)(angle/5)*4) + 1)] + yOffset;
					SRcoords[2]= bnDRCA[(((uint8_t)(angle/5)*4) + 2)];
					SRcoords[3]= bnDRCA[(((uint8_t)(angle/5)*4) + 3)];
					//printf("angle %f angle/5 %f\n",angle,(angle/5));
				}
				else if ((angle <=-180.0f) && (angle >= -220.0f)){
					angle *= -1.0f;
					angle -=180.0f;
					SRcoords[0]= xOffset + 220 - bnDRCA[(((uint8_t)(angle/5)*4))] - 
					bnDRCA[(((uint8_t)(angle/5)*4) + 2)];
					SRcoords[1]= 181 - bnDRCA[(((uint8_t)(angle/5)*4) + 1)] - bnDRCA[(((uint8_t)(angle/5)*4) + 3)]+ yOffset;
					SRcoords[2]= bnDRCA[(((uint8_t)(angle/5)*4) + 2)];
					SRcoords[3]= bnDRCA[(((uint8_t)(angle/5)*4) + 3)];
					if (frames == 29) printf("angle %f SRcoord[1] %d\n",angle,SRcoords[1]);
				}
				vgSeti(VG_SCISSORING, VG_TRUE);
				//printf("x,%ld, y, %ld, w, %ld, h %ld, angle %lf deg \n",SRcoords[0], SRcoords[1], SRcoords[2], SRcoords[3], angle);
			} //else if (BIG_NEEDLE.....

	}//else 
}


/***************************************************************/

RenderMeeterUpdate(int xOffset, int yOffset, int panel, int needle, VGfloat scale){
	
	VGfloat temp;
	volatile VGfloat Increment = 0.0f;
	volatile VGfloat PosAngle = 0.0f;

	if (panel == SPEED_PANEL){
		vgLoadIdentity();
		vgTranslate(xOffset + 220,yOffset + 181);
		vgScale(scale,scale);
		vgTranslate(-220,-181);
		SetDR(xOffset, yOffset,1,needle, panel, angleLatch[BufferFlag]);
		//else SetDR(xOffset, yOffset,1,needle,panel,angleLatch[0]);
		vgSeti(VG_SCISSORING, VG_FALSE);
		vgSetPixels(SRcoords[0], SRcoords[1], imgSpeed_base, SRcoords[0]-xOffset, 
		SRcoords[1]-yOffset, SRcoords[2],SRcoords[3]);
		//vgFinish();
		//SpeedSignBlink(xOffset,yOffset);
	} 
	else if (panel == RPM_PANEL){
		vgLoadIdentity();
		vgTranslate(xOffset + 220,yOffset + 181);
		vgScale(scale,scale);
		vgTranslate(-220,-181);
		SetDR(xOffset, yOffset,1,needle,panel,angleLatch[BufferFlag + 3]);
		//else SetDR(xOffset, yOffset,1, needle, panel,angleLatch[2]);
		vgSeti(VG_SCISSORING, VG_FALSE);
		vgSetPixels(SRcoords[0], SRcoords[1], imgRPM_base, SRcoords[0]-xOffset, 
		SRcoords[1]-yOffset, SRcoords[2],SRcoords[3]); 
		//vgFinish();
	}
	if (needle == SMALL_NEEDLE){
		vgTranslate(220,181);
		if (panel == SPEED_PANEL) {
			SpeedCurrentSpeed = NeedlePhysics(SpeedCurrentPos, SpeedCurrentSpeed, SpeedValue + 40.0f);	
			if (SpeedCurrentPos > SpeedValue + 40.0f) SpeedCurrentPos -= SpeedCurrentSpeed; 
			else SpeedCurrentPos += SpeedCurrentSpeed;
			angleLatch[BufferFlag] = SpeedCurrentPos;
			SetDR(xOffset, yOffset,0,needle,panel,SpeedCurrentPos);
			Increment = SpeedCurrentSpeed;
			PosAngle = SpeedCurrentPos;
		}
		else if (panel == RPM_PANEL) {
			RPMCurrentSpeed = NeedlePhysics(RPMCurrentPos, RPMCurrentSpeed, RPMValue + 40.0f);	
			if (RPMCurrentPos > RPMValue + 40.0f) RPMCurrentPos -= RPMCurrentSpeed; 
			else RPMCurrentPos += RPMCurrentSpeed;
			angleLatch[BufferFlag + 3] = RPMCurrentPos;
			SetDR(xOffset, yOffset,0,needle,panel,RPMCurrentPos);
			Increment = RPMCurrentSpeed;
			PosAngle = RPMCurrentPos;
		}

		if (Shadow){
			vgSeti(VG_BLEND_MODE, VG_BLEND_SRC_OVER);
			vgTranslate(-6,-6);
			vgRotate(PosAngle);
			vgTranslate(-175,-22);
			vgDrawImage(spsd);
			vgLoadIdentity();
			vgTranslate(xOffset + 220,yOffset + 181);
	    }
		vgRotate(PosAngle);
		vgTranslate(-175,-22);
		// for demo purpouses we inhibit the dynamic blurr on the RPM meeter.
		vgSeti(VG_BLEND_MODE, VG_BLEND_SRC_OVER);
		if (!Blurr) vgDrawImage(sp0);
		else if (Increment < 3.0f) vgDrawImage(sp0);
		else if (Increment < 4.0f) vgDrawImage(sp3);
		else if (Increment < 5.0f) vgDrawImage(sp4);
		else if (Increment < 6.0f) vgDrawImage(sp5);
		else if (Increment < 7.0f) vgDrawImage(sp6);
		else if (Increment < 8.0f) vgDrawImage(sp7);
		else if (Increment < 9.0f) vgDrawImage(sp8);
		else if (Increment < 10.0f) vgDrawImage(sp9);
		else if (Increment < 11.0f) vgDrawImage(sp10);
		else if (Increment < 12.0f) vgDrawImage(sp11);
		else vgDrawImage(sp12);

		//vgFinish();
	}//if (needle == SMALL_NEEDLE){
	else if (needle == BIG_NEEDLE){
		vgTranslate(220,181);
		if (panel == SPEED_PANEL) {
			SpeedCurrentSpeed = NeedlePhysics(SpeedCurrentPos, SpeedCurrentSpeed, SpeedValue + 40.0f);	
			if (SpeedCurrentPos > SpeedValue + 40.0f) SpeedCurrentPos -= SpeedCurrentSpeed; 
			else SpeedCurrentPos += SpeedCurrentSpeed;
			angleLatch[BufferFlag] = SpeedCurrentPos;
			SetDR(xOffset, yOffset,0,needle,panel,SpeedCurrentPos);
			Increment = SpeedCurrentSpeed;
			PosAngle = SpeedCurrentPos;
		}
		else if (panel == RPM_PANEL) {
			RPMCurrentSpeed = NeedlePhysics(RPMCurrentPos, RPMCurrentSpeed, RPMValue + 40.0f);	
			if (RPMCurrentPos > RPMValue + 40.0f) RPMCurrentPos -= RPMCurrentSpeed; 
			else RPMCurrentPos += RPMCurrentSpeed;
			angleLatch[BufferFlag + 3] = RPMCurrentPos;
			SetDR(xOffset, yOffset,0,needle,panel,RPMCurrentPos);
			Increment = RPMCurrentSpeed;
			PosAngle = RPMCurrentPos;
		}

		vgRotate(PosAngle);
		vgTranslate(-175,-22);
		vgSeti(VG_BLEND_MODE, VG_BLEND_SRC_OVER);
		if (Increment < 3.0f) vgDrawImage(bp0);
		else if (Increment < 4.0f) vgDrawImage(bp3);
		else if (Increment < 5.0f) vgDrawImage(bp4);
		else if (Increment < 6.0f) vgDrawImage(bp5);
		else if (Increment < 7.0f) vgDrawImage(bp6);
		else if (Increment < 8.0f) vgDrawImage(bp7);
		else if (Increment < 9.0f) vgDrawImage(bp8);
		else if (Increment < 10.0f) vgDrawImage(bp9);
		else if (Increment < 11.0f) vgDrawImage(bp10);
		else if (Increment < 12.0f) vgDrawImage(bp11);
		else vgDrawImage(bp12);
		//vgFinish();
	}//else if (needle == BIG_NEEDLE){
	
	if ((panel == SPEED_PANEL) && (SpeedSign)) SpeedSignBlink(xOffset,yOffset);
		
	   	
}

void SpeedSignBlink(xOffset, yOffset){
	
	static int BlinkLatch, buffCount;
	static VGfloat scale, scaleinc;
	
	if (SpeedSign == 1){
		scale = 0.5f;
		SpeedSign++;
	return;
	} 	
	else if (SpeedSign > 5){
		vgClear(xOffset + 155, yOffset + 115, 130,130);
		if (++SpeedSign == 8) SpeedSign = 0;
			return;
	}
	if (SpeedCurrentPos < -40.0f){
		if (frames == 0) BlinkLatch = 0;
		if (frames > BlinkLatch){
			BlinkLatch = frames;
			if (scale > 0.9f) scaleinc = -0.02f;
			else if (scale < 0.6f) scaleinc = 0.1f;
			scale += scaleinc;
		}
		
		vgLoadIdentity();
		vgTranslate(xOffset + 220,yOffset + 180);
		vgScale(scale,scale);
		vgTranslate(-65,-65);
		if (scaleinc < 0) vgClear(xOffset + 155, yOffset + 115, 130,130);
		vgDrawImage(imgSpeed_sign);
		buffCount=0;
	}
	else if (buffCount < 3){
		//scale = 0.8f;
		vgLoadIdentity();
		vgTranslate(xOffset + 220,yOffset + 180);
		vgScale(0.8f,0.8f);
		vgTranslate(-65,-65);
		vgClear(xOffset + 155, yOffset + 115, 130,130);
		vgDrawImage(imgSpeed_sign);
		buffCount++;
	}
}

/**
* \brief	Needle movement physics.  
* \author	DB, r54930
* \param	Current Speed, speed in this case is the same as angle increments / frame. 
* \param	CurrentPosition, angle relative to 9 o'clock. Positive angle moves CCW.
* \param	TargetPosition,  angle relative to 9 o'clock. Positive angle moves CCW.
* \return	Speed, that is the angle increment for the current frame. return value is direction agnostic
* \todo		Need to be tested properly, only a simple quick and dirty test has been performed
* misc. 	ACCMAX and SPEEDMAX defines are at the top of the file and can be adjusted.
* 			SPEEDMAX, sets the max increment eg. the max speed.
*			ACCMAX, I have no idea what this does I wrote the code a few months ago. 
*  			Will check it later and potentially update the comments. 
*/
VGfloat NeedlePhysics(VGfloat CurrentPosition, VGfloat CurrentSpeed, VGfloat TargetPosition)
{
	VGfloat delta;

	// Calculate the delta between current position and the target position
	if(CurrentPosition < TargetPosition)
		delta = TargetPosition - CurrentPosition;
	else 
		delta = CurrentPosition - TargetPosition;
	
	if ((CurrentSpeed <	ACCMAX) && (delta)) // if we are in the acc or deacc zone & delta is not = 0.
	{	
		if((CurrentSpeed + delta) > ACCSETPOINT[(uint8_t)(CurrentSpeed/4)]) // we need to .... 
		{
			if (delta <= 1.0f) return delta;
			else if(delta < 2.0f) return 1.0f; 
			else if (CurrentSpeed == 0.0f) return 0.1f;
			else return CurrentSpeed +=0.8f ; // accelerate
		}
		else 
		{
			if (delta <= 1.0f) return delta; // deccelerate
			else if(delta < 2.0f) return 1.0f; 
			else if(CurrentSpeed == 1.0f) return 0.1f;
			else if (CurrentSpeed == 0.0f) return 0.1f;
			else return CurrentSpeed -=0.8f ;
		}
	}
	else if(CurrentSpeed == ACCMAX) // if we have reached the acc or decc point. 
	{
		if (delta >= ((SPEEDMAX * 2) + ACCSETPOINT[(uint8_t)(ACCMAX)])) 
			return SPEEDMAX;
		if (delta >= (ACCMAX + SPEEDMAX + ACCSETPOINT[(uint8_t)(ACCMAX)])) 
			return SPEEDMAX/2;
		else if (delta >= (SPEEDMAX + ACCSETPOINT[(uint8_t)(ACCMAX)])) 
			return delta - ACCSETPOINT[(uint8_t)(ACCMAX)]-ACCMAX;
		else if (delta > ((ACCMAX * 2) + ACCSETPOINT[(uint8_t)(ACCMAX)])) 
			return delta - ACCSETPOINT[(uint8_t)(ACCMAX)]-ACCMAX;
		else if (delta > (ACCMAX + ACCSETPOINT[(uint8_t)(ACCMAX)])) 
			return ACCMAX;
		else 
			return CurrentSpeed -=1.0f;
	}
	else 	// we are out of the acc/decc zone.  
	{
		if (delta >= ((SPEEDMAX * 2) + ACCSETPOINT[(uint8_t)(ACCMAX)])) return SPEEDMAX;
		else if (delta >= (SPEEDMAX + ACCSETPOINT[(uint8_t)(ACCMAX)])) return (delta - ACCSETPOINT[(uint8_t)(ACCMAX)])/2;
		else if (delta > ((ACCMAX * 2) + ACCSETPOINT[(uint8_t)(ACCMAX)])) return delta - ACCSETPOINT[(uint8_t)(ACCMAX)] - ACCMAX;
		else return ACCMAX;
	}
	
}
