#include "pds/configsql/DbClient.hh"
#include "pds/configsql/QueryProcessor.hh"
#include "pds/config/PdsDefs.hh"

using Pds_ConfigDb::XtcEntry;
using Pds_ConfigDb::KeyEntry;
using Pds_ConfigDb::Key;
using Pds_ConfigDb::DeviceEntryRef;
using Pds_ConfigDb::DeviceEntries;
using Pds_ConfigDb::DeviceType;
using Pds_ConfigDb::ExptAlias;

#include <mysql/mysql.h>

#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>

using std::hex;
using std::setw;
using std::setfill;
using std::ostringstream;

#define USE_TABLE_LOCKS

using namespace Pds_ConfigDb::Sql;

static void _handle_no_lock(const char* s)
{
  std::string _no_lock_exception(s);
  printf("_handle_no_lock:%s\n",s);
  throw _no_lock_exception;
}

static std::string trunc(const std::string& name)
{
  unsigned len = name.size();
  if (len>32) return name.substr(0,32);
  return name;
}

//-------------
// Functions --
//-------------

/**
  * Parse the next line of a configuration file.
  *
  * The lines are expected to have the following format:
  *
  *   <key>=[<value>]
  *
  * The function will also check that the specified key is the one found
  * in the line.
  *
  * @return 'true' and set a value of the parameter, or 'false' otherwise
  */
static bool
parse_next_line (std::string& value, const char* key, std::ifstream& str)
{
    std::string line;
    if( str >> line ) {
        const size_t separator_pos = line.find( '=' );
        if( separator_pos != std::string::npos ) {
            const std::string key = line.substr( 0, separator_pos );
            if( key == line.substr( 0, separator_pos )) {
                value = line.substr( separator_pos + 1 );
                return true;
            }
        }
    }
    return false;
}

/**
  * Return a pointinr onto a C-style string or null pointer if the input string is empty
  */
static inline
const char*
string_or_null (const std::string& str)
{
    if( str.empty()) return 0 ;
    return str.c_str();
}

/**
  * Establish database connection for the specified parameters
  */
static MYSQL*
connect2server (const std::string& host,
                const std::string& user,
                const std::string& password,
                const std::string& db) throw (DatabaseError)
{
    // Prepare the connection object
    //
    MYSQL* mysql = 0;
    if( !(mysql = mysql_init( mysql )))
        throw DatabaseError( "error in mysql_init(): insufficient memory to allocate an object" );

    // Set trhose attributes which should be initialized before setting up
    // a connection.
    //
    ;

    // Connect now
    //
    if( !mysql_real_connect(
        mysql,
        string_or_null( host ),
        string_or_null( user ),
        string_or_null( password ),
        string_or_null( db ),
        0,  // connect to the default TCP port
        0,  // no defaulkt UNIX socket
        0   // no default client flag
        ))
        throw DatabaseError( std::string( "error in mysql_real_connect(): " ) + mysql_error(mysql));

    // Set connection attributes
    //
    if( mysql_query( mysql, "SET SESSION SQL_MODE='ANSI'" ) ||
        mysql_query( mysql, "SET SESSION AUTOCOMMIT=0" ))
        throw DatabaseError( std::string( "error in mysql_query(): " ) + mysql_error(mysql));

    return mysql;
}

static void simpleQuery (MYSQL* mysql, const std::string& query) throw (DatabaseError)
{
  //  printf("mysql_real_query : %s [%d]\n",query.c_str(),query.size());
  if (mysql_real_query (mysql, query.c_str(), query.size()))
    throw DatabaseError( std::string( "error in mysql_real_query('"+query+"'): " ) + mysql_error(mysql));
}

static char timestamp_buff[32];

static const char* timestamp(time_t t)
{
  strftime(timestamp_buff, 32, "'%F %T'", localtime(&t));
  return timestamp_buff;
}

static std::string blob(const void* payload, int len)
{
  std::ostringstream s;
  //  s.buf().reserve(4+len*2);
  s << "x'" << hex;
  const uint8_t* p = reinterpret_cast<const uint8_t*>(payload);
  const uint8_t* e = p+len;
  while(p<e)
    s << setw(2) << setfill('0') << unsigned(*p++);
  s << "'";
  return s.str();
}

