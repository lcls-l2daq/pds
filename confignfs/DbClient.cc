#include "pds/confignfs/DbClient.hh"
#include "pds/confignfs/XML.hh"
#include "pds/confignfs/CfgPath.hh"
#include "pds/confignfs/XtcTable.hh"
#include "pds/config/PdsDefs.hh"

using Pds_ConfigDb::XtcEntry;
using Pds_ConfigDb::XtcEntryT;
using Pds_ConfigDb::KeyEntry;
using Pds_ConfigDb::KeyEntryT;
using Pds_ConfigDb::Key;
using Pds_ConfigDb::DeviceEntryRef;
using Pds_ConfigDb::DeviceEntries;
using Pds_ConfigDb::DeviceType;
using Pds_ConfigDb::ExptAlias;
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <glob.h>
#include <libgen.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <map>
#include <list>

//#define DBUG

using std::hex;
using std::setw;
using std::setfill;
using std::ostringstream;
using std::string;
using std::list;

static int _symlink(const char* dst, const char* src) {
  
  int r = symlink(dst,src);
  if (r<0) {
    char buff[256];
    sprintf(buff,"symlink %s -> %s",src,dst);
    perror(buff);
  }
#ifdef DBUG
  printf("_symlink %s\n",dst);
#endif
  return r;
}

static int _unlink(const char* dst) {
  int r = remove(dst);
  if (r<0) {
    char buff[256];
    sprintf(buff,"remove %s",dst);
    perror(buff);
  }
#ifdef DBUG
  printf("_remove %s\n",dst);
#endif
  return r;
}

static int _mkdir(const char* dst, mode_t m) {
  struct stat64 s;
  if (stat64(dst,&s)==0 &&
      S_ISDIR(s.st_mode)) return 0;
  
  int r = mkdir(dst,m);
  if (r<0) {
    char buff[256];
    sprintf(buff,"mkdir %s",dst);
    perror(buff);
  }
#ifdef DBUG
  printf("_mkdir %s\n",dst);
#endif
  return r;
}

static void _handle_no_lock(const char* s)
{
  std::string _no_lock_exception(s);
  printf("_handle_no_lock:%s\n",s);
  throw _no_lock_exception;
}

#if 0
static string _id(const Pds::Src& d)
{
  ostringstream o;
  o << std::hex << setfill('0') << setw(8) << d.log() << '.' << setw(8) << d.phy();
  return o.str();
}
#endif

static std::string src_path(const Pds::Src& d)
{
  ostringstream o;
  if (d.level()!=Pds::Level::Source)
    o << std::hex << d.level();
  else
    o << std::hex << setfill('0') << setw(8) << d.phy();
  return o.str();
}

static const mode_t _fmode = S_IROTH | S_IXOTH | S_IRGRP | S_IXGRP | S_IRWXU;

namespace Pds_ConfigDb {
  namespace Nfs {
    class NfsState {
    public:
      NfsState(FILE*);
    public:
      void write(FILE*) const;
    public:
      const Device* _device(const std::string&) const;
    public:
      Table                table;
      std::list<Device>    devices;
      mutable time_t       time_db;
      time_t               time_key;
    };
  };
};

using namespace Pds_ConfigDb::Nfs;

DbClient::DbClient(const char* path) :
  _path(path),
  _lock(NoLock)
{
  _open();
}

DbClient::~DbClient()
{
  _close();
}

void                 DbClient::lock  ()
{
  if (_lock!=Lock) {
    _close();
    _lock = Lock;
    _open();
  }
}

void                 DbClient::unlock ()
{
  if (_lock==Lock) {
    _close();
    _lock = NoLock;
    _open();
  }
}

void DbClient::_close()
{
  if (_f)
    fclose(_f);
}

