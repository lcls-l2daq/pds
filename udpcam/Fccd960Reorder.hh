// $Id$

#ifndef _FCCD960REORDER_H
#define _FCCD960REORDER_H

void fccd960Initialize(uint16_t* mapCol, uint16_t* mapCric, uint16_t* mapAddr, uint16_t* chanMap, uint16_t* topBot);
void fccd960Reorder(const uint16_t* mapCol, const uint16_t* mapCric, const uint16_t* mapAddr, const uint16_t* chanMap, const uint16_t* topBot, unsigned char* in, uint16_t* out);

#endif