DbClient::DbClient(const char* path) :
  _lock(NoLock)
{
  //
  // Read configuration parameters and decript the passwords.
  //
  std::string spath(path);
  std::ifstream config_file( path );
  if( !config_file.good())
    throw WrongParams( "error in Connection::open(): failed to open the configuration file: '"+spath+"'" );

  std::string host;
  std::string user;
  std::string password;
  std::string db;

  if( !( parse_next_line( host,     "host",     config_file ) &&
         parse_next_line( user,     "user",     config_file ) &&
         parse_next_line( password, "password", config_file ) &&
         parse_next_line( db,       "db",       config_file ) ) )
    throw WrongParams( "error in Connection::open(): failed to parse the configuration file: '"+spath+"'" );

  _mysql = connect2server(host,user,password,db);

  { std::ostringstream sql;
    sql << "CREATE TABLE IF NOT EXISTS experiment"
        << " ( alias  VARCHAR(32) NOT NULL, "
        << "   runkey INT UNSIGNED        , "
        << "   device VARCHAR(32)         , "
        << "   dalias VARCHAR(32)         );";
    simpleQuery(_mysql,sql.str()); }

  { std::ostringstream sql;
    sql << "CREATE TABLE IF NOT EXISTS devices"
        << " ( device VARCHAR(32) NOT NULL, "
        << "   dalias VARCHAR(32)         , "
        << "   typeid INT UNSIGNED        , "
        << "   xtcname VARCHAR(32)        );";
    simpleQuery(_mysql,sql.str()); }

  { std::ostringstream sql;
    sql << "CREATE TABLE IF NOT EXISTS sources"
        << " ( device VARCHAR(32)     NOT NULL, "
        << "   source BIGINT UNSIGNED NOT NULL);";
    simpleQuery(_mysql,sql.str()); }

  { std::ostringstream sql;
    sql << "CREATE TABLE IF NOT EXISTS runkeys"
        << " ( runkey  INT UNSIGNED NOT NULL,"
        << "   keytime TIMESTAMP            ,"
        << "   keyname VARCHAR(32)  NOT NULL,"
        << "   source  BIGINT UNSIGNED NOT NULL,"
        << "   typeid  INT UNSIGNED NOT NULL,"
        << "   xtcname VARCHAR(32)  NOT NULL,"
        << "   xtctime TIMESTAMP    NOT NULL,"
        << "   index keyq (runkey,source,typeid) );";
    simpleQuery(_mysql,sql.str()); }

  { std::ostringstream sql;
    sql << "CREATE TABLE IF NOT EXISTS xtc"
        << " ( typeid  INT UNSIGNED NOT NULL,"
        << "   xtcname VARCHAR(32)  NOT NULL,"
        << "   xtctime TIMESTAMP            ,"
        << "   payload LONGBLOB     NOT NULL,"
        << "   index runq (typeid,xtcname,xtctime) );";
    simpleQuery(_mysql,sql.str()); }
}

DbClient::~DbClient()
{
  mysql_close( _mysql );
}

void                 DbClient::lock  ()
{
  if (_lock!=Lock) {
    //    _close();
    _lock = Lock;
    //    _open ();
  }
}

void                 DbClient::unlock ()
{
  if (_lock==Lock) {
    //    _close();
    _lock = NoLock;
    //    _open ();
  }
}

int                  DbClient::begin ()
{
  lock();
  simpleQuery (_mysql, "BEGIN;");
  return 0;
}

int                  DbClient::commit()
{
  simpleQuery (_mysql, "COMMIT;");
  unlock();
  return 0;
}

int                  DbClient::abort ()
{
  simpleQuery (_mysql, "ROLLBACK;");
  unlock();
  return 0;
}