void DbClient::_open()
{
  string fname = _path.expt() + ".xml";
  _f = fopen(fname.c_str(),_lock==Lock ? "r+":"r");
  if (!_f) {
    perror("fopen expt.xml");
  }
  else if (_lock==Lock) {  // lock it
    struct flock flk;
    flk.l_type   = F_WRLCK;
    flk.l_whence = SEEK_SET;
    flk.l_start  = 0;
    flk.l_len    = 0;
    if (fcntl(fileno(_f), F_SETLK, &flk)<0) {
      perror("Experiment fcntl F_SETLK");
      if (fcntl(fileno(_f), F_GETLK, &flk)<0) 
        perror("Experiment fcntl F_GETLK");
      else if (flk.l_type == F_UNLCK)
        printf("F_GETLK: lock is available\n");
      else {
        char buff[256];
        if (flk.l_pid == 0)
          _handle_no_lock("Lock held by remote process");
        else {
          sprintf(buff,"Lock held by process %d\n", flk.l_pid);
          _handle_no_lock(buff);
        }
      }
    }
  }
}

int                  DbClient::begin () { lock(); return 0; }
 
int                  DbClient::commit() { unlock(); return 0; }

int                  DbClient::abort () { return 0; }

std::list<ExptAlias> DbClient::getExptAliases()
{
  NfsState nfs(_f);

  std::list<ExptAlias> alist;
  for(std::list<TableEntry>::iterator it=nfs.table.entries().begin();
                it!=nfs.table.entries().end(); it++) {
    ExptAlias alias;
    alias.name = it->name();
    
    char* endptr;
    alias.key  = strtoul(it->key().c_str(),&endptr,16);
    if (endptr == it->key().c_str())
      alias.key = -1U;
    
    std::list<DeviceEntryRef> dlist;
    for(std::list<FileEntry>::const_iterator e_it=it->entries().begin();
        e_it!=it->entries().end(); e_it++) {
      DeviceEntryRef d;
      d.device = e_it->name ();
      d.name   = e_it->entry();
      dlist.push_back(d);
    }
    alias.devices = dlist;
    alist.push_back(alias);
  }
  return alist;
}

int                  DbClient::setExptAliases(const std::list<ExptAlias>& alist)
{
  if (!_f || _lock==NoLock) {
    _handle_no_lock("write");
    return -1;
  }

  NfsState nfs(_f);

  nfs.table.entries().clear();
  for(std::list<ExptAlias>::const_iterator it=alist.begin(); it!=alist.end(); it++) {
    ostringstream key;
    key << hex << setw(8) << setfill('0') << it->key;

    std::list<FileEntry> entries;
    for(std::list<DeviceEntryRef>::const_iterator e_it=it->devices.begin();
        e_it!=it->devices.end(); e_it++)
      entries.push_back( FileEntry( e_it->device, e_it->name ) );
    
    nfs.table.entries().push_back(TableEntry(it->name, key.str(), entries));

    if (nfs.table._next_key <= it->key)
      nfs.table._next_key = it->key+1;
  }

  nfs.write(_f);

  return 0;
}

std::list<DeviceType> DbClient::getDevices()
{
  NfsState nfs(_f);

  std::list<DeviceType> dlist;
  for(std::list<Device>::const_iterator it=nfs.devices.begin();
      it!=nfs.devices.end(); it++) {
    DeviceType d;
    d.name    = it->name();

    for(std::list<TableEntry>::const_iterator t_it=it->table().entries().begin();
        t_it!=it->table().entries().end(); t_it++) {
      DeviceEntries entry;
      entry.name = t_it->name();

      for(std::list<FileEntry>::const_iterator f_it=t_it->entries().begin();
          f_it!=t_it->entries().end(); f_it++) {
        XtcEntry x;
        const Pds::TypeId* t = PdsDefs::typeId(UTypeName(f_it->name()));
        if (!t) {
          printf("Error looking up PdsDefs::typeId(%s)\n",f_it->name().c_str());
          abort();
        }
        x.type_id = *t;
        x.name    = f_it->entry();
        entry.entries.push_back(x);
      }
      d.entries.push_back(entry);
    }

    for(std::list<DeviceEntry>::const_iterator d_it=it->src_list().begin();
        d_it!=it->src_list().end(); d_it++)
      d.sources.push_back(d_it->value());

    dlist.push_back(d);
  }

  return dlist;
}

