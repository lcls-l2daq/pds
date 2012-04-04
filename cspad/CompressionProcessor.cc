#include <stdlib.h>
#include <memory.h>
#include <omp.h>
#include <vector>
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/cspad/ElementV2.hh"
#include "pdsdata/cspad/ElementIterator.hh"
#include "pds/cspad/CompressionProcessor.hh"
#include "pds/config/CsPadConfigType.hh"
#include "pds/xtc/CDatagram.hh"
#include "pdsdata/cspad/CompressorOMP.hh"
#include "pdsdata/cspad/CspadCompressor.hh"

namespace Pds {  

static const int 
  iColumnsPerASIC = Pds::CsPad::ColumnsPerASIC, // 185
  iMaxRowsPerASIC = Pds::CsPad::MaxRowsPerASIC, // 194
  iSectionSize    = iColumnsPerASIC * 2 * iMaxRowsPerASIC * sizeof(uint16_t); // == sizeof(Pds::CsPad::Section)
static int          iNumThreads         = 0;  

static double       fTimeCompressionWall= 0;
static double       fTimeCompressionProc= 0;
static double       fTimeCopy           = 0;
static int          iMaxImagesPerElement= 0;
static int          iNumImageElements   = 0;
static int64_t      i64TotalOrgImgSize  = 0;
static int64_t      i64ReducedSize      = 0;
static int          iOutputTotalSize    = 0;

static bool         bCspadConfigured    = false;
static CsPadConfigType* 
                    pCspadConfig        = NULL;
static Pds::CsPad::CompressorOMP<Pds::CsPad::CspadCompressor>* 
                    pCompressor         = NULL;
static void**       lpInData            = NULL;                    
static Pds::CsPad::CspadCompressor::ImageParams*
                    lCompressor_params  = NULL;    
static void**       lpOutData           = NULL;
static size_t*      lOutDataSize        = NULL;
static int*         lCompressionStatus  = NULL;
static int          iDataIndex          = 0;

class XtcIterCspadData : public XtcIterator {
private:
  struct ElementData
  {
    const Pds::CsPad::ElementHeader* pHeader;
    int                              iNumSection;
    uint32_t                         u32QuadWord;      
  };  

  CspadCompressionProcessor* _pProcessor;
  std::vector<ElementData>   _lElementData;
  int                        _iNumCspadElements;
public:
  enum {Stop, Continue};
  XtcIterCspadData(Xtc* xtc, CspadCompressionProcessor* pProcessor) : 
    XtcIterator(xtc), _pProcessor(pProcessor), _iNumCspadElements(0) {}
    
