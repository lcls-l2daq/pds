/*
** ++
**  Package:
**      Service
**
**  Synposis:
**
**      char* Path(char* file, char* buffer)
**
**  Abstract:
**      A "C" function whose purpose is to create a path specification
**      derived from the ROM's own platform and crate number. The
**      functon takes two arguments:
**       file   - A pointer a string (NULL terminatted) which is to be
**                appended to the created path name.
**       buffer - A pointer to a buffer into which the created path
**                specification string will be copied. It is ASSUMED
**                (and not checked) that the buffer is long enough
**                to contain the reslutant string and its NULL termimation.
**      The function returns the "buffer" argument. This buffer will
**      contain the formatted path specification. The path is of the form
**          "/server/dataflow/XX/YY/file", where:
**       "/server/ - is assumed to the mount point of the DataFlow File Server.
**       "XX"      - Is the platform number encoded as a HEX digit (for
**                   example "7F" for the IR-2 platform).
**       "YY"      - Is the ROM  type number encoded as a HEX digit. A type
**                   number is the ROM's detector number with a single bit
**                   or'd into the fourth bit to indicate whether the ROM
**                   is a SLOT-1 ROM. For example a value of "14" would be
**                   the SLOT-1 ROM for the EMC, a value of "04" would be
**                   any EMC ROM other than its SLOT-1 ROMs.
**       "file"    - Is simply the input argument.
**
**
**  Author:
**      Michael Huffer, SLAC, (415) 926-4269
**
**  Creation Date:
**	000 - January 11 1,1999
**
**  Revision History:
**      001 - January 25, 1999 - RiC - Make function accessible from C++
**
** --
*/

#ifndef PDS_PATH_HH
#define PDS_PATH_HH

#ifdef __cplusplus
extern "C" {
#endif

extern char *Path (char *file, char *buffer);

#ifdef __cplusplus
}
#endif

#endif
