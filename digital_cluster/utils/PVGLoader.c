/**
  Copyright (c) 2010 Freescale Semiconductor

  \file  	  PVG.h
  \brief	  This is the PVG Driver File
  \brief	  Translates the PVG vector graphics data into OpenVG Calls
  \author	  Freescale Semiconductor
  \author	  Systems Enablment
  \author	  Ioseph Martinez, b06623
  \version	  0.8
  \date  	  07/Jan/2010

  * History:  07/Jan/2010 - Initial Version

* Copyright (c) 2010, Freescale, Inc.  All rights reserved.
*
*
* No part of this document must be reproduced in any form - including copied,
* transcribed, printed or by any electronic means - without specific written
* permission from Freescale Semiconductor.
*

*/

#include "PVGLoader.h"
#include <VG/openvg.h>


const PVG_Object_t*	PVG_ObjectPtr[PVG_MAX_OBJECTS];
VGPath			PVG_OVGStartPath[PVG_MAX_OBJECTS];
VGPaint			PVG_OVGStartPaint[PVG_MAX_OBJECTS];
int			PVG_initialized = 0;

void PVG_Init(void)
{
	unsigned int i;

	for(i = 0; i < PVG_MAX_OBJECTS; i++)
	{
		if(PVG_ObjectPtr[i]!= NULL && PVG_initialized)
		{
			PVG_Destroy(i);
		}
		PVG_ObjectPtr[i] = NULL;
	}
	PVG_initialized = 1;
}


PVG_Element_t PVG_Load(const PVG_Object_t* PVGObject)
{
	unsigned int i;
	unsigned int j;
	PVG_Element_t element;
	VGPaint tempPaint;
	VGPath tempPath;

	/* -1 means no element found */
	element = -1;
	i = 0;

	if(PVG_initialized)
	{
		/* Search for a free element */
		while(i < PVG_MAX_OBJECTS)
		{
			if(PVG_ObjectPtr[i] == NULL)
			{
				element = i;
				i = PVG_MAX_OBJECTS;
			}
			i++;
		}

		/* There is a free element to be allocated */
		if(element != -1)
		{
			PVG_ObjectPtr[element] = PVGObject;

			/* Create OVG paints */
			for(i = 0; i < PVG_ObjectPtr[element]->u32NumPaints; i++)
			{
				/* Create Paint, all the other paints are subsequent paints from the first */
				if(i == 0)
				{
					PVG_OVGStartPaint[element] = vgCreatePaint();
					tempPaint = PVG_OVGStartPaint[element];
				}
				else
				{
					tempPaint = vgCreatePaint();
				}

				vgSetParameteri(tempPaint, VG_PAINT_TYPE, PVG_ObjectPtr[element]->sptrPaints[i].u32PaintStyle);

				switch (PVG_ObjectPtr[element]->sptrPaints[i].u32PaintStyle)
				{
					case VG_PAINT_TYPE_LINEAR_GRADIENT:
					{
						j = PVG_ObjectPtr[element]->sptrPaints[i].u32Ramp;
						vgSetParameterfv(tempPaint, VG_PAINT_LINEAR_GRADIENT, 4, PVG_ObjectPtr[element]->sptrPaints[i].fPaintParams);
						vgSetParameteri(tempPaint, VG_PAINT_COLOR_RAMP_SPREAD_MODE, VG_COLOR_RAMP_SPREAD_PAD);
						vgSetParameterfv(tempPaint, VG_PAINT_COLOR_RAMP_STOPS, PVG_ObjectPtr[element]->sptrRamps[j].u32NumRamps*5,
										 PVG_ObjectPtr[element]->sptrRamps[j].fptrRampsStart);
						break;
					}
					case VG_PAINT_TYPE_RADIAL_GRADIENT:
					{
						j = PVG_ObjectPtr[element]->sptrPaints[i].u32Ramp;
						vgSetParameterfv(tempPaint, VG_PAINT_RADIAL_GRADIENT, 5, PVG_ObjectPtr[element]->sptrPaints[i].fPaintParams);
						vgSetParameteri(tempPaint, VG_PAINT_COLOR_RAMP_SPREAD_MODE, VG_COLOR_RAMP_SPREAD_PAD);
						vgSetParameterfv(tempPaint, VG_PAINT_COLOR_RAMP_STOPS, PVG_ObjectPtr[element]->sptrRamps[j].u32NumRamps*5,
										 PVG_ObjectPtr[element]->sptrRamps[j].fptrRampsStart);
						break;
					}
					case VG_PAINT_TYPE_PATTERN:
					case VG_PAINT_TYPE_COLOR:
					default:
					{
						vgSetColor(tempPaint, 0x00FF00FF);
						vgSetParameteri(tempPaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
						vgSetParameterfv(tempPaint, VG_PAINT_COLOR, 4, PVG_ObjectPtr[element]->sptrPaints[i].fPaintParams);
						break;
					}
				}

			}

			/* Create OVG Paths */
			for(i = 0; i < PVG_ObjectPtr[element]->u32NumPaths; i++)
			{
				/* Create Path, all the other paints are subsequent paints from the first */
				if(i == 0)
				{
					PVG_OVGStartPath[element] = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
												1.0f, 0.0f, 0, 0, (unsigned int)VG_PATH_CAPABILITY_ALL);
					tempPath = PVG_OVGStartPath[element];
				}
				else
				{
					tempPath = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
												1.0f, 0.0f, 0, 0, (unsigned int)VG_PATH_CAPABILITY_ALL);
				}
				/* Append Path Data */
				vgAppendPathData(tempPath, PVG_ObjectPtr[element]->sptrPaths[i].u32NumSegments,
				PVG_ObjectPtr[element]->sptrPaths[i].u8ptrCommands, PVG_ObjectPtr[element]->sptrPaths[i].fptrCommands);
			}
		}
	}
	return element;
}

