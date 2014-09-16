#include "pds/client/FrameTrim.hh"

#include "pdsdata/xtc/Xtc.hh"

using namespace Pds;

FrameTrim::FrameTrim(uint32_t*& pwrite, 
		     const Src& src) : 
  _pwrite(pwrite), _src(src) 
{
}

void FrameTrim::_write(const void* p, 
		       ssize_t sz) 
{
  const uint32_t* pread = (uint32_t*)p;
  if (_pwrite!=pread) {
    const uint32_t* const end = pread+(sz>>2);
    while(pread < end)
      *_pwrite++ = *pread++;
  }
  else
    _pwrite += sz>>2;
}

void FrameTrim::iterate(Xtc* root) 
{
  if (root->damage.value() & ( 1 << Damage::IncompleteContribution))
    return _write(root,root->extent);

  int remaining = root->sizeofPayload();
  Xtc* xtc     = (Xtc*)root->payload();

  uint32_t* pwrite = _pwrite;
  _write(root, sizeof(Xtc));
    
  while(remaining > 0) {
    unsigned extent = xtc->extent;
    process(xtc);
    remaining -= extent;
    xtc        = (Xtc*)((char*)xtc+extent);
  }

  reinterpret_cast<Xtc*>(pwrite)->extent = (_pwrite-pwrite)*sizeof(uint32_t);
}

void FrameTrim::process(Xtc* xtc) 
{
  switch(xtc->contains.id()) {
  case (TypeId::Id_Xtc):
    { FrameTrim iter(_pwrite,_src);
      iter.iterate(xtc);
      break; }
  case (TypeId::Id_Frame):
    if (xtc->src == _src)
      break;
  default :
    _write(xtc,xtc->extent);
    break;
  }
}
