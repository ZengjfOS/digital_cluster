/**
  Copyright (c) 2009 Freescale Semiconductor
* 
* No part of this document must be reproduced in any form - including copied,
* transcribed, printed or by any electronic means - without specific written
* permission from Freescale Semiconductor.
*
* Auth: D.Bogavac 2009-09-07
*
* V0.1 Initial code to demo speed modulated motion blur & 2.5D transformations.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include "mxcfb.h"
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>

#define UNREF(X) ((void)(X))
//#define MXCFB_WAIT_FOR_VSYNC	_IOW('F', 0x20, u_int32_t)

#define GET_STATS 1
#define VG_on	1

#include "VG/openvg.h"
#include "VG/vgu.h"
#include "EGL/egl.h"

//#include <vdk.h>

#include "meter.h"
#include "FB_utils.h"
#include "VectorMeter.h"
#include "w_meterRS.h"

/*
typedef struct 
{
	const int winWidth;
	const int winHeight;
	const int redSize;
	const int greenSize;
	const int blueSize;
	const int alphaSize;
	const int depthSize;
	const int stencilSize;
	const int samples;
	const int openvgbit;
	const int frameCount;
	const int save;
	const int fps;
}appAttribs_t;
*/
struct appAttribs
{
	int winWidth;
	int winHeight;
	int redSize;
	int greenSize;
	int blueSize;
	int alphaSize;
	int depthSize;
	int stencilSize;
	int samples;
	int openvgbit;
	int frameCount;
	int save;
	int fps;
};

struct appAttribs const cmdAttrib = {1920,1080,8,8,8,8,-1,-1,-1,2,0,0,0};
/*--------------------------------------------------------------*/


/* VG EGL variable*/
static EGLDisplay			egldisplay;
static EGLConfig			eglconfigs[64];
static EGLConfig			eglconfig;
static EGLSurface			eglsurface;
static EGLContext			eglcontext;

//static VGImage imgBase_Speed;
//static VGImage imgBig_Needle;
//static VGImage imgRPM_base;
//static VGImage imgSmall_needle, imgPattern;
//static VGfloat m1[9], m2[9];
int frames=0;
EGLint width = 0;
EGLint height = 0;

/*--------------------------------------------------------------*/

/*--------------------------------------------------------------*/

#define FB_MULTI_BUFFER 2
#define FB_FRAMEBUFFER 1
static void handle_egl_error(const char *name)
{
  static char * const error_strings[] =
    {
      "EGL_SUCCESS",
      "EGL_NOT_INITIALIZED",
      "EGL_BAD_ACCESS",
      "EGL_BAD_ALLOC",
      "EGL_BAD_ATTRIBUTE",
      "EGL_BAD_CONFIG",
      "EGL_BAD_CONTEXT",
      "EGL_BAD_CURRENT_SURFACE",
      "EGL_BAD_DISPLAY",
      "EGL_BAD_MATCH",
      "EGL_BAD_NATIVE_PIXMAP",
      "EGL_BAD_NATIVE_WINDOW",
      "EGL_BAD_PARAMETER",
      "EGL_BAD_SURFACE"
    };
  EGLint error_code;
  error_code=eglGetError();

  fprintf(stderr," %s: egl error \"%s\" (0x%x)\n", name,
          error_strings[error_code-EGL_SUCCESS], error_code);
  exit(EXIT_FAILURE);  
}



/*--------------------------------------------------------------*/

