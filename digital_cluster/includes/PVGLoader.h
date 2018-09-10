/**
  Copyright (c) 2010 Freescale Semiconductor

  \file       PVG.h
  \brief      This is the PVG Driver Header File
  \brief      Translates the PVG vector graphics data into OpenVG Calls
  \author     Freescale Semiconductor
  \author     Systems Enablment
  \author     Ioseph Martinez, b06623
  \version    0.8
  \date       07/Jan/2010

  * History:  07/Jan/2010 - Initial Version

* Copyright (c) 2010, Freescale, Inc.  All rights reserved.
*
*
* No part of this document must be reproduced in any form - including copied,
* transcribed, printed or by any electronic means - without specific written
* permission from Freescale Semiconductor.
*

*/

#ifndef _PVGLOADER_H_
#define _PVGLOADER_H_

#include <stddef.h>

/* Configuration Parameters */
#define PVG_MAX_OBJECTS 64


/* PVG Error type Enumeration */
typedef  enum
{
        PVG_ERROR_OK,               /**< Succesful execution return code*/
        PVG_ERROR_UNINIT,           /**< Driver is unitialized */
        PVG_ERROR_OBJECT_NULL,      /**< The input PVG object is null */
        PVG_ERROR_ELEMENT_NULL,     /**< The input PVG element has a null object */
        PVG_ERROR_ELEMENT_RANGE,    /**< The input PVG element range is wrong */
} PVG_Error_t;


/* PVG Ramp type object definition */
typedef  struct
{
    unsigned int u32NumRamps;       /**< Number of Ramps*/
    const float*  fptrRampsStart;         /**< Points to the start of the current ramp*/
} PVG_Ramps_t;

/* PVG Paints type object definition */
typedef  struct
{
    unsigned int u32PaintID;        /**< Paint ID*/
    unsigned int u32PaintStyle;     /**< Color, linear gradient, radial grad. or pattern types */
    float fPaintParams[5];          /**< Number of Ramps*/
    unsigned int u32Ramp;           /**< Points to the RAMP index, ignored if paint it is not gradient type */
    unsigned int u32Pattern;        /**< Points to the PATTERN index, ignored if paint it is not pattern type */
} PVG_Paints_t;

/* PVG Path object type definition */
typedef  struct
{
    unsigned int u32PathType;       /**< Tells the path type: fill or stroke */
    unsigned int u32Clipping;       /**< Clipping = 1, not clipping = 0 */
    unsigned int u32AlphaBlend;     /**< 0 = no alpha. 1 = 0% opaque. 255 = 100% opaque */
    unsigned int u32StrokePaint;    /**< Index to stroke paint in paints */
    unsigned int u32FillPaint;      /**< Index to fill paint in paints */
    unsigned int u32StrokeDash;     /**< 0 if none, otherwise index to dash structure array */
    unsigned int u32FillRule;       /**< even_odd or non_zero */
    unsigned int u32StrokeJoin;     /**< miter, join_round or join_bevel */
    unsigned int u32StrokeCap;      /**< but, round or square */
    float fStrokeWidth;             /**< Self Explanatory */
    float fStrokeMiterLimit;        /**< Self Explanatory */
    unsigned int u32NumSegments;    /**< Number of segments */
    unsigned int u32DataSize;       /**< Byte size for data */
    const unsigned char* u8ptrCommands;       /**< Segment commands pointer */
    const float*      fptrCommands;       /**< Data information for commands (pointer) */
} PVG_Paths_t;

/* PVG Dashes object type definition */
typedef  struct
{
    unsigned int u32PathType;       /**< Tells the path type: fill or stroke */
} PVG_Dashes_t;

/* PVG Pattern object type definition */
typedef  struct
{
    unsigned int u32PathType;       /**< Tells the path type: fill or stroke */
} PVG_Patterns_t;

/* PVG Object type definition */
typedef  struct
{
    unsigned int u32NumPaths;       /**< Total number of paths */
    unsigned int u32NumPaints;      /**< Total number of Paints */
    unsigned int u32NumRamps;       /**< Total number of Ramps */
    unsigned int u32NumDashes;      /**< Total number of Dashes */
    unsigned int u32NumPatterns;    /**< Total number of Patterns */
    float fLeft;                    /**< Self-Explanatory*/
    float fTop;                     /**< Self-Explanatory*/
    float fRight;                   /**< Self-Explanatory*/
    float fBottom;                  /**< Self-Explanatory*/
    const PVG_Ramps_t* sptrRamps;         /**< Pointer to Ramp structure array data */
    const PVG_Paints_t* sptrPaints;       /**< Pointer to Paints structure array data */
    const PVG_Paths_t* sptrPaths;         /**< Pointer to Paths structure array data */
    const PVG_Dashes_t* sptrDashes;       /**< Pointer to Dashes structure array data */
    const PVG_Patterns_t* sptrPatterns;   /**< Pointer to Patterns structure array data */
} PVG_Object_t;

typedef int PVG_Element_t;

/**
* \brief    Initializes the PVG loader driver, destroys any previous PVG Element and OpenVG entity.
* \author   IM, b06623
* \param    void
* \return   void
*/
void PVG_Init(void);

/**
* \brief    Loads a PVG object (resident in memory) using OpenVG calls
* \author   IM, b06623
* \param    PVGObject a PVG object created in Freescale Font Generator that has the Vector Graphics information.
* \return   Returns a PVG Element which is actually an index to the PVG Object
*/
PVG_Element_t PVG_Load(const PVG_Object_t* PVGObject);


/**
* \brief    Draws a PVG element. A 3D transform will be used if defined in PVG_SetTransform
* \author   IM, b06623
* \param    PVGElement a PVG element which will be drawn and transformed using the defined matrix
* \return   Returns a PVG Error code of the operation. If Succesful returns PVG_ERROR_OK
*/
PVG_Error_t PVG_Draw(PVG_Element_t element, int Pfr, int Pto);

/**
* \brief    Destroys all the OpenVG paints and paths related to the specified PVG Element
* \author   IM, b06623
* \param    PVGElement a PVG element which will be destroyed from the OpenVG context.
* \return   Returns a PVG Error code of the operation. If Succesful returns PVG_ERROR_OK
*/
PVG_Error_t PVG_Destroy(PVG_Element_t PVGElement);



#endif