  void processCspadElement(const DetInfo& info, Xtc* xtc) {
    
    ++_iNumCspadElements;
    if (_iNumCspadElements > 1)
    {
      printf("XtcIterCspadData::processCspadElement(): %d CspadElements found in xtc data. Only the first element"
        " was processed; the others are skipped\n", _iNumCspadElements);      
      return;
    }

    /*
     * initialize statistics variables
     */
    fTimeCompressionWall= 0;
    fTimeCompressionProc= 0;
    fTimeCopy           = 0;
    iNumImageElements   = 0;
    i64TotalOrgImgSize  = 0;
    i64ReducedSize      = 0;
    iOutputTotalSize    = 0;       
        
    CsPad::ElementIterator* pIter;
    if (pCspadConfig != NULL)
      pIter = new CsPad::ElementIterator(*pCspadConfig, *xtc);
    else
    {
      printf("XtcIterCspadData::processCspadElement(): No CsPad Config objects found before the data section\n");
      return;
    }
    
    CsPad::ElementIterator& iter = *pIter;
    
    int iElement = 0;
    while( const Pds::CsPad::ElementHeader* element=iter.next() ) {  // loop over elements (quadrants)        
      unsigned uSectorMask = pCspadConfig->roiMask(element->quad());      
      const Pds::CsPad::Section* s            = NULL;
      const Pds::CsPad::Section* sPrev        = NULL;      
      int                        iNumSection  = 0;
      unsigned section_id;
      while( (s=iter.next(section_id)) ) {  // loop over sections (two by one's)
        sPrev = s;
        if (uSectorMask & (1<<section_id))
        {      
          ++iNumSection;
          lpInData[iDataIndex] = (void*)&s->pixel[0][0];       
          ++iDataIndex;
          
          if (iDataIndex > iMaxImagesPerElement)
          {
            printf("!!! XtcIterCspadData::processCspadElement(): Found more image sections than the specified limit (%d)\n",
              iMaxImagesPerElement);
            iDataIndex = 0;
            abort();
          }          
        }
      }        
      
      const uint32_t* u = reinterpret_cast<const uint32_t*>(sPrev+1);

      //!! debug only
      //printf("XtcIterCspadData::processCspadElement(): quade %d num section %d\n", element->quad(), iNumSection);
      
      _lElementData.push_back(ElementData());
      ElementData& elementData = _lElementData.back();
      elementData.pHeader      = element;
      elementData.iNumSection  = iNumSection;
      elementData.u32QuadWord  = *u;
      ++iElement;      
    }

    if (iDataIndex > 0)
    {
      runCompression();
      iDataIndex = 0; // reset the data index, since all stored data have been processed
    }   
    
    int     iXtcSize = xtc->extent;
    int64_t i64CompressedImgSize = i64TotalOrgImgSize-i64ReducedSize; 
    double  fThroughputWall      = i64TotalOrgImgSize / fTimeCompressionWall / 1048576;
    double  fThroughputThread    = i64TotalOrgImgSize / fTimeCompressionProc / 1048576;
    double  fThroughputCopy      = iOutputTotalSize / fTimeCopy / 1048576;
    int64_t i64CompressedNewSize = iXtcSize-i64ReducedSize;    
                
    if (_pProcessor->_iNumProcessedEvents % _pProcessor->_iDispPrint == 0)
    {
      printf("\n# 2x1 Images %d  # Image total size %.1f MB  Avg size %.1f MB/Element  Xtc Datas size %.1f MB\n",
        iNumImageElements, i64TotalOrgImgSize / 1048576.0, 
        i64TotalOrgImgSize / (double) iNumImageElements / 1048576, 
        iXtcSize / 1048576.0 );        
      
      printf("# Compression Wall time %4.1f ms  Throughput %8.3f MB/S\n",
        fTimeCompressionWall * 1000.0, fThroughputWall);

      printf("# Compression CPU  time %4.1f ms  Throughput %8.3f MB/S (per thread)\n",
        fTimeCompressionProc * 1000.0, fThroughputThread);

      printf("# Ideal       CPU  time %4.1f ms  Throughput %8.3f MB/S (parallel %d threads)\n",
        fTimeCompressionProc * 1000.0 / iNumThreads, fThroughputThread*iNumThreads, iNumThreads); 
            
      printf("# Image compression ratio %.1f%% (%.1f MB/ %.1f MB)\n",
        i64CompressedImgSize * 100.0 / i64TotalOrgImgSize,
        i64CompressedImgSize / 1048576.0, i64TotalOrgImgSize / 1048576.0 );
        
      printf("# Xtc   compression ratio %.1f%% (%.1f MB/ %.1f MB)\n",
        i64CompressedNewSize * 100.0 / iXtcSize,
        i64CompressedNewSize / 1048576.0, iXtcSize / 1048576.0);
        
      printf("# Copy time %4.1f ms (Size %.1f MB) Throughput %8.3f MB/S\n",
        fTimeCopy * 1000.0, iOutputTotalSize/ 1048576.0, fThroughputCopy);
    }
    
    delete pIter;    
  }

  int runCompression()
  {    
    // constraint: this function is only called once by processCspadElement()
    
    const int iCompressionInDataSize = iSectionSize * iDataIndex;
    struct timespec tsStart,  tsEnd;
    struct timespec tsProcStart,  tsProcEnd;
    
    clock_gettime(CLOCK_REALTIME, &tsStart);
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tsProcStart);          
    
    /*
     * call the main compression algorithm
     */
    pCompressor->compress((const void**)lpInData, lCompressor_params, 
      lpOutData, lOutDataSize, lCompressionStatus, iDataIndex);
              
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tsProcEnd);            
    clock_gettime(CLOCK_REALTIME, &tsEnd);
              
    double  fSectionTimeCompressionWall = tsEnd.tv_sec - tsStart.tv_sec + tsEnd.tv_nsec * 1e-9 - tsStart.tv_nsec * 1e-9;
    fTimeCompressionWall  += fSectionTimeCompressionWall;        