std::list<ExptAlias> DbClient::getExptAliases()
{
  std::list<ExptAlias> alist;

  //
  // Formulate and execute the query
  //
  std::ostringstream sql;
  sql << "SELECT alias,runkey,device,dalias FROM experiment;";

  QueryProcessor query (_mysql);
  query.execute (sql.str());

  while(query.next_row()) {
    //!!debug only
    //static int iRow = 0;
    //++iRow;
    //string sDevice;
    //query.get (sDevice, "device", true);
    //printf("DbClient::getExptAliases():0 row %d device %s\n", iRow, sDevice.c_str());


    ExptAlias* alias = new ExptAlias;
    query.get (alias->name, "alias");
    query.get (alias->key , "runkey");

    //printf("DbClient::getExptAliases():1 row %d alias %s key %d\n", iRow, alias->name.c_str(), alias->key);

    for(std::list<ExptAlias>::iterator it=alist.begin();
        (true); it++) {
      if (it==alist.end()) {
        alist.push_back(*alias);
        delete alias;
        alias = &alist.back();
        break;
      }
      if (it->name == alias->name) {
        delete alias;
        alias = &(*it);
        break;
      }
    }

    //printf("DbClient::getExptAliases():2 row %d alias %s key %d\n", iRow, alias->name.c_str(), alias->key);

    DeviceEntryRef d;
    query.get (d.device, "device", true);

    //printf("DbClient::getExptAliases():3 row %d device %s\n", iRow, d.device.c_str());

    if (d.device.size()) {
      query.get (d.name  , "dalias");
      alias->devices.push_back(d);
    }
  }

  return alist;
}

int                  DbClient::setExptAliases(const std::list<ExptAlias>& alist)
{
#ifdef USE_TABLE_LOCKS
  { std::ostringstream sql;
    sql << "LOCK TABLES experiment WRITE;";
    simpleQuery(_mysql,sql.str()); }
#endif

  //
  //  Clear the list of aliases
  //
  { std::ostringstream sql;
    sql << "DELETE FROM experiment;";
    simpleQuery(_mysql,sql.str()); }

  //
  //  Add the aliases
  //
  for(std::list<ExptAlias>::const_iterator it=alist.begin(); it!=alist.end(); it++) {

    if (it->devices.empty()) {
      std::ostringstream sql;
      sql << "INSERT INTO experiment (alias,runkey) "
          << "VALUES('" << it->name << "'," << it->key << ");";
      simpleQuery(_mysql,sql.str());
    }
    else {
      std::ostringstream sql;
      sql << "INSERT INTO experiment (alias,runkey,device,dalias) VALUES";
      for(std::list<DeviceEntryRef>::const_iterator dit=it->devices.begin();
          dit!=it->devices.end(); ) {
        sql << "('" << it->name << "'," << it->key << ",'"
            << dit->device << "','" << dit->name << "')";
        dit++;
        sql << (dit==it->devices.end() ? ";" : "," );
      }
      simpleQuery(_mysql,sql.str());
    }
  }

#ifdef USE_TABLE_LOCKS
  { std::ostringstream sql;
    sql << "UNLOCK TABLES;";
    simpleQuery(_mysql,sql.str()); }
#endif

  return 0;
}

std::list<DeviceType> DbClient::getDevices()
{
  std::list<DeviceType> dlist;

  //
  // Formulate and execute the query
  //
  QueryProcessor query (_mysql);
  {
    std::ostringstream sql;
    sql << "SELECT device,dalias,typeid,xtcname FROM devices;";
    query.execute (sql.str());

    while(query.next_row()) {

      DeviceType* d = new DeviceType;
      query.get (d->name, "device");

      for(std::list<DeviceType>::iterator it=dlist.begin();
          (true); it++) {
        if (it==dlist.end()) {
          dlist.push_back(*d);
          delete d;
          d = &dlist.back();
          break;
        }
        if (it->name == d->name) {
          delete d;
          d = &(*it);
          break;
        }
      }

      DeviceEntries* e = new DeviceEntries;
      query.get (e->name, "dalias", true);
      if (!e->name.empty()) {

        for(std::list<DeviceEntries>::iterator e_it=d->entries.begin(); (true); e_it++) {
          if (e_it->name == e->name) {
            delete e;
            e = &(*e_it);
            break;
          }
          if (e_it==d->entries.end()) {
            d->entries.push_back(*e);
            delete e;
            e = &d->entries.back();
            break;
          }
        }

        XtcEntry x;
        query.get (x.type_id, "typeid", true);
        query.get (x.name   , "xtcname", true);
        if (!x.name.empty())
          e->entries.push_back(x);
      }
    }
  }

  {
    std::ostringstream sql;
    sql << "SELECT device,source FROM sources;";
    query.execute (sql.str());

    while(query.next_row()) {

      DeviceType* d = new DeviceType;
      query.get (d->name, "device");

      for(std::list<DeviceType>::iterator it=dlist.begin();
          (true); it++) {
        if (it==dlist.end()) {
          dlist.push_back(*d);
          delete d;
          d = &dlist.back();
          break;
        }
        if (it->name == d->name) {
          delete d;
          d = &(*it);
          break;
        }
      }

      uint64_t s;
      query.get (s, "source");
      d->sources.push_back(s);
    }
  }

  return dlist;
}