PVG_Error_t PVG_Draw(PVG_Element_t element, int Pfr, int Pto)
{
	unsigned int i,x;
	unsigned int j;
	VGPaintMode paintMode;
	VGPaint paint;

	if(!PVG_initialized)
	{
		return PVG_ERROR_UNINIT;
	}
	if(element < 0 || element > PVG_MAX_OBJECTS)
	{
		return PVG_ERROR_ELEMENT_RANGE;
	}
	if(PVG_ObjectPtr[element] == NULL)
	{
		return PVG_ERROR_ELEMENT_NULL;
	}

	/* Draw Paths */
	j = PVG_OVGStartPath[element];

	if ((Pfr == 0) && (Pto == 0)) {i=0; x = PVG_ObjectPtr[element]->u32NumPaths;} // if both from and to are 0, draw all paths.
	else if (Pto == 0xFFFFFFFF) {i = Pfr; x = Pfr + 1;} // if to is max draw only the path in from.
	else {i = Pfr; x = Pto;} // else draw the range.
	for(; i < x; i++)
	{

		if(PVG_ObjectPtr[element]->sptrPaths[i].u32AlphaBlend == 0)
		{
			vgSeti(VG_BLEND_MODE, VG_BLEND_SRC);
		}
		else
		{
			vgSeti(VG_BLEND_MODE, VG_BLEND_SRC_OVER);
		}

		paintMode = (VGPaintMode)PVG_ObjectPtr[element]->sptrPaths[i].u32PathType;
		if(paintMode != 0)
		{

			// Fill the parameters for the current Fill Paint
			// Check if the paint is present, if it is different than the previous one or if it is the first one.
			if(paintMode & VG_FILL_PATH)
			{
				vgSeti(VG_FILL_RULE, PVG_ObjectPtr[element]->sptrPaths[i].u32FillRule);
				paint = (VGPaint)PVG_ObjectPtr[element]->sptrPaths[i].u32FillPaint + PVG_OVGStartPaint[element];
				vgSetPaint(paint,VG_FILL_PATH);
			}

			// Fill the parameters for the current Stroke Paint
			if(paintMode & VG_STROKE_PATH)
			{
				vgSetf(VG_STROKE_LINE_WIDTH, PVG_ObjectPtr[element]->sptrPaths[i].fStrokeWidth);
				vgSeti(VG_STROKE_CAP_STYLE,	 PVG_ObjectPtr[element]->sptrPaths[i].u32StrokeCap);
				vgSeti(VG_STROKE_JOIN_STYLE, PVG_ObjectPtr[element]->sptrPaths[i].u32StrokeJoin);
				vgSetf(VG_STROKE_MITER_LIMIT,PVG_ObjectPtr[element]->sptrPaths[i].fStrokeMiterLimit);

				// Get current stroke 'dash' if any
				if (PVG_ObjectPtr[element]->sptrPaths[i].u32StrokeDash != 666)
				{
					//vgSetf(VG_STROKE_DASH_PHASE, m_pPaths[i].m_fDashPhase);
					//vgSetfv(VG_STROKE_DASH_PATTERN, m_pPaths[i].m_ui32NumDashes, m_pPaths[i].m_fDashValues);
				}
				else
				{
					vgSetfv(VG_STROKE_DASH_PATTERN, 0, (VGfloat *) 0); // disable dashes
				}

				paint = (VGPaint)PVG_ObjectPtr[element]->sptrPaths[i].u32StrokePaint + PVG_OVGStartPaint[element];
				vgSetPaint(paint, VG_STROKE_PATH);
			}

		}
		vgSetColor(paint, 0x0048FFFF);
		vgDrawPath((VGPath)(j+i), (VGPaintMode)PVG_ObjectPtr[element]->sptrPaths[i].u32PathType);
	}

	return PVG_ERROR_OK;
}


PVG_Error_t PVG_Destroy(PVG_Element_t PVGElement)
{
	unsigned int i;
	if(!PVG_initialized)
	{
		return PVG_ERROR_UNINIT;
	}
	if(PVGElement < 0 || PVGElement > PVG_MAX_OBJECTS)
	{
		return PVG_ERROR_ELEMENT_RANGE;
	}
	if(PVG_ObjectPtr[PVGElement] == NULL)
	{
		return PVG_ERROR_ELEMENT_NULL;
	}

	for(i = 0; i < PVG_ObjectPtr[PVGElement]->u32NumPaths ;i++)
	{
		vgDestroyPath((VGPath)(PVG_OVGStartPath[PVGElement] + i));
	}

	for(i = 0; i < PVG_ObjectPtr[PVGElement]->u32NumPaints ;i++)
	{
		vgDestroyPaint((VGPaint)(PVG_OVGStartPaint[PVGElement] + i));
	}

	return PVG_ERROR_OK;
}