    double  fSectionTimeCompressionProc = tsProcEnd.tv_sec - tsProcStart.tv_sec + tsProcEnd.tv_nsec * 1e-9 - tsProcStart.tv_nsec * 1e-9;
    fTimeCompressionProc  += fSectionTimeCompressionProc;        
    
    int iOutSizeSum = 0;
    for (int iData = 0; iData < iDataIndex; ++iData)
      iOutSizeSum += (int) lOutDataSize[iData];
                           
    i64ReducedSize     += iCompressionInDataSize - iOutSizeSum;            
    i64TotalOrgImgSize += iCompressionInDataSize;
    iNumImageElements  += iDataIndex;   
            
    // output data
    struct timespec tsCopyStart,  tsCopyEnd;
    clock_gettime(CLOCK_REALTIME, &tsCopyStart);
    
    //char* pOutput   = _pProcessor->_pOutData + iOutputTotalSize;
    char* pOutput   = _pProcessor->_pOutData;
    
    //for (int iData = 0; iData < iDataIndex; ++iData)
    //{
    //  memcpy(pOutput, lpOutData[iData], lOutDataSize[iData]);
    //  pOutput += lOutDataSize[iData];
    //}    
    
    Xtc* pXtcOut    = (Xtc*) pOutput; pOutput += sizeof(Xtc);
    Xtc* pXtcCspad  = (Xtc*) pOutput; pOutput += sizeof(Xtc);

    char* pCspadDataStart = pOutput;
    int   iSection        = 0;    
    
    for (int iElement = 0; iElement < (int) _lElementData.size(); ++iElement)
    {
      ElementData& elementData = _lElementData[iElement];
      
      Pds::CsPad::ElementHeader* element = (Pds::CsPad::ElementHeader*) pOutput;
      *element =  *elementData.pHeader;
      pOutput  += sizeof(Pds::CsPad::ElementHeader);
      
      for (int iSectionInCurElement = 0; iSectionInCurElement < elementData.iNumSection; 
        ++iSectionInCurElement, ++iSection)
      {
          memcpy(pOutput, lpOutData[iSection], lOutDataSize[iSection]);
          pOutput += lOutDataSize[iSection];
      }
      
      *(uint32_t*) pOutput = elementData.u32QuadWord;
      pOutput += sizeof(uint32_t);
    }
    
    int iSizeMod4 = (pOutput - pCspadDataStart) & 0x3;
    if (iSizeMod4 != 0)
      pOutput += (4 - iSizeMod4);
      
    int iSizeData       = pOutput - pCspadDataStart;
    pXtcCspad->extent   = sizeof(Xtc) + iSizeData;
    pXtcOut  ->extent   = sizeof(Xtc) + pXtcCspad->extent;    
    
    /*
     * Set type id: Set version to "2", because it is a compressed data based on ElementV2
     */
    pXtcCspad->contains = TypeId(TypeId::Id_CspadCompressedElement, CsPad::ElementV2::Version); 
    
    iOutputTotalSize = pOutput - _pProcessor->_pOutData;

    clock_gettime(CLOCK_REALTIME, &tsCopyEnd);
    double  fSectionTimeCopy = tsCopyEnd.tv_sec - tsCopyStart.tv_sec + tsCopyEnd.tv_nsec * 1e-9 - tsCopyStart.tv_nsec * 1e-9;
    fTimeCopy += fSectionTimeCopy;
    
    return 0;
  }  
  
  int process(Xtc* xtc) {    
    DetInfo& info = *(DetInfo*)(&xtc->src);
    
    switch (xtc->contains.id()) {
    //case (TypeId::Id_Xtc) : {
    //  XtcIterCspadData(xtc, _pProcessor).iterate();
    //  break;
    //}
    case (TypeId::Id_CspadElement) :
    {
      switch (xtc->contains.version())
      {        
      case 1:
      case 2:
        processCspadElement((DetInfo&) info, xtc);
        break;
      default:
        printf("XtcIterCspadData::process(): Unsupported CspadElement version %d\n", xtc->contains.version());
        break;
      }
      break;
    }        
    default :
      printf("XtcIterCspadData::process(): Unsupported Data Type: %s\n", TypeId::name(xtc->contains.id()));
      break;
    } // switch (xtc->contains.id())
    return Continue;
  }
};

