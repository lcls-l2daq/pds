#ifndef Pds_XtcClientSql_hh
#define Pds_XtcClientSql_hh

#include "pds/config/XtcClient.hh"

#include <string>

namespace Pds_ConfigDb {
  namespace Sql {
    class DbClient;
    class XtcClient : public Pds_ConfigDb::XtcClient {
    private:
      XtcClient(const char*);
    public:
      ~XtcClient();
    public:
      static XtcClient* open ( const char* path );
    public:
      int       getXTC( unsigned           key,
                        const Pds::Src&    src,
                        const Pds::TypeId& type_id,
                        void*              dst,
                        unsigned           maxSize);
    private:
      DbClient* _db;
    };
  };
};

#endif
