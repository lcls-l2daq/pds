// $URL: http://subversion:8080/svn/gsc/trunk/drivers/LINUX/16AI32SSC/docsrc/16ai32ssc_dsl.h $
// $Rev: 1588 $
// $Date$

#ifndef __16AI32SSC_DSL_HH
#define __16AI32SSC_DSL_HH

#include "driver/16ai32ssc.h"

// prototypes	***************************************************************

#ifdef __cplusplus
extern "C" {
#endif

// int	ai32ssc_dsl_close(int fd);
int	ai32ssc_dsl_ioctl(int fd, int request, void *arg);
// int	ai32ssc_dsl_open(unsigned int board);
int	ai32ssc_dsl_read(int fd, __u32 *buf, size_t samples);

#ifdef __cplusplus
}
#endif

#endif