int                   DbClient::setDevices(const std::list<DeviceType>& dlist)
{
  NfsState nfs(_f);

  nfs.devices.clear();
  for(std::list<DeviceType>::const_iterator it=dlist.begin(); it!=dlist.end(); it++) {
    std::list<DeviceEntry> src_list;
    for(std::list<uint64_t>::const_iterator r_it=it->sources.begin();
        r_it!=it->sources.end(); r_it++)
      src_list.push_back(DeviceEntry(*r_it));

    Device d("",it->name,src_list);

    for(std::list<DeviceEntries>::const_iterator r_it=it->entries.begin();
        r_it!=it->entries.end(); r_it++) {
      std::list<FileEntry> entries;
      for(std::list<XtcEntry>::const_iterator x_it=r_it->entries.begin();
          x_it!=r_it->entries.end(); x_it++) {
        entries.push_back(FileEntry(PdsDefs::utypeName(x_it->type_id),
                                    x_it->name));
      }
      d.table().entries().push_back(TableEntry(r_it->name,"fffffff",entries));
    }
    nfs.devices.push_back(d);

    mkdir(_path.key_path(it->name,"").c_str(),_fmode);
  }

  nfs.write(_f);
  
  return 0;
}

void                 DbClient::updateKeys()
{
  if (_lock==NoLock)
    _handle_no_lock("updateKeys");

  NfsState nfs(_f);

  //  Flag all xtc files that have been modified since the last update
  XtcTable xtc(_path.base());
  xtc.update(_path,nfs.time_key);

#ifdef DBUG
  xtc.dump();
#endif

  //  Generate device keys / update XTC file extensions
  for(list<Device>::iterator it=nfs.devices.begin(); it!=nfs.devices.end(); it++)
    it->update_keys(_path,xtc,nfs.time_key);

  //  Generate experiment keys
  for(list<TableEntry>::iterator iter=nfs.table.entries().begin();
      iter!=nfs.table.entries().end(); iter++) {
    bool changed = iter->initial();

    if (!changed) {
      string kpath = _path.key_path(iter->key());
      struct stat s;
      changed = stat(kpath.c_str(),&s)!=0;
    }

    if (!changed) {
      for(list<FileEntry>::const_iterator it=iter->entries().begin(); it!=iter->entries().end(); it++) {
        const Device* dev = nfs._device(it->name());
        if (!dev) {
          printf("No device %s found for key %s\n",it->name().c_str(),iter->name().c_str()); 
          continue;
        }
      
        const TableEntry* te = dev->table().get_top_entry(it->entry());
        if (!te) {
          printf("No alias %s/%s found for key %s\n",it->name().c_str(),it->entry().c_str(),iter->name().c_str()); 
          continue;
        }

        changed |= te->updated();
#ifdef DBUG
        if (te->updated())
          printf("expt:%s dev:%s:%s %s\n", iter->name().c_str(), it->name().c_str(), it->entry().c_str(),
                 te->updated() ? "changed" : "unchanged");
#endif

	for(list<DeviceEntry>::const_iterator dit=dev->src_list().begin(); 
	    dit!=dev->src_list().end() && !changed; dit++) {
	  ostringstream o;
	  o << _path.key_path(iter->key()) << "/" << src_path(*dit);
	  struct stat s;
	  changed |= stat(o.str().c_str(),&s)!=0;
	}
      }
    }

    if (changed) {
      // make the key
      mode_t mode = _fmode;
      string kpath = _path.key_path(nfs.table._next_key);
#ifdef DBUG
      printf("mkdir %s\n",kpath.c_str());
#endif
      mkdir(kpath.c_str(),mode);
      iter->update(nfs.table._next_key++);

      // persist the alias name that created this key
      { string spath = kpath + "/Info";
        std::ofstream of(spath.c_str());
        of << iter->name() << std::endl; }

      for(list<FileEntry>::const_iterator it=iter->entries().begin(); it!=iter->entries().end(); it++) {
        const Device* dev = nfs._device(it->name());
        if (!dev) continue;
        const TableEntry* te = dev->table().get_top_entry(it->entry());
        if (!te) continue;

        for(list<DeviceEntry>::const_iterator src=dev->src_list().begin(); src!=dev->src_list().end(); src++) {
          ostringstream o;
          o << kpath.c_str() << "/" << src_path(*src);
          mkdir(o.str().c_str(),mode);
        
          for(list<FileEntry>::const_iterator fit=te->entries().begin();
              fit!=te->entries().end(); fit++) {
            UTypeName utype(fit->name());
            QTypeName qtype = PdsDefs::qtypeName(utype);
            XtcFileEntry* fe = xtc.entry(qtype)->entry(fit->entry());
            if (!fe) {
              printf("No file entry %s/%s\n",qtype.c_str(),fit->entry().c_str());
              continue;
            }

            ostringstream ko;
            ko << o.str() << "/" << hex << setw(8) << setfill('0') 
               << PdsDefs::typeId(utype)->value();

            ostringstream po;
            po << "../../../xtc/" << qtype << "/" 
               << fe->name() << "." << fe->ext();
#ifdef DBUG
            printf("symlink %s -> %s\n",ko.str().c_str(), po.str().c_str());

#endif
            _symlink(po.str().c_str(),ko.str().c_str());
          }
        }
      }
    }
  }

  nfs.time_key = time(0);
  nfs.write(_f);

  xtc.write(_path.base());
}

