#ifndef Pds_MonDESC_HH
#define Pds_MonDESC_HH

namespace Pds {

  class MonDesc {
  public:
    MonDesc(const char* name);
    MonDesc(const MonDesc& desc);
    ~MonDesc();

    const char* name() const;
    int short id() const;
    unsigned short nentries() const;
    void added  ();
    void removed();
    void reset  ();

  private:
    friend class MonGroup;
    friend class MonCds;
    void id(int short i);

  private:
    enum {NameSize=128};
    char _name[NameSize];
    int short _id;
    unsigned short _nentries;
  };
};

#endif
