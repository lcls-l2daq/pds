// $Id$

#ifndef _FCCD960REORDER_H
#define _FCCD960REORDER_H

void fccd960Initialize(uint16_t* chanMap, uint16_t* topBot);
void fccd960Reorder(const uint16_t* chanMap, const uint16_t* topBot, unsigned char* in, uint16_t* out);

#endif