int                   DbClient::setDevices(const std::list<DeviceType>& dlist)
{
#ifdef USE_TABLE_LOCKS
  { std::ostringstream sql;
    sql << "LOCK TABLES devices WRITE, sources WRITE;";
    simpleQuery(_mysql,sql.str()); }
#endif

  { std::ostringstream sql;
    sql << "DELETE FROM devices;";
    simpleQuery(_mysql,sql.str()); }

  { std::ostringstream sql;
    sql << "DELETE FROM sources;";
    simpleQuery(_mysql,sql.str()); }

  //
  //  Add the devices
  //
  for(std::list<DeviceType>::const_iterator it=dlist.begin(); it!=dlist.end(); it++) {

    if (it->entries.empty()) {
      std::ostringstream sql;
      sql << "INSERT INTO devices (device) VALUES('" << it->name << "');";
      simpleQuery(_mysql,sql.str());
    }
    else {
      for(std::list<DeviceEntries>::const_iterator r_it=it->entries.begin();
          r_it!=it->entries.end(); r_it++) {

        if (r_it->entries.empty()) {
          std::ostringstream sql;
          sql << "INSERT INTO devices (device,dalias) VALUES('"
              << it->name << "','" << r_it->name << "');";
          simpleQuery(_mysql,sql.str());
        }
        else {
          std::ostringstream sql;
          sql << "INSERT INTO devices (device,dalias,typeid,xtcname) VALUES";
          for(std::list<XtcEntry>::const_iterator x_it=r_it->entries.begin();
              x_it!=r_it->entries.end();) {
            sql << "('" << it->name << "','" << r_it->name
                << "'," << x_it->type_id.value() << ",'" << x_it->name << "')";
            x_it++;
            sql << (x_it==r_it->entries.end() ? ";" : ",");
          }
          simpleQuery(_mysql,sql.str());
        }
      }
    }

    if (!it->sources.empty()) {
      std::ostringstream sql;
      sql << "INSERT INTO sources (device,source) VALUES";
      for(std::list<uint64_t>::const_iterator s_it=it->sources.begin();
          s_it!=it->sources.end();) {
        sql << "('" << it->name << "'," << *s_it << ")";
        s_it++;
        sql << (s_it==it->sources.end() ? ";" : ",");
      }
      simpleQuery(_mysql,sql.str());
    }
  }

#ifdef USE_TABLE_LOCKS
  { std::ostringstream sql;
    sql << "UNLOCK TABLES;";
    simpleQuery(_mysql,sql.str()); }
#endif

  return 0;
}

void                 DbClient::updateKeys()
{
  if (_lock==NoLock)
    _handle_no_lock("updateKeys");

#ifdef USE_TABLE_LOCKS
  { std::ostringstream sql;
    sql << "LOCK TABLES runkeys WRITE, xtc WRITE, experiment READ, devices READ, sources READ;";
    simpleQuery(_mysql,sql.str()); }
#endif

  std::list<ExptAlias > alist = getExptAliases();
  std::list<DeviceType> dlist = getDevices    ();

  //
  //  Create non-existent keys
  //
  for(std::list<ExptAlias>::iterator it=alist.begin();
      it!=alist.end(); it++)
    if (it->key == 0)
      _updateKey(it->name,alist,dlist);
    else {
      std::ostringstream sql;
      sql << "SELECT DISTINCT experiment.alias FROM experiment,runkeys "
          << "WHERE experiment.runkey=runkeys.runkey;";
      QueryProcessor query(_mysql);
      query.execute(sql.str());
      if (query.empty())
        _updateKey(it->name,alist,dlist);
    }

  //
  //  Update keys where the XTC has been modified
  //
  { std::ostringstream sql;
    sql << "SELECT DISTINCT experiment.alias FROM experiment,devices,sources,runkeys,xtc "
        << "WHERE experiment.runkey=runkeys.runkey AND experiment.device=devices.device "
        << "AND devices.dalias=experiment.dalias AND xtc.xtcname=devices.xtcname "
        << "AND xtc.typeid=devices.typeid AND runkeys.keytime<xtc.xtctime;";

    QueryProcessor query(_mysql);
    query.execute(sql.str());

    while(query.next_row()) {
      std::string alias;
      query.get(alias, "alias");
      _updateKey(alias,alist,dlist);
    }
  }

#ifdef USE_TABLE_LOCKS
  { std::ostringstream sql;
    sql << "UNLOCK TABLES;";
    simpleQuery(_mysql,sql.str()); }
#endif

  setExptAliases(alist);
}

