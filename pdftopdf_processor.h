#ifndef PDFTOPDF_PROCESSOR_H
#define PDFTOPDF_PROCESSOR_H

#include "pptypes.h"
#include "nup.h"
#include "intervalset.h"
#include <vector>

struct ProcessingParameters {
  ProcessingParameters() 
    : jobId(0),numCopies(1),
      user(0),title(0),
      fitplot(false),
      orientation(ROT_0),
      duplex(false),
      border(NONE),
      reverse(false),

//      pageLabel(NULL),
      evenPages(true),oddPages(true),

      mirror(false),

      xpos(CENTER),ypos(CENTER),

      collate(false),
      evenDuplex(false),

      emitJCL(true),deviceCopies(1),
      setDuplex(false),unsetCollate(false)
  {
    page.width=612.0; // letter
    page.height=792.0;
    page.top=page.height-36.0;
    page.bottom=36.0;
    page.left=18.0;
    page.right=page.width-18.0;

    // everything
    pageRange.add(1);
    pageRange.finish();
  }

  int jobId, numCopies;
  const char *user, *title; // will stay around
  bool fitplot;
  PageRect page;
  Rotation orientation;
  bool duplex;
  BorderType border;
  NupParameters nup;
  bool reverse;

  // std::string pageLabel; // or NULL?  must stay/dup!
  bool evenPages,oddPages;
  IntervalSet pageRange;

  bool mirror;

  Position xpos,ypos;

  bool collate;
/*
  ...
  ...

  ??? shuffle 
*/
  bool evenDuplex; // make number of pages a multiple of 2

  bool emitJCL;
  int deviceCopies;

  // ppd changes
  bool setDuplex;
  // unsetMirror  (always)
  bool unsetCollate;

  // helper functions
  bool withPage(int outno) const; // 1 based
  void dump() const;
};

#include <stdio.h>
#include <memory>

enum ArgOwnership { WillStayAlive,MustDuplicate,TakeOwnership };

class PDFTOPDF_PageHandle {
public:
  virtual ~PDFTOPDF_PageHandle() {}
  virtual PageRect getRect() const =0;
  virtual void add_border_rect(const PageRect &rect,BorderType border,float fscale) =0;
  virtual void add_subpage(const std::shared_ptr<PDFTOPDF_PageHandle> &sub,float xpos,float ypos,float scale) =0; // or simply: const NupPageEdit &edit
  virtual void mirror() =0;
  virtual void rotate(Rotation rot) =0;
};

// TODO: ... error output?
class PDFTOPDF_Processor { // abstract interface
public:
  virtual ~PDFTOPDF_Processor() {}

// TODO: problem qpdf wants password at load time
  virtual bool loadFile(FILE *f,ArgOwnership take=WillStayAlive) =0;
  virtual bool loadFilename(const char *name) =0;

// TODO: virtual bool may_modify/may_print/?
  virtual bool check_print_permissions() =0;

  virtual std::vector<std::shared_ptr<PDFTOPDF_PageHandle>> get_pages() =0; // shared_ptr because of type erasure (deleter)

 // virtual bool setProcess(const ProcessingParameters &param) =0;

// TODO: landscape
  virtual std::shared_ptr<PDFTOPDF_PageHandle> new_page(float width,float height) =0;

  virtual void add_page(std::shared_ptr<PDFTOPDF_PageHandle> page,bool front) =0; // at back/front -- either from get_pages() or new_page()+add_subpage()-calls  (or [also allowed]: empty)

//  void remove_page(std::shared_ptr<PDFTOPDF_PageHandle> ph);  // not needed: we construct from scratch, at least conceptually.

  virtual void multiply(int copies) =0;

  virtual void addCM(const char *defaulticc,const char *outputicc) =0;

  virtual void emitFile(FILE *dst,ArgOwnership take=WillStayAlive) =0;
  virtual void emitFilename(const char *name) =0; // NULL -> stdout
};


class PDFTOPDF_Factory {
public:
  // never NULL, but may throw.
  static PDFTOPDF_Processor *processor();
};

// This is all we want: 
bool processPDFTOPDF(PDFTOPDF_Processor &proc,const ProcessingParameters &param);


#endif
