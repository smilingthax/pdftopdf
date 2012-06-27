#include "qpdf_pdftopdf_processor.h"
#include <stdio.h>
#include <stdarg.h>
//#include <assert.h>
#include <stdexcept>

QPDF_PDFTOPDF_Processor::QPDF_PDFTOPDF_Processor()
  : pdf(NULL)
{
}

QPDF_PDFTOPDF_Processor::~QPDF_PDFTOPDF_Processor()
{
  if (pdf) {
    delete pdf;
  }
}

void QPDF_PDFTOPDF_Processor::closeFile() // {{{
{
  if (pdf) {
    delete pdf;
    pdf=NULL;
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
  try {
    pdf=new QPDF;
  } catch (...) {
    if (take==TakeOwnership) {
      fclose(f);
    }
    throw;
  }
  switch (take) {
  case WillStayAlive:
    pdf->processFile("temp file",f,false);
    break;
  case TakeOwnership:
    pdf->processFile("temp file",f,true);
    break;
  case MustDuplicate:
    error("loadFile with MustDuplicate is not supported");
    return false;
  }
  return true;
}
// }}}

bool QPDF_PDFTOPDF_Processor::loadFilename(const char *name) // {{{
{
  closeFile();
  pdf=new QPDF;
  pdf->processFile(name);
  return true;
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
