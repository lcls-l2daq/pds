#include "pds/mon/MonEntryFactory.hh"
#include "pds/mon/MonEntryScalar.hh"
#include "pds/mon/MonEntryTH1F.hh"
#include "pds/mon/MonEntryTH2F.hh"
#include "pds/mon/MonEntryProf.hh"
#include "pds/mon/MonEntryImage.hh"
#include "pds/mon/MonEntryWaveform.hh"

using namespace Pds;

MonEntry* MonEntryFactory::entry(const MonDescEntry& desc)
{
  MonEntry* entry = 0;
  switch (desc.type()) {
  case MonDescEntry::Scalar:
    entry = new MonEntryScalar((const MonDescScalar&)desc);
    break;
  case MonDescEntry::TH1F:
    entry = new MonEntryTH1F((const MonDescTH1F&)desc);
    break;
  case MonDescEntry::TH2F:
    entry = new MonEntryTH2F((const MonDescTH2F&)desc);
    break;
  case MonDescEntry::Prof:
    entry = new MonEntryProf((const MonDescProf&)desc);
    break;
  case MonDescImage::Image:
    entry = new MonEntryImage((const MonDescImage&)desc);
    break;
  case MonDescWaveform::Waveform:
    entry = new MonEntryWaveform((const MonDescWaveform&)desc);
    break;
  }
  return entry;
}
