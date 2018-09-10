
/**
  Copyright (c) 2009 Freescale Semiconductor
* 
* No part of this document must be reproduced in any form - including copied,
* transcribed, printed or by any electronic means - without specific written
* permission from Freescale Semiconductor.
*
* Auth: D.Bogavac 2009-10-31
*
* Framebuffer utilities. This code assumes that ....
* 1. FB0 and FB2 are existing devices and that FB0 is the foreground plane.
* 2. Both FB's are 32bpp 
* 2009.Oct.31, V0.1. Initial version
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "mxcfb.h"


//#define MXCFB_SET_GBL_ALPHA     _IOW('F', 0x21, struct mxcfb_gbl_alpha)
/*struct mxcfb_gbl_alpha {
	int enable;
	int alpha;
};*/

/*-------------------------------------------------*/
/* sets the position from top left. Only fb0 or fb2 works */
/*-------------------------------------------------*/
SetPos(short x, short y, unsigned char fb){

	if (fb == 0) {
	int fd = open("/dev/fb0", O_RDWR);
 		//struct fb_var_screeninfo screeninfo;
 		struct mxcfb_pos pos;
 		//ioctl(fd, FBIOGET_VSCREENINFO, &screeninfo);
 		pos.x = x;
 		pos.y = y;
 		ioctl(fd, MXCFB_SET_OVERLAY_POS, &pos);
 		close(fd);
	}
	else if (fb == 2) {
	int fd = open("/dev/fb2", O_RDWR);
 		//struct fb_var_screeninfo screeninfo;
 		struct mxcfb_pos pos;
 		//ioctl(fd, FBIOGET_VSCREENINFO, &screeninfo);
 		pos.x = x;
 		pos.y = y;
 		ioctl(fd, MXCFB_SET_OVERLAY_POS, &pos);
 		close(fd);
	}

}




/*-------------------------------------------------*/
/* Enables per pixel alpha on the FG plane */
/*-------------------------------------------------*/

SetppAlpha(){

 int fd = open("/dev/fb0", O_RDWR);
 //struct fb_var_screeninfo screeninfo;
 struct mxcfb_gbl_alpha ga;
 //ioctl(fd, FBIOGET_VSCREENINFO, &screeninfo);
 ga.enable = 0;
 ga.alpha = 0xFF;
 ioctl(fd, MXCFB_SET_GBL_ALPHA, &ga);
 close(fd);
}


/*-------------------------------------------------*/
/* Enables global alpha and sets the plane alpha */
/*-------------------------------------------------*/

SetgAlpha(unsigned char alpha){

 int fd = open("/dev/fb0", O_RDWR);
 //struct fb_var_screeninfo screeninfo;
 struct mxcfb_gbl_alpha ga;
 //ioctl(fd, FBIOGET_VSCREENINFO, &screeninfo);
 ga.enable = 1;
 ga.alpha = alpha;
 ioctl(fd, MXCFB_SET_GBL_ALPHA, &ga);
 close(fd);
}

/*-------------------------------------------------*/
/* Fills the fbdev with clearColor */
/*-------------------------------------------------*/

FB_CLS(int fbdev, int clearColor){

int row =0, column = 0, field = 0, fd;
 // open framebuffer device and read out info
 if (fbdev) fd = open("/dev/fb0", O_RDWR);
 else fd = open("/dev/fb2", O_RDWR);
 struct fb_var_screeninfo screeninfo;
  
 // continue only if 32bit colour depth
 if (screeninfo.bits_per_pixel == 32) {

	// determine size
 	int width = screeninfo.xres;
 	int height = screeninfo.yres;

  	// embed framebuffer into memory
 	unsigned int *data = (unsigned int*)
 	mmap(0, width * height * 4 ,PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  	// load the whole screen with the clearcolor
 	for (field=0; field < (width*height);field++) data[field] = clearColor;
 	// unmap framebuffer memory
 	munmap(data, width * height);
	close(fd);
 }
} //end FB_CLS()

 