void                 DbClient::purge()
{
  //
  //  Remove any detector keys that are no longer referenced
  //
  std::map< string, std::list<unsigned> > dkeys;
  { string kname = "[!0-9]*";
    string kpath = _path.key_path(kname);
    glob_t g;
    glob(kpath.c_str(),0,0,&g);
    for(unsigned k=0; k<g.gl_pathc; k++) {
      string det(basename(g.gl_pathv[k]));
      string dkeyp = string(g.gl_pathv[k])+"/[0-9]*";
      glob_t h;
      glob(dkeyp.c_str(),0,0,&h);
      for(unsigned l=0; l<h.gl_pathc; l++)
	dkeys[det].push_back( strtoul(basename(h.gl_pathv[l]),NULL,16) );
      globfree(&h);
    } 
    globfree(&g);
  }
  //
  //  Remove any (versioned) xtcs that are no longer referenced
  //
  std::list< string > xtcs;
  { ostringstream p;
    p << _path.data_path(QTypeName()) << "/*/*.[0-9]*";
    glob_t g;
    glob(p.str().c_str(),0,0,&g);
    for(unsigned k=0; k<g.gl_pathc; k++) {
      string s(g.gl_pathv[k]);
      xtcs.push_back(s.substr(s.rfind('/',s.rfind('/')-1)+1));
    }
    globfree(&g);
  }
  //  Include temporary XTCs used for scanning
  { ostringstream p;
    p << _path.data_path(QTypeName()) << "/*/[0-9]*";
    glob_t g;
    glob(p.str().c_str(),0,0,&g);
    for(unsigned k=0; k<g.gl_pathc; k++) {
      char* endptr;
      string b(basename(g.gl_pathv[k]));
      strtoul(b.c_str(),&endptr,16);
      if ((endptr-b.c_str())==16) {
	string s(g.gl_pathv[k]);
	xtcs.push_back(s.substr(s.rfind('/',s.rfind('/')-1)+1));
      }
    }
    globfree(&g);
  }

  { string kname = "[0-9]*/[0-9]*";
    string kpath = _path.key_path(kname);
    glob_t g;
    glob(kpath.c_str(),0,0,&g);
    const int bsz=256;
    char buff[bsz];
    for(unsigned k=0; k<g.gl_pathc; k++) {
      ssize_t sz = readlink(g.gl_pathv[k],buff,bsz);
      if (sz>0) {
	buff[sz]=0;
	string det(buff);
	det.erase(0,det.find('/')+1);
	det.erase(det.find('/'));
	unsigned ukey = strtoul( basename(buff),NULL,16);
	dkeys[det].remove(ukey);
      }

      string t(g.gl_pathv[k]);
      glob_t h;
      glob((t+"/*").c_str(),0,0,&h);
      for(unsigned l=0; l<h.gl_pathc; l++) {
	ssize_t sz = readlink(h.gl_pathv[l],buff,bsz);
	if (sz>0) {
	  buff[sz]=0;
	  string s(buff);
	  xtcs.remove(s.substr(s.rfind('/',s.rfind('/')-1)+1));
	}
      }
      globfree(&h);
    }
    globfree(&g);
  }

  printf("--- Detector keys to be purged ---\n");
  for(std::map< string, std::list<unsigned> >::iterator it=dkeys.begin();
      it!=dkeys.end(); it++) {
    printf("%s: ",it->first.c_str());
    for(std::list<unsigned>::iterator uit=it->second.begin();
	  uit!=it->second.end(); uit++)
      printf(" %x",*uit);
    printf("\n");
  }
  printf("--- Detector configurations to be purged ---\n");
  for(std::list< string >::iterator it=xtcs.begin();
      it!=xtcs.end(); it++)
    printf("%s\n",it->c_str());

  printf("Continue? [y/N]\n");
  char c = 'n';
  scanf("%c",&c);

  if (c=='y' || c=='Y') {
    for(std::map< string, std::list<unsigned> >::iterator it=dkeys.begin();
	it!=dkeys.end(); it++) {
      for(std::list<unsigned>::iterator uit=it->second.begin();
	  uit!=it->second.end(); uit++) {
	ostringstream p;
	p << _path.key_path(it->first) << '/' << setw(8) << setfill('0') << hex << *uit;
	remove(p.str().c_str());
      }
    }
    for(std::list< string >::iterator it=xtcs.begin();
	it!=xtcs.end(); it++)
      remove( _path.data_path(QTypeName(*it)).c_str() );
  }
}