void                 DbClient::_updateKey(const std::string& alias,
                                          std::list<ExptAlias>&  alist,
                                          std::list<DeviceType>& dlist)
{
  unsigned key=getNextKey();

  ExptAlias* a = 0;
  for(std::list<ExptAlias>::iterator it=alist.begin(); it!=alist.end(); it++) {
    if (it->name == alias)
      a = &(*it);
  }

  a->key = key;

  for(std::list<DeviceEntryRef>::iterator r_it=a->devices.begin();
      r_it!=a->devices.end(); r_it++) {
    for(std::list<DeviceType>::iterator it=dlist.begin(); it!=dlist.end(); it++) {
      if (it->name == r_it->device) {
        for(std::list<DeviceEntries>::iterator e_it=it->entries.begin();
            e_it!=it->entries.end(); e_it++) {
          if (e_it->name == r_it->name) {
            for(std::list<XtcEntry>::iterator x_it=e_it->entries.begin();
                x_it!=e_it->entries.end(); x_it++) {

              time_t xtctime=0;
              std::ostringstream sql;
              sql << "SELECT xtctime FROM xtc "
                  << "WHERE xtc.typeid=" << x_it->type_id.value()
                  << " AND xtc.xtcname='" << trunc(x_it->name)
                  << "' ORDER BY xtc.xtctime DESC;";
              QueryProcessor q(_mysql);
              q.execute(sql.str());
              if (q.empty()) {
                printf("No %s_v%u/%s found for %s/%s\n",
                       Pds::TypeId::name(x_it->type_id.id()),
                       x_it->type_id.version(),
                       x_it->name.c_str(),
                       r_it->device.c_str(),
                       r_it->name.c_str());
                continue;
              }

              q.next_row();
              q.get_time(xtctime,"xtctime");

              for(std::list<uint64_t>::iterator s_it=it->sources.begin();
                  s_it!=it->sources.end(); s_it++) {
                std::ostringstream sql;
                sql << "INSERT INTO runkeys (runkey,keyname,source,typeid,xtcname,xtctime) "
                    << "VALUES(" << key << ",'" << alias << "'," << *s_it
                    << "," << x_it->type_id.value() << ",'" << x_it->name
                    << "'," << timestamp(xtctime) << ");";
                simpleQuery(_mysql,sql.str());
              }
            }
            break;
          }
        }
        break;
      }
    }
  }
}

unsigned             DbClient::getNextKey()
{
  unsigned key=0;

  std::ostringstream sql;
  sql << "SELECT DISTINCT runkey FROM runkeys;";
  QueryProcessor query(_mysql);
  query.execute(sql.str());

  while(query.next_row()) {
    unsigned k;
    query.get(k, "runkey");
    if (k > key) key=k;
  }

  return ++key;
}

/// Get the list of known run keys
std::list<Key>       DbClient::getKeys()
{
  std::list<Key> keys;

  std::ostringstream sql;
  sql << "SELECT DISTINCT runkey,keytime,keyname FROM runkeys;";
  QueryProcessor query(_mysql);
  query.execute(sql.str());

  while(query.next_row()) {
    Key key;
    query.get(key.key , "runkey");
    query.get_time(key.time, "keytime");
    query.get(key.name, "keyname");
    keys.push_back(key);
  }

  return keys;
}

