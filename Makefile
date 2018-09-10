#*-*- Makefile -*-************************************************************
#
#              Freescale Confidential and Proprietary
#
# $Source: /home/$
#
# content:     build script 
# language:    Makefile
# author:      Michael Staudenmaier
# created:     31.07.2009
# version:     $Revision: 1.3 $
# modified:    $Date: 2011/11/14 16:55:44 $
# by:          $Author: davor $
#
#*****************************************************************************

include Makerules

sources = source/OVGApp.c utils/PVGLoader.c utils/VGFont.c resources/Speed.c resources/TEST.c source/VG.c source/meter.c resources/v2_images_no_tr.c resources/v2_s_pointer.c resources/v2_b_pointer.c utils/FB_utils.c resources/shadows.c resources/PointerL.c resources/n_center_r.c resources/CPshort.c resources/SPH.c source/w_meterRS.c resources/blank.c
objects = $(subst .c,.o,$(sources))

CFLAGS += -Wall -fsigned-char -I includes/ -I source/ -I resources/ -I utils/ 
#LFLAGS += -m64

.PHONY: all
all:	vgapp

vgapp: $(objects)
	$(CC)   -o $@ $^ $(LDFLAGS_OVG)

.PHONY: clean
clean:
	rm -f vgapp $(objects)
