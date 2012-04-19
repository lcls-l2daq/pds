#include "PrincetonUtils.hh"

#include <stdio.h>

namespace PICAM
{
  
char *lsParamAccessMode[] =
{ "ACC_ERROR", "ACC_READ_ONLY", "ACC_READ_WRITE", "ACC_EXIST_CHECK_ONLY", "ACC_WRITE_ONLY"
};

void printPvError(const char *sPrefixMsg)
{
  char sErrorMsg[ERROR_MSG_LEN];

  int16 iErrorCode = pl_error_code();
  pl_error_message(iErrorCode, sErrorMsg);
  printf("%s\n  PvCam library error code [%i] %s\n", sPrefixMsg,
         iErrorCode, sErrorMsg);
}

/* this routine assumes the param id is an enumerated type, it will print out all the */
/* enumerated values that are allowed with the param id and display the associated    */
/* ASCII text.                                                                        */
static void displayParamEnumInfo(int16 hCam, uns32 uParamId)
{
  boolean status;             /* status of pvcam functions.                */
  uns32 count, index;         /* counters for enumerated types.            */
  char enumStr[100];          /* string for enum text.                     */
  int32 enumValue;            /* enum value returned for index & param id. */
  uns32 uCurVal;

  status = pl_get_param(hCam, uParamId, ATTR_CURRENT, (void *) &uCurVal);
  if (status)
    printf(" current value = %lu\n", uCurVal);
  else
  {
    printf(" parameter %lu is not available\n", uParamId);
  }

  /* get number of enumerated values. */
  status = pl_get_param(hCam, uParamId, ATTR_COUNT, (void *) &count);
  printf(" count = %lu\n", count);
  if (status)
  {
    for (index = 0; index < count; index++)
    {
      /* get enum value and enum string */
      status = pl_get_enum_param(hCam, uParamId, index, &enumValue,
                                 enumStr, 100);
      /* if everything alright print out the results. */
      if (status)
      {
        printf(" [%ld] enum value = %ld, text = %s\n",
               index, enumValue, enumStr);
      }
      else
      {
        printf
          ("functions failed pl_get_enum_param, with error code %d\n",
           pl_error_code());
      }
    }
  }
  else
  {
    printf("functions failed pl_get_param, with error code %d\n",
           pl_error_code());
  }
}                             /* end of function DisplayEnumInfo */

static void displayParamValueInfo(int16 hCam, uns32 uParamId)
{
  /* current, min&max, & default values of parameter id */
  union
  {
    double dval;
    unsigned long ulval;
    long lval;
    unsigned short usval;
    short sval;
    unsigned char ubval;
    signed char bval;
  } currentVal, minVal, maxVal, defaultVal, incrementVal;
  uns16 type;                 /* data type of paramater id */
  boolean status = false, status2 = false, status3 = false, status4 = false, status5 = false; /* status of pvcam functions */

  /* get the data type of parameter id */
  status = pl_get_param(hCam, uParamId, ATTR_TYPE, (void *) &type);
  /* get the default, current, min and max values for parameter id. */
  /* Note : since the data type for these depends on the parameter  */
  /* id you have to call pl_get_param with the correct data type    */
  /* passed for pParamValue.                                        */
  if (status)
  {
    switch (type)
    {
    case TYPE_INT8:
      status = pl_get_param(hCam, uParamId, ATTR_CURRENT,
                            (void *) &currentVal.bval);
      status2 = pl_get_param(hCam, uParamId, ATTR_DEFAULT,
                             (void *) &defaultVal.bval);
      status3 = pl_get_param(hCam, uParamId, ATTR_MAX,
                             (void *) &maxVal.bval);
      status4 = pl_get_param(hCam, uParamId, ATTR_MIN,
                             (void *) &minVal.bval);
      status5 = pl_get_param(hCam, uParamId, ATTR_INCREMENT,
                             (void *) &incrementVal.bval);
      printf(" current value = %c\n", currentVal.bval);
      printf(" default value = %c\n", defaultVal.bval);
      printf(" min = %c, max = %c\n", minVal.bval, maxVal.bval);
      printf(" increment = %c\n", incrementVal.bval);
      break;
    case TYPE_UNS8:
      status = pl_get_param(hCam, uParamId, ATTR_CURRENT,
                            (void *) &currentVal.ubval);
      status2 = pl_get_param(hCam, uParamId, ATTR_DEFAULT,
                             (void *) &defaultVal.ubval);
      status3 = pl_get_param(hCam, uParamId, ATTR_MAX,
                             (void *) &maxVal.ubval);
      status4 = pl_get_param(hCam, uParamId, ATTR_MIN,
                             (void *) &minVal.ubval);
      status5 = pl_get_param(hCam, uParamId, ATTR_INCREMENT,
                             (void *) &incrementVal.ubval);
      printf(" current value = %uc\n", currentVal.ubval);
      printf(" default value = %uc\n", defaultVal.ubval);
      printf(" min = %uc, max = %uc\n", minVal.ubval, maxVal.ubval);
      printf(" increment = %uc\n", incrementVal.ubval);
      break;
    case TYPE_INT16:
      status = pl_get_param(hCam, uParamId, ATTR_CURRENT,
                            (void *) &currentVal.sval);
      status2 = pl_get_param(hCam, uParamId, ATTR_DEFAULT,
                             (void *) &defaultVal.sval);
      status3 = pl_get_param(hCam, uParamId, ATTR_MAX,
                             (void *) &maxVal.sval);
      status4 = pl_get_param(hCam, uParamId, ATTR_MIN,
                             (void *) &minVal.sval);
      status5 = pl_get_param(hCam, uParamId, ATTR_INCREMENT,
                             (void *) &incrementVal.sval);
      printf(" current value = %i\n", currentVal.sval);
      printf(" default value = %i\n", defaultVal.sval);
      printf(" min = %i, max = %i\n", minVal.sval, maxVal.sval);
      printf(" increment = %i\n", incrementVal.sval);
      break;
    case TYPE_UNS16:
      status = pl_get_param(hCam, uParamId, ATTR_CURRENT,
                            (void *) &currentVal.usval);
      status2 = pl_get_param(hCam, uParamId, ATTR_DEFAULT,
                             (void *) &defaultVal.usval);
      status3 = pl_get_param(hCam, uParamId, ATTR_MAX,
                             (void *) &maxVal.usval);
      status4 = pl_get_param(hCam, uParamId, ATTR_MIN,
                             (void *) &minVal.usval);
      status5 = pl_get_param(hCam, uParamId, ATTR_INCREMENT,
                             (void *) &incrementVal.usval);
      printf(" current value = %u\n", currentVal.usval);
      printf(" default value = %u\n", defaultVal.usval);
      printf(" min = %u, max = %u\n", minVal.usval, maxVal.usval);
      printf(" increment = %u\n", incrementVal.usval);
      break;
    case TYPE_INT32:
      status = pl_get_param(hCam, uParamId, ATTR_CURRENT,
                            (void *) &currentVal.lval);
      status2 = pl_get_param(hCam, uParamId, ATTR_DEFAULT,
                             (void *) &defaultVal.lval);
      status3 = pl_get_param(hCam, uParamId, ATTR_MAX,
                             (void *) &maxVal.lval);
      status4 = pl_get_param(hCam, uParamId, ATTR_MIN,
                             (void *) &minVal.lval);
      status5 = pl_get_param(hCam, uParamId, ATTR_INCREMENT,
                             (void *) &incrementVal.lval);
      printf(" current value = %ld\n", currentVal.lval);
      printf(" default value = %ld\n", defaultVal.lval);
      printf(" min = %ld, max = %ld\n", minVal.lval, maxVal.lval);
      printf(" increment = %ld\n", incrementVal.lval);
      break;
    case TYPE_UNS32:
      status = pl_get_param(hCam, uParamId, ATTR_CURRENT,
                            (void *) &currentVal.ulval);
      status2 = pl_get_param(hCam, uParamId, ATTR_DEFAULT,
                             (void *) &defaultVal.ulval);
      status3 = pl_get_param(hCam, uParamId, ATTR_MAX,
                             (void *) &maxVal.ulval);
      status4 = pl_get_param(hCam, uParamId, ATTR_MIN,
                             (void *) &minVal.ulval);
      status5 = pl_get_param(hCam, uParamId, ATTR_INCREMENT,
                             (void *) &incrementVal.ulval);
      printf(" current value = %ld\n", currentVal.ulval);
      printf(" default value = %ld\n", defaultVal.ulval);
      printf(" min = %ld, max = %ld\n", minVal.ulval, maxVal.ulval);
      printf(" increment = %ld\n", incrementVal.ulval);
      break;
    case TYPE_FLT64:
      status = pl_get_param(hCam, uParamId, ATTR_CURRENT,
                            (void *) &currentVal.dval);
      status2 = pl_get_param(hCam, uParamId, ATTR_DEFAULT,
                             (void *) &defaultVal.dval);
      status3 = pl_get_param(hCam, uParamId, ATTR_MAX,
                             (void *) &maxVal.dval);
      status4 = pl_get_param(hCam, uParamId, ATTR_MIN,
                             (void *) &minVal.dval);
      status5 = pl_get_param(hCam, uParamId, ATTR_INCREMENT,
                             (void *) &incrementVal.dval);
      printf(" current value = %g\n", currentVal.dval);
      printf(" default value = %g\n", defaultVal.dval);
      printf(" min = %g, max = %g\n", minVal.dval, maxVal.dval);
      printf(" increment = %g\n", incrementVal.dval);
      break;
    case TYPE_BOOLEAN:
      status = pl_get_param(hCam, uParamId, ATTR_CURRENT,
                            (void *) &currentVal.bval);
      status2 = pl_get_param(hCam, uParamId, ATTR_DEFAULT,
                             (void *) &defaultVal.bval);
      printf(" current value = %d\n", (int) currentVal.bval);
      printf(" default value = %d\n", (int) defaultVal.bval);
      break;
    default:
      printf(" data type %d not supported in this functions\n", type );      
      break;
    }
    if (!status || !status2 || !status3 || !status4 || !status5)
    {
      printf("functions failed pl_get_param, with error code %d\n",
             pl_error_code());
    }
  }
  else
  {
    printf("functions failed pl_get_param, with error code %d\n",
           pl_error_code());
  }
}

/* This is an example that will display information we can get from parameter id.      */
void displayParamIdInfo(int16 hCam, uns32 uParamId,
                        const char sDescription[])
{
  boolean status, status2;    /* status of pvcam functions                           */
  boolean avail_flag;         /* ATTR_AVAIL, is param id available                   */
  uns16 access = 0;           /* ATTR_ACCESS, get if param is read, write or exists. */
  uns16 type;                 /* ATTR_TYPE, get data type.                           */

  printf("%s (param id %lu):\n", sDescription, uParamId);
  status = pl_get_param(hCam, uParamId, ATTR_AVAIL, (void *) &avail_flag);
  /* check for errors */
  if (status)
  {
/* check to see if paramater id is supported by hardware, software or system.  */
    if (avail_flag)
    {
      /* we got a valid parameter, now get access writes and data type */
      status = pl_get_param(hCam, uParamId, ATTR_ACCESS, (void *) &access);
      status2 = pl_get_param(hCam, uParamId, ATTR_TYPE, (void *) &type);
      if (status && status2)
      {
        printf(" access mode = %s\n", lsParamAccessMode[access]);

        if (access == ACC_EXIST_CHECK_ONLY)
        {
          printf(" param id %lu exists\n", uParamId);
        }
        else if ((access == ACC_READ_ONLY) || (access == ACC_READ_WRITE))
        {
          /* now we can start displaying information. */
          /* handle enumerated types separate from other data */
          if (type == TYPE_ENUM)
          {
            displayParamEnumInfo(hCam, uParamId);
          }
          else
          {                   /* take care of he rest of the data types currently used */
            displayParamValueInfo(hCam, uParamId);
          }
        }
        else
        {
          printf(" error in access check for param id %lu\n", uParamId);
        }
      }
      else
      {                       /* error occured calling function. */
        printf
          ("functions failed pl_get_param, with error code %d\n",
           pl_error_code());
      }

    }
    else
    {                         /* parameter id is not available with current setup */
      printf
        (" parameter %lu is not available with current hardware or software setup\n",
         uParamId);
    }
  }
  else
  {                           /* error occured calling function print out error and error code */
    printf("functions failed pl_get_param, with error code %d\n",
           pl_error_code());
  }
}                             /* end of function displayParamIdInfo */

int getAnyParam(int16 hCam, uns32 uParamId, void *pParamValue, EnumGetParam eumGetParam)
{
  if (pParamValue == NULL)
  {
    printf("getAnyParam(): Invalid parameter: pParamValue=%p\n", pParamValue );
    return 1;
  }

  rs_bool bStatus, bParamAvailable;
  uns16 uParamAccess;

  bStatus =
    pl_get_param(hCam, uParamId, ATTR_AVAIL, (void *) &bParamAvailable);
  if (!bStatus)
  {
    printf("getAnyParam(): pl_get_param(param id = %lu, ATTR_AVAIL) failed\n",
     uParamId);
    return 2;
  }
  else if (!bParamAvailable)
  {
    printf("getAnyParam(): param id %lu is not available\n", uParamId);
    return 3;
  }

  bStatus = pl_get_param(hCam, uParamId, ATTR_ACCESS, (void *) &uParamAccess);
  if (!bStatus)
  {
    printf("getAnyParam(): pl_get_param(param id = %lu, ATTR_ACCESS) failed\n",
     uParamId);
    return 4;
  }
  else if (uParamAccess != ACC_READ_WRITE && uParamAccess != ACC_READ_ONLY)
  {
    printf("getAnyParam(): param id %lu is not writable, access mode = %s\n",
     uParamId, lsParamAccessMode[uParamAccess]);
    return 5;
  }

  switch (eumGetParam)
  {
  case GET_PARAM_CURRENT:
    bStatus = pl_get_param(hCam, uParamId, ATTR_CURRENT, pParamValue);
    if (!bStatus)
    {
      printf("getAnyParam(): pl_get_param(param id = %lu, ATTR_CURRENT) failed\n", uParamId);
      return 6;
    }
    break;
  case GET_PARAM_MIN:
    bStatus = pl_get_param(hCam, uParamId, ATTR_MIN, pParamValue);
    if (!bStatus)
    {
      printf("getAnyParam(): pl_get_param(param id = %lu, ATTR_MIN) failed\n", uParamId);
      return 6;
    }
    break;
  case GET_PARAM_MAX:
    bStatus = pl_get_param(hCam, uParamId, ATTR_MAX, pParamValue);
    if (!bStatus)
    {
      printf("getAnyParam(): pl_get_param(param id = %lu, ATTR_MAX) failed\n", uParamId);
      return 6;
    }
    break;
  case GET_PARAM_DEFAULT:
    bStatus = pl_get_param(hCam, uParamId, ATTR_DEFAULT, pParamValue);
    if (!bStatus)
    {
      printf("getAnyParam(): pl_get_param(param id = %lu, ATTR_DEFAULT) failed\n", uParamId);
      return 6;
    }
    break;
  case GET_PARAM_INC:
    bStatus = pl_get_param(hCam, uParamId, ATTR_INCREMENT, pParamValue);
    if (!bStatus)
    {
      printf("getAnyParam(): pl_get_param(param id = %lu, ATTR_INCREMENT) failed\n", uParamId);
      return 6;
    }
    break;
  default:
    printf("getAnyParam(): Invalid ENUM for GetParam : %d\n", eumGetParam);
    return 7;
  };
  
  return 0;
}

int setAnyParam(int16 hCam, uns32 uParamId, const void *pParamValue)
{
  rs_bool bStatus, bParamAvailable;
  uns16 uParamAccess;

  bStatus =
    pl_get_param(hCam, uParamId, ATTR_AVAIL, (void *) &bParamAvailable);
  if (!bStatus)
  {
    printf("setAnyParam(): pl_get_param(param id = %lu, ATTR_AVAIL) failed\n", uParamId);
    return 2;
  }
  else if (!bParamAvailable)
  {
    printf("setAnyParam(): param id %lu is not available\n", uParamId);
    return 3;
  }

  bStatus = pl_get_param(hCam, uParamId, ATTR_ACCESS, (void *) &uParamAccess);
  if (!bStatus)
  {
    printf("setAnyParam(): pl_get_param(param id = %lu, ATTR_ACCESS) failed\n", uParamId);
    return 4;
  }
  else if (uParamAccess != ACC_READ_WRITE && uParamAccess != ACC_WRITE_ONLY)
  {
    printf("setAnyParam(): param id %lu is not writable, access mode = %s\n",
     uParamId, lsParamAccessMode[uParamAccess]);
    return 5;
  }

  bStatus = pl_set_param(hCam, uParamId, (void*) pParamValue);
  if (!bStatus)
  {
    printf("setAnyParam(): pl_set_param(param id = %lu) failed\n", uParamId);
    return 6;
  }

  return 0;
}

void printROI(int iNumRoi, rgn_type* roi)
{
  for (int i = 0; i < iNumRoi; i++)
  {
    printf("ROI %i set to { s1 = %i, s2 = %i, sin = %i, p1 = %i, p2 = %i, pbin = %i }\n",
       i, roi[i].s1, roi[i].s2, roi[i].sbin, roi[i].p1, roi[i].p2,
       roi[i].pbin);
  }
}

} // namespace PICAM
 
