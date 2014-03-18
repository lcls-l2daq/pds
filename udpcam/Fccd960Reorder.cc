// $Id$

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "Fccd960Reorder.hh"

void fccd960Initialize(uint16_t* mapCol, uint16_t* mapCric, uint16_t* mapAddr, uint16_t* chanMap, uint16_t* topBot) {

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
    //    printf("%d\n", i);
    chanMap[mapCol[i]] = 16*(6-(int)mapCric[i])+mapAddr[i];
    //    printf("chanMap 0: %d, %d, %d\n",i, mapCol[i], chanMap[mapCol[i]]); 
    chanMap[mapCol[i]+4] = 16*(9-(int)mapCric[i])+mapAddr[i];
    //    printf("chanMap 4: %d, %d, %d\n",i+4, mapCol[i]+4, chanMap[mapCol[i]+4]); 
    chanMap[mapCol[i]+8] = chanMap[mapCol[i]];
    chanMap[mapCol[i]+12] = chanMap[mapCol[i]+4];
    //    printf("chanMap 12: %d, %d, %d\n",i+12, mapCol[i]+12, chanMap[mapCol[i]+12]); 
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

void fccd960Reorder(const uint16_t* mapCol, const uint16_t* mapCric, const uint16_t* mapAddr, const uint16_t* chanMap, const uint16_t* topBot, unsigned char* buffer, uint16_t* data) {

  memset(data, 0, sizeof(uint16_t)*960*960);

  const unsigned OverScan = 0;
  const unsigned CCDcols = 96;
  // since the pipeline is not flushed, we read one more row than is displayed
  const unsigned CCDreg = OverScan+10;
  const unsigned CCDsizeX = CCDcols*CCDreg;
  const unsigned CCDsizeY = 480;//random guess
  const unsigned YTOT = 2*CCDsizeY;

  unsigned i;

#if 0
  FILE *dataFile = fopen(fileName, "r");
  
  unsigned readLength = 2*192*(1+CCDsizeY)*CCDreg;
  unsigned char buffer[readLength];
  memset(buffer, 0, sizeof(unsigned char)*readLength);
  size_t bytes_read = 0;
  bytes_read = fread(buffer, sizeof(unsigned char), readLength, dataFile);
  if (bytes_read != readLength) {
    printf("tried to read %d bytes, got %d\n", readLength, bytes_read);
    // throw exception?
  }
#endif
  unsigned ktr = 0;
  // the first 7 converts are from the stale pipeline data. Simply skip over them.
  //  (ktr is bytes)
  ktr = 7*192*2;
  unsigned pVal = 0;

  unsigned y, x;
  for(y=0; y<CCDsizeY; y++){
    for(x=0; x<CCDreg; x++){
      for(i=0; i<192;i++){
	//	printf("%d, %d, %d, %d\n", y, x, i, ktr);
	unsigned fH = buffer[ktr];//charCodeAt(s,ktr);
	unsigned fL = buffer[ktr+1];//charCodeAt(s,ktr+1);
	//	printf("got to here, %d %d\n", fH, fL);
	// deal with the gain bits.
	// Precise calibration requires both a gain and an offset
	// Gain = 00 is 4096 - epsilon to 8192 (pedestal = 4096 - epsilon)
	// Gain = 01 is (modulo offset and exact gain) 8192 + 4*(value - 4096)
	// Gain = 10 is (modulo ..) 8192 + 4*4096 + 8*(value - 4096)
#if 0
	if((fH & 0xC0) != 0){
	  pVal = 4096 + 8 * (fL + ((fH & 0x0F) << 8));
	}
	else if((fH & 0x40) != 0){
	  pVal = 4096 + 4 * (fL + ((fH & 0x0F) << 8));
	}
	else {
	  pVal = fL + (fH << 8);
	}
#else
  pVal = fL + (fH << 8);
#endif
	//	printf("pre xdex: %d, %d, %d\n", i, chanMap[i], CCDreg);
	long xdex = chanMap[i]+(CCDreg - x - 1);
	//	printf("xdex: %d, %d, %d, %d\n", xdex, i, chanMap[i], CCDreg);
	if(topBot[i] == 0){
	  unsigned long idex = CCDsizeX * y + xdex;
	  //	  printf("got to here......0: %d, %d, %d, %d\n", idex, CCDsizeX, y, xdex);
	  data[idex] = pVal;
	}
	else {
	  //	  printf("got to here......1\n");
	  unsigned long idex = CCDsizeX * (YTOT-y-1) + (CCDsizeX-xdex-1);
	  data[idex] = pVal;
	}
	ktr=ktr+2;
      }
    }
  }
}
