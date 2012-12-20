#include <memory.h>
#include <new>

#include "EvrL1Data.hh"

namespace Pds
{

  static const ClockTime _invalid(-1,-1);

EvrL1Data::EvrL1Data( int iMaxNumFifoEvent, int iNumBuffers ) :
  _iMaxNumFifoEvent (iMaxNumFifoEvent),
  _iNumBuffers      (iNumBuffers),
  _iDataReadIndex   (-1),
  _iDataWriteIndex  (0),
  _lDataCircBuffer  ( new char[ iNumBuffers * EvrDataUtil::size( iMaxNumFifoEvent ) ]  ),
  _lbDataFull       ( new bool[ iNumBuffers ] ),
  _lbDataIncomplete ( new bool[ iNumBuffers ] ),
  _liCounter        ( new ClockTime[ iNumBuffers ] )
{
}

EvrL1Data::~EvrL1Data()
{
  delete[] _lDataCircBuffer;
  delete[] _lbDataFull;
  delete[] _lbDataIncomplete;
  delete[] _liCounter;
}

void EvrL1Data::reset()
{        
  // No need to clear _lDataCircBuffer, since it will be
  // cleared prior to the next update
  _iDataReadIndex  = -1;
  _iDataWriteIndex = 0;    
  
  memset( _lbDataFull,       0, _iNumBuffers * sizeof(bool) );
  memset( _lbDataIncomplete, 0, _iNumBuffers * sizeof(bool) ); 
  memset( _liCounter,        0, _iNumBuffers * sizeof(ClockTime) ); 
}

EvrDataUtil& EvrL1Data::getDataRead()
{ 
  //printf("EvrL1Data::getDataRead(): Read Index = %d\n", _iDataReadIndex );// !! debug
  EvrDataUtil& data = *(EvrDataUtil*)
    ( _lDataCircBuffer + _iDataReadIndex * EvrDataUtil::size( _iMaxNumFifoEvent ) );
  return data;
}

EvrDataUtil& EvrL1Data::getDataWrite()       
{ 
  //printf("EvrL1Data::getDataRead(): Write Index = %d\n", _iDataWriteIndex );// !! debug
  EvrDataUtil& data = *(EvrDataUtil*)
    ( _lDataCircBuffer + _iDataWriteIndex * EvrDataUtil::size( _iMaxNumFifoEvent ) );
  
  new (&data) EvrDataType( 0, NULL );
  return data;
}

//static bool bDataWriteAbnormal; // !! debug

void EvrL1Data::finishDataRead()  // move on to the next data position for data read
{    
  LockData lockData; // lock data indexes
  
  //if ( bDataWriteAbnormal ) // !! debug
  //{
  //  printf("EvrL1Data::finishDataRead(): Old Read  Index = %d\n", _iDataReadIndex );// !! debug
  //  printf("EvrL1Data::finishDataRead(): Old Write Index = %d\n", _iDataWriteIndex );// !! debug
  //}
  
  do
  {
    _iDataReadIndex = ((_iDataReadIndex+1) % _iNumBuffers);
    if ( _iDataReadIndex == _iDataWriteIndex ) // no valid data in queue
    {
      _iDataReadIndex = -1;   
      break;
    }
  } while ( _liCounter[_iDataReadIndex] == _invalid ); // if current data counter is invalid, keep moving forward
  
  //if ( _iDataReadIndex != -1 || bDataWriteAbnormal ) // !! debug
  //{
  //  printf("EvrL1Data::finishDataRead(): New Read  Index = %d\n", _iDataReadIndex );// !! debug
  //  bDataWriteAbnormal = false;
  //}
}

void EvrL1Data::finishDataWrite() // move on to the next data position for data write
{    
  LockData lockData; // lock data indexes
  
  //if ( _iDataReadIndex != -1 )// !! debug
  //{
  //  printf("EvrL1Data::finishDataWrite(): Old Read  Index = %d\n", _iDataReadIndex );// !! debug
  //  printf("EvrL1Data::finishDataWrite(): Old Write Index = %d\n", _iDataWriteIndex );// !! debug
  //  bDataWriteAbnormal = true;
  //}
  
  if ( _iDataReadIndex == -1 ) // No previously buffered data -> set read index to updated data
    _iDataReadIndex  = _iDataWriteIndex;
  _iDataWriteIndex = ((_iDataWriteIndex+1) % _iNumBuffers);
}  

/*
 * find and get data with counter
 */
int EvrL1Data::findDataWithCounter(const ClockTime& iCounter) 
{
  for ( int iDataIndex = 0; iDataIndex < _iNumBuffers; iDataIndex++ )
    if ( _liCounter[iDataIndex] == iCounter )
      return iDataIndex;
      
  return -1;
}

EvrDataUtil& EvrL1Data::getDataWithIndex(int iDataIndex)
{
  EvrDataUtil& data = *(EvrDataUtil*)
    ( _lDataCircBuffer + iDataIndex * EvrDataUtil::size( _iMaxNumFifoEvent ) );      
  return data;
}

void EvrL1Data::markDataAsInvalid(int iDataIndex)
{
  _liCounter[iDataIndex] = _invalid; // Mark the counter as invalid
}

pthread_mutex_t EvrL1Data::LockData::_mutexData = PTHREAD_MUTEX_INITIALIZER;    

} // namespace Pds
