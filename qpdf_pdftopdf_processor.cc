#include "qpdf_pdftopdf_processor.h"
#include <stdio.h>
#include <stdarg.h>
//#include <assert.h>
#include <stdexcept>

QPDF_PDFTOPDF_Processor::QPDF_PDFTOPDF_Processor()
  : f(NULL)
{
}

QPDF_PDFTOPDF_Processor::~QPDF_PDFTOPDF_Processor()
{
  if (f) {
    fclose(f);
  }
}

void QPDF_PDFTOPDF_Processor::closeFile() // {{{
{
  if (f) {
    fclose(f);
    f=NULL;
  }
}
// }}}

void QPDF_PDFTOPDF_Processor::error(const char *fmt,...) // {{{
{
  va_list ap;

  va_start(ap,fmt);
  vfprintf(stderr,fmt,ap);
  va_end(ap);
}
// }}}

bool QPDF_PDFTOPDF_Processor::loadFile(FILE *f,ArgOwnership take) // {{{
{
  closeFile();
  if (!f) {
    throw std::invalid_argument("loadFile(NULL,...) not allowed");
  }
  switch (take) {
  case WillStayAlive:
    // just open PDF, don't store f 
// TODO open pdf
    break;
  case TakeOwnership:
// TODO open pdf  or close f
    this->f=f;
    break;
  case MustDuplicate:
    error("loadFile with MustDuplicate is not supported");
    return false;
  }
  return false;
}
// }}}

bool QPDF_PDFTOPDF_Processor::loadFilename(const char *name) // {{{
{
  FILE *f=fopen(name,"rb");
  if (!f) {
    error("Could not open file: %s",name);
    return false;
  }
  return loadFile(f,TakeOwnership);
}
// }}}

bool QPDF_PDFTOPDF_Processor::setProcess(const ProcessingParameters &param) // {{{
{
/*
  if (!pdf) {
    error("No PDF loaded");
    return false;
  }
*/
//  ...
  return false;
}
// }}}

void QPDF_PDFTOPDF_Processor::emitFile(FILE *f) // {{{
{
/*
  if (!pdf) {
    return;
  }
*/
//  ...
}
// }}}

  // TODO:
  //   loadPDF();   success?
  //   okToPrint()?
