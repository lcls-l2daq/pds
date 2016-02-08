#include "pds/ioc/IocControl.hh"
#include "pds/ioc/IocNode.hh"
#include "pds/ioc/IocConnection.hh"
#include "pds/ioc/IocHostCallback.hh"
#include "pds/utility/Occurrence.hh"
#include<unistd.h>
#include<string>
#include<fstream>  //for std::ifstream
#include<sstream>  //for std::istringstream
#include<iterator> //for std::istream_iterator
#include<iostream>
#include<vector>

//#define DBUG

using namespace Pds;

IocControl::IocControl() :
  _station(0),
  _expt_id(0),
  _recording(0),
  _initialized(0),
  _pool      (sizeof(UserMessage),4)
{
#ifdef DBUG
  printf("IocControl::ctor empty\n");
#endif
}

IocControl::IocControl(const char* offlinerc,
		       const char* instrument,
		       unsigned    station,
		       unsigned    expt_id,
                       const char* controlrc) :
  _instrument(instrument),
  _station   (station),
  _expt_id   (expt_id),
  _recording (0),
  _pool      (sizeof(UserMessage),4)
{
#ifdef DBUG
  printf("IocControl::ctor offlinerc %s  instrument %s  controlrc %s\n",
         offlinerc, instrument, controlrc);
#endif

    std::ifstream *in;
    std::string line, host;
    IocNode *curnode = NULL;

    /*
     * If we don't have a configuration file, we're not doing anything!
     */
    if (!controlrc) {
        _report_error("IocControl: no configuration file provided");
        return;
    }

    /*
     * Read in the logbook parameters.
     */
    in = new std::ifstream(offlinerc);
    if (!in) {
        _report_error("IocControl: Cannot open logbook credential file " +
                      (std::string) offlinerc);
        return;
    }
    while (getline(*in, line)) {
        size_t p = line.find("opr");
        if (p != std::string::npos && p>=3)
          line = line.replace(p-3,3,"dia");

        _offlinerc.push_back("logbook " + line + "\n");
    }
    delete in;

    /*
     * Read the control recorder configuration file.  All we really
     * want here are the host and camera lines.  Everything else is
     * GUI, BLD, or PV-related.
     */
    in = new std::ifstream(controlrc);
    if (!in) {
        _report_error("IocControl: Cannot open controls configuration file " +
                      (std::string) controlrc);
        return;
    }
    while (getline(*in, line)) {
        std::istringstream ss(line);
        std::istream_iterator<std::string> begin(ss), end;
        std::vector<std::string> arrayTokens(begin, end);

        if (arrayTokens.size() == 0)
            continue;
        if (arrayTokens[0] == "host" && arrayTokens.size() >= 2) {
            host = arrayTokens[1];
        } else if (arrayTokens[0] == "camera" && arrayTokens.size() >= 5) {
            /* Skip the cameras that want to be recorded as BLD. */
          if (arrayTokens[2].compare(0, 4, "BLD-")) {
#ifdef DBUG
            printf("IocControl::ctor adding IocNode host: %s  line: %s\n",
                   host.c_str(), line.c_str());
#endif
            curnode = new IocNode(host, line, arrayTokens[1], arrayTokens[2],
                                  arrayTokens[3], arrayTokens[4],
                                  arrayTokens.size() >= 6 ? arrayTokens[5]
                                                          : (std::string)"");
            _nodes.push_back(curnode);
          }
        } else if (arrayTokens[0] == "pv" && arrayTokens.size() >= 3 && curnode != NULL) {
            curnode->addPV(arrayTokens[1], line);
        }
    }
    delete in;
}

IocControl::~IocControl()
{
    IocConnection::clear_all();
    _nodes.clear();
    _selected_nodes.clear();
}

void IocControl::write_config(IocConnection *c, unsigned run, unsigned stream)
{
    char buf[1024];

    c->transmit("hostname " + c->host() + "\n");
    sprintf(buf, "dbinfo daq %d %d %d\n", _expt_id, run, stream);
    c->transmit(buf);
    for(std::list<std::string>::iterator it=_offlinerc.begin();
        it!=_offlinerc.end(); it++)
        c->transmit(*it);
    sprintf(buf, "output daq/xtc/e%d-r%04d-s%02d\n", _expt_id, run, stream);
    c->transmit(buf);
    c->transmit("quiet\n");
}