unsigned             DbClient::getNextKey()
{
  return NfsState(_f).table._next_key;
}

/// Get the list of known run keys
std::list<Key>       DbClient::getKeys()
{
  std::list<Key> keys;

  // populate key list
  string kname = "[0-9]*";
  string kpath = _path.key_path(kname);
  glob_t g;
  glob(kpath.c_str(),0,0,&g);
  for(unsigned k=0; k<g.gl_pathc; k++) {
    struct stat64 s;
    stat64(g.gl_pathv[k],&s);

    Key key;
    key.key  = strtoul(basename(g.gl_pathv[k]),NULL,16);
    key.time = s.st_mtime;
    key.name = "";

    string info_path(g.gl_pathv[k]);
    info_path += "/Info";
    std::ifstream fi(info_path.c_str());
    if (fi.good()) {
      char buff[32];
      fi.getline(buff,32);
      key.name = buff;
    }
    keys.push_back(key);
  }
  globfree(&g);
  return keys;
}
    
std::list<KeyEntry>  DbClient::getKey(unsigned key)
{
  std::list<KeyEntry> entries;

  size_t buff_size = 512;
  char* buff = new char[buff_size];

  // populate key list
  ostringstream kname;
  kname << hex << setw(8) << setfill('0') << key;
  kname << "/[0-9]*";
  string kpath = _path.key_path(kname.str());
  glob_t g;
  glob(kpath.c_str(),0,0,&g);
  for(unsigned k=0; k<g.gl_pathc; k++) {
    char* bname = basename(g.gl_pathv[k]);
    unsigned phy = strtoul(bname,NULL,16);
    KeyEntry entry;
    if (strlen(bname)==1) {
      entry.source = phy;
      entry.source <<= 56;
    }
    else {
      entry.source  = phy;
      entry.source |= 1ULL<<56;
    }

    ostringstream hpath;
    hpath << g.gl_pathv[k] << "/[0-9]*";
    glob_t h;
    glob(hpath.str().c_str(),0,0,&h);
    for(unsigned m=0; m<h.gl_pathc; m++) {
      unsigned vtyp = strtoul(basename(h.gl_pathv[m]),NULL,16);
      entry.xtc.type_id = (Pds::TypeId&)vtyp;
      
      ssize_t bsiz=readlink(h.gl_pathv[m],buff,buff_size);
      if (bsiz>=0) {
        buff[bsiz]=0;
        entry.xtc.name = basename(buff);
        entries.push_back(entry);
      }
    }
    globfree(&h);
  }
  globfree(&g);
  delete[] buff;
  return entries;
}