std::list<KeyEntry>  DbClient::getKey(unsigned key)
{
#ifdef USE_TABLE_LOCKS
  { std::ostringstream sql;
    sql << "LOCK TABLES runkeys READ;";
    simpleQuery(_mysql,sql.str()); }
#endif

  std::list<KeyEntry> entries;

  std::ostringstream sql;
  sql << "SELECT DISTINCT source,typeid,xtcname FROM runkeys "
      << "WHERE runkeys.runkey=" << key;
  QueryProcessor query(_mysql);
  query.execute(sql.str());

  while(query.next_row()) {
    KeyEntry entry;
    query.get(entry.source     , "source");
    query.get(entry.xtc.type_id, "typeid");
    query.get(entry.xtc.name   , "xtcname");
    entries.push_back(entry);
  }

#ifdef USE_TABLE_LOCKS
  { std::ostringstream sql;
    sql << "UNLOCK TABLES;";
    simpleQuery(_mysql,sql.str()); }
#endif

  return entries;
}

int                  DbClient::setKey(const Key& key,
                                      std::list<KeyEntry> entries)
{
#ifdef USE_TABLE_LOCKS
  { std::ostringstream sql;
    sql << "LOCK TABLES runkeys WRITE, xtc WRITE;";
    simpleQuery(_mysql,sql.str()); }
#endif

  //  Remove any entries for this key
  { std::ostringstream sql;
    sql << "DELETE FROM runkeys where runkey=" << key.key << ";";
    simpleQuery(_mysql,sql.str()); }

  time_t keytime = key.time;
  if (keytime==0)
    keytime = time(0);

  for(std::list<KeyEntry>::iterator it=entries.begin();
      it!=entries.end(); it++) {

    time_t xtctime=0;
    { std::ostringstream sql;
      sql << "SELECT xtctime FROM xtc"
          << " WHERE typeid=" << it->xtc.type_id.value()
          << " AND xtcname='" << trunc(it->xtc.name)
          << "' ORDER BY xtctime DESC;";
      QueryProcessor query(_mysql);
      query.execute(sql.str());
      query.next_row();
      if (query.empty()) {
        printf("No %s_v%u/%s found [%s]\n",
               Pds::TypeId::name(it->xtc.type_id.id()),
               it->xtc.type_id.version(),
               it->xtc.name.c_str(),
               sql.str().c_str());
        continue;
      }
      query.get_time(xtctime,"xtctime"); }

    { std::ostringstream sql;
      sql << "INSERT INTO runkeys (runkey,keytime,keyname,source,typeid,xtcname,xtctime) "
          << "VALUES(" << key.key << "," << timestamp(keytime)
          << ",'" << key.name << "'," << it->source
          << "," << it->xtc.type_id.value() << ",'" << it->xtc.name
          << "'," << timestamp(xtctime) << ");";
      simpleQuery(_mysql,sql.str()); }
  }

  //  Remove any unused XTCs (except the latest)
  QueryProcessor query(_mysql);
  { std::ostringstream sql;
    sql << "SELECT DISTINCT typeid,xtcname FROM xtc;";
    query.execute(sql.str()); }
  while(query.next_row()) {
    unsigned t;
    std::string name;
    query.get(t   ,"typeid");
    query.get(name,"xtcname");

    std::ostringstream s;
    s << "SELECT xtctime FROM xtc"
      << " WHERE typeid=" << t
      << " AND xtcname='" << name
      << "' ORDER BY xtctime DESC;";
    QueryProcessor q(_mysql);
    q.execute(s.str());
    if (!q.empty()) {
      q.next_row();  // keep the latest
      while(q.next_row()) {
        time_t xtim;
        q.get_time(xtim, "xtctime");
        std::ostringstream o;
        o << "SELECT 1 FROM runkeys"
          << " WHERE typeid=" << t
          << " AND xtcname='" << name
          << "' AND xtctime=" << timestamp(xtim) << ";";
        QueryProcessor r(_mysql);
        r.execute(o.str());
        if (r.empty()) {
          printf("Removing archaic xtc %08x/%s [%s]\n",
                 t, name.c_str(), timestamp(xtim));
          std::ostringstream o;
          o << "DELETE FROM xtc"
            << " WHERE typeid=" << t
            << " AND xtcname='" << name
            << "' AND xtctime=" << timestamp(xtim) << ";";
          simpleQuery(_mysql,o.str());
        }
      }
    }
  }

#ifdef USE_TABLE_LOCKS
  { std::ostringstream sql;
    sql << "UNLOCK TABLES;";
    simpleQuery(_mysql,sql.str()); }
#endif

  return 0;
}

