#include "pds/client/XtcStripper.hh"

#include "pdsdata/xtc/Dgram.hh"

#include <stdio.h>

using namespace Pds;

//
//  Need an iterator that is safe for backwards copy
//
XtcStripper::XtcStripper(Xtc* root, uint32_t*& pwrite) : 
  _root  (root), 
  _pwrite(pwrite) 
{
}

XtcStripper::~XtcStripper() {}

uint32_t* XtcStripper::_write(const void* p, ssize_t sz) 
{
  uint32_t* pwrite = _pwrite;
  const uint32_t* pread = (uint32_t*)p;
  if (_pwrite!=pread) {
    const uint32_t* const end = pread+(sz>>2);
    while(pread < end)
      *_pwrite++ = *pread++;
  }
  else
    _pwrite += sz>>2;
  return pwrite;
}

void XtcStripper::iterate() { iterate(_root); }

void XtcStripper::iterate(Xtc* root) 
{
  if (root->damage.value() & ( 1 << Damage::IncompleteContribution)) {
    _write(root,root->extent);
    return;
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
        abort();
	break; // try to skip corrupt event
      }
      process(xtc);
      remaining -= extent;
      xtc        = (Xtc*)((char*)xtc+extent);
    }

  reinterpret_cast<Xtc*>(pwrite)->extent = (_pwrite-pwrite)*sizeof(uint32_t);
}

void XtcStripper::process(Xtc* xtc) 
{
  _write(xtc,xtc->extent);
}
