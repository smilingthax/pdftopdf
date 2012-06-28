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

      xpos(CENTER),ypos(CENTER),

      emitJCL(true),deviceCopies(1),
      setDuplex(false)
  {
    page.width=612.0; // letter
    page.height=792.0;
    page.top=page.height-36.0;
    page.bottom=36.0;
    page.left=18.0;
    page.right=page.width-18.0;
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

  Position xpos,ypos;
/*
  ...
    collate
  ...

  evenDuplex (was: even)

  ??? shuffle 
*/
  bool emitJCL;
  int deviceCopies;

  // ppd changes
  bool setDuplex;

  void dump() const;
};

#include <stdio.h>

enum ArgOwnership { WillStayAlive,MustDuplicate,TakeOwnership };

// TODO: ... error output?
class PDFTOPDF_Processor { // abstract interface
public:
  virtual ~PDFTOPDF_Processor() {}

// TODO: virtual bool may_modify/may_print/?

// TODO: problem qpdf wants password at load time

  virtual bool loadFile(FILE *f,ArgOwnership take=WillStayAlive) =0;
  virtual bool loadFilename(const char *name) =0;

  virtual bool setProcess(const ProcessingParameters &param) =0;
  virtual void emitFile(FILE *dst,ArgOwnership take=WillStayAlive) =0;
  virtual void emitFilename(const char *name) =0; // NULL -> stdout
};

class PDFTOPDF_Factory {
public:
  // never NULL, but may throw.
  static PDFTOPDF_Processor *processor();
};

#endif
