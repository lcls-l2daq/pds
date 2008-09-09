/*
** ++
**
**  Facility:  Service
**
** Abstract:
**
**      NB - Don't include this header file in your code! Use Exc.hh instead.
**
**      This file contains a macro with which to terminate a process when a
**      nonrecoverable condition is detected.  It is meant to provide more or
**      less the same functionality as a similar macro for VxWorks based code
**      as defined in ExcVx.hh so that the same piece of code can be compiled
**      for both VxWorks and Unix.
**
**      Additional features could (should?) be provided (e.g., an ExcInit
**      that allows the caller to select whether the process is exited, enters
**      an infinite loop or invokes a debugger) to make a better utility.
**      Maybe some day or as required...
**
**  Author:
**	R. Claus, SLAC/PEP-II BaBar Online DataFlow Project
**
**  History:
**	December 5, 2000 - Created.
**
**  Copyright:
**                                Copyright 2000
**                                      by
**                         The Board of Trustees of the
**                       Leland Stanford Junior University.
**                              All rights reserved.
**
**
**         Work supported by the U.S. Department of Energy under contract
**       DE-AC03-76SF00515.
**
**                               Disclaimer Notice
**
**        The items furnished herewith were developed under the sponsorship
**   of the U.S. Government.  Neither the U.S., nor the U.S. D.O.E., nor the
**   Leland Stanford Junior University, nor their employees, makes any war-
**   ranty, express or implied, or assumes any liability or responsibility
**   for accuracy, completeness or usefulness of any information, apparatus,
**   product or process disclosed, or represents that its use will not in-
**   fringe privately-owned rights.  Mention of any product, its manufactur-
**   er, or suppliers shall not, nor is it intended to, imply approval, dis-
**   approval, or fitness for any particular use.  The U.S. and the Univer-
**   sity at all times retain the right to use and disseminate the furnished
**   items for any purpose whatsoever.                       Notice 91 02 01
**
** --
*/

#ifndef PDS_EXC_UX_HH
#define PDS_EXC_UX_HH

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
*
* BugCheck - Bug check an executable
*
*  This macro can be used to terminate an executable when some unhandleable
*  condition is detected.  Process termination is carried out by the exit()
*  standard system call, which implies that additional work can be done at
*  exit time by registering such work using the atexit(3C) call.
*
* PARAMETERS: _msg_ - A message to be printed upon program termination.
*
* RETURNS: doesn't
*/
#define BugCheck(_msg_) \
{ \
  printf("Bug checking:\n%s\n", _msg_); \
  exit(0xdeadbeef); \
}

#ifdef __cplusplus
}
#endif

#endif
