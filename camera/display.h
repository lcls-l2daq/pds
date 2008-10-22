/**display.h
 * Header file for display.cc
 */

#ifndef __DISPLAY_H
#define __DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

int display_init(void);
int display_image(char *image, unsigned long size, unsigned int width, unsigned int height);

#ifdef __cplusplus
}
#endif

#endif	/* #ifndef __DISPLAY_H */
