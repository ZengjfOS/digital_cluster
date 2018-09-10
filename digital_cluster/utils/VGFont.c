/**
  Copyright (c) 2010 Freescale Semiconductor
  
  \file  	  VGFont.c
  \brief	  This is the VGFont Driver File
  \brief	  Interprets vgf font files and loads them as OVG fonts
  \author	  Freescale Semiconductor
  \author	  Systems Enablment
  \author	  Ioseph Martinez, b06623
  \version	  0.8
  \date  	  09/Feb/2010
  
  * History:  22/Jan/2010 - Initial Version
			  09/Feb/2010 - Alpha version, added all functionality

* Copyright (c) 2010, Freescale, Inc.  All rights reserved.
*
*
* No part of this document must be reproduced in any form - including copied,
* transcribed, printed or by any electronic means - without specific written
* permission from Freescale Semiconductor.
*
  
*/

#include "VGFont.h"
#include "wchar.h"


#define VGF_PI (3.141592653589793f)
#define VGF_HALF_PI (1.570796326794897f)

const VGfloat VFG_MIRROR_V_MATRIX[] = {-1.0f,0,0,0,1.0f,0,1.0f,0,1.0f};
const VGfloat VFG_MIRROR_H_MATRIX[] = {1.0f,0,0,0,-1.0f,0,0,1.0f,1.0f};

VGF_Font_t* VGF_FontPtr[VGF_MAX_FONTS];
VGPath VGF_OVGStartPath[VGF_MAX_FONTS];
VGuint VGF_NumPaths[VGF_MAX_FONTS];
VGFont VGF_OVGFont[VGF_MAX_FONTS];
int VGF_initialized = 0;


static int VGF_SearchValue(unsigned short value, const unsigned short *list, unsigned short listsize);

void VGF_Init(void)
{
	unsigned int i;

	for(i = 0; i < VGF_MAX_FONTS; i++)
	{	
		if(VGF_FontPtr[i]!= NULL && VGF_initialized)
		{
			VGF_Destroy(i);			
		}
		VGF_FontPtr[i] = NULL;
	}
	VGF_initialized = 1;
}

VGF_Element_t VGF_Load(VGF_Font_t* font)
{
	unsigned int i;
	unsigned int j;
	unsigned int k;
	unsigned int m;

	VGF_Element_t element;
	VGPath tempPath;
	VGfloat glyphOrigin[2];
	VGfloat glyphEscape[2];

	if(VGF_initialized == 0)
		return VGF_ERROR_UNINIT;
	
	/* -1 means no element found (yet) */
	element = -1;
	i = 0;

	
	/* Search for a free element */
	while(i < VGF_MAX_FONTS)
	{
		if(VGF_FontPtr[i] == NULL)
		{
			element = i;
			i = VGF_MAX_FONTS;
		}
		i++;
	}

	/* -1 means no element found */
	if(element == -1)
		return VGF_ERROR_FONT_FULL;

	/* There is a free element to be allocated */
	VGF_FontPtr[element] = font;
	VGF_NumPaths[element] = 0;
	
	/* Create Font */
	VGF_OVGFont[element] = vgCreateFont(VGF_FontPtr[element]->u32NumGlyphs);
	/* Create OVG Paths */
	for(i = 0; i < VGF_FontPtr[element]->u32NumGlyphs; i++)
	{
		/* Create Path, all the other paths are subsequent paints from the first */
		if(i == 0)
		{
			VGF_OVGStartPath[element] = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_S_32,
										1.0f, 0.0f, 0, 0, (unsigned int)VG_PATH_CAPABILITY_ALL);
			tempPath = VGF_OVGStartPath[element];
		}
		else
		{
			tempPath = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_S_32,
										1.0f, 0.0f, 0, 0, (unsigned int)VG_PATH_CAPABILITY_ALL);
		}
		/* Append Path Data (if not pathless glyph such space)*/
		if(VGF_FontPtr[element]->CharSet[i].cmdIdx != VGF_NOPATH)
		{
			VGF_NumPaths[element]++;
			/* Segment count */
			j = VGF_FontPtr[element]->CharSet[i + 1].cmdIdx - VGF_FontPtr[element]->CharSet[i].cmdIdx;
			/* Command start idx */
			k = VGF_FontPtr[element]->CharSet[i].cmdIdx;
			/* Data start idx */
			m = VGF_FontPtr[element]->CharSet[i].datIdx;
			/*Append Path data/command */
			vgAppendPathData(tempPath, j, &VGF_FontPtr[element]->CmdSet[k], &VGF_FontPtr[element]->DataSet[m]);
			vgPathBounds(tempPath, &glyphOrigin[0], &glyphOrigin[1], &glyphEscape[0], &glyphEscape[1]);
			glyphOrigin[0] = 0.0f;			//leave X origin intact, since we want to make X origin = 0;
			glyphEscape[0] = VGF_FontPtr[element]->CharSet[i].xadvance;// - glyphOrigin[0];
									// advance as stated on the font parameters but fix it by
									// substracting the added value result of leaving X origin intact.

			glyphEscape[1] = 0;
			//glyphEscape[1] = glyphOrigin[1];	//make Y escape return to baseline after printing glyph.
			glyphOrigin[1] = 0.0f;				//make Y origin zero, so glyph below baseline will be shifted downwards
				
			m = VGF_FontPtr[element]->unicode_glyph[i];
			vgSetGlyphToPath(VGF_OVGFont[element], m, tempPath, VG_FALSE, glyphOrigin, glyphEscape);
		}
		else
		{
			m = VGF_FontPtr[element]->unicode_glyph[i];
			glyphOrigin[0] = 0.0f;
			glyphOrigin[1] = 0.0f;
			glyphEscape[0] = VGF_FontPtr[element]->CharSet[i].xadvance;
			glyphEscape[1] = 0.0f;
			vgSetGlyphToPath(VGF_OVGFont[element], m, tempPath, VG_FALSE, glyphOrigin, glyphEscape);
		}
	}			
	
	return element;
}