std::list<KeyEntryT>  DbClient::getKeyT(unsigned key)
{
  std::list<KeyEntryT> entries;

  size_t buff_size = 512;
  char* buff = new char[buff_size];

  // populate key list
  ostringstream kname;
  kname << hex << setw(8) << setfill('0') << key;
  kname << "/[0-9]*";
  string kpath = _path.key_path(kname.str());
  glob_t g;
  glob(kpath.c_str(),0,0,&g);
  for(unsigned k=0; k<g.gl_pathc; k++) {
    char* bname = basename(g.gl_pathv[k]);
    unsigned phy = strtoul(bname,NULL,16);
    KeyEntryT entry;
    if (strlen(bname)==1) {
      entry.source = phy;
      entry.source <<= 56;
    }
    else {
      entry.source  = phy;
      entry.source |= 1ULL<<56;
    }

    ostringstream hpath;
    hpath << g.gl_pathv[k] << "/[0-9]*";
    glob_t h;
    glob(hpath.str().c_str(),0,0,&h);
    for(unsigned m=0; m<h.gl_pathc; m++) {
      unsigned vtyp = strtoul(basename(h.gl_pathv[m]),NULL,16);
      entry.xtc.xtc.type_id = (Pds::TypeId&)vtyp;
      
      ssize_t bsiz=readlink(h.gl_pathv[m],buff,buff_size);
      if (bsiz>=0) {
        buff[bsiz]=0;
        entry.xtc.xtc.name = basename(buff);

        struct stat64 s;
        stat64(buff,&s);
        entry.xtc.time = s.st_mtime;

        const size_t bsz=256;
        char buff[bsz];
        strftime(buff, bsz, "%F %T", localtime(&entry.xtc.time));
        entry.xtc.stime = std::string(buff);

        entries.push_back(entry);
      }
    }
    globfree(&h);
  }
  globfree(&g);
  delete[] buff;
  return entries;
}

int                  DbClient::setKey(const Key& key,
                                      std::list<KeyEntry> entries)
{
  std::string kpath = _path.key_path(key.key);

  //  Remove the key
  struct stat64 s;
  if (stat64(kpath.c_str(),&s)==0)
  { 
    glob_t g;
    glob(kpath.c_str(),0,0,&g);
    for(unsigned k=0; k<g.gl_pathc; k++) {
      ostringstream hpath;
      hpath << g.gl_pathv[k] << "/[0-9]*";
      glob_t h;
      glob(hpath.str().c_str(),0,0,&h);
      for(unsigned m=0; m<h.gl_pathc; m++) {
	ostringstream qpath;
	qpath << h.gl_pathv[m] << "/[0-9]*";
	glob_t q;
	glob(qpath.str().c_str(),0,0,&q);
	for(unsigned n=0; n<q.gl_pathc; n++)
	  _unlink(q.gl_pathv[n]);
	globfree(&q);

        _unlink(h.gl_pathv[m]);
      }
      globfree(&h);
    }
    globfree(&g);
  }
  else if (!entries.empty())
    mkdir(kpath.c_str(),_fmode);

  //  Create the key
  for(std::list<KeyEntry>::iterator it=entries.begin();
      it!=entries.end(); it++) {
#ifdef DBUG
    printf("\t..%llx..%08x..%s\n",it->source,it->xtc.type_id.value(),it->xtc.name.c_str());
#endif
    _mkdir((kpath+"/"+src_path(DeviceEntry(it->source))).c_str(),_fmode);

    ostringstream lpath;
    lpath << kpath << "/" << src_path(DeviceEntry(it->source)) << "/"
          << hex << setw(8) << setfill('0') << it->xtc.type_id.value();

    ostringstream dpath;
    dpath << "../../../xtc/" << PdsDefs::qtypeName(it->xtc.type_id) << "/"
          << it->xtc.name;
    _symlink(dpath.str().c_str(),lpath.str().c_str());
  }  

  string spath = kpath + "/Info";
  if (entries.empty()) {
    _unlink(spath.c_str());
    if (rmdir(kpath.c_str()))
      perror("Removing key directory");
  }
  else {
    // persist the alias name that created this key
    std::ofstream of(spath.c_str());
    of << key.name << std::endl; 
  }
  
  begin();
  NfsState nfs(_f);
  if (nfs.table._next_key <= key.key)
    nfs.table._next_key = key.key+1;
  nfs.write(_f);
  commit();

  return 0;
}

