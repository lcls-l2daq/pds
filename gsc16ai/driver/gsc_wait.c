// This software is covered by the GNU GENERAL PUBLIC LICENSE (GPL).
// $URL:$
// $Rev$
// $Date$

#include "main.h"



// #defines	*******************************************************************

// gsc_wait_t.io flags
#if (defined(DEV_SUPPORTS_READ) && defined(DEV_SUPPORTS_WRITE))
	#define	_WAIT_IO_ALL	GSC_WAIT_IO_ALL
#elif (defined(DEV_SUPPORTS_READ))
	#define	_WAIT_IO_ALL	GSC_WAIT_IO_RX_ALL
#elif (defined(DEV_SUPPORTS_WRITE))
	#define	_WAIT_IO_ALL	GSC_WAIT_IO_TX_ALL
#else
	#define	_WAIT_IO_ALL	0
#endif



//*****************************************************************************
static void _wait_list_node_add(dev_data_t* dev, gsc_wait_node_t* node)
{
	gsc_irq_access_lock(dev, 0);
	node->next		= dev->wait_list;
	dev->wait_list	= node;
	gsc_irq_access_unlock(dev, 0);
}



//*****************************************************************************
static int _wait_list_node_remove(dev_data_t* dev, gsc_wait_node_t* node)
{
	int					found	= 0;
	gsc_wait_node_t*	list;

	// It is presumed here that access is already locked.

	if (dev->wait_list == node)
	{
		found			= 1;
		dev->wait_list	= node->next;
		node->next		= NULL;
	}
	else
	{
		for (list = dev->wait_list; list;)
		{
			if (list->next == node)
			{
				list->next	= node->next;
				node->next	= NULL;
				found		= 1;
				break;
			}
			else
			{
				list	= list->next;
			}
		}
	}

	return(found);
}



/******************************************************************************
*
*	Function:	_wait_resume
*
*	Purpose:
*
*		Resume waiting threads per the given criteria.
*
*	Arguments:
*
*		dev		The device data structure.
*
*		flags	The flags for the given criteria.
*
*		wait	The structure with the desired critiria.
*
*	Returned:
*
*		>= 0	The number of resumptions.
*
******************************************************************************/

static int _wait_resume(dev_data_t* dev, u32 flags, gsc_wait_t* wait)
{
	u32					count	= 0;
	gsc_wait_node_t*	list;
	gsc_wait_node_t*	next;
	u32					test;

	for (list = dev->wait_list; list; list = next)
	{
		next	= list->next;

		if ((list->wait->flags & GSC_WAIT_FLAG_INTERNAL) !=
			(wait->flags & GSC_WAIT_FLAG_INTERNAL))
		{
			continue;
		}

		test	= list->wait->main & wait->main;

		if (test)
		{
			count++;
			list->wait->flags	= flags;
			list->wait->main	= test;
			list->wait->gsc		= 0;
			list->wait->alt		= 0;
			list->wait->io		= 0;
			_wait_list_node_remove(dev, list);
			EVENT_RESUME_IRQ(&list->queue, list->condition);
			continue;
		}

		test	= list->wait->gsc & wait->gsc;

		if (test)
		{
			count++;
			list->wait->flags	= flags;
			list->wait->main	= 0;
			list->wait->gsc		= test;
			list->wait->alt		= 0;
			list->wait->io		= 0;
			_wait_list_node_remove(dev, list);
			EVENT_RESUME_IRQ(&list->queue, list->condition);
			continue;
		}

		test	= list->wait->alt & wait->alt;

		if (test)
		{
			count++;
			list->wait->flags	= flags;
			list->wait->main	= 0;
			list->wait->gsc		= 0;
			list->wait->alt		= test;
			list->wait->io		= 0;
			_wait_list_node_remove(dev, list);
			EVENT_RESUME_IRQ(&list->queue, list->condition);
			continue;
		}

		test	= list->wait->io & wait->io;

		if (test)
		{
			count++;
			list->wait->flags	= flags;
			list->wait->main	= 0;
			list->wait->gsc		= 0;
			list->wait->alt		= 0;
			list->wait->io		= test;
			_wait_list_node_remove(dev, list);
			EVENT_RESUME_IRQ(&list->queue, list->condition);
			continue;
		}
	}

	wait->count	= count;
	return(count);
}