VGParamType VGF_matrixMode;
VGfloat	VGF_tempMatrix[9];
unsigned int VGF_glyphCount;
VGuint	*VGF_ptrStr;
VGfloat	*VGF_kernX;
VGbitfield VGF_paintMode;
unsigned int VGF_orientation = 0x55;
VGF_Element_t VGF_Currentfont;

/* on Rainbow, memory shall be allocated for this, depending on the size of the input! */
VGuint  VGF_reversed[100];
VGfloat	VGF_kerns[100];

VGF_Error_t VGF_PrepareText(VGF_Element_t font, wchar_t *text, 
							VGuint	size, VGF_Options_t options, VGfloat *data)
{
	VGfloat fOrigin[2];

	/* iterator, etc */
	unsigned int i;
	int keep;
	int schr;
	int index;
	VGParamType matrixMode;
	VGfloat	tempMatrix[9];
	
	VGF_orientation = 0x55;
	VGF_kernX = NULL;
	VGF_glyphCount = 0; 
	VGF_Currentfont = font;


	if(VGF_initialized == 0)
		return VGF_ERROR_UNINIT;
	if(VGF_FontPtr[font] == NULL)
		return VGF_ERROR_FONT_NULL;

	/* Check size of the text string */
	for(i=0; i < 100; i++)
	{
		if(text[i] == 0x0000)
		{
			VGF_glyphCount = i;
			break;
		}
	}

	if(VGF_glyphCount == 0)
		return VGF_ERROR_ZERO_GLYPHS;

	// this part reverses text //
	if((options&VGF_RIGHT2LEFT) != 0)
	{
		// reverse text
		for(i = 1; i <= VGF_glyphCount; i++)
		{
			VGF_reversed[i-1] = (VGuint)text[VGF_glyphCount - i];	
		}
		
	}
	else
	{
		// copy text
		for(i = 0; i < VGF_glyphCount; i++)
		{
			VGF_reversed[i] = (VGuint)text[i];	
		}
	}
	VGF_ptrStr = VGF_reversed;

	// this part calculates the kerning values //
	if((options&VGF_KERNING) != 0)
	{
		/* Use kerning, if not top to bottom */
		if((options&VGF_TOP2BOTTOM) == 0)
		{
			/* And... has a map! */
			if(VGF_FontPtr[font]->kern_data != NULL)
			{
				for(i = 0; i <= (VGF_glyphCount - 1); i++)
				{
					index = VGF_SearchValue(VGF_ptrStr[i],VGF_FontPtr[font]->unicode_g1,VGF_FontPtr[font]->numKerns);
					if(index != -1)
					{							
						VGF_kerns[i] = 0.0f;

						//search downwards
						schr = index;
						keep = 255;
						while(keep > 0)
						{
							if(VGF_FontPtr[font]->kern_data[schr].unicode_g2 == VGF_ptrStr[i+1])
							{
								VGF_kerns[i] = -1.0f*VGF_FontPtr[font]->kern_data[schr].kern;
								break;
							}
							schr--;
							if(schr < 0)
								break;
							if(VGF_FontPtr[font]->unicode_g1[schr] != VGF_ptrStr[i])
								break;
							keep--;
						}
						
						schr = index;
						if(VGF_kerns[i]!= 0.0f)
						{
							//search upwards							
							keep = 255;
							while(keep > 0)
							{
								schr++;
								if(schr > VGF_FontPtr[font]->numKerns)
									break;
								if(VGF_FontPtr[font]->unicode_g1[schr] != VGF_ptrStr[i])
									break;
								if(VGF_FontPtr[font]->kern_data[schr].unicode_g2 == VGF_ptrStr[i+1])
								{
									VGF_kerns[i] = -1.0f*VGF_FontPtr[font]->kern_data[schr].kern;
									break;
								}
								keep--;
							}
						}
					}
					else
					{
						VGF_kerns[i] = 0;
					}
				}
				VGF_kerns[(VGF_glyphCount - 1)] = 0;
				VGF_kernX = VGF_kerns;
			}
		}
	}

	matrixMode = vgGeti(VG_MATRIX_MODE);
	vgGetMatrix(tempMatrix);

	vgSeti(VG_MATRIX_MODE, VG_MATRIX_GLYPH_USER_TO_SURFACE);
	vgLoadIdentity();

	/* if not default mode, you need to calculate the width of the text */
	if(options != VGF_DEFAULT)
	{
		fOrigin[0] = 5000.0f;
		fOrigin[1] = 5000.0f;
		vgSetfv(VG_GLYPH_ORIGIN, 2, fOrigin);

		/* Paint mode = NULL to obtain bounds */
		vgDrawGlyphs(VGF_OVGFont[font], VGF_glyphCount, VGF_ptrStr, VGF_kernX, NULL, VG_FILL_PATH, VG_TRUE);
		vgGetfv(VG_GLYPH_ORIGIN, 2, fOrigin);	

		fOrigin[0] -= 5000.0f;
		fOrigin[1] -= 5000.0f;
		fOrigin[0] = (fOrigin[0]*size)/(float)VGF_FontPtr[font]->unitsperemm;
		fOrigin[1] = (fOrigin[1]*size)/(float)VGF_FontPtr[font]->unitsperemm;
	}
	
	/* Align text */
	if(data != NULL)
	{
		if((options&VGF_LEFTALIG) != 0)
		{
			vgTranslate(data[0],data[1]);
		}
		else if((options&VGF_RIGHTALIG) != 0)
		{
			/* validate data (later, but do it!!)*/
			if(data[2] >= fOrigin[0]){fOrigin[0] = data[0] + data[2] - fOrigin[0];}
			else{fOrigin[0] = data[0];}
			vgTranslate(fOrigin[0],data[1]);
		}
		else if((options&VGF_CENTERALIG) != 0)
		{
			fOrigin[0] = data[0] + (data[2] - fOrigin[0])/2.0f;
			vgTranslate(fOrigin[0],data[1]);
		}
		else
		{
			if((options&VGF_RIGHT2LEFT) != 0)
			{
				/* validate */
				if(data[2] >= fOrigin[0]){fOrigin[0] = data[0] + data[2] - fOrigin[0];}
				else{fOrigin[0] = data[0];}
				vgTranslate(fOrigin[0],data[1]);
			}
			else
			{
				vgTranslate(data[0],data[1]);
			}

		}
	}

	vgScale(size/(VGfloat)VGF_FontPtr[font]->unitsperemm,size/(VGfloat)VGF_FontPtr[font]->unitsperemm);


	/* Transform text */
	if((options&VGF_ROTATE90) != 0)
	{
		vgRotate(90);		
	}
	else if((options&VGF_ROTATE270) != 0)
	{
		vgRotate(-90);
	}
	else if((options&VGF_MIRRORV) != 0)
	{
		vgMultMatrix(VFG_MIRROR_V_MATRIX);
	}
	else if((options&VGF_MIRRORH) != 0)
	{
		vgMultMatrix(VFG_MIRROR_H_MATRIX);
	}

	/* Set origin to zero, anyway */
	fOrigin[0] = 0.0f;
	fOrigin[1] = 0.0f;
	vgSetfv(VG_GLYPH_ORIGIN, 2, fOrigin);

	VGF_paintMode = 0;

	if((options&VGF_STROKE) != 0)
	{
		VGF_paintMode = VG_STROKE_PATH;
	}
	
	if((options&VGF_FILL) != 0)
	{
		VGF_paintMode |= VG_FILL_PATH;
	}

	if(VGF_paintMode == 0)
	{
		VGF_paintMode = VG_FILL_PATH;
	}



	if((options&VGF_TOP2BOTTOM) != 0)
	{
		VGF_orientation = 0;
	}
	else
	{
		VGF_orientation = 1;
	}

	/* restore context */
	VGF_matrixMode = vgGeti(VG_MATRIX_MODE);
	vgGetMatrix(VGF_tempMatrix);
	
	vgSeti(VG_MATRIX_MODE, matrixMode);
	vgLoadMatrix(tempMatrix);
	
	return VGF_ERROR_OK;
}


