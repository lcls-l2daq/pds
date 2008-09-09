#ifndef PDS_LISTENTRY
#define PDS_LISTENTRY

#include "ListPool.hh"

namespace Pds {

class ListEntry
{
        public:
                ListEntry(void*);
                ListEntry(void*, unsigned tag);

                ~ListEntry() {}
        private:
                friend class ListPool;
                private:
                void *_list;         //point to itself ?
                unsigned _reserved;  // used to maintain sizeof allocation
                unsigned _tag;       // used to maintain quadword alignment and
                                     // to indicate to what type of heap the entry belongs
                                     // 0 = regular heap
                                     // 1 = resource wait heap
                                     // anything else = generic pool
                ListEntry *_flink;
                ListEntry *_blink;
};

}

inline Pds::ListEntry::ListEntry(void* list) :
        _list(list),
        _tag(0),
        _flink((ListEntry *)0),
        _blink((ListEntry *)0)
        {
        }


inline Pds::ListEntry::ListEntry(void* list, unsigned tag) :
        _list(list),
        _tag(tag),
        _flink((ListEntry *)0),
        _blink((ListEntry *)0)
        {
        }

#endif