void IocControl::host_rollcall(IocHostCallback* cb)
{
  if (cb) {
    for(std::list<IocNode*>::iterator it=_nodes.begin();
	it!=_nodes.end(); it++)
      cb->connected(*(*it));
  }
}

void IocControl::set_partition(const std::list<DetInfo>& iocs)
{
  _selected_nodes.clear();
  for(std::list<DetInfo>::const_iterator it=iocs.begin();
      it!=iocs.end(); it++)
    for(std::list<IocNode*>::iterator nit=_nodes.begin();
	nit!=_nodes.end(); nit++)
      if (it->phy() == (*nit)->src().phy())
	_selected_nodes.push_back(*nit);
}

InDatagram* IocControl::events(InDatagram* dg)
{
    printf("events: _initialized = %d\n", _initialized);
    if (!dg->seq.isEvent() && dg->seq.service() == TransitionId::BeginRun) {
        printf("events(BeginRun): _initialized = %d\n", _initialized);
        if (!_initialized)
            dg->xtc.damage = Damage::Uninitialized;
    }
    return dg;
}

Transition* IocControl::transitions(Transition* tr)
{
  char trans[1024];
#ifdef DBUG
  printf("IocControl::transitions %s %d.%09d\n",
         TransitionId::name(tr->id()), 
         tr->sequence().clock().seconds(),
         tr->sequence().clock().nanoseconds());
#endif  
  switch(tr->id()) {
  case TransitionId::Configure:
      sprintf(_trans, "trans %d %d %d %d\n",
              TransitionId::Configure, tr->sequence().clock().seconds(), 
              tr->sequence().clock().nanoseconds(), tr->sequence().stamp().fiducials());
      break;
  case TransitionId::BeginRun: 
      if (tr->size() != sizeof(Transition)) {

          /* Make connections to the world. */
          for(std::list<IocNode*>::iterator it=_selected_nodes.begin();
              it!=_selected_nodes.end(); it++) {
              (*it)->get_connection(this);
          }

          _initialized = IocConnection::check_all();
          printf("transitions: _initialized = %d\n", _initialized);

          unsigned run = tr->env().value();
          unsigned stream = 80;

          /* Send the per connection configuration information.         */
          /* Most important, the filename containing the stream number! */
          for(std::list<IocConnection*>::iterator it=IocConnection::_connections.begin();
              it!=IocConnection::_connections.end(); it++) {
              write_config(*it, run, stream);
              stream++;
          }

          /* Now send the node configuration for each node. */
          for(std::list<IocNode*>::iterator it=_selected_nodes.begin();
              it!=_selected_nodes.end(); it++) {
              IocConnection *c = (*it)->get_connection(this);
              (*it)->write_config(c);
          }

          /* Finally, send the saved Configure transition. */
          for(std::list<IocConnection*>::iterator it=IocConnection::_connections.begin();
              it!=IocConnection::_connections.end(); it++) {
              (*it)->transmit(_trans);
          }
          _recording = 1;
      } else {
          break;
      }
      /* FALL THROUGH!!! */
  case TransitionId::BeginCalibCycle:
  case TransitionId::Enable:
  case TransitionId::Disable:
  case TransitionId::EndCalibCycle:
      if (!_recording)
          break;
      sprintf(trans, "trans %d %d %d %d\n",
              tr->id(), tr->sequence().clock().seconds(), 
              tr->sequence().clock().nanoseconds(), tr->sequence().stamp().fiducials());
      IocConnection::transmit_all(trans);
      break;
  case TransitionId::EndRun:
      if (!_recording)
          break;
      /* Pass along the transition, and then tear down all of the connections. */
      sprintf(trans, "trans %d %d %d %d\n",
              tr->id(), tr->sequence().clock().seconds(), 
              tr->sequence().clock().nanoseconds(), tr->sequence().stamp().fiducials());
      IocConnection::transmit_all(trans);
      IocConnection::clear_all();
      for(std::list<IocNode*>::iterator it=_selected_nodes.begin();
          it!=_selected_nodes.end(); it++) {
          (*it)->clear_conn();
      }
      _recording = 0;
      break;
  default:
      break;
  }
  return tr;
}

void IocControl::_report_error(const std::string& msg)
{
  printf("%s\n",msg.c_str());
  //  UserMessage* usr = new(&_pool) UserMessage(msg.c_str());
  //  post(usr);
}
