/*
 * Processor.cc
 *
 *  Created on: Mar 22, 2011
 *      Author: jackp
 */

#include <omp.h>
#include "pds/fexamp/Processor.hh"

namespace Pds {

  namespace Fexamp {

    Processor::~Processor() {}

//    FexampInFrame Processor::_testData[numberOfFrames];
//    FexampOutFrame Processor::_destData[numberOfFrames];

    void Processor::routine(void) {
//      unsigned ioIndex = _ioIndex;
//      omp_set_num_threads(_numbThreads);
//#pragma omp parallel for default(none)   shared(ioIndex)
//      for (int b=0; b<numberOfInputBlocks; b++) {
////      shared(b, _destData, _testData, ioIndex) //schedule(guided)
//        for (int j=0; j<numberOfRows; j++) {
//          //          printf("Thread %d, row %d\n", tid, j);
//          uint32_t* outputp =  &_destData[ioIndex].blocks[b].rows[j].columns[0];
//          unsigned counter = 0;
//          for (int g=0; g<numberOfGroups; g++) {
//            for (int k=0; k<numbEntriesOutputGroup; k++) {
//              uint32_t* inputp = (uint32_t*)&_testData[ioIndex].blocks[b].rows[j].groups[g].inputWords[k];
//              outputp[((counter&0x1f)<<4)+counter>>5] = (*inputp >> (k<<1)) & 0x3ffff;
//              counter += 1;
//            }
//          }
//        }
//      }
//      delete this;
    }
  }

}
