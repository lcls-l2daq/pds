#ifndef _EventSequencerSim_HH

#include <pthread.h>
#include <string>
#include <vector>
#include <map>

class EventSequencerSim {
public:
  static const int iSocketPort = 10200;
  EventSequencerSim();
  ~EventSequencerSim();

  /*
   * PV server
   */
  int accept();
  int process(int fClient);

private:
  bool              _bStopThreads;

  int startPvServer();
  int initPvMap();

  int               _fSocket;
  pthread_t         _threadAccept;
  int               _numThreadProc;
  pthread_mutex_t   _mutexUpdate;

  /*
   * PV control
   */

  struct  TPvValue {
    enum TPvType { TYPE_INT, TYPE_STRING, TYPE_INTARRAY }
                        eType;
    union
    {
    int                 iVal;
    std::string*        pStr;
    std::vector<int>*   pVecInt;
    };
    TPvValue(int iVal);
    TPvValue(const char* sVal);
    TPvValue(const std::string& sVal);
    TPvValue(int iCount, int iVal);
    TPvValue(const std::vector<int>& vVal);
    TPvValue(const TPvValue& ref);
    ~TPvValue();

    TPvValue& swap(TPvValue& ref);
    TPvValue& operator=(TPvValue ref);

    std::string to_string();
  };
  typedef     std::map<std::string, TPvValue> TPvMap;
  TPvMap      _mapPv;

  static const unsigned BUFFER_SIZE = 32768;
  char* _cmdBuffer;
  char* _outBuffer;

  pthread_rwlock_t  _rwlockPv;

  int setPv (const std::string& sPv, const std::vector<std::string>& lsParam, int iIndexStart, std::string& sVal);
  int getPv (const std::string& sPv, std::string& sVal);
  int listPv(const std::string& sPvMatch, std::vector<std::string>& lsPv);
  int notifySequencer(const std::string& sPv);

  int setPvAsync(const std::string& sPv, int  iVal);
  int setPvAsync(const std::string& sPv, const std::string& sVal);
  int getPvAsync(const std::string& sPv, int& iVal);
  int getPvAsync(const std::string& sPv, std::string*& ppStr);
  int getPvAsync(const std::string& sPv, std::vector<int>*& ppliVal);

  /*
   * Event sequencer control
   */
public:
  int seqUpdate();

  static const int numSequencer = 8;
private:
  int startSeqUpdater();

  pthread_t  _threadSeqUpdate;

  struct TEventSequencer {
    TEventSequencer();

    // iStart: 0 stop seqeuence  1 start (may repeat)   2 finish current round and goto next round
    int   play        (int iStart, unsigned fiducial);
    int   loadSeq     (int len, std::vector<int>& vA, std::vector<int>& vB, std::vector<int>& vC, std::vector<int>& vD);
    int   getSeqEvent (unsigned fiducial, std::vector<int>& lEvents);
    int   computeDelay(unsigned fiducial, int iStep);

    int       iSeqId;
    unsigned  uCurStepFid;

    /*
     *  output to pv
     */
    int   iPlayStatus; // 0 Stop  1 Playing  2 Wait to play
    int   iCurStep;
    int   iPlayCount;
    int   iTotalPlayCount;
    int   iErrType;
    int   iErrIndex;

    /*
     * input from pv
     */
    float fBeamRate;
    int   iPlayMode;
    int   iRepeatCount;
    int   iSyncMarker;
    int   iNextSync;
    int   iBeamPulseReq;

    struct TEventStep {
      int event;
      int dbeam;
      int dfidu;
      int burst;
    };
    std::vector<TEventStep> eventSeq;

    // need to synchronize to pv
    bool  bSeqStatusUpdated;

    // need to synchronize from pv input
    bool  bPvUpdated;
  };
  TEventSequencer _lSeq[numSequencer];

  int updatePvFromSeq(TEventSequencer& seq);
  int updateSeqFromPv(TEventSequencer& seq);

  /*
   * Event sequence output
   */
public:
  int getCodeDelay(int iEvent);
  int getSeqEvent (unsigned fiducial, int& nSeqEvents, int*& lSeqEvents);

private:
  unsigned          _curFiducial;
  std::vector<int>  _lSeqEvents;
};

#endif // #ifndef _EventSequencerSim_HH
