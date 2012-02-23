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
    pthread_cond_t _condition;
    pthread_mutex_t _mutex;

  public:
    SafeQueue() :
      _stop(false) {
      pthread_cond_init(&_condition, NULL);
      pthread_mutex_init(&_mutex, NULL);
    }

    ~SafeQueue() {
      pthread_mutex_lock(&_mutex);
      _stop = true;
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
      pthread_cond_broadcast(&_condition);
      pthread_mutex_unlock(&_mutex);
    }

    void unblock() {
      pthread_mutex_lock(&_mutex);
      _stop = true;
      pthread_cond_broadcast(&_condition);
      pthread_mutex_unlock(&_mutex);
    }  

    void push(T item) {
      pthread_mutex_lock(&_mutex);
      _queue.push(item);
      pthread_cond_signal(&_condition);
      pthread_mutex_unlock(&_mutex);
    }

    T pop() {
      pthread_mutex_lock(&_mutex);
      for (;;) {
        if (_stop) {
          pthread_cond_signal(&_condition);
          pthread_mutex_unlock(&_mutex);
          return NULL;
        }
        if (! _queue.empty()) {
          break;
        }
        pthread_cond_wait(&_condition, &_mutex);
      }
      T item = _queue.front();
      _queue.pop();
      pthread_mutex_unlock(&_mutex);
      return item;
    }
  };
}

#endif