CspadCompressionProcessor::CspadCompressionProcessor(Appliance& appProcessor, unsigned iNumThreads1, int iImagesPerElement1) : 
  _appProcessor(appProcessor), _iNumThreads(iNumThreads1), _iImagesPerElement(iImagesPerElement1), 
  _poolOutputData(_iMaxOutputDataSize, _iPoolDataCount), _pOutDg(NULL),
  _histTimeRouine(0,100,1), _histTimeCall(0,100,1), _histInpLevel(0,20,1), _histOutLevel(0,_iPoolDataCount,1)
{
  printf("CspadCompressionProcessor::CspadCompressionProcessor() starts\n");
  _pTaskCompression = new Task(TaskObject("CspadCompression"));
  
  iNumThreads           = _iNumThreads;
  iMaxImagesPerElement  = _iImagesPerElement;

  printf("Thread #: %d  Images/Element: %d\n", iNumThreads, _iImagesPerElement);
    
  // setup compressors
  pCompressor         = new Pds::CsPad::CompressorOMP<Pds::CsPad::CspadCompressor>
                                  (_iImagesPerElement, _iNumThreads);
  lCompressor_params  = new Pds::CsPad::CspadCompressor::ImageParams
                                  [_iImagesPerElement];    
  lpOutData           = new void* [_iImagesPerElement];
  lOutDataSize        = new size_t[_iImagesPerElement];
  lCompressionStatus  = new int   [_iImagesPerElement];

  lpInData            = new void* [_iImagesPerElement];
  for (int iData = 0; iData < _iImagesPerElement; ++iData)
  {
    Pds::CsPad::CspadCompressor::ImageParams& compressor_params = 
      lCompressor_params[iData];
    compressor_params.width   = iSectionSize/2;
    compressor_params.height  = 1;
    compressor_params.depth   = 2;        
  }  
}

CspadCompressionProcessor::~CspadCompressionProcessor() 
{
  if ( _pTaskCompression != NULL )
    _pTaskCompression->destroy(); // task object will destroy the thread and release the object memory by itself
  
  delete[] lpInData;            lpInData            = NULL;
  delete[] lCompressionStatus;  lCompressionStatus  = NULL;
  delete[] lOutDataSize;        lOutDataSize        = NULL;
  delete[] lpOutData;           lpOutData           = NULL;
  delete[] lCompressor_params;  lCompressor_params  = NULL;
  delete   pCompressor;         pCompressor         = NULL;
  delete   pCspadConfig;        pCspadConfig        = NULL;
}

class XtcIterCspadConfig : public XtcIterator {
public:
  enum {Stop, Continue};
  XtcIterCspadConfig(Xtc* xtc) : XtcIterator(xtc)
  {}

  void processCspadConfig(DetInfo& info, Xtc* xtc) {    
    if (xtc->sizeofPayload() != sizeof(CsPadConfigType))
    {
      printf("XtcIterCspadConfig::processCspadConfig(): Incorrect payload size: Get %d; expected %d\n",
        xtc->sizeofPayload(), sizeof(CsPadConfigType));
      return;
    }
    
    pCspadConfig = (CsPadConfigType*) new char[xtc->sizeofPayload()];
    memcpy(pCspadConfig, xtc->payload(), xtc->sizeofPayload());
  }

  int process(Xtc* xtc) {
    DetInfo& info = *(DetInfo*)(&xtc->src);
    
    switch (xtc->contains.id()) {
    case (TypeId::Id_Xtc) : {
      XtcIterCspadConfig(xtc).iterate();
      break;
    }
    case (TypeId::Id_CspadConfig) :
    {
      processCspadConfig( (DetInfo&) info, xtc);
      bCspadConfigured = true;      
      break;
    } // case (TypeId::Id_CspadConfig) 
    
    default :
      printf("XtcIterCspadConfig::process(): Unsupported Config Type: %s\n", TypeId::name(xtc->contains.id()));
      break;
    } // switch (xtc->contains.id())
    return Stop;
  }
  
};  