int initEGL(void)
{

// Assumes there are global vars called EGLint width, height.
// If not add them locally here
int errorcode=0, i;

  static const EGLint gl_context_attribs[] =
    {
      EGL_CONTEXT_CLIENT_VERSION, 1,
      EGL_NONE
    };
   static EGLint s_configAttribs[] =
        {
                EGL_RED_SIZE,               8,
                EGL_GREEN_SIZE,             8,
                EGL_BLUE_SIZE,              8,
                EGL_ALPHA_SIZE,             8,
                //EGL_DEPTH_SIZE,         16,
               EGL_STENCIL_SIZE,       0,
                EGL_SURFACE_TYPE,           EGL_WINDOW_BIT,
                EGL_SAMPLES,            0,
                EGL_RENDERABLE_TYPE,    EGL_OPENVG_BIT,
                EGL_NONE
        };
/*
	static EGLint s_configAttribs[] =
	{
		EGL_RED_SIZE,		8,
		EGL_GREEN_SIZE, 	8,
		EGL_BLUE_SIZE,		8,
		EGL_ALPHA_SIZE, 	8,
		EGL_SAMPLES,		2,
		//EGL_SURFACE_TYPE,	EGL_SWAP_BEHAVIOR_PRESERVED_BIT,
        EGL_STENCIL_SIZE,   0,
        //EGL_DEPTH_SIZE,		16,
	    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
*/
	EGLint numconfigs;
	EGLint majorVersion;
  	EGLint minorVersion;
	//setenv("FB_FRAMEBUFFER_0", "/dev/fb1", 1);
	#if(1)  // Vivante GPU version 
	  EGLNativeDisplayType native_display = fbGetDisplayByIndex(0);
	printf("fbGetdisplay return 0x%x\n", native_display);
	fbGetDisplayGeometry(native_display, &width, &height);
	printf("Display w,h %d,%d\n",width, height);	
	EGLNativeWindowType  native_window  = fbCreateWindow(native_display, 0, 0, 0, 0);
	printf("fbCreateWindow return 0x%x\n", native_window);
	#else //AMD Z430 version
	  EGLNativeWindowType  native_window = open("/dev/fb0", O_RDWR);
	#endif
	//
	eglBindAPI(EGL_OPENVG_API);	
	//	
	egldisplay = eglGetDisplay(native_display);
	printf("egldisplay 0x%x\n",egldisplay);
	//	
	eglInitialize(egldisplay, NULL, NULL);
	errorcode = eglGetError();	
	if (errorcode != EGL_SUCCESS){
	  printf("%s:eglInitialize EGL Error, errorcode 0x%x\n", __func__, errorcode);
	  exit(EXIT_FAILURE); 
	}
	else printf("eglInitialize OK\n");
	//
	eglGetConfigs(egldisplay, NULL, 0, &numconfigs);
	printf("numconfigs %d\n",numconfigs);
	//
	eglChooseConfig(egldisplay, s_configAttribs, &eglconfigs[0], 32, &numconfigs);
	printf("%s:configs %d\n", __func__, numconfigs);
	assert(eglGetError() == EGL_SUCCESS);
	//assert(numconfigs < 1);
     for (i = 0; i < numconfigs; i++)
     {

        int redSize = 0;
        int greenSize = 0;
        int blueSize = 0;
        int alphaSize = 0;
        int depthSize = 0;
        int stencilSize = 0;
        int samples = 0;
        int id = 0;
        int openglbit = 0;
	int surfacebit = 0;

        eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_RED_SIZE, &redSize);
        eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_GREEN_SIZE, &greenSize);
        eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_BLUE_SIZE, &blueSize);
        eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_ALPHA_SIZE, &alphaSize);
        eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_DEPTH_SIZE, &depthSize);
        eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_STENCIL_SIZE, &stencilSize);
        eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_SAMPLES, &samples);
        eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_RENDERABLE_TYPE, &openglbit);
        eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_CONFIG_ID, &id);
        eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_SURFACE_TYPE, &surfacebit);

	if (((cmdAttrib.redSize != -1)&&(redSize != cmdAttrib.redSize)) ||
		    ((cmdAttrib.greenSize != -1)&&(greenSize != cmdAttrib.greenSize)) ||
		    ((cmdAttrib.blueSize != -1)&&(blueSize != cmdAttrib.blueSize)) ||
		    ((cmdAttrib.alphaSize != -1)&&(alphaSize != cmdAttrib.alphaSize)) ||
		    ((cmdAttrib.depthSize != -1)&&(depthSize != cmdAttrib.depthSize)) ||
		    ((cmdAttrib.stencilSize != -1)&&(stencilSize != cmdAttrib.stencilSize)) ||
		    ((cmdAttrib.samples != -1)&&(samples != cmdAttrib.samples))
	)
	{
	   printf("config %i no match\n",i);
	   printf("id=%d, a,b,g,r=%d,%d,%d,%d, d,s=%d,%d, AA=%d,openglbit=%d, surfacebit=%d\n",
          id, alphaSize, blueSize, greenSize, redSize, depthSize, stencilSize,
          samples, openglbit, surfacebit);
		
	}else
	{
	   eglconfig = eglconfigs[i];
           printf("%s:use config 0x%x\n", __func__, eglconfig);		

           if (eglconfig != NULL)
           {
               printf("id=%d, a,b,g,r=%d,%d,%d,%d, d,s=%d,%d, AA=%d,openvgbit=%d, surfacebit=%d\n",
                       id, alphaSize, blueSize, greenSize, redSize, depthSize, stencilSize,
                       samples, openglbit, surfacebit);
           }
  	   break;
	}
     }

        //	
	eglsurface = eglCreateWindowSurface(egldisplay, eglconfig, native_window, NULL);
	if (eglsurface == EGL_NO_SURFACE)
	    {
	      handle_egl_error("eglCreateWindowsSurface");
	    }else{
	    printf("%s:eglCreateWindowsSurface ok\n", __func__);  
	}	
	//


	eglcontext = eglCreateContext(egldisplay, eglconfig, 0, NULL);
	if (eglcontext == EGL_NO_CONTEXT)
	    {
	      handle_egl_error("eglCreateContext");

	    }else{
	    printf("%s:eglCreateContext ok\n", __func__);  
	 }
	//
	eglMakeCurrent(egldisplay, eglsurface, eglsurface, eglcontext);

	errorcode = eglGetError();	
	if (errorcode != EGL_SUCCESS){
	  printf("%s:eglMakeCurrent Failed, errorcode 0x%x\n", __func__, errorcode);
	  exit(EXIT_FAILURE); 

	}
	else printf("eglMakeCurrent OK\n");
	//assert(eglGetError() == EGL_SUCCESS);
	vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_BETTER);
	//

	printf("GPU ..%s\n", vgGetString(VG_RENDERER));
	printf("Vendor ..%s\n", vgGetString(VG_VENDOR));
	printf("Version ..%s\n", vgGetString(VG_VERSION));

}

