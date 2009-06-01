#ifndef PDS_CONTROLCALLBACK_HH
#define PDS_CONTROLCALLBACK_HH

namespace Pds {
  
  class Node;
  class SetOfStreams;
  
  class ControlCallback {
  public:
    enum Reason {PlatformUnavailable, 
		 CratesUnavailable, 
		 FcpmUnavailable,
		 Reasons};

    virtual ~ControlCallback() {}

    virtual void attached(SetOfStreams& streams) = 0;
    virtual void failed  (Reason reason) = 0;
    //    virtual void ready(const Partition* partition) = 0;
    //    virtual void notready(const Partition* partition, 
    //			  const PartitionException* exception) = 0;
    virtual void dissolved(const Node& who) = 0;
  };

};

#endif
