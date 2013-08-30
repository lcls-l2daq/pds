#ifndef EPICS_DBR_TOOLS_H
#define EPICS_DBR_TOOLS_H

#include "pdsdata/psddl/epics.ddl.h"

#include <string>
#include <stdint.h>
#include <stdio.h>

#if defined(INCLcadefh)
#define EPICS_HEADERS_INCLUDED
#endif

namespace Pds
{

/*
 * Imported from Epics library
 */        
namespace Epics
{

#ifndef EPICS_HEADERS_INCLUDED    
    
/*
 * Imported from Epics header file: ${EPICS_PROJECT_DIR}/base/current/include/epicsTypes.h
 */    

#if __STDC_VERSION__ >= 199901L
    typedef int8_t          epicsInt8;
    typedef uint8_t         epicsUInt8;
    typedef int16_t         epicsInt16;
    typedef uint16_t        epicsUInt16;
    typedef epicsUInt16     epicsEnum16;
    typedef int32_t         epicsInt32;
    typedef uint32_t        epicsUInt32;
    typedef int64_t         epicsInt64;
    typedef uint64_t        epicsUInt64;
#else
    typedef char            epicsInt8;
    typedef unsigned char   epicsUInt8;
    typedef short           epicsInt16;
    typedef unsigned short  epicsUInt16;
    typedef epicsUInt16     epicsEnum16;
    typedef int             epicsInt32;
    typedef unsigned        epicsUInt32;
#endif
typedef float           epicsFloat32;
typedef double          epicsFloat64;
typedef unsigned long   epicsIndex;
typedef epicsInt32      epicsStatus;   
typedef char            epicsOldString[MAX_STRING_SIZE]; 

/*
 * Imported from Epics header file: ${EPICS_PROJECT_DIR}/base/current/include/db_access.h
 */    

#define dbr_type_is_TIME(type)   \
        ((type) >= DBR_TIME_STRING && (type) <= DBR_TIME_DOUBLE)
        
#define dbr_type_is_CTRL(type)   \
        ((type) >= DBR_CTRL_STRING && (type) <= DBR_CTRL_DOUBLE)

typedef epicsOldString dbr_string_t;
typedef epicsUInt8 dbr_char_t;
typedef epicsInt16 dbr_short_t;
typedef epicsUInt16 dbr_enum_t;
typedef epicsInt32 dbr_long_t;
typedef epicsFloat32 dbr_float_t;
typedef epicsFloat64 dbr_double_t;
typedef epicsUInt16 dbr_put_ackt_t;
typedef epicsUInt16 dbr_put_acks_t;
typedef epicsOldString dbr_stsack_string_t;
typedef epicsOldString dbr_class_name_t;

#endif 
// #ifndef EPICS_HEADERS_INCLUDED    

/*
 * Imported from Epics header file: ${EPICS_PROJECT_DIR}/base/current/include/alarm.h
 */    

/*
 * Imported from Epics source file: ${EPICS_PROJECT_DIR}/base/current/src/ca/access.cpp
 *  
 */
extern const char *dbr_text[35];
extern const char* epicsAlarmSeverityStrings[];
extern const char* epicsAlarmConditionStrings[];

} // namespace Epics
    
        
namespace EpicsDbrTools
{
    
const int iSizeBasicDbrTypes  = 7;  // Summarized from Epics namespace
const int iSizeAllDbrTypes    = 35; // Summarized from Epics namespace

/**
 *  Maximum PV Name Length: 
 * 
 *  Manually defined hered. By convention, formal Epics PV names are usually less than 40 characters, 
 *  so here we use a big enough value as the buffer size.
 *  Note that this value corresponds to some epics data structure size in existing xtc files.
 **/
const int iMaxPvNameLength    = 64; 

using namespace Epics;
/*
 * CaDbr Tool classes
 */
template <int iDbrType> struct DbrTypeFromInt { char a[-1 + iDbrType*0]; }; // Default should not be used. Will generate compilation error.
template <> struct DbrTypeFromInt<DBR_STRING> { typedef dbr_string_t TDbr; };
template <> struct DbrTypeFromInt<DBR_SHORT>  { typedef dbr_short_t  TDbr; };
template <> struct DbrTypeFromInt<DBR_FLOAT>  { typedef dbr_float_t  TDbr; };
template <> struct DbrTypeFromInt<DBR_ENUM>   { typedef dbr_enum_t   TDbr; };
template <> struct DbrTypeFromInt<DBR_CHAR>   { typedef dbr_char_t   TDbr; };
template <> struct DbrTypeFromInt<DBR_LONG>   { typedef dbr_long_t   TDbr; };
template <> struct DbrTypeFromInt<DBR_DOUBLE> { typedef dbr_double_t TDbr; };

template <> struct DbrTypeFromInt<DBR_TIME_STRING> { typedef dbr_time_string TDbr; };
template <> struct DbrTypeFromInt<DBR_TIME_SHORT>  { typedef dbr_time_short  TDbr; };
template <> struct DbrTypeFromInt<DBR_TIME_FLOAT>  { typedef dbr_time_float  TDbr; };
template <> struct DbrTypeFromInt<DBR_TIME_ENUM>   { typedef dbr_time_enum   TDbr; };
template <> struct DbrTypeFromInt<DBR_TIME_CHAR>   { typedef dbr_time_char   TDbr; };
template <> struct DbrTypeFromInt<DBR_TIME_LONG>   { typedef dbr_time_long   TDbr; };
template <> struct DbrTypeFromInt<DBR_TIME_DOUBLE> { typedef dbr_time_double TDbr; };

template <> struct DbrTypeFromInt<DBR_CTRL_STRING> { typedef dbr_sts_string  TDbr; };
template <> struct DbrTypeFromInt<DBR_CTRL_SHORT>  { typedef dbr_ctrl_short  TDbr; };
template <> struct DbrTypeFromInt<DBR_CTRL_FLOAT>  { typedef dbr_ctrl_float  TDbr; };
template <> struct DbrTypeFromInt<DBR_CTRL_ENUM>   { typedef dbr_ctrl_enum   TDbr; };
template <> struct DbrTypeFromInt<DBR_CTRL_CHAR>   { typedef dbr_ctrl_char   TDbr; };
template <> struct DbrTypeFromInt<DBR_CTRL_LONG>   { typedef dbr_ctrl_long   TDbr; };
template <> struct DbrTypeFromInt<DBR_CTRL_DOUBLE> { typedef dbr_ctrl_double TDbr; };

template <int iDbrType> struct PdsTypeFromInt { char a[-1 + iDbrType*0]; }; // Default should not be used. Will generate compilation error.
template <> struct PdsTypeFromInt<DBR_TIME_STRING> { typedef EpicsPvTimeString TDbr; };
template <> struct PdsTypeFromInt<DBR_TIME_SHORT>  { typedef EpicsPvTimeShort  TDbr; };
template <> struct PdsTypeFromInt<DBR_TIME_FLOAT>  { typedef EpicsPvTimeFloat  TDbr; };
template <> struct PdsTypeFromInt<DBR_TIME_ENUM>   { typedef EpicsPvTimeEnum   TDbr; };
template <> struct PdsTypeFromInt<DBR_TIME_CHAR>   { typedef EpicsPvTimeChar   TDbr; };
template <> struct PdsTypeFromInt<DBR_TIME_LONG>   { typedef EpicsPvTimeLong   TDbr; };
template <> struct PdsTypeFromInt<DBR_TIME_DOUBLE> { typedef EpicsPvTimeDouble TDbr; };

template <> struct PdsTypeFromInt<DBR_CTRL_STRING> { typedef EpicsPvCtrlString TDbr; };
template <> struct PdsTypeFromInt<DBR_CTRL_SHORT>  { typedef EpicsPvCtrlShort  TDbr; };
template <> struct PdsTypeFromInt<DBR_CTRL_FLOAT>  { typedef EpicsPvCtrlFloat  TDbr; };
template <> struct PdsTypeFromInt<DBR_CTRL_ENUM>   { typedef EpicsPvCtrlEnum   TDbr; };
template <> struct PdsTypeFromInt<DBR_CTRL_CHAR>   { typedef EpicsPvCtrlChar   TDbr; };
template <> struct PdsTypeFromInt<DBR_CTRL_LONG>   { typedef EpicsPvCtrlLong   TDbr; };
template <> struct PdsTypeFromInt<DBR_CTRL_DOUBLE> { typedef EpicsPvCtrlDouble TDbr; };

template <int iDbrType> struct DbrTPtrFromInt { char a[-1 + iDbrType*0]; }; // Default should not be used. Will generate compilation error.
template <> struct DbrTPtrFromInt<DBR_STRING> { typedef const char*         TDbr; };
template <> struct DbrTPtrFromInt<DBR_SHORT>  { typedef const dbr_short_t*  TDbr; };
template <> struct DbrTPtrFromInt<DBR_FLOAT>  { typedef const dbr_float_t*  TDbr; };
template <> struct DbrTPtrFromInt<DBR_ENUM>   { typedef const dbr_enum_t*   TDbr; };
template <> struct DbrTPtrFromInt<DBR_CHAR>   { typedef const dbr_char_t*   TDbr; };
template <> struct DbrTPtrFromInt<DBR_LONG>   { typedef const dbr_long_t*   TDbr; };
template <> struct DbrTPtrFromInt<DBR_DOUBLE> { typedef const dbr_double_t* TDbr; };

template <int iDbrType> struct DbrTypeTraits
{ 
    enum { 
      iDiffDbrOrgToDbrTime = DBR_TIME_DOUBLE - DBR_DOUBLE,
      iDiffDbrOrgToDbrCtrl = DBR_CTRL_DOUBLE - DBR_DOUBLE };
      
    enum { 
      iDbrTimeType = iDbrType + iDiffDbrOrgToDbrTime,
      iDbrCtrlType = iDbrType + iDiffDbrOrgToDbrCtrl };
        
    typedef typename DbrTypeFromInt<iDbrType    >::TDbr TDbrOrg;
    typedef typename DbrTPtrFromInt<iDbrType    >::TDbr TDbrOrgP;
    typedef typename DbrTypeFromInt<iDbrTimeType>::TDbr TDbrTime;
    typedef typename DbrTypeFromInt<iDbrCtrlType>::TDbr TDbrCtrl;    
    typedef typename PdsTypeFromInt<iDbrTimeType>::TDbr TPdsTime;    
    typedef typename PdsTypeFromInt<iDbrCtrlType>::TDbr TPdsCtrl;    
}; 

template <class TDbr> struct DbrIntFromType { char a[-1 + (TDbr)0]; }; // Default should not be used. Will generate compilation error.
template <class TDbr> struct DbrIntFromType<const TDbr> : DbrIntFromType<TDbr> {}; // remove const qualifier
template <> struct DbrIntFromType<dbr_string_t> { enum {iTypeId = DBR_STRING }; };
template <> struct DbrIntFromType<dbr_short_t>  { enum {iTypeId = DBR_SHORT  }; };
template <> struct DbrIntFromType<dbr_float_t>  { enum {iTypeId = DBR_FLOAT  }; };
template <> struct DbrIntFromType<dbr_enum_t>   { enum {iTypeId = DBR_ENUM   }; };
template <> struct DbrIntFromType<dbr_char_t>   { enum {iTypeId = DBR_CHAR   }; };
template <> struct DbrIntFromType<dbr_long_t>   { enum {iTypeId = DBR_LONG   }; };
template <> struct DbrIntFromType<dbr_double_t> { enum {iTypeId = DBR_DOUBLE }; };

template <class TDbr> struct DbrIntFromCtrlType { char a[-1 + (TDbr)0]; }; // Default should not be used. Will generate compilation error.
template <class TDbr> struct DbrIntFromCtrlType<const TDbr> : DbrIntFromCtrlType<TDbr> {}; // remove const qualifier
template <> struct DbrIntFromCtrlType<dbr_sts_string>  { enum {iTypeId = DBR_STRING }; };
template <> struct DbrIntFromCtrlType<dbr_ctrl_short>  { enum {iTypeId = DBR_SHORT  }; };
template <> struct DbrIntFromCtrlType<dbr_ctrl_float>  { enum {iTypeId = DBR_FLOAT  }; };
template <> struct DbrIntFromCtrlType<dbr_ctrl_enum>   { enum {iTypeId = DBR_ENUM   }; };
template <> struct DbrIntFromCtrlType<dbr_ctrl_char>   { enum {iTypeId = DBR_CHAR   }; };
template <> struct DbrIntFromCtrlType<dbr_ctrl_long>   { enum {iTypeId = DBR_LONG   }; };
template <> struct DbrIntFromCtrlType<dbr_ctrl_double> { enum {iTypeId = DBR_DOUBLE }; };

extern const char* sDbrPrintfFormat[];

template <class TDbr> 
void printValue( TDbr* pValue ) 
{ 
    enum { iDbrType = DbrIntFromType<TDbr>::iTypeId };
    printf( sDbrPrintfFormat[iDbrType], *pValue );    
}

template <> inline
void printValue( const dbr_string_t* pValue )
{ 
    printf( "%s", (char*) pValue );
}

template <class TCtrl> // Default not to print precision field
void printPrecisionField(TCtrl& pvCtrlVal)  {}

template <> inline 
void printPrecisionField(const dbr_ctrl_double& pvCtrlVal)
{
  printf( "Precision: %d\n", pvCtrlVal.precision() );
}

template <> inline 
void printPrecisionField(const dbr_ctrl_float& pvCtrlVal)
{
  printf( "Precision: %d\n", pvCtrlVal.precision() );    
}

template <class TCtrl> inline
void printCtrlFields(TCtrl& pvCtrlVal)
{
    printPrecisionField(pvCtrlVal);
    printf( "Units: %s\n", pvCtrlVal.units() );        
    
    enum { iDbrType = DbrIntFromCtrlType<TCtrl>::iTypeId };    

    std::string sFieldFmt = sDbrPrintfFormat[iDbrType];
    std::string sOutputString = std::string() +
      "Hi Disp : " + sFieldFmt + "  Lo Disp : " + sFieldFmt + "\n" + 
      "Hi Alarm: " + sFieldFmt + "  Hi Warn : " + sFieldFmt + "\n" +
      "Lo Warn : " + sFieldFmt + "  Lo Alarm: " + sFieldFmt + "\n" +
      "Hi Ctrl : " + sFieldFmt + "  Lo Ctrl : " + sFieldFmt + "\n";
     
    printf( sOutputString.c_str(), 
            pvCtrlVal.upper_disp_limit(), pvCtrlVal.lower_disp_limit(), 
            pvCtrlVal.upper_alarm_limit(), pvCtrlVal.upper_warning_limit(), 
            pvCtrlVal.lower_warning_limit(), pvCtrlVal.lower_alarm_limit(), 
            pvCtrlVal.upper_ctrl_limit(), pvCtrlVal.lower_ctrl_limit()
      );
    
    return;
}

template <> inline 
void printCtrlFields(const dbr_sts_string& pvCtrlVal) {}

template <> inline 
void printCtrlFields(const dbr_ctrl_enum& pvCtrlVal) 
{
  if ( pvCtrlVal.no_str() > 0 )
    {
      printf( "EnumState Num: %d\n", pvCtrlVal.no_str() );
        
      for (int iEnumState = 0; iEnumState < pvCtrlVal.no_str(); iEnumState++ )
        printf( "EnumState[%d]: %s\n", iEnumState, pvCtrlVal.strings(iEnumState) );
    }
}

} // namespace EpicsDbrTools

} // namespace Pds

#endif