std::list<XtcEntry>  DbClient::getXTC(unsigned type)
{
  std::list<XtcEntry> xlist;

  const Pds::TypeId& typeId = (const Pds::TypeId&)(type);
  XtcEntry x;
  x.type_id = typeId;

  std::list<std::string> files = 
    _path.xtc_files(std::string(), PdsDefs::utypeName(typeId));
  for(std::list<std::string>::iterator it=files.begin();
      it!=files.end(); it++) {
    x.name = basename(const_cast<char*>(it->c_str()));
    xlist.push_back(x);
  }

  return xlist;
}

int                  DbClient::getXTC(const    XtcEntry& x)
{
  ostringstream p;
  p << _path.data_path(PdsDefs::qtypeName(x.type_id)).c_str()
    << "/" << x.name.c_str();
  struct stat64 s;
  if (stat64(p.str().c_str(),&s))
    return -1;

  return s.st_size;
}

/// Get the payload of the XTC matching type and name
int                  DbClient::getXTC(const    XtcEntry& x,
                                      void*    payload,
                                      unsigned payload_size)
{
  ostringstream p;
  p << _path.data_path(PdsDefs::qtypeName(x.type_id)).c_str()
    << "/" << x.name.c_str();
  FILE* f = fopen(p.str().c_str(),"r");
  if (!f)
    return -1;

  int len = fread(payload, 1, payload_size, f);
  fclose(f);
  return len;
}

int                  DbClient::setXTC(const XtcEntry& x,
                                      const void*     payload,
                                      unsigned        payload_size)
{
  ostringstream p;
  p << _path.data_path(PdsDefs::qtypeName(x.type_id)).c_str()
    << "/" << x.name.c_str();

  bool changed=true;

  FILE* f = fopen(p.str().c_str(),"r");
  if (f) {
    size_t in_payload_size = payload_size+32;
    char* in_payload = new char[in_payload_size];
    size_t len = fread(in_payload, 1, in_payload_size, f);
    changed = !(len == payload_size && memcmp(payload, in_payload, payload_size)==0);
    delete[] in_payload;
    fclose(f);
  }

  f = fopen(p.str().c_str(),"w");
  if (!f)
    return -1;

  int len = fwrite(payload, payload_size, 1, f);
  fclose(f);

  if (changed) {
    XtcTable xtc(_path.base());
    xtc.update_xtc(PdsDefs::qtypeName(x.type_id), x.name);
    xtc.write(_path.base());
  }

  return len;
}

std::list<XtcEntryT> DbClient::getXTC(uint64_t source,
				      unsigned type)
{
  std::list<XtcEntryT> result;

  std::ostringstream ostr;
  ostr << "0*/" 
       << std::hex << setw(8) << setfill('0') << ((source>>0)&0xffffffff) << "/"
       << std::hex << setw(8) << setfill('0') << type;
  string kname = ostr.str();
  string kpath = _path.key_path(kname);
  glob_t g;
  glob(kpath.c_str(),0,0,&g);

  XtcEntryT t;
  t.xtc.type_id = reinterpret_cast<const Pds::TypeId&>(type);
  for(unsigned k=0; k<g.gl_pathc; k++) {
    struct stat64 s;
    stat64(g.gl_pathv[k],&s);
    t.time = s.st_mtime;

    const size_t bsz=256;
    char buff[bsz];
    strftime(buff, bsz, "%F %T", localtime(&t.time));
    t.stime = std::string(buff);

    ssize_t isz = readlink(g.gl_pathv[k],buff,bsz);
    buff[isz]=0;
    t.xtc.name = std::string(basename(buff));
    result.push_back(t);
  }
  globfree(&g);
  return result;
}