//*****************************************************************************
int gsc_wait_resume_io(dev_data_t* dev, u32 io)
{
	gsc_wait_t	wait;

	wait.flags		= GSC_WAIT_FLAG_INTERNAL;
	wait.main		= 0;
	wait.gsc		= 0;
	wait.alt		= 0;
	wait.io			= io;
	wait.timeout_ms	= 0;
	wait.count		= 0;

	// We must gain controlled access.
	gsc_irq_access_lock(dev, 0);
	_wait_resume(dev, GSC_WAIT_FLAG_DONE, &wait);
	gsc_irq_access_unlock(dev, 0);

	wait.flags		= 0;
	wait.main		= 0;
	wait.gsc		= 0;
	wait.alt		= 0;
	wait.io			= io;
	wait.timeout_ms	= 0;
	wait.count		= 0;

	// We must gain controlled access.
	gsc_irq_access_lock(dev, 0);
	_wait_resume(dev, GSC_WAIT_FLAG_DONE, &wait);
	gsc_irq_access_unlock(dev, 0);
	return((int) wait.count);
}



//*****************************************************************************
int gsc_wait_resume_irq_alt(dev_data_t* dev, u32 alt)
{
	gsc_wait_t	wait;

	// Access should already be locked.
	wait.flags		= GSC_WAIT_FLAG_INTERNAL;
	wait.main		= 0;
	wait.gsc		= 0;
	wait.alt		= alt;
	wait.io			= 0;
	wait.timeout_ms	= 0;
	wait.count		= 0;
	_wait_resume(dev, GSC_WAIT_FLAG_DONE, &wait);

	// Access should already be locked.
	wait.flags		= 0;
	wait.main		= 0;
	wait.gsc		= 0;
	wait.alt		= alt;
	wait.io			= 0;
	wait.timeout_ms	= 0;
	wait.count		= 0;
	_wait_resume(dev, GSC_WAIT_FLAG_DONE, &wait);
	return((int) wait.count);
}



//*****************************************************************************
int gsc_wait_resume_irq_gsc(dev_data_t* dev, u32 gsc)
{
	gsc_wait_t	wait;

	// Access should already be locked.
	wait.flags		= GSC_WAIT_FLAG_INTERNAL;
	wait.main		= 0;
	wait.gsc		= gsc;
	wait.alt		= 0;
	wait.io			= 0;
	wait.timeout_ms	= 0;
	wait.count		= 0;
	_wait_resume(dev, GSC_WAIT_FLAG_DONE, &wait);

	// Access should already be locked.
	wait.flags		= 0;
	wait.main		= 0;
	wait.gsc		= gsc;
	wait.alt		= 0;
	wait.io			= 0;
	wait.timeout_ms	= 0;
	wait.count		= 0;
	_wait_resume(dev, GSC_WAIT_FLAG_DONE, &wait);
	return((int) wait.count);
}



//*****************************************************************************
int gsc_wait_resume_irq_main(dev_data_t* dev, u32 main)
{
	gsc_wait_t	wait;

	// Access should already be locked.
	wait.flags		= GSC_WAIT_FLAG_INTERNAL;
	wait.main		= main;
	wait.gsc		= 0;
	wait.alt		= 0;
	wait.io			= 0;
	wait.timeout_ms	= 0;
	wait.count		= 0;
	_wait_resume(dev, GSC_WAIT_FLAG_DONE, &wait);

	// Access should already be locked.
	wait.flags		= 0;
	wait.main		= main;
	wait.gsc		= 0;
	wait.alt		= 0;
	wait.io			= 0;
	wait.timeout_ms	= 0;
	wait.count		= 0;
	_wait_resume(dev, GSC_WAIT_FLAG_DONE, &wait);

	return((int) wait.count);
}



