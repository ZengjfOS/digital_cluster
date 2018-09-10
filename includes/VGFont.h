/**
  Copyright (c) 2010 Freescale Semiconductor
  
  \file  	  VGFont.h
  \brief	  This is the VGFont Driver Header File
  \brief	  Interprets vgf font files and loads them as OVG fonts
  \author	  Freescale Semiconductor
  \author	  Systems Enablment
  \author	  Ioseph Martinez, b06623
  \version	  0.9
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

#ifndef	_VGFONT_H_
#define	_VGFONT_H_


#include <VG/openvg.h>
#include "wchar.h"

/* Configuration Parameters */
#define VGF_MAX_FONTS	8

/* VGFONT Error type Enumeration */
typedef  enum
{
        VGF_ERROR_OK,				/**< Succesful execution return code*/ 
		VGF_ERROR_UNINIT,			/**< Driver is unitialized */ 
		VGF_ERROR_FONT_NULL,		/**< The input VGFONT font is null */
		VGF_ERROR_FONT_FULL,		/**< The maximum number of VGF fonts had been allocated */
		VGF_ERROR_ZERO_GLYPHS		/**< String is not null terminater or too big */
} VGF_Error_t;

/* VGFONT Printing options Enumeration */
typedef  enum
{
		VGF_DEFAULT				=	(0<<0),		/**< Default mode: Left to right, no kern., left alig. no transform */ 

        VGF_KERNING				=	(1<<0),		/**< Kerning (if the font has it available) */ 

		VGF_LEFT2RIGHT			=	(1<<1),		/**< Left to right, char order is kept the same (default)*/ 			
		VGF_RIGHT2LEFT			=	(1<<2),		/**< Right to left, char order inverted, if align isn't specified uses right aligned */ 	
		VGF_TOP2BOTTOM			=	(1<<3),		/**< Top to bottom, text glyph advances is done on the Y axis */ 

		VGF_LEFTALIG			=	(1<<4),		/**< Text is aligned to the left (default) */ 
		VGF_CENTERALIG			=	(1<<5),		/**< Text is aligned to the center */ 
		VGF_RIGHTALIG			=	(1<<6),		/**< Text is aligned to the right */ 

		VGF_ROTATE90			=	(1<<7),		/**< After all operations, text is rotated 90 degrees */ 
		VGF_ROTATE270			=	(1<<8),		/**< After all operations, text is rotated 270 degrees */ 
		VGF_MIRRORV				=	(1<<9),		/**< After all operations, text is mirrored vertically */ 
		VGF_MIRRORH				=	(1<<10),	/**< After all operations, text is mirrored horizontally */

		VGF_FILL				=	(1<<11),	/**< Fill the paths with the current fill paint (default) */
		VGF_STROKE				=	(1<<12)		/**< Stroke the paths with the current stroke paint (can be used along with fill) */
} VGF_Options_t;

/* VGF Charset row struct */
typedef  struct
{	
	const unsigned short	xadvance;				/**< Defines the scapement for glyphs on X */ 
	const unsigned int	cmdIdx;						/**< Defines the index of the starting command for the glyph */ 
	const unsigned int	datIdx;						/**< Defines the index of the starting data for the glyph */ 
} VGF_CharSet_t;

/* VGF Kerning row struct */
typedef  struct
{
	const unsigned short unicode_g2;				/**< Defines second glyph unicode value for kerning */ 	
	const unsigned short kern;						/**< Defines the kerning value */ 	
	const unsigned short orientation;				/**< Defines the kerning orientation */ 	
} VGF_Kerning_t;

/* VGF font type definition */
typedef  struct
{
	const unsigned int u32NumGlyphs;			/**< Number of glyphs on this font */
	const unsigned short unitsperemm;			/**< Original size defined on units per emm */
	const short ascent;							/**< defines the upper limit of this font */
	const short descent;						/**< defines the lower limit of this font */

	const unsigned short *unicode_glyph;		/**< Unicode index for charset table */
	const VGF_CharSet_t *CharSet;				/**< Charset table */
	const unsigned char *CmdSet;				/**< OpenVG Path command data-block */
	const unsigned int *DataSet;				/**< OpenVG Path data data-block */

	const short numKerns;						/**< Number of rows for kerning table */
	const unsigned short *unicode_g1;			/**< Unicode index for kerning table */
	const VGF_Kerning_t *kern_data;				/**< Kerning table */
} VGF_Font_t;

typedef unsigned int VGF_Element_t;

#define VGF_NOPATH	0xFFFF

void VGF_Init(void);
VGF_Element_t VGF_Load(VGF_Font_t* font);
VGF_Error_t VGF_Destroy(VGF_Element_t font);
VGFont VGF_GetHandle(VGF_Element_t element);
VGF_Error_t VGF_PrepareText(VGF_Element_t font, wchar_t *text, VGuint	size, VGF_Options_t options, VGfloat *data);

#endif
