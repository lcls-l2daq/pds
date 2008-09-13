#ifndef PDS_INXTCITERATORTEMPLATE_HH
#define PDS_INXTCITERATORTEMPLATE_HH

/*
** ++
**  Package:
**	OdfContainer
**
**  Abstract:
**      
**  Author:
**      James Swain
**
**  Creation Date:
**	000 - August 20, 2001
**
**  Revision History:
**	None.
**
** --
*/

namespace Pds {

template<class C>
class InXtcIteratorTemplate
{
public:
  InXtcIteratorTemplate(C* root):
    _root(root){}
  InXtcIteratorTemplate() {}
  virtual ~InXtcIteratorTemplate() {}

  virtual int process(C* root) = 0;

  void iterate();
  void iterate(C*);

protected:

  C* _root; // Collection to process in the absence of an argument...

};

/*
** ++
**
**    This function will commence iteration over the collection specified
**    by the constructor. 
**
** --
*/

template<class C> inline 
void InXtcIteratorTemplate<C>::iterate() 
{
  iterate(_root);
} 

template<class C> inline
void InXtcIteratorTemplate<C>::iterate(C* root) 
{
  if (root->damage.value() & ( 1 << Damage::IncompleteContribution)){
    return;
  }
  
  C* payload     = (C*)root->payload();
  int remaining = root->sizeofPayload();
  
  while(remaining > 0)
    {
      if(!process(payload)) break;
      remaining -= payload->sizeofPayload() + sizeof(C);
      payload    = payload->next();
    }
  
  return;
}
}
#endif