int CspadCompressionProcessor::readConfig(Xtc* xtc)
{
  if (xtc == NULL)
  {
    printf("CspadCompressionProcessor::readConfig(): Config data is empty\n");
    return 1;
  }

  bCspadConfigured = false;
  
  XtcIterCspadConfig(xtc).iterate();
  
  if (bCspadConfigured)
    printf("CspadCompressionProcessor::readConfig(): Read config successfully\n");
  else
    printf("!!! CspadCompressionProcessor::readConfig(): Failed to read config\n");  
      
  /*
   * reset run-related variables
   */
  _iNumEvents       = 0;
  _iNumProcessedEvents 
                    = 0;
  _fTimeRun         = 0;
  _fTimeTotalProcessorCall 
                    = 0;
  _iDispPrint       = 10;
  _iBusyPrint       = 1;
  _iContinuousBusyCount = _iContinuousOkayCount = _iTotalBusyCount = 0;

  if (_lpInpDg.size() > 0)
    printf( "!!! CspadCompressionProcessor::readConfig(): Input data buffer is not empty: %d used.\n",
      _lpInpDg.size());
  
  if (_poolOutputData.numberOfAllocatedObjects() != 0)
    printf( "!!! CspadCompressionProcessor::readConfig(): Data pool is not empty: %d/%d allocated\n", 
      _poolOutputData.numberOfAllocatedObjects(), _poolOutputData.numberofObjects());  
  
  /*
   * reset routine-based variables
   */
  _lpInpDg.clear();
  _pOutDg           = NULL;
  _pOutData         = NULL;
  //_iOutSize         = 0;
  
  _histTimeRouine .reset();
  _histTimeCall   .reset();
  _histInpLevel   .reset();
  _histOutLevel   .reset();
    
  return 0;
}

/*
 * Thread syncronization (lock/unlock) functions
 */
static pthread_mutex_t _mutexCompressionFifo = PTHREAD_MUTEX_INITIALIZER;    

inline static void lockFifoData(char* sDescription)
{
  if ( pthread_mutex_lock(&_mutexCompressionFifo) )
    printf( "lockFifoData(): pthread_mutex_lock() failed for %s\n", sDescription );
}
inline static void unlockFifoData()
{
  pthread_mutex_unlock(&_mutexCompressionFifo);
}

int CspadCompressionProcessor::compressData(InDatagram& dg, bool bForceSkip)
{  
  //printf("CspadCompressionProcessor::compressData(): input datagram %p\n", &dg); //!!debug
  
  static timespec tsProcessorCallStart;
  clock_gettime(CLOCK_REALTIME, &tsProcessorCallStart);
  
  ++_iNumEvents;  
  
  double fTimeProcessorCall;
  if (_iNumEvents == 1) // Note: dont process the first call time, because of no referenced timestamp 
    fTimeProcessorCall =  0;
  else
    fTimeProcessorCall = tsProcessorCallStart.tv_sec - _tsPrevProcessorStart.tv_sec + 
                            tsProcessorCallStart.tv_nsec * 1e-9 - _tsPrevProcessorStart.tv_nsec * 1e-9;
                              
  _fTimeTotalProcessorCall += fTimeProcessorCall;
  _tsPrevProcessorStart    =  tsProcessorCallStart;    
  
  _histInpLevel.addValue(_lpInpDg.size());
  _histTimeCall.addValue(fTimeProcessorCall * 1000.0 );
 
  lockFifoData("Add data to Input Buffer");
  _lpInpDg.push_front(InpData(&dg, bForceSkip, fTimeProcessorCall, *this));
  //printf("CspadCompressionProcessor::compressData(): [%d] input buffer size = %d\n", _iNumEvents, _lpInpDg.size()); //!!debug
  unlockFifoData();   
  
  //if ( _iNumProcessedEvents == 1 )
  //{    
  //  struct timespec tsStart,  tsEnd;
  //  
  //  clock_gettime(CLOCK_REALTIME, &tsStart);

  //  _pXtc     = (Xtc*) new char[xtc->extent];
  //  memcpy(_pXtc, xtc, xtc->extent);
  //  
  //  clock_gettime(CLOCK_REALTIME, &tsEnd);
  //  double  fTimeCopy       = tsEnd.tv_sec - tsStart.tv_sec + tsEnd.tv_nsec * 1e-9 - tsStart.tv_nsec * 1e-9;
  //  double  fThroughputCopy = xtc->extent / fTimeCopy / 1048576.0;
  //  printf("### First memory copy for xtc data size %.1f MB  Time %4.1f ms  throuput %8.3f MB/s\n",
  //    xtc->extent / 1048576.0, fTimeCopy * 1000.0, fThroughputCopy);
  //}
    
  _pTaskCompression->call( & (_lpInpDg.front().routine) );
  
  return 0;
}