/*
void init(NativeWindowType window)
{
int errorcode=0, width, height, i;
#ifndef LINUX
#define LINUX
#endif
#ifndef EGL_API_FB
#define EGL_API_FB   1
#endif


        static EGLint s_configAttribsVG[] =
        {
                EGL_RED_SIZE,               8,
                EGL_GREEN_SIZE,             8,
                EGL_BLUE_SIZE,              8,
                EGL_ALPHA_SIZE,             8,
                EGL_DEPTH_SIZE,         16,
               EGL_STENCIL_SIZE,       0,
                EGL_SURFACE_TYPE,           EGL_WINDOW_BIT,
                EGL_SAMPLES,            0,
                EGL_RENDERABLE_TYPE,    EGL_OPENVG_BIT,
                EGL_NONE
        };

	static const EGLint s_configAttribsPB1[] =
	{
		EGL_RED_SIZE,		8,
		EGL_GREEN_SIZE, 	8,
		EGL_BLUE_SIZE,		8,
		EGL_ALPHA_SIZE, 	8,
		EGL_LUMINANCE_SIZE, EGL_DONT_CARE,			//EGL_DONT_CARE
		EGL_SURFACE_TYPE,	EGL_PBUFFER_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENVG_BIT,
		EGL_NONE
	};
 static const EGLint gl_context_attribs[] =
    {
      EGL_CONTEXT_CLIENT_VERSION, 1,
      EGL_NONE
    };
  
  static const EGLint s_configAttribs[] =
    {
      EGL_RED_SIZE,	8,
      EGL_GREEN_SIZE, 	8,
      EGL_BLUE_SIZE,	8,
      EGL_ALPHA_SIZE, 	8,
      EGL_NONE
    };

	//#define VG_THROUGH_3D    0
	EGLint numconfigs;
	EGLint majorVersion;
  	EGLint minorVersion;

	#if(1)  // Vivante GPU version 
	  EGLNativeDisplayType native_display = fbGetDisplay();
	printf("fbGetdisplay return 0x%x\n", native_display);
	fbGetDisplayGeometry(native_display, &width, &height);
	printf("Display w,h %d,%d\n",width, height);	
	  EGLNativeWindowType  native_window  = fbCreateWindow(native_display, 0, 0, 0, 0);
	printf("fbCreateWindow return 0x%x\n", native_window);
	#else
	  EGLNativeWindowType  native_window = open("/dev/fb0", O_RDWR);
	#endif
	//
	eglBindAPI(EGL_OPENVG_API);	
	//	
	egldisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	printf("egldisplay 0x%x\n",egldisplay);
	//	
	eglInitialize(egldisplay, NULL, NULL);
	errorcode = eglGetError();	
	if (errorcode != EGL_SUCCESS){
	  printf("%s:eglInitialize EGL Error, errorcode 0x%x\n", __func__, errorcode);
	  exit(EXIT_FAILURE); 
	}
	else printf("eglInitialize OK\n");
	//
	eglGetConfigs(egldisplay, NULL, 0, &numconfigs);
	printf("numconfigs %d\n",numconfigs);
	//
	eglChooseConfig(egldisplay, s_configAttribsVG, &eglconfigs[0], 10, &numconfigs);
	printf("%s:configs %d\n", __func__, numconfigs);
	assert(eglGetError() == EGL_SUCCESS);
	//assert(numconfigs < 1);
     for (i = 0; i < numconfigs; i++)
     {

        int redSize = 0;
        int greenSize = 0;
        int blueSize = 0;
        int alphaSize = 0;
        int depthSize = 0;
        int stencilSize = 0;
        int samples = 0;
        int id = 0;
        int openvgbit = 0;
	int surfacebit = 0;

        eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_RED_SIZE, &redSize);
        eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_GREEN_SIZE, &greenSize);
        eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_BLUE_SIZE, &blueSize);
        eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_ALPHA_SIZE, &alphaSize);
        eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_DEPTH_SIZE, &depthSize);
        eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_STENCIL_SIZE, &stencilSize);
        eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_SAMPLES, &samples);
        eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_RENDERABLE_TYPE, &openvgbit);
        eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_CONFIG_ID, &id);
        eglGetConfigAttrib(egldisplay, eglconfigs[i], EGL_SURFACE_TYPE, &surfacebit);

	if (((cmdAttrib.redSize != -1)&&(redSize != cmdAttrib.redSize)) ||
		    ((cmdAttrib.greenSize != -1)&&(greenSize != cmdAttrib.greenSize)) ||
		    ((cmdAttrib.blueSize != -1)&&(blueSize != cmdAttrib.blueSize)) ||
		    ((cmdAttrib.alphaSize != -1)&&(alphaSize != cmdAttrib.alphaSize)) ||
		    ((cmdAttrib.depthSize != -1)&&(depthSize != cmdAttrib.depthSize)) ||
		    ((cmdAttrib.stencilSize != -1)&&(stencilSize != cmdAttrib.stencilSize)) ||
		    ((cmdAttrib.samples != -1)&&(samples != cmdAttrib.samples))
	)
	{
	   printf("config %i no match\n",i);
		
	}else
	{
	   eglconfig = eglconfigs[i];
           printf("%s:use config 0x%x\n", __func__, eglconfig);		

           if (eglconfig != NULL)
           {
               printf("id=%d, a,b,g,r=%d,%d,%d,%d, d,s=%d,%d, AA=%d,openvgbit=%d, surfacebit=%d\n",
                       id, alphaSize, blueSize, greenSize, redSize, depthSize, stencilSize,
                       samples, openvgbit, surfacebit);
           }
  	   break;
	}
     }

        //	
	eglsurface = eglCreateWindowSurface(egldisplay, eglconfig, native_window, NULL);
	if (eglsurface == EGL_NO_SURFACE)
	    {
	      handle_egl_error("eglCreateWindowsSurface");
	    }else{
	    printf("%s:eglCreateWindowsSurface ok\n", __func__);  
	}	
	//
	eglcontext = eglCreateContext(egldisplay, eglconfig, 0, NULL);
	if (eglcontext == EGL_NO_CONTEXT)
	    {
	      handle_egl_error("eglCreateContext");
	    }else{
	    printf("%s:eglCreateContext ok\n", __func__);  
	 }
	//
	eglMakeCurrent(egldisplay, eglsurface, eglsurface, eglcontext);
	errorcode = eglGetError();	
	if (errorcode != EGL_SUCCESS){
	  printf("%s:eglMakeCurrent Failed, errorcode 0x%x\n", __func__, errorcode);
	  exit(EXIT_FAILURE); 
	}
	else printf("eglMakeCurrent OK\n");
	//assert(eglGetError() == EGL_SUCCESS);
	vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_BETTER);
	//
	printf("GPU ..%s\n", vgGetString(VG_RENDERER));
	printf("Vendor ..%s\n", vgGetString(VG_VENDOR));
	printf("Version ..%s\n", vgGetString(VG_VERSION));
}

*/
/*--------------------------------------------------------------*/