/******************************************************************************
*
*	Function:	gsc_wait_event
*
*	Purpose:
*
*		Implement a generic Wait Event service.
*
*	Arguments:
*
*		dev		The device data structure.
*
*		wait	The wait structure to utilize.
*
*		setup	The function to call before sleeping. NULL is OK.
*
*		arg		The arbitrary argument to pass to the above function.
*
*	Returned:
*
*		0		All is well.
*		< 0		There was an error.
*
******************************************************************************/

int gsc_wait_event(
	dev_data_t*		dev,
	gsc_wait_t*		wait,
	int				(*setup)(dev_data_t* dev, unsigned long arg),
	unsigned long	arg,
	gsc_sem_t*		sem)
{
	int				i;
	u32				ms;
	gsc_wait_node_t	node;
	int				ret		= 0;
	unsigned long	timeout;	// in jiffies
	struct timeval	tv_end;
	u32				us;

	for (;;)
	{
		if ((dev == NULL) || (wait == NULL))
		{
			ret	= -EINVAL;
			break;
		}

		if ((s32) wait->timeout_ms < 0)
			timeout	= - wait->timeout_ms;	// already in siffies.
		else
			timeout	= MS_TO_JIFFIES(wait->timeout_ms);

		// Initialize the wait node.
		node.next		= NULL;
		node.wait		= wait;
		node.condition	= 0;
		do_gettimeofday(&node.tv_start);
		memset(&node.entry, 0, sizeof(node.entry));
		WAIT_QUEUE_HEAD_INIT(&node.queue);
		WAIT_QUEUE_ENTRY_INIT(&node.entry, current);
		SET_CURRENT_STATE(TASK_INTERRUPTIBLE);

		add_wait_queue(&node.queue, &node.entry);
		_wait_list_node_add(dev, &node);

		if (setup)
			ret	= (setup)(dev, arg);

		if (ret == 0)
		{
			gsc_sem_unlock(sem);
			EVENT_WAIT_IRQ_TO(&node.queue, node.condition, timeout);
			gsc_sem_lock(sem);
		}

		remove_wait_queue(&node.queue, &node.entry);
		SET_CURRENT_STATE(TASK_RUNNING);
		i	= _wait_list_node_remove(dev, &node);

		if (i)
		{
			// The node was still on the list, which means the wait request timed out.
			wait->flags	= GSC_WAIT_FLAG_TIMEOUT;
			wait->main	= 0;
			wait->gsc	= 0;
			wait->alt	= 0;
			wait->io	= 0;
		}

		// Compute the amount of time the thread waited.
		do_gettimeofday(&tv_end);

		if (tv_end.tv_sec == node.tv_start.tv_sec)
		{
			us	= tv_end.tv_usec - node.tv_start.tv_usec;
			ms	= (us + 999) / 1000;
		}
		else
		{
			us	= tv_end.tv_usec + 1000000l - node.tv_start.tv_usec;
			ms	= (us + 999) / 1000;
			ms	+= (tv_end.tv_sec - node.tv_start.tv_sec - 1) * 1000;
		}

		wait->timeout_ms	= ms;
		break;
	}

	return(ret);
}


/******************************************************************************
*
*	Function:	gsc_wait_event_ioctl
*
*	Purpose:
*
*		Implement the generic portion of the Wait Event IOCTL service.
*
*	Arguments:
*
*		dev		The device data structure.
*
*		arg		The argument required for the service.
*
*	Returned:
*
*		0		All is well.
*		< 0		There was an error.
*
******************************************************************************/