std::list<XtcEntry>  DbClient::getXTC(unsigned type)
{
  std::list<XtcEntry> xlist;

  const Pds::TypeId& typeId = (const Pds::TypeId&)(type);
  XtcEntry x;
  x.type_id = typeId;

  std::ostringstream sql;
  sql << "SELECT DISTINCT xtcname FROM xtc"
      << " WHERE xtc.typeid=" << type << ";";
  QueryProcessor query(_mysql);
  query.execute(sql.str());
  while(query.next_row()) {
    query.get(x.name,"xtcname");
    xlist.push_back(x);
  }

  return xlist;
}

int                  DbClient::getXTC(const    XtcEntry& x)
{
  std::ostringstream sql;
  sql << "SELECT LENGTH(payload) FROM xtc"
      << " WHERE xtc.typeid=" << x.type_id.value()
      << " AND xtc.xtcname='" << trunc(x.name)
      << "' ORDER BY xtc.xtctime DESC;";
  QueryProcessor query(_mysql);
  query.execute(sql.str());
  if (query.empty())
    return -1;

  query.next_row();
  unsigned len;
  query.get(len,"LENGTH(payload)");
  return len;
}

/// Get the payload of the XTC matching type and name
int                  DbClient::getXTC(const    XtcEntry& x,
                                      void*    payload,
                                      unsigned payload_size)
{
#ifdef USE_TABLE_LOCKS
  { std::ostringstream sql;
    sql << "LOCK TABLES xtc READ;";
    simpleQuery(_mysql,sql.str()); }
#endif

  std::ostringstream sql;
  sql << "SELECT payload FROM xtc"
      << " WHERE xtc.typeid=" << x.type_id.value()
      << " AND xtc.xtcname='" << trunc(x.name)
      << "' ORDER BY xtc.xtctime DESC;";
  QueryProcessor query(_mysql);
  query.execute(sql.str());
  if (query.empty())
    return -1;

  query.next_row();
  int len;
  query.get(payload,len,"payload");

#ifdef USE_TABLE_LOCKS
  { std::ostringstream sql;
    sql << "UNLOCK TABLES;";
    simpleQuery(_mysql,sql.str()); }
#endif

  return len;
}

int                  DbClient::setXTC(const XtcEntry& x,
                                      const void*     payload,
                                      unsigned        payload_size)
{
  QueryProcessor query(_mysql);

  { std::ostringstream sql;
    sql << "SELECT xtctime FROM xtc"
        << " WHERE xtc.typeid=" << x.type_id.value()
        << " AND xtc.xtcname='" << trunc(x.name)
        << "' ORDER BY xtc.xtctime DESC;";
    query.execute(sql.str()); }

  if (!query.empty()) {
    time_t xtctime=0;
    query.next_row();
    query.get_time(xtctime,"xtctime");

    { std::ostringstream sql;
      sql << "SELECT runkey FROM runkeys"
          << " WHERE runkeys.typeid=" << x.type_id.value()
          << " AND runkeys.xtcname='" << trunc(x.name)
          << "' AND runkeys.xtctime=" << timestamp(xtctime) << ";";
      query.execute(sql.str()); }

    if (query.empty()) { // replace
      std::ostringstream sql;
      sql << "DELETE from xtc"
          << " WHERE xtc.typeid=" << x.type_id.value()
          << " AND xtc.xtcname='" << trunc(x.name)
          << "' AND xtc.xtctime=" << timestamp(xtctime) << ";";
      simpleQuery(_mysql,sql.str());
    }
  }

  std::ostringstream sql;
  sql << "INSERT INTO xtc (typeid,xtcname,payload) VALUES("
      << x.type_id.value() << ",'"
      << x.name << "',"
      << blob(payload,payload_size) << ");";
  simpleQuery(_mysql,sql.str());

  return payload_size;
}


DbClient*  DbClient::open  (const char* path)
{
  return new DbClient(path);
}

