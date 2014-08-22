#ifndef Pds_EvrL1Data_hh
#define Pds_EvrL1Data_hh

#include <stdio.h>
#include <pthread.h>

#include "EvrDataUtil.hh"
#include "pdsdata/xtc/ClockTime.hh"

namespace Pds
{
/*
 * Circular queue for storing EVR L1 Data
 *
 * [Interfaces]
 *   - Writing to the bottom of circular queue
 *   - Reading from the top of circular queue
 *   - Checking if the reading/wrting is allowed
 *       - If there is no buffered data, the data reading is not ready
 *       - If all buffers in the circular queue are used, the data writing is not ready
 *   - Finishing the reading/writing to move the cursor to next buffer
 *
 * [Data strcuture for EVR L1 Data]
 *   - Main EVR L1 Object  : EvrDataUtil 
 *   - DataFull flag       : bool
 *   - DataIncompelte flag : bool
 *   - Counter             : int
 *       Special value: -1 means invalid counter, will be used to mark the out-of-order data
 *    
 */

class EvrL1Data
{
public:

  EvrL1Data ( int iNumBuffers, int iBufferSize );  
  ~EvrL1Data();
  
  void reset();
  
  //  void setDataWriteFull      (bool bVal)    { _lbDataFull       [_iDataWriteIndex] = bVal; }
  //  void setDataWriteIncomplete(bool bVal)    { _lbDataIncomplete [_iDataWriteIndex] = bVal; }
  //  void setCounterWrite       (const ClockTime& iCounter) { _liCounter        [_iDataWriteIndex] = iCounter; }
  bool getDataReadFull       ()             { return _lbDataFull       [_iDataReadIndex]; }
  bool getDataReadIncomplete ()             { return _lbDataIncomplete [_iDataReadIndex]; }
  const ClockTime& getCounterRead() const   { return _liCounter        [_iDataReadIndex]; }
  bool isDataWriteReady      ()             { return ( _iDataWriteIndex != _iDataReadIndex ); }  
  bool isDataReadReady       ()             { return ( _iDataReadIndex  != -1 ); }  
  
  int  readIndex             () const       { return _iDataReadIndex;  }
  int  writeIndex            () const       { return _iDataWriteIndex; }
  int  numOfBuffers          ()             { return _iNumBuffers;     }
      
  void* getDataRead();
  //  EvrDataUtil& getDataWrite();  
  void* nextWrite(const ClockTime&,bool,bool);
  
  void finishDataRead ();  // move on to the next data position for data read  
  //  void finishDataWrite(); // move on to the next data position for data write

  /*
   * find and get data with counter
   */
  int           findDataWithCounter(const ClockTime& iCounter);  
  void*         getDataWithIndex   (int iDataIndex);
  void          markDataAsInvalid  (int iDataIndex);
  
private:
  int    _iNumBuffers;
  int    _iBufferSize;
  
  int    _iDataReadIndex;
  int    _iDataWriteIndex;
  char*  _lDataCircBuffer; // Circular buffer for storing Evr L1Accept events

  bool*  _lbDataFull;
  bool*  _lbDataIncomplete;  
  ClockTime* _liCounter;
  
  
  /*
   * Thread syncronization (lock/unlock) functions
   */  
  class LockData
  {
  public:
    LockData() 
    {
      if ( pthread_mutex_lock(&_mutexData) )
        printf( "EvrL1Data::LockData::LockData(): pthread_mutex_timedlock() failed\n" );
    }
    
    ~LockData() 
    { 
      pthread_mutex_unlock(&_mutexData);
    }
    
  private:
    static pthread_mutex_t _mutexData;    
  }; 
  
}; //class EvrL1Data

} // namespace Pds

#endif // #ifndef Pds_EvrL1Data_hh