int                  DbClient::getXTC(const    XtcEntryT& x)
{
  ostringstream p;
  p << _path.data_path(PdsDefs::qtypeName(x.xtc.type_id)).c_str()
    << "/" << x.xtc.name.c_str();
  glob_t g;
  glob(p.str().c_str(),0,0,&g);

  for(unsigned k=0; k<g.gl_pathc; k++) {
    struct stat64 s;
    stat64(g.gl_pathv[k],&s);
    if (s.st_mtime == x.time)
      return s.st_size;
  }
  globfree(&g);
  return -1;
}

/// Get the payload of the XTC matching type and name
int                  DbClient::getXTC(const    XtcEntryT& x,
                                      void*    payload,
                                      unsigned payload_size)
{
  ostringstream p;
  p << _path.data_path(PdsDefs::qtypeName(x.xtc.type_id)).c_str()
    << "/" << x.xtc.name.c_str();
  glob_t g;
  glob(p.str().c_str(),0,0,&g);
  for(unsigned k=0; k<g.gl_pathc; k++) {
    struct stat64 s;
    stat64(g.gl_pathv[k],&s);
    if (s.st_mtime == x.time) {
      FILE* f = fopen(p.str().c_str(),"r");
      if (!f)
	return -1;

      int len = fread(payload, 1, payload_size, f);
      fclose(f);
      globfree(&g);
      return len;
    }
  }
  globfree(&g);

  return -1;
}

DbClient*  DbClient::open  (const char* path)
{
  return new DbClient(path);
}


NfsState::NfsState(FILE* f) :
  time_db (0),
  time_key(0)
{
  if (!f) {
    perror("fopen expt.xml");
  }
  else {
    fseek(f,0,SEEK_SET);
    struct stat64 s;
    if (fstat64(fileno(f),&s))
      perror("fstat64 expt.xml");
    else {
      const char* TRAILER = "</Document>";
      char* buff = new char[s.st_size+strlen(TRAILER)+1];
      if (fread(buff, 1, s.st_size, f) != size_t(s.st_size))
        perror("fread expt.xml");
      else {
        strcpy(buff+s.st_size,TRAILER);
        const char* p = buff;

        XML_iterate_open(p,tag)
          if      (tag.name == "_table") {
            table.load(p); 
          }
          else if (tag.name == "_devices") {
            Device d;
            d.load(p);
            devices.push_back(d);
          }
          else if (tag.name == "_time_db")
            time_db  = XML::IO::extract_i(p);
          else if (tag.name == "_time_key")
            time_key = XML::IO::extract_i(p);
        XML_iterate_close(Experiment,tag);
      }
      delete[] buff;
    }
  }
}

void NfsState::write(FILE* f) const
{
  time_t _time_db = time_db;

  fseek(f,0,SEEK_SET);
  const int MAX_SIZE = 0x100000;
  char* buff = new char[MAX_SIZE];
  char* p = buff;

  time_db = time(0);

  XML_insert(p, "Table"   , "_table", table.save(p));
  XML_insert(p, "time_t"  , "_time_db" , XML::IO::insert(p, (unsigned)time_db));
  XML_insert(p, "time_t"  , "_time_key", XML::IO::insert(p, (unsigned)time_key));
  for(list<Device>::const_iterator it=devices.begin(); it!=devices.end(); it++) {
    XML_insert(p, "Device", "_devices", it->save(p) );
  }

  if (p > buff+MAX_SIZE) {
    perror("save overwrites array");
    time_db = _time_db;
  } 
  else {
    size_t nwr = fwrite(buff, 1, p-buff, f);
    if (nwr != size_t(p-buff)) {
      perror("Error writing to database");
    }
    else {
      ftruncate(fileno(f),nwr);
    }
  }

  delete[] buff;
}

const Device* NfsState::_device(const std::string& name) const
{
  for(std::list<Device>::const_iterator it=devices.begin();
      it!=devices.end(); it++)
    if (it->name() == name)
      return &(*it);
  return 0;
}

