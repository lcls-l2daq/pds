#include "pds/vmon/VmonRecord.hh"

#include "pds/mon/MonClient.hh"
#include "pds/mon/MonCds.hh"
#include "pds/mon/MonEntryTH1F.hh"
#include "pds/mon/MonEntryTH2F.hh"
#include "pds/mon/MonGroup.hh"

#include <string.h>
#include <new>

using namespace Pds;

VmonRecord::VmonRecord(VmonRecord::Type type,
		       const ClockTime& time,
		       unsigned len) :
  _type(type),
  _time(time),
  _len (len)
{
}

VmonRecord::~VmonRecord()
{}

int VmonRecord::len() const
{
  return _len;
}

const ClockTime& VmonRecord::time() const
{
  return _time;
}

void VmonRecord::time(const ClockTime& t)
{
  _time = t;
}

int VmonRecord::append(const MonClient& client)
{
  int rval(0);

  const MonCds& cds = client.cds();
  char* where = (char*)this + _len;
  *new (where) Src(client.src()); where += sizeof(Src);
  //  record the description
#define copy_bytes(from, len) { memcpy(where, from, len); where += len; }
  copy_bytes(&cds.desc(), sizeof(MonDesc));
  for (unsigned short g=0; g<cds.ngroups(); g++) {
    const MonGroup* gr = cds.group(g);
    copy_bytes(&gr->desc(), sizeof(MonDesc));
    for (unsigned short e=0; e<gr->nentries(); e++) {
      const MonEntry* entry = gr->entry(e); 
      copy_bytes(&entry->desc(), entry->desc().size());
      switch(entry->desc().type()) {
      case MonDescEntry::TH1F: rval += sizeof(MonStats1D); break;
      case MonDescEntry::TH2F: rval += sizeof(MonStats2D); break;
      default: break;
      }
    }
  }
#undef copy_bytes
  _len = where - (char*)this;
  return rval;
}

void VmonRecord::append(const MonClient& client, int offset)
{
  char* where = (char*)(this+1) + offset;
  //  record the payload
  const MonCds& cds = client.cds();
  for(unsigned short g=0; g<cds.ngroups(); g++) {
    const MonGroup&  gr = *cds.group(g);
    for(unsigned short e=0; e<gr.nentries(); e++) {
      MonEntry& en  = const_cast<MonEntry&>(* gr.entry(e));
      switch(en.desc().type()) {
      case MonDescEntry::TH1F:
	{ MonEntryTH1F& h = dynamic_cast<MonEntryTH1F&>(en);
	  h.stats();
	  *new (where) MonStats1D(h);
	  where += sizeof(MonStats1D); }
	break;
      case MonDescEntry::TH2F:
	{ MonEntryTH2F& h = dynamic_cast<MonEntryTH2F&>(en);
  	  h.stats();
	  *new (where) MonStats2D(h);
	  where += sizeof(MonStats2D); }
	break;
      default:
	break;
      }
    }
  }
}

// process the header
int  VmonRecord::extract(vector<Src    >& src_vector, 
			 vector<MonCds*>& cds_vector, 
			 vector<int*   >& offset_vector) 
{
  int offset=sizeof(*this);

  char* where = (char*)(this+1);
  char* const end = (char*)this + _len;
  while( where < end ) {
    src_vector.push_back(*reinterpret_cast<const Src*>(where));
    where += sizeof(Src);
    int offsets[64];
    int n=0;
    //  record the description
    const MonDesc& cds_d = *reinterpret_cast<const MonDesc*>(where); 
    where += sizeof(MonDesc);
    MonCds* cds = new MonCds( cds_d.name() );
    cds_vector.push_back(cds);
    for (unsigned short g=0; g<cds_d.nentries(); g++) {
      const MonDesc& gr_d = *reinterpret_cast<const MonDesc*>(where);
      where += sizeof(MonDesc);
      MonGroup* gr = new MonGroup(gr_d.name());
      cds->add(gr);
      for (unsigned short e=0; e<gr_d.nentries(); e++) {
	const MonDescEntry& en_d = *reinterpret_cast<const MonDescEntry*>(where);
	where += en_d.size();
	switch(en_d.type()) {
	case MonDescEntry::TH1F: 
	  { const MonDescTH1F& d = static_cast<const MonDescTH1F&>(en_d);
	    gr->add( new MonEntryTH1F(d) );
	    offsets[n++] = offset;
	    offset += sizeof(MonStats1D); 
	  }
	  break;
	case MonDescEntry::TH2F: 
	  { const MonDescTH2F& d = static_cast<const MonDescTH2F&>(en_d);
	    gr->add( new MonEntryTH2F(d) );
	    offsets[n++] = offset;
	    offset += sizeof(MonStats2D); 
	  }	  
	default: break;
	}
      }
    }
    int* o = new int[n];
    memcpy(o, offsets, n*sizeof(int));
    offset_vector.push_back(o);
  }

  return offset;
}
