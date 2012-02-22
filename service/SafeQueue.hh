#ifndef PDS_SAFEQUEUE_HH
#define PDS_SAFEQUEUE_HH

#include <queue>
#include <semaphore.h>

namespace Pds {

  template <typename T, bool isArrayType>
  class SafeQueue {
  private:
    bool _stop;
    std::queue<T> _queue;
    sem_t _sem;

  public:
    SafeQueue() :
      _stop(false) {
      sem_init(&_sem, 0, 0);
    }

    ~SafeQueue() {
      sem_destroy(&_sem);
      while(!_queue.empty()) {
        T item = _queue.front();
        if (item != NULL) {
          if (isArrayType) {
            delete[] item;
          } else {
            delete item;
          }
        }
        _queue.pop();
      }
    }

    void unblock() {
      _stop = true;
      sem_post(&_sem);
    }  

    void push(T item) {
      _queue.push(item);
      sem_post(&_sem);
    }

    T pop() {
      while (sem_wait(&_sem)); // keep running sem_wait(), if it is interrupted by signal
      if (_stop) {
        return NULL;
      }
      T item = _queue.front();
      _queue.pop();
      return item;
    }
  };
}

#endif
