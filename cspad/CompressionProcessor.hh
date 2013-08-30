#ifndef COMPRESSION_PROCESSOR_HH_
#define COMPRESSION_PROCESSOR_HH_

#include <vector>
#include <list>
#include "pdsdata/xtc/Xtc.hh"
#include "pds/utility/Appliance.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Task.hh"
#include "pds/service/GenericPool.hh"


namespace Pds {

#if 1
class CspadCompressionProcessor
{
public:
  CspadCompressionProcessor(Appliance& appProcessor, unsigned iNumThreads, int iImagesPerElement, unsigned int uDebugFlag) {}
  virtual ~CspadCompressionProcessor() {}
  
  int  readConfig(Xtc* xtc) { return 0; }
  int  compressData(InDatagram& dg, bool bForceSkip = false) { return 0; }
  int  postData(InDatagram& dg) { return 0; }
  void routineCompression() {}
};   

#else

class HistReport
{  
public:  
  HistReport(float fFrom, float fTo, float fInterval) :
    _fFrom(fFrom), _fTo(fTo), _fInterval(fInterval)
  {
    reset();
    
    if ( _fFrom == (int) _fFrom && _fTo == (int) _fTo && _fInterval == (int) _fInterval )
      _bIntRange = true;
    else
      _bIntRange = false;
  }   
  
  void reset()
  {
    // assert( _fFrom < _fTo && _fInterval > 0 );
    int iSizeHist = 1 + (int) ((_fTo - _fFrom) / _fInterval);
    
    _lCounts.resize(iSizeHist+2,0);
    _lCounts.assign(iSizeHist+2,0); // see the comment of _lCounts below
    _iTotalCounts = 0;
  }
  
  void addValue(double fValue)
  {   
    int iIndex = (int) ((fValue - _fFrom) / _fInterval + 1e-4);
    
    if (iIndex < 0)    
      iIndex = (int) _lCounts.size() - 2;
    else if (iIndex >= (int) _lCounts.size() - 2)
      iIndex = (int) _lCounts.size() - 1;
    
    // assert( _fFrom < _fTo && _fInterval > 0 );
    ++(_lCounts[iIndex]);
    ++_iTotalCounts;
  }
  
  void report(const char* sTitle)
  {
    printf("# Historgram -- %s -- Total count: %d\n", sTitle, _iTotalCounts );            
        
    if ( _lCounts[ _lCounts.size() - 2 ] != 0 )
      printf("#    ( -Inf , %-4g ) : %7d (%4.1f%%)\n", _fFrom, _lCounts[ _lCounts.size() - 2 ], _lCounts[ _lCounts.size() - 2 ] * 100.0 / _iTotalCounts );
      
    
    for (unsigned iIndex = 0; iIndex < _lCounts.size()-2; ++iIndex)
    {
      if ( _lCounts[ iIndex ] != 0 )
        if (_bIntRange)      
          printf("#             %4d   : %7d (%4.1f%%)\n", (int)(_fFrom + iIndex * _fInterval),
            _lCounts[ iIndex ], _lCounts[ iIndex ] * 100.0 / _iTotalCounts  );
        else
          printf("#    [ %-4g , %-4g ) : %7d (%4.1f%%)\n", _fFrom + iIndex * _fInterval,
            _fFrom + (1+iIndex) * _fInterval, _lCounts[ iIndex ], _lCounts[ iIndex ] * 100.0 / _iTotalCounts  );
    }    
    
    if ( _lCounts[ _lCounts.size() - 1 ] != 0 )
      printf("#    [ %-4g , +Inf ) : %7d (%4.1f%%)\n", _fFrom + (_lCounts.size() - 2) * _fInterval, 
        _lCounts[ _lCounts.size() - 1 ], _lCounts[ _lCounts.size() - 1 ] * 100.0 / _iTotalCounts );
  }
  
private:
  float             _fFrom;
  float             _fTo;
  float             _fInterval;
  std::vector<int>  _lCounts; // The last two elements for storing (v < _fFrom) and ( v >= _fTo + _fInterval ) values
  int               _iTotalCounts;
  bool              _bIntRange;
};

  
  
class CspadCompressionProcessor
{
public:
  CspadCompressionProcessor(Appliance& appProcessor, unsigned iNumThreads, int iImagesPerElement, unsigned int uDebugFlag);
  virtual ~CspadCompressionProcessor();
  
  int  readConfig(Xtc* xtc);
  int  compressData(InDatagram& dg, bool bForceSkip = false);
  int  postData(InDatagram& dg) { return compressData(dg, true); }
  void routineCompression();
   
private:
  /*
   * private class: CspadCompressionProcessorRoutine
   *
   *   Support the routine interface for using the Pds::Task object
   */
  class CspadCompressionProcessorRoutine : public Routine 
  {
    public:
      CspadCompressionProcessorRoutine(CspadCompressionProcessor& processor) : _processor(processor) {}    
      void routine() { _processor.routineCompression(); }
      
    private:
      CspadCompressionProcessor& _processor;
  };
  
  struct InpData
  {
    InDatagram* pInpDg;
    bool        bSkip;
    float       fTimeProcessorCall;
    CspadCompressionProcessorRoutine
                routine;
    InpData(InDatagram* pInpDg1, bool bSkip1, float fTimeProcessorCall1, CspadCompressionProcessor& processor): 
      pInpDg(pInpDg1), bSkip(bSkip1), fTimeProcessorCall(fTimeProcessorCall1), routine(processor) {}    
  };
  
  static const int                 _iMaxOutputDataSize = 5242880; // 5 MB
  static const int                 _iPoolDataCount     = 10;

  Appliance&                       _appProcessor;
  unsigned int                     _iNumThreads;
  int                              _iImagesPerElement;
  unsigned int                     _uDebugFlag;
  
  GenericPool                      _poolOutputData;
  Task*                            _pTaskCompression;
  std::list<InpData>               _lpInpDg;   
  InDatagram*                      _pOutDg;
  char*                            _pOutData;
  bool                             _bPrintInfo;
  //int                              _iOutSize;
 
  // Routine-based variables
  struct timespec                  _tsCompStart, _tsCompEnd;
  double                           _fTimeProcessorCall, _fTimeRoutine;
  struct timespec                  _tsPrevProcessorStart;
  
  // Run-based variables
  int                              _iNumEvents, _iNumProcessedEvents;
  double                           _fTimeRun, _fTimeTotalProcessorCall;
  int                              _iDispPrint;
  int                              _iBusyPrint;
  int                              _iContinuousBusyCount, _iContinuousOkayCount;
  int                              _iTotalBusyCount;
  HistReport                       _histTimeRouine, _histTimeCall;
  HistReport                       _histInpLevel, _histOutLevel;
  
  friend class XtcIterCspadData;
};
#endif
  
}

#endif // #ifndef COMPRESSION_PROCESSOR_HH_
