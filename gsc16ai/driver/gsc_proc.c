// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL: http://subversion:8080/svn/gsc/trunk/drivers/gsc_common/linux/driver/gsc_proc.c $
// $Rev: 9009 $
// $Date$

#include "gsc_main.h"
#include "main.h"



/******************************************************************************
*
*   Function:   gsc_proc_read
*
*   Purpose:
*
*       Implement the read service for the module's /proc file system entry.
*		Read the documentation on this service for details, as we ignore some
*		arguments according to our needs and the documentation.
*
*   Arguments:
*
*       page    The data produced is put here.
*
*       start   Records pointer to where the data in "page" begins.
*
*       offset  The offset into the file where we're to begin.
*
*       count   The limit to the amount of data we can produce.
*
*       eof		Set this flag when we hit EOF.
*
*       data    A private data item that we may use, but don't.
*
*   Returned:
*
*       int The number of characters written.
*
******************************************************************************/
int gsc_proc_read(
	char*  page,
    char** start,
    off_t  offset,
    int    count,
    int*   eof,
    void*  data)
{
#define _PROC_MSG_SIZE  1024

#if PAGE_SIZE < _PROC_MSG_SIZE
#error  "/proc file size may be too big."
#endif
	const char*	comma;
    int			i;
	int			j;
	const char*	support;

	for (;;)	// A convenience loop.
	{
		i	= gsc_module_count_inc();

		if (i)
		{
			i	= 0;
			break;
		}

		switch (gsc_global.ioctl_32bit)
		{
			default:
			case GSC_IOCTL_32BIT_ERROR:

				support	= "INTERNAL ERROR";
				break;

			case GSC_IOCTL_32BIT_DISABLED:

				support	= "disabled";
				break;

			case GSC_IOCTL_32BIT_NATIVE:

				support	= "yes (native)";
				break;

			case GSC_IOCTL_32BIT_COMPAT:
			case GSC_IOCTL_32BIT_TRANSLATE:

				support	= "yes";
				break;

			case GSC_IOCTL_32BIT_NONE:

				support	= "no";
				break;
		}

		i   = sprintf(	page,
						"version: %s\n"
						"built: %s\n"
						"32-bit support: %s\n"
						"boards: %d\n"
						"models: ",
						GSC_DRIVER_VERSION,
						gsc_global.built,
						support,
						gsc_global.dev_qty);
		comma	= "";

		for (j = 0; j < ARRAY_ELEMENTS(gsc_global.dev_list); j++)
		{
			if (gsc_global.dev_list[j])
			{
				i		= strlen(page);
				i		= sprintf(	page + i,
									"%s%s",
									comma,
									gsc_global.dev_list[j]->model);
				comma	= ",";
			}
		}

		strcat(page, "\n");

#if defined(DEV_SUPPORTS_PROC_ID_STR)

		// This is intended to accommodate board identification
		// for those that have user jumpers.
		i		= strlen(page);
		i		= sprintf(page + i, "ids: ");
		comma	= "";

		for (j = 0; j < ARRAY_ELEMENTS(gsc_global.dev_list); j++)
		{
			if (gsc_global.dev_list[j])
			{
				i		= strlen(page);
				i		= sprintf(	page + i,
									"%s%s",
									comma,
									gsc_global.dev_list[j]->proc_id_str);
				comma	= ",";
			}
		}

		strcat(page, "\n");

#endif

		i   = strlen(page) + 1;

		if (i >= PAGE_SIZE)
		{
			printk(	"%s: gsc_proc_read, line %d:"
					" /proc/%s is larger than PAGE_SIZE\n",
					DEV_NAME,
					__LINE__,
					DEV_NAME);
			i	= PAGE_SIZE - 1;
		}

		i--;
		eof[0]  = 1;
		gsc_module_count_dec();
		break;
	}

    return(i);
}



/******************************************************************************
*
*	Function:	gsc_proc_start
*
*	Purpose:
*
*		initialize use of the /proc file system.
*
*	Arguments:
*
*		None.
*
*	Returned:
*
*		0	All went well.
*		!0	We failed.
*
******************************************************************************/

int gsc_proc_start(void)
{
	struct proc_dir_entry*	proc;
	int						ret;

	if (gsc_global.dev_qty)
	{
		proc	= create_proc_entry(DEV_NAME, S_IRUGO, NULL);

		if (proc)
		{
			ret						= 0;
			gsc_global.proc_enabled	= 1;
			proc->read_proc			= gsc_proc_read;
			PROC_GET_INFO(proc, gsc_proc_get_info);
		}
		else
		{
			ret	= 1;
			printk(	"%s: gsc_proc_start, line %d:"
					" create_proc_entry() failure.\n",
					DEV_NAME,
					__LINE__);
		}
	}
	else
	{
		ret	= 0;
	}

	return(ret);
}



/******************************************************************************
*
*	Function:	gsc_proc_stop
*
*	Purpose:
*
*		Stop use of the /proc file system.
*
*	Arguments:
*
*		None.
*
*	Returned:
*
*		None.
*
******************************************************************************/

void gsc_proc_stop(void)
{
	if (gsc_global.proc_enabled)
	{
		remove_proc_entry(DEV_NAME, NULL);
		gsc_global.proc_enabled	= 0;
	}
}