void deinit(void)
{
//	vgDestroyImage(imgBase_Speed);
//	vgDestroyImage(imgBig_Needle);
	eglMakeCurrent(egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	assert(eglGetError() == EGL_SUCCESS);
	eglTerminate(egldisplay);
	assert(eglGetError() == EGL_SUCCESS);
	OVG_KillApp();
}


/*--------------------------------------------------------------*/

//#include <sys/select.h>
//#include <fcntl.h>
int kbhit(void)
{
  struct timeval tv;
  fd_set read_fd;

  /* Do not wait at all, not even a microsecond */
  tv.tv_sec=0;
  tv.tv_usec=0;

  /* Must be done first to initialize read_fd */
  FD_ZERO(&read_fd);

  /* Makes select() ask if input is ready:
   * 0 is the file descriptor for stdin    */
  FD_SET(0,&read_fd);

  /* The first parameter is the number of the
   * largest file descriptor to check + 1. */
  if(select(1, &read_fd, NULL, NULL, &tv) == -1)
    return 0;  /* An error occured */

  /*  read_fd now holds a bit map of files that are
   * readable. We test the entry for the standard
   * input (file 0). */
  if(FD_ISSET(0,&read_fd))
    /* Character pending on stdin */
    return 1;

  /* no characters were pending */
  return 0;
}

//#include <sys/time.h>
//#include "linux/time.h"
int main(int argc, char *argv[])
{
//	#define MXCFB_WAIT_FOR_VSYNC	_IOW('F', 0x20, int)
	VGErrorCode err;
	struct timeval tv;
	time_t curtime;	
	int w, h,frame=0;
	static VGfloat speedLatch = 0.4f, rpmLatch = 0.2f, scaleSpeed = 1.0f;
	unsigned static int starttime, best, worst, accutime, eglaccutime; 
	printf("Press Enter key to terminate \n");
	
	//Init EGL for Z160
	//init(open("/dev/fb0", O_RDWR)); // Init the surface etc.. that is used by the VG app   
	// init EGL for Vivante	
	//if (!InitEGLViv()) exit(EXIT_FAILURE); 
	if(!initEGL()) {
		printf("failed init EGL\n");
		return 0;
	}

	InitMeeter();
	//meterRS_init();
    //OVG_InitApp();   // init the OpenVG utils for fonts and OVG path load

	gettimeofday(&tv, NULL);
	//starts = tv.tv_sec;
	starttime = tv.tv_usec;
	accutime =0;
	best = 0;
	worst = 0xFFFFFFFF;

	//SetppAlpha(); //Set the FG layer to per pixel alpha 
//	SetgAlpha(0x80); // Set the FG layer alpha to global
	//FB_CLS(0,0x00000000);
	//FB_CLS(2,0x00000000);
	//SetPos(320,20,2); // move the fb2 layer
	float clearColor[4] = {0.0f,0.0f,0.0f,0.0f};
	vgSetfv(VG_CLEAR_COLOR, 4, clearColor);

	// clear both the front and back buffers
/*	vgClear(0, 0, XRES,YRES);
	vgFinish();
	eglSwapBuffers(egldisplay, eglsurface);
	vgClear(0, 0, XRES,YRES);
	FB_CLS(0,0x00000000);
	vgFinish();
	eglSwapBuffers(egldisplay, eglsurface);
	vgClear(0, 0, XRES,YRES);
	vgFinish();
	eglSwapBuffers(egldisplay, eglsurface);
*/
	// Load all the resources for the meeter movie clip
	//InitMeeter();
	// Load the base panel meeter for both meeters in both front and back buffer.
	vgSeti(VG_SCISSORING, VG_FALSE);
	RenderMeeterFirst(0, 20, SPEED_PANEL, SMALL_NEEDLE, 1.0f);
	//RenderMeeterFirst(820, 40, RPM_PANEL, SMALL_NEEDLE, 1.0f);
	vgFinish();
	eglSwapBuffers(egldisplay, eglsurface);
	RenderMeeterFirst(0, 20, SPEED_PANEL, SMALL_NEEDLE, 1.0f);
	//RenderMeeterFirst(820, 40, RPM_PANEL, SMALL_NEEDLE, 1.0f);
	vgFinish();
	eglSwapBuffers(egldisplay, eglsurface);
	RenderMeeterFirst(0, 20, SPEED_PANEL, SMALL_NEEDLE, 1.0f);
	//RenderMeeterFirst(820, 40, RPM_PANEL, SMALL_NEEDLE, 1.0f);
	vgFinish();
	eglSwapBuffers(egldisplay, eglsurface);
	RenderMeeterFirst(0, 20, SPEED_PANEL, SMALL_NEEDLE, 1.0f);
	//RenderMeeterFirst(820, 40, RPM_PANEL, SMALL_NEEDLE, 1.0f);
	vgFinish();
	eglSwapBuffers(egldisplay, eglsurface);
	RenderMeeterFirst(840, 20, RPM_PANEL, SMALL_NEEDLE, 1.0f);
	//RenderMeeterFirst(820, 40, RPM_PANEL, SMALL_NEEDLE, 1.0f);
	vgFinish();
	eglSwapBuffers(egldisplay, eglsurface);
	RenderMeeterFirst(840, 20, RPM_PANEL, SMALL_NEEDLE, 1.0f);
	//RenderMeeterFirst(820, 40, RPM_PANEL, SMALL_NEEDLE, 1.0f);
	vgFinish();
	eglSwapBuffers(egldisplay, eglsurface);
	RenderMeeterFirst(840, 20, RPM_PANEL, SMALL_NEEDLE, 1.0f);
	//RenderMeeterFirst(820, 40, RPM_PANEL, SMALL_NEEDLE, 1.0f);
	vgFinish();
	eglSwapBuffers(egldisplay, eglsurface);

/*	RenderMeeterFirst(960, 20, SPEED_PANEL, SMALL_NEEDLE, 1.0f);
	//RenderMeeterFirst(820, 40, RPM_PANEL, SMALL_NEEDLE, 1.0f);
	vgFinish();
	eglSwapBuffers(egldisplay, eglsurface);
	RenderMeeterFirst(960, 20, SPEED_PANEL, SMALL_NEEDLE, 1.0f);
	//RenderMeeterFirst(820, 40, RPM_PANEL, SMALL_NEEDLE, 1.0f);
	vgFinish();
	eglSwapBuffers(egldisplay, eglsurface);
	RenderMeeterFirst(960, 20, SPEED_PANEL, SMALL_NEEDLE, 1.0f);
	//RenderMeeterFirst(820, 40, RPM_PANEL, SMALL_NEEDLE, 1.0f);
	vgFinish();
	eglSwapBuffers(egldisplay, eglsurface);
	RenderMeeterFirst(1440, 20, RPM_PANEL, SMALL_NEEDLE, 1.0f);
	//RenderMeeterFirst(820, 40, RPM_PANEL, SMALL_NEEDLE, 1.0f);
	vgFinish();
	eglSwapBuffers(egldisplay, eglsurface);
	RenderMeeterFirst(1440, 20, RPM_PANEL, SMALL_NEEDLE, 1.0f);
	//RenderMeeterFirst(820, 40, RPM_PANEL, SMALL_NEEDLE, 1.0f);
	vgFinish();
	eglSwapBuffers(egldisplay, eglsurface);
	RenderMeeterFirst(1440, 20, RPM_PANEL, SMALL_NEEDLE, 1.0f);
	//RenderMeeterFirst(820, 40, RPM_PANEL, SMALL_NEEDLE, 1.0f);
	vgFinish();
	eglSwapBuffers(egldisplay, eglsurface);
*/	// init the speed and RPM values. 
	SpeedValue = -140.0f;
	RPMValue = -140.0f;
	RectInit();
		
	for(;;)
	{
/*
		int ch = kbhit();
		if(ch != 0)
		{
			break;
		}
*/
#if GET_STATS
		gettimeofday(&tv, NULL);
		starttime = tv.tv_usec;	
#endif
#if VG_on
		if (SpeedValue > 0.0f) speedLatch = -0.40f;
		else if (SpeedValue < -259.6f) SpeedValue = 0.0f;
	
		if (RPMValue > 0.0f) rpmLatch = -0.40f;
		else if (RPMValue < -259.6f) RPMValue = 0.0f;  

		SpeedValue += speedLatch; 
		RPMValue += rpmLatch;
		vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
		//used to keep track of what buffer we draw in. Counts from 0 to 2 because we have 3 buffers
		if (BufferFlag == 2) BufferFlag = 0;
		else BufferFlag++;
		//meterRS_drawSpeed(SpeedValue, 1.0f, 0);
	
		if (frames < 2000) {
			RenderMeeterUpdate(0, 20, SPEED_PANEL, SMALL_NEEDLE, 1.0f);
			RenderMeeterUpdate(840, 20, RPM_PANEL, SMALL_NEEDLE, 1.0f);
			//RenderMeeterUpdate(960, 20, SPEED_PANEL, SMALL_NEEDLE, 1.0f);
			//RenderMeeterUpdate(1440, 20, RPM_PANEL, SMALL_NEEDLE, 1.0f);  
		}
		else if (frames == 2001) {
			Blurr = 1; 
			printf("\n ******  Motion Blur enabled  ******\n");
			RenderMeeterUpdate(0, 20, SPEED_PANEL, SMALL_NEEDLE, 1.0f);
			RenderMeeterUpdate(840, 20, RPM_PANEL, SMALL_NEEDLE, 1.0f);  
			//RenderMeeterUpdate(960, 20, SPEED_PANEL, SMALL_NEEDLE, 1.0f);
			//RenderMeeterUpdate(1440, 20, RPM_PANEL, SMALL_NEEDLE, 1.0f);  
		}
		else if (frames < 3000) {
			RenderMeeterUpdate(0, 20, SPEED_PANEL, SMALL_NEEDLE, 1.0f);
			RenderMeeterUpdate(840, 20, RPM_PANEL, SMALL_NEEDLE, 1.0f);
			//RenderMeeterUpdate(960, 20, SPEED_PANEL, SMALL_NEEDLE, 1.0f);
			//RenderMeeterUpdate(1440, 20, RPM_PANEL, SMALL_NEEDLE, 1.0f);  
		}
		else if (frames ==3001) { 
			SpeedSign = 1; 
			printf("\n ******  Speed sign detected   ******\n");
			RenderMeeterUpdate(0, 20, SPEED_PANEL, SMALL_NEEDLE, 1.0f);
			RenderMeeterUpdate(840, 20, RPM_PANEL, SMALL_NEEDLE, 1.0f); 
			//RenderMeeterUpdate(960, 20, SPEED_PANEL, SMALL_NEEDLE, 1.0f);
			//RenderMeeterUpdate(1440, 20, RPM_PANEL, SMALL_NEEDLE, 1.0f);  
		} 
		else if (frames < 4000) {
			RenderMeeterUpdate(0, 20, SPEED_PANEL, SMALL_NEEDLE, 1.0f);
			RenderMeeterUpdate(840, 20, RPM_PANEL, SMALL_NEEDLE, 1.0f);  
			//RenderMeeterUpdate(960, 20, SPEED_PANEL, SMALL_NEEDLE, 1.0f);
			//RenderMeeterUpdate(1440, 20, RPM_PANEL, SMALL_NEEDLE, 1.0f);  
		}
		else if (frames ==4001) {
			Shadow = 1; 
			printf("\n ******  Drop shadow enabled  ****** \n"); 
			RenderMeeterUpdate(0, 20, SPEED_PANEL, SMALL_NEEDLE, 1.0f);
			RenderMeeterUpdate(840, 20, RPM_PANEL, SMALL_NEEDLE, 1.0f); 
			//RenderMeeterUpdate(960, 20, SPEED_PANEL, SMALL_NEEDLE, 1.0f);
			//RenderMeeterUpdate(1440, 20, RPM_PANEL, SMALL_NEEDLE, 1.0f);  
			SpeedSign = 6; //turn it off
		}
		else if (frames < 5000) {
			RenderMeeterUpdate(0, 20, SPEED_PANEL, SMALL_NEEDLE, 1.0f);
			RenderMeeterUpdate(840, 20, RPM_PANEL, SMALL_NEEDLE, 1.0f); 
			//RenderMeeterUpdate(960, 20, SPEED_PANEL, SMALL_NEEDLE, 1.0f);
			//RenderMeeterUpdate(1440, 20, RPM_PANEL, SMALL_NEEDLE, 1.0f);
			SpeedSign = 6; //turn it off  
		} 
		else {
			RenderMeeterUpdate(0, 20, SPEED_PANEL, BIG_NEEDLE, 1.0f);
			RenderMeeterUpdate(840, 20, RPM_PANEL, SMALL_NEEDLE, 1.0f); 
			//RenderMeeterUpdate(960, 20, SPEED_PANEL, SMALL_NEEDLE, 1.0f);
			//RenderMeeterUpdate(1440, 20, RPM_PANEL, SMALL_NEEDLE, 1.0f);  
			
		}
		//RenderMeeterFirst(840, 20, RPM_PANEL, 0, 1.04f);
		//OVG_DrawScene(); 
	vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_BETTER);
		//DrawRectangle(520,50,1);
		//DrawRectangle(520,150,2);
		//DrawRectangle(520,300,0);
		//DrawRectangle(520,450,3);
		
/*		vgClear(0,0,640,840);
		RotateMeterBase(0, 0, 0.9f);	
		RotateMeterBase(200, 80, 0.7f);
		RotateMeterBase(0, 80, 0.6f);
*/
		vgFinish();
		frames++;
#endif	
#if GET_STATS
		gettimeofday(&tv, NULL);
		if (tv.tv_usec > starttime) {
			accutime += (tv.tv_usec - starttime);
			if ((tv.tv_usec - starttime) < worst) worst = (tv.tv_usec - starttime);
			if ((tv.tv_usec - starttime) > best) best = (tv.tv_usec - starttime);
		}		
		else {
			accutime +=(1000000 - starttime + tv.tv_usec);
			if ((1000000 - starttime + tv.tv_usec) < worst) worst = (1000000 - starttime + tv.tv_usec);
			if ((1000000 - starttime + tv.tv_usec) > best) best = (1000000 - starttime + tv.tv_usec);
		}  
		starttime = tv.tv_usec;
#endif

#if VG_on

		//ioctl(VGplane, MXCFB_WAIT_FOR_VSYNC,0);
		gettimeofday(&tv, NULL);
		starttime = tv.tv_usec;  	// start timer for EGL swap measurement
		eglSwapBuffers(egldisplay, eglsurface); // swap the VG buffer
		gettimeofday(&tv, 0);		// get the current time for EGL time calculations
		if (tv.tv_usec > starttime) {
			eglaccutime += (tv.tv_usec - starttime);
		}		
		else {
			eglaccutime +=(1000000 - starttime + tv.tv_usec);
		}  

		if (!(frames %100)) { // if 100 frames have passed
			printf("2D us/frame ==> avg,%ld, fastest, %ld, slowest, %ld, EGLavg %ld\n",accutime/100, worst,
			best,eglaccutime/100);
			eglaccutime = best = accutime = 0;
			worst = 0x0FFFFFFF;		
		}

#endif
	}
	deinit();
 	return 0;
}

/*--------------------------------------------------------------*/

/*--------------------------------------------------------------*/


