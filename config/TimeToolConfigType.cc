#include "pds/config/TimeToolConfigType.hh"

using namespace Pds::TimeTool;

static const char* op_name[] = { "OR", "AND", "OR_NOT", "AND_NOT", NULL };

static void dump_logic(const ndarray<const EventLogic,1>& logic)
{
  for(unsigned i=0; i<logic.size(); i++)
    printf("%s %u", op_name[logic[i].logic_op()], logic[i].event_code());
  printf("\n");
}

void Pds::TimeTool::dump(const TimeToolConfigType& c)
{
  printf("--beam logic--\n");
  dump_logic(c.beam_logic());
  printf("--laser logic--\n");
  dump_logic(c.laser_logic());
}
