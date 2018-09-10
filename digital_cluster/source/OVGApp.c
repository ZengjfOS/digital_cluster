//#include <Windows.h>
#include <VG/openvg.h>
#include <VG/vgu.h>
#include "wchar.h"
//#include <VG/vgext.h>

#include "SPEEDOMETER.h"
#include "TEST.h"
#include "Speed.h"

/* Global variables current windows dimensions */
//extern VGint windowWidth;
//extern VGint windowHeight;

/* Global variables mouse position */
//extern VGint mouse_x;
//extern VGint mouse_y;

//extern int openVGInitialized;

/** Aplication Starts from here */

PVG_Element_t element1;
VGF_Element_t element2;
VGF_Element_t element3;
VGFont  m_vgPathFont;

/* Called once, inits the application */
VGfloat glyphOrigin[2] = {0,0};
VGfloat glyphEscape[2] = {0,0};
VGPath  path;

VGPaint m_vgFontPaint;
VGPaint m_vgFontPaint2;
VGfloat NeedleAngle = 40;
VGfloat NeedleDir = -0.5f;
VGfloat SpeedScale = 0.5f;

/* Called once, inits the application */
void OVG_InitApp(void)
{
	VGErrorCode err;	
	VGfloat afClearColour[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	vgSetfv(VG_CLEAR_COLOR, 4, afClearColour);
	VGF_Init();
	PVG_Init();

	element1 = PVG_Load(&Speed_PVG);
	element2 = VGF_Load(&TEST_VGF);
	// Create font paint
	m_vgFontPaint = vgCreatePaint();
	m_vgFontPaint2 = vgCreatePaint();

	vgSetParameteri(m_vgFontPaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
	vgSetParameteri(m_vgFontPaint2, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
	vgSetColor(m_vgFontPaint, 0x80B0FFFF);


	vgSetColor(m_vgFontPaint2, 0x10A030FF);
}


VGfloat fOrigin[] = {0,0,0,0};
const wchar_t testString_c[] = L"Vector Text! ";
wchar_t testString_v[] =  L"Vector Text! ";
wchar_t SC_v[] =  L"V";
const unsigned int TextPos[13] = {0,43,78,110,128,164,200,200,240,276,308,330,330};  
VGuint ProgresCount = 0;
int tempi = 0;
VGuint TextSize = 460;
/* Called frame by frame */
void OVG_DrawScene(void)
{
	//vgClear(0, 0, windowWidth, windowHeight);
	volatile VGint SR[8]={0,0,0,0,0,0,0,0};
	//vgSeti(VG_SCISSORING, VG_TRUE);
	
	NeedleAngle += NeedleDir;
	if (NeedleAngle < -220.0f) NeedleDir = 1.2f;
	else if (NeedleAngle > 39.91f) NeedleDir = -0.5f;
	SpeedScale *= 1.001f;
	if (SpeedScale > 3.7f) SpeedScale = 0.2f; 
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
	
// Start by drawing the left meter base	
	//vgLoadIdentity();
	//vgScale(1.0,1.0);
	//PVG_Draw(element1,0,115);	//	Draw the base meter plate.
// Rotate and draw the needle
	//vgTranslate(240, 220);
	//vgRotate(NeedleAngle);
	//vgTranslate(-240, -220);
	//PVG_Draw(element1,115,117); //	Draw the needle

	SR[0]=640;
	SR[1]=100;
	SR[2]=200;
	SR[3]=100;
	SR[4]=400;
	SR[5]=80;
	SR[6]=400;
	SR[7]=64;	
	vgSetiv(VG_SCISSOR_RECTS, 4, SR);
/*	vgLoadIdentity();
	//vgScale(1.0,1.0);
	vgTranslate(840,220);
	//vgScale(SpeedScale,SpeedScale);
	vgTranslate(-240,-220);
	PVG_Draw(element1,0,115);	//	Draw the base meter plate.
*/
	
	//vgClear(830,80,420,400);
/*	vgSeti(VG_SCISSORING, VG_FALSE);	
	vgLoadIdentity();
	//vgScale(0.6,0.6);
	vgTranslate(1060,201);
	//vgScale(SpeedScale,SpeedScale);
	vgRotate(NeedleAngle);
	vgTranslate(-240,-220);
	PVG_Draw(element1,115,117); //	Draw the needle

	vgSetiv(VG_SCISSOR_RECTS, 4, &SR[4]);
*/	
	// Set the current fill and stroke paint... for text
	vgSetPaint(m_vgFontPaint2, VG_STROKE_PATH);
	vgSetPaint(m_vgFontPaint, VG_FILL_PATH);
	vgClear(590,0,600,768);
	vgSetf(VG_STROKE_LINE_WIDTH, 20.0f);
	
	TextSize-=8;
	if (TextSize <= 64) {
		TextSize = 460;
		ProgresCount++;
		if (ProgresCount == 13) ProgresCount = 0;
		SC_v[0] = testString_c[ProgresCount];
		testString_v[ProgresCount - 1] = testString_c[ProgresCount - 1];
		testString_v[ProgresCount] = 0;
	}
	int index_i = 0;
	//for (index_i=0;index_i<10;index_i++){
		fOrigin[0] = 600.0f + TextPos[ProgresCount];
		fOrigin[1] = 10.0f + (index_i * 80);
		vgSetColor(m_vgFontPaint2, 0x10A030FF + (index_i << 3));
		vgSetPaint(m_vgFontPaint2, VG_STROKE_PATH);
		VGF_PrepareText(element2, SC_v, TextSize, VGF_KERNING | VGF_STROKE | VGF_FILL, fOrigin);
		VGF_Draw();
		fOrigin[0] = 600.0f;
		fOrigin[1] = 10.0f + (index_i * 80);

		if (ProgresCount) {
			VGF_PrepareText(element2, testString_v, 64, VGF_KERNING | VGF_STROKE | VGF_FILL, fOrigin);
			VGF_Draw();
		}
	//}
}


/*
	fOrigin[0] = 460.0f;
	fOrigin[1] = 40.0f;
	VGF_PrepareText(element2, testString, 64, VGF_KERNING | VGF_STROKE | VGF_FILL, fOrigin);
	VGF_Draw(1.0f,1.0f);
	vgSeti(VG_SCISSORING, VG_FALSE);	

}*/

/* Called when application is being closed*/
void OVG_KillApp(void)
{
	VGF_Destroy(element2);
	PVG_Destroy(element1);
}