int gsc_wait_event_ioctl(dev_data_t* dev, void* arg)
{
	gsc_wait_t*	wait	= arg;
	int			ret		= 0;

	if ((wait->flags)								||
		(wait->main		& (u32) ~GSC_WAIT_MAIN_ALL)	||
		(wait->gsc		& (u32) ~DEV_WAIT_GSC_ALL)	||
		(wait->alt		& (u32) ~DEV_WAIT_ALT_ALL)	||
		(wait->io		& (u32) ~_WAIT_IO_ALL)		||
		(wait->count)								||
		(wait->timeout_ms > GSC_WAIT_TIMEOUT_MAX)	||
		(wait->timeout_ms <= 0))
	{
		ret	= -EINVAL;
	}
	else if ((wait->main) || (wait->gsc) || (wait->alt) || (wait->io))
	{
		wait->count	= 0;
		ret			= gsc_wait_event(dev, wait, NULL, 0, &dev->sem);
	}
	else
	{
		ret	= -EINVAL;
	}

	return(ret);
}



/******************************************************************************
*
*	Function:	gsc_wait_cancel_ioctl
*
*	Purpose:
*
*		Implement the generic portion of the Wait Cancel IOCTL service.
*
*	Arguments:
*
*		dev		The device data structure.
*
*		arg		The argument required for the service.
*
*	Returned:
*
*		0		All is well.
*		< 0		There was an error.
*
******************************************************************************/

int gsc_wait_cancel_ioctl(dev_data_t* dev, void* arg)
{
	gsc_wait_t*	wait	= arg;

	wait->flags	&= ~ (u32) GSC_WAIT_FLAG_INTERNAL;
	gsc_irq_access_lock(dev, 0);
	_wait_resume(dev, GSC_WAIT_FLAG_CANCEL, wait);
	gsc_irq_access_unlock(dev, 0);
	return(0);
}



/******************************************************************************
*
*	Function:	gsc_wait_status_ioctl
*
*	Purpose:
*
*		Count the number of waiting threads which mwaiting any of the given
*		criereia.
*
*	Arguments:
*
*		dev		The device data structure.
*
*		arg		The argument required for the service.
*
*	Returned:
*
*		0		All is well.
*		< 0		There was an error.
*
******************************************************************************/

int gsc_wait_status_ioctl(dev_data_t* dev, void* arg)
{
	gsc_wait_node_t*	list;
	gsc_wait_t*			wait	= arg;

	wait->count	= 0;
	gsc_irq_access_lock(dev, 0);

	for (list = dev->wait_list; list; list = list->next)
	{
		if (list->wait->flags & GSC_WAIT_FLAG_INTERNAL)
			continue;

		if ((list->wait->main		& wait->main)	||
			(list->wait->gsc		& wait->gsc)	||
			(list->wait->alt		& wait->alt)	||
			(list->wait->io			& wait->io))
		{
			wait->count++;
		}
	}

	gsc_irq_access_unlock(dev, 0);
	return(0);
}



/******************************************************************************
*
*	Function:	gsc_wait_close
*
*	Purpose:
*
*		Make sure all wait nodes are released.
*
*	Arguments:
*
*		dev		The device data structure.
*
*	Returned:
*
*		None.
*
******************************************************************************/

void gsc_wait_close(dev_data_t* dev)
{
	gsc_wait_t	wait;

	gsc_irq_access_lock(dev, 0);

	wait.flags			= GSC_WAIT_FLAG_INTERNAL;
	wait.main			= 0xFFFFFFFF;
	wait.gsc			= 0xFFFFFFFF;
	wait.alt			= 0xFFFFFFFF;
	wait.io				= 0xFFFFFFFF;
	wait.timeout_ms		= 0;
	wait.count			= 0;
	_wait_resume(dev, GSC_WAIT_FLAG_CANCEL, &wait);

	wait.flags			= 0;
	wait.main			= 0xFFFFFFFF;
	wait.gsc			= 0xFFFFFFFF;
	wait.alt			= 0xFFFFFFFF;
	wait.io				= 0xFFFFFFFF;
	wait.timeout_ms		= 0;
	wait.count			= 0;
	_wait_resume(dev, GSC_WAIT_FLAG_CANCEL, &wait);

	gsc_irq_access_unlock(dev, 0);
}