VGF_Error_t VGF_Draw(VGfloat scalex, VGfloat scaley)
{
	int i;
	VGfloat fOrigin[2];
	VGParamType matrixMode;
	VGfloat	tempMatrix[9];

	if(VGF_orientation == 0x55)
		return VGF_ERROR_UNINIT;

	matrixMode = vgGeti(VG_MATRIX_MODE);
	vgGetMatrix(tempMatrix);
	
	vgSeti(VG_MATRIX_MODE, VGF_matrixMode);
	vgLoadMatrix(VGF_tempMatrix);
	
	//vgScale(scalex,scaley);
	if(VGF_orientation == 1)
	{
		vgDrawGlyphs(VGF_OVGFont[VGF_Currentfont], VGF_glyphCount, VGF_ptrStr, VGF_kernX, NULL, VGF_paintMode, VG_TRUE);
	}
	else if(VGF_orientation == 0)
	{
		for(i = 0; i < VGF_glyphCount; i++)
		{
			fOrigin[0] = 0.0f;
			fOrigin[1] -= VGF_FontPtr[VGF_Currentfont]->unitsperemm;
			vgSetfv(VG_GLYPH_ORIGIN, 2, fOrigin);
			vgDrawGlyph(VGF_OVGFont[VGF_Currentfont], VGF_ptrStr[i], VGF_paintMode, VG_TRUE);
		}
	}

	/* restore context */
	vgSeti(VG_MATRIX_MODE, matrixMode);
	vgLoadMatrix(tempMatrix);

	return VGF_ERROR_OK;
}


VGFont VGF_GetHandle(VGF_Element_t element)
{
	return VGF_OVGFont[element];
}

VGF_Error_t VGF_Destroy(VGF_Element_t font)	
{
	unsigned int i;

	if(VGF_initialized == 0)
		return VGF_ERROR_UNINIT;
	if(VGF_FontPtr[font] == NULL)
		return VGF_ERROR_FONT_NULL;

	vgDestroyFont(VGF_OVGFont[font]);
	for(i = 0; i < VGF_NumPaths[font]; i++)
	{
		vgDestroyPath((VGPath)(VGF_OVGStartPath[font] + i));
	}
	
	VGF_FontPtr[font] = NULL;

	return VGF_ERROR_OK;
}

static int VGF_SearchValue(unsigned short value, const unsigned short *list, unsigned short listsize)
{
	unsigned short min = 0;
	unsigned short max = listsize - 1;	 
	unsigned short mid;	 

	while(min <= max) 
	{
		mid = (min + max)>>1;
		if(value < list[mid])
			max = mid - 1;
		else if(value > list[mid]) 
			min = mid + 1;
		else
			return mid;
	}
	return -1;
}