class XtcIterCspadShuffle {
public:
  enum Status {Stop, Continue};
  XtcIterCspadShuffle(Xtc* root, uint32_t*& pwrite) : _root(root), _pwrite(pwrite) {}

private:
  void _write(const void* p, ssize_t sz) 
  {
    if (_pwrite!=(uint32_t*)p) 
      memmove(_pwrite, p, sz);
      
    _pwrite += sz>>2;
  }

public:
  void iterate() { iterate(_root); }

private:
  void iterate(Xtc* root) 
  {
    if (root->damage.value() & ( 1 << Damage::IncompleteContribution)) {
      return _write(root,root->extent);
    }

    int remaining = root->sizeofPayload();
    Xtc* xtc     = (Xtc*)root->payload();

    uint32_t* pwrite = _pwrite;
    _write(root, sizeof(Xtc));
    
    while(remaining > 0)
    {
      unsigned extent = xtc->extent;
      if(extent==0) {
        printf("Breaking on zero extent\n");
        break; // try to skip corrupt event
      }
      process(xtc);
      remaining -= extent;
      xtc        = (Xtc*)((char*)xtc+extent);
    }

    reinterpret_cast<Xtc*>(pwrite)->extent = (_pwrite-pwrite)*sizeof(uint32_t);
  }

  void process(Xtc * xtc)
  {
    switch (xtc->contains.id()) 
    {
    case (TypeId::Id_Xtc):
    {
      XtcIterCspadShuffle iter(xtc, _pwrite);
      iter.iterate();
      return;
    }
    case (TypeId::Id_CspadElement):
    {
      if (xtc->damage.value())
        break;
      if (xtc->contains.version() != 1)
        break;      
      if (pCspadConfig == NULL)
      {
        printf("XtcIterCspadShuffle::process(): No CsPad Config objects found before the data section\n");
        break;
      }
      const CsPadConfigType& cfg = *pCspadConfig;
      CsPad::ElementIterator iter(cfg, *xtc);

      // Copy the xtc header
      uint32_t *pwrite = _pwrite;
      xtc->contains = TypeId(TypeId::Id_CspadElement, CsPad::ElementV2::Version);
      _write(xtc, sizeof(Xtc));

      const CsPad::ElementHeader* hdr;
      while ((hdr = iter.next()))
      {
        _write(hdr, sizeof(*hdr));

        unsigned smask = cfg.roiMask(hdr->quad());
        unsigned id;
        const CsPad::Section * s = 0, *end = 0;
        while ((s = iter.next(id)))
        {
          if (smask & (1 << id))
            _write(s, sizeof(*s));
          end = s;
        }
        //  Copy the quadrant trailer
        _write(reinterpret_cast < const uint16_t * >(end + 1), 2 * sizeof(uint16_t));
      }
      //  Update the extent of the container
      reinterpret_cast < Xtc * >(pwrite)->extent = (_pwrite - pwrite) * sizeof(uint32_t);
      return;
    }
    default:
      break;
    } // switch (xtc->contains.id()) 
    _write(xtc, xtc->extent);
  }

private:
  Xtc*       _root;
  uint32_t*& _pwrite;
};

