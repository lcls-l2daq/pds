// $Id$

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "Fccd960Reorder.hh"

void fccd960Initialize(uint16_t* chanMap, uint16_t* topBot) {
  uint16_t mapCol[48];
  uint16_t mapCric[48];
  uint16_t mapAddr[48];
  memset(mapCol, 0, sizeof(uint16_t)*48);// these are unneded
  memset(mapCric, 0, sizeof(uint16_t)*48);
  memset(mapAddr, 0, sizeof(uint16_t)*48);

  unsigned i;
  for (i=0; i<16; i++) {
    mapCol[i] = (129+i*16)%193;
    mapCric[i] = 4;
    mapAddr[i]= i%16;
  }

  for (i=16; i<32; i++) {
    mapCol[i] = (66+(i%16)*16)%193;
    mapCric[i] = 5;
    mapAddr[i]= i%16;
  }

  for (i=32; i<48; i++) {
    mapCol[i] = (3+(i%16)*16)%193;
    mapCric[i] = 6;
    mapAddr[i]= i%16;
  }

  for (i=0; i<48; i++){
    chanMap[mapCol[i]] = 16*(6-(int)mapCric[i]) + (int)mapAddr[i];
    chanMap[mapCol[i]+4] = 16*(9-(int)mapCric[i]) + (int)mapAddr[i];
    chanMap[mapCol[i]+8] = chanMap[mapCol[i]];
    chanMap[mapCol[i]+12] = chanMap[mapCol[i]+4];
    topBot[mapCol[i]] = 0; 
    topBot[mapCol[i]+4] = 0;
    topBot[mapCol[i]+8] = 1;
    topBot[mapCol[i]+12] = 1;
  }

  const unsigned OverScan = 0; // should be in the class...
  const unsigned CCDreg = OverScan+10;
  for (i=0; i<192; i++) chanMap[i] = chanMap[i]*CCDreg;

  //  printf("done with init\n");
}

void fccd960Reorder(const uint16_t* chanMap, const uint16_t* topBot, unsigned char* buffer, uint16_t* data) {

  const unsigned OverScan = 0;
  const unsigned CCDcols = 96;
  // since the pipeline is not flushed, we read one more row than is displayed
  const unsigned CCDreg = OverScan+10;
  const unsigned CCDsizeX = CCDcols*CCDreg;
  const unsigned CCDsizeY = 480;//random guess
  const unsigned YTOT = 2*CCDsizeY;

  unsigned i;

  unsigned ktr = 0;
  // the first 7 converts are from the stale pipeline data. Simply skip over them.
  //  (ktr is bytes)
  ktr = 7*192*2;
  unsigned pVal = 0;

  unsigned y, x;
  for(y=0; y<CCDsizeY; y++){
    for(x=0; x<CCDreg; x++){
      for(i=0; i<192;i++){
        //      printf("%d, %d, %d, %d\n", y, x, i, ktr);
        unsigned fH = buffer[ktr];//charCodeAt(s,ktr);
        unsigned fL = buffer[ktr+1];//charCodeAt(s,ktr+1);
        //      printf("got to here, %d %d\n", fH, fL);
        pVal = fL + (fH << 8);
        //      printf("pre xdex: %d, %d, %d\n", i, chanMap[i], CCDreg);
        long xdex = chanMap[i]+(CCDreg - x - 1);
        //      printf("xdex: %d, %d, %d, %d\n", xdex, i, chanMap[i], CCDreg);
        if(topBot[i] == 0){
          unsigned long idex = CCDsizeX * y + xdex;
          //      printf("got to here......0: %d, %d, %d, %d\n", idex, CCDsizeX, y, xdex);
          data[idex] = pVal;
        }
        else {
          //      printf("got to here......1\n");
          unsigned long idex = CCDsizeX * (YTOT-y-1) + (CCDsizeX-xdex-1);
          data[idex] = pVal;
        }
        ktr=ktr+2;
      }
    }
  }
}
