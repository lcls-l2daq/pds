#include "THist.hh"

#include "pds/mon/MonDescTH1F.hh"
#include "pds/mon/MonEntryTH1F.hh"

#include <stdio.h>

using namespace Pds;

THist::THist(const char* title) 
{
  MonDescTH1F desc(title,"time","entries",64,0.,.064);
  _entry = new MonEntryTH1F(desc);
}

THist::~THist() { delete _entry; }

void THist::accumulate(const timespec& curr,
                       const timespec& prev)
{
  double delta = 
    (double(curr.tv_sec)+double(curr.tv_nsec)*1.e-9) -
    (double(prev.tv_sec)+double(prev.tv_nsec)*1.e-9);
  _entry->addcontent(1.,delta);
}

void THist::print(unsigned count) const
{
  if ((count%12000)==0) {
  printf("%s\n",_entry->desc().name());
  for(unsigned i=0; i<64; i++)
    printf("%.8d%c",int(_entry->content(i)),(i%8)==7 ? '\n':' ');
  printf("%.8d:%.8d:%.8d\n",
         int(_entry->info(MonEntryTH1F::Underflow)),
         int(_entry->info(MonEntryTH1F::Overflow)),
         int(_entry->info(MonEntryTH1F::Normalization)));
  }
}
