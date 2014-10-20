#ifndef PDS_INLETWIRE_HH
#define PDS_INLETWIRE_HH

namespace Pds {

class Server;
class Transition;
class Occurrence;
class InDatagram;
class InletWireIns;
class Task;

class InletWire {
public:
  InletWire(int taskpriority, const char* taskname);
  virtual ~InletWire();

  Task& task();

  virtual void connect() = 0;
  virtual void disconnect() = 0;
  virtual void post(const Transition& tr) = 0;
  virtual void post(const Occurrence& tr) = 0;
  virtual void post(const InDatagram& dg) = 0;
  virtual void add_input   (Server*) = 0;
  virtual void add_input_nonblocking(Server* srv) = 0;
  virtual void remove_input(Server*) = 0;
  virtual void trim_input  (Server*) = 0;
  virtual void add_output(const InletWireIns& iwi) = 0;
  virtual void remove_output(const InletWireIns& iwi) = 0;
  virtual void trim_output(const InletWireIns& iwi) = 0;
  virtual void flush_inputs() = 0;
  virtual void flush_outputs() = 0;
  //  virtual unsigned short port(unsigned id) const = 0;

  // Debugging
  virtual void dump(int detail) {}
  virtual void dumpCounters(int detail) {}
  virtual void resetCounters() {}
  virtual void dumpHistograms(unsigned tag, const char* path) {}
  virtual void resetHistograms() {}

private:
  Task* _task;
};
}

#endif
