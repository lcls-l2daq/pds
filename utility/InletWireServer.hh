#ifndef PDS_INLETWIRESERVER_HH
#define PDS_INLETWIRESERVER_HH

#include "InletWire.hh"
#include "pds/service/ServerManager.hh"
#include "pds/service/GenericPool.hh"
#include "pds/service/SelectDriver.hh"
//#include "StreamParams.hh"
#include "pds/service/EbBitMaskArray.hh"
#include "pds/service/Semaphore.hh"

namespace Pds {

class Datagram;
class Transition;
class Inlet;
class OutletWire;

class InletWireServer : public InletWire, public ServerManager {
public:
  InletWireServer(Inlet& inlet,
		  OutletWire& outlet,
		  int ipaddress,
		  int stream, 
		  int taskpriority,
		  const char* taskname,
		  unsigned timeout = 0);

  // Implements InletWire thread safely (through unblock)
  void connect();
  void disconnect();
  void post(const Transition&);
  void post(const Occurrence&);
  void post(const InDatagram&);

  void add_input   (Server*);
  void remove_input(Server*);
  void trim_input  (Server*);
  void flush_inputs();
  void flush_outputs();

  void add_output(const InletWireIns& iwi);
  void remove_output(const InletWireIns& iwi);
  void remove_outputs();
  void trim_output(const InletWireIns& iwi);

  // Must be reimplemented by those inlets which need to add and remove servers
  virtual Server* accept(Server*) = 0;
  virtual void remove(unsigned id) = 0;
  virtual void flush() = 0;

  //protected:
public:
  virtual ~InletWireServer();

private:
  // Implements the server in ServerManager
  virtual char* payload();
  virtual int commit(char* datagram, char* load, int size, const Ins& src); 
  virtual int handleError(int value);

  // Implements ServerManager
  virtual int processIo(Server* srv);
  virtual int processTmo();

  // Used internally, not really necessary
  void _add_input   (Server*);
  void _remove_input(Server*);

  void add_output(unsigned id, const Ins& rcvr);
  void remove_output(unsigned id);

  virtual void _flush_inputs();
  virtual void _flush_outputs();
public:
  Inlet& _inlet;
protected:
  OutletWire& _outlet;
  //  StreamParams::StreamType _stream;
  int _ipaddress;

private:
  EbBitMaskArray _outputs;
  char*          _payload;
  SelectDriver   _driver;
  Semaphore      _sem;
};
}
#endif

