#include "QuadWord.hh"

#define QW_U32_MAX ((uint32_t)(-1))

using namespace Pds;


QuadWord::QuadWord(uint32_t upper, uint32_t lower){

  _upperword = upper;
  _lowerword = lower;
}

QuadWord::QuadWord(){

  _upperword = 0;
  _lowerword = 0;
}

QuadWord::QuadWord(const QuadWord& oldwords){

  _upperword = oldwords.upperword();
  _lowerword = oldwords.lowerword();
}

QuadWord::~QuadWord(){

}

QuadWord QuadWord::operator*(uint32_t factor){

  uint32_t i,j;
  QuadWord temp;

  // do multiplication by doing shifts and adds.
  for (i=0;i<=(sizeof(uint32_t)*8 - 1);i++){
    j = 1 << i;
    if (factor & j) {
      temp = temp+(*this<<i);
    }
  }
  return temp;
}

QuadWord QuadWord::operator-(QuadWord subtract){
  uint32_t tempupperword, templowerword;

  // check for negative result, which isn't supported here.
  if ((_upperword < subtract.upperword()) ||
      ((_upperword == subtract.upperword()) && (_lowerword < subtract.lowerword()))){
    //    cout << "QuadWord subtraction result < 0 not supported" << endl;
    return *this;}

  if (_lowerword >= subtract.lowerword()){
    // we don't have to borrow
    templowerword = _lowerword-subtract.lowerword();
    tempupperword = _upperword-subtract.upperword();}
  else{
    // we have to borrow. get fancy: do subtraction in reverse
    // order, take complement, and add 1.
    templowerword = ~(subtract.lowerword()-_lowerword)+1;
    // subtract 1 here for the borrow;
    tempupperword = _upperword-subtract.upperword()-1;
  }

  return QuadWord(tempupperword,templowerword);
}

QuadWord QuadWord::operator/(uint16_t divisor){
  QuadWord temp;
  uint16_t remainder;
  temp = Divide(divisor, remainder);
  return temp;
}

uint16_t QuadWord::operator%(uint16_t divisor){
  QuadWord temp;
  uint16_t remainder;
  temp = Divide(divisor, remainder);
  return remainder;
}

QuadWord QuadWord::Divide(uint16_t divisor, uint16_t &remainder){

  // do the long division in units of "short"

  uint32_t piece;
  uint32_t tempupperword,templowerword;

  tempupperword = 0;
  templowerword = 0;

  // do the top 16 bits (48:63)
  piece = Extract(_upperword,16,31);
  tempupperword = Insert(tempupperword,piece/divisor,16,31);
  remainder = piece%divisor;

  // do the next 16 bits (32:47)
  piece = Extract(_upperword,0,15);
  piece = piece + (remainder*0x10000);
  tempupperword = Insert(tempupperword,piece/divisor,0,15);
  remainder = piece%divisor;

  // do the next 16 bits (16:31)
  piece = Extract(_lowerword,16,31);
  piece = piece + (remainder*0x10000);
  templowerword = Insert(templowerword,piece/divisor,16,31);
  remainder = piece%divisor;

  // do the lowest 16 bits (0:15)
  piece = Extract(_lowerword,0,15);
  piece = piece + (remainder*0x10000);
  templowerword = Insert(templowerword,piece/divisor,0,15);
  remainder = piece%divisor;

  return QuadWord(tempupperword,templowerword);
}

QuadWord QuadWord::operator<<(uint32_t factor){

  uint32_t carrybits;
  uint32_t tempupperword,templowerword;

  tempupperword = 0;
  templowerword = 0;

  if (factor == 0) {
    templowerword = _lowerword;
    tempupperword = _upperword;
    return QuadWord(tempupperword,templowerword);}
  if (factor == 32) {
    // the "<<32" doesn't seem to work, so do this case by hand.
    tempupperword = _lowerword;
    templowerword = 0;
    return QuadWord(tempupperword,templowerword);}
  if (factor > 32) {
    //    cout << "QuadWord left shift > 32 error!" << endl;
    return QuadWord(tempupperword,templowerword);}
  carrybits = Extract(_lowerword, 32 - factor, 31);
  templowerword = _lowerword << factor;
  tempupperword = _upperword << factor;
  tempupperword = tempupperword + carrybits;
  return QuadWord(tempupperword,templowerword);
}

QuadWord QuadWord::operator+(QuadWord factor){

  uint32_t tempupperword,templowerword;

  tempupperword = 0;
  templowerword = 0;

  if (_upperword > QW_U32_MAX - factor.upperword()){
    //    cout << "Error, 64 Bit overflow in addition." << endl;
    return QuadWord(tempupperword,templowerword);}

  tempupperword = _upperword + factor.upperword();

  if (_lowerword > QW_U32_MAX - factor.lowerword()) tempupperword++;

  templowerword = _lowerword + factor.lowerword();

  return QuadWord(tempupperword,templowerword);
}