void CspadCompressionProcessor::routineCompression()  
{   
  if (_lpInpDg.size() == 0) 
  {
    printf("!!! CspadCompressionProcessor::routineCompression(): [%d] Input Data Buffer is empty\n", _iNumEvents);
    return;
  }
  
  lockFifoData("Get data from Input Buffer");
  //printf("CspadCompressionProcessor::routineCompression(): [%d] input buffer size = %d\n", _iNumEvents, _lpInpDg.size()); //!!debug
  InpData     inpData = _lpInpDg.back(); 
  _lpInpDg.pop_back();
  unlockFifoData();

  InDatagram* pInpDg = inpData.pInpDg;
  _histOutLevel.addValue(_poolOutputData.numberOfAllocatedObjects());  
  
  bool bSkip = inpData.bSkip;
  if (!bSkip  && _poolOutputData.numberOfFreeObjects() == 0)
  {    
    ++_iContinuousBusyCount;
    ++_iTotalBusyCount;
    _iContinuousOkayCount = 0;
    
    if (_iContinuousBusyCount % _iBusyPrint == 0)
      printf("!!! [%d] Busy<%d> Data pool is full: %d/%d allocated. Skip rate %4.1f%% (%d/%d). Previous routine time %4.1f ms. "
        "Call interval %4.1f ms\n", 
        _iNumEvents, _iContinuousBusyCount, 
        _poolOutputData.numberOfAllocatedObjects(), _poolOutputData.numberofObjects(),
        _iTotalBusyCount * 100.0 / _iNumEvents, 
        _iTotalBusyCount, _iNumEvents, _fTimeRoutine * 1000, inpData.fTimeProcessorCall * 1000);

    if (_iContinuousBusyCount >= _iBusyPrint*10 && _iBusyPrint < 1000)
      _iBusyPrint *= 10;       
      
    bSkip = true;        
  }
  
  if (++_iContinuousOkayCount > 10)
  {
    _iContinuousBusyCount = 0;
    _iBusyPrint           = 1;
  }  
  
  if (bSkip)
  {
    /*
     * post the output data
     */
    //printf("CspadCompressionProcessor::routineCompression(): skip datagram %p\n", pInpDg); //!!debug    
    Xtc* pPayloadXtc = (Xtc*) pInpDg->xtc.payload();
    if (pPayloadXtc->contains.id() == Pds::TypeId::Id_CspadElement)
    {
      uint32_t* pDg = reinterpret_cast<uint32_t*>(&(pInpDg->xtc));
      XtcIterCspadShuffle(&(pInpDg->xtc), pDg).iterate();
    }        
    _appProcessor.post(pInpDg);
    return;
  }
  
  clock_gettime(CLOCK_REALTIME, &_tsCompStart);
  ++_iNumProcessedEvents;
  
  _pOutDg = 
    new ( &_poolOutputData ) CDatagram( TypeId(TypeId::Any,0), DetInfo(0,DetInfo::NoDetector,0,DetInfo::NoDevice,0) );
    
  /*
   * copy the datagram and xtc header from input datagram
   */
  //_pOutDg->datagram() = pInpDg->datagram();
  memcpy( & _pOutDg->datagram(), & pInpDg->datagram(), sizeof(Datagram) + sizeof(Xtc) );
  //memcpy( _pOutDg->datagram().xtc.payload(), pInpDg->datagram().xtc.payload(), sizeof(Xtc) );
  _pOutData = (char*) &(_pOutDg->datagram().xtc);
  //_iOutSize = 0;
      
  XtcIterCspadData(&(pInpDg->datagram().xtc), this).iterate();
        
  delete pInpDg;
  //printf("CspadCompressionProcessor::routineCompression(): post datagram %p\n", _pOutDg); //!!debug
  _appProcessor.post(_pOutDg);
  
  ///*
  // * post the input data for now, since event level doesn't know about the compressed data
  // */
  //delete _pOutDg;
  ////printf("CspadCompressionProcessor::routineCompression(): post datagram %p\n", pInpDg); //!!debug
  //_appProcessor.post(pInpDg);
  
  /*
   * timing and report
   */  
  clock_gettime(CLOCK_REALTIME, &_tsCompEnd);
            
  _fTimeRoutine =  _tsCompEnd.tv_sec - _tsCompStart.tv_sec + _tsCompEnd.tv_nsec * 1e-9 - _tsCompStart.tv_nsec * 1e-9;
  _fTimeRun     += _fTimeRoutine;
  
  _histTimeRouine.addValue( _fTimeRoutine * 1000 );
  
  if (_iNumProcessedEvents % _iDispPrint == 0)
  {
    // Note: the first processor call time is 0, because of no referenced timestamp 
    printf("### Compressor Routine Wall time %4.1f ms  Processed Event# %d  Total# %d  Avg Wall time %4.1f ms  Avg Call time %4.1f ms\n",
      _fTimeRoutine * 1000.0, _iNumProcessedEvents, _iNumEvents, 
      _fTimeRun * 1000 / _iNumProcessedEvents, _fTimeTotalProcessorCall * 1000 / (_iNumEvents-1)
      );
    _histTimeRouine .report("Time Routine (ms)");
    _histTimeCall   .report("Time Call    (ms)");
    _histInpLevel   .report("InpData Buffer Use Count");
    _histOutLevel   .report("OutData Buffer Use Count");
      
  }  
  if (_iNumProcessedEvents >= _iDispPrint*10 && _iDispPrint < 1000)
    _iDispPrint *= 10;          
  
  // reset routine data
  _pOutDg           = NULL;
  _pOutData         = NULL;
  //_iOutSize         = 0;
}  

}
