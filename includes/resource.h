/* Global Variables to be used by the Graphics Application */
#include <Windows.h>


extern "C" VGint windowWidth;
extern "C" VGint windowHeight;

extern "C" VGfloat windowScaleX;
extern "C" VGfloat windowScaleY;

extern "C" VGint mouse_x;
extern "C" VGint mouse_y;

extern BOOL openVGInitialized;

unsigned int getElapsedTimeMS(void);