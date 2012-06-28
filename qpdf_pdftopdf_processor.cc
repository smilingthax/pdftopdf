#include "qpdf_pdftopdf_processor.h"
#include <stdio.h>
#include <stdarg.h>
//#include <assert.h>
#include <stdexcept>
#include <qpdf/QPDFWriter.hh>

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
  fputs("\n",stderr);
  va_end(ap);
}
// }}}

// TODO?  try/catch for PDF parsing errors?

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
    try {
      pdf->processFile("temp file",f,false);
    } catch (const std::exception &e) {
      error("loadFile failed: %s",e.what());
      return false;
    }
    break;
  case TakeOwnership:
    try {
      pdf->processFile("temp file",f,true);
    } catch (const std::exception &e) {
      error("loadFile failed: %s",e.what());
      return false;
    }
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
  try {
    pdf=new QPDF;
    pdf->processFile(name);
  } catch (const std::exception &e) {
    error("loadFilename failed: %s",e.what());
    return false;
  }
  return true;
}
// }}}

bool QPDF_PDFTOPDF_Processor::setProcess(const ProcessingParameters &param) // {{{
{
  if (!pdf) {
    error("No PDF loaded");
    return false;
  }
/*
  ..
  .
*/
error("TODO setProcess");
//  ...
return true;
  return false;
}
// }}}

void QPDF_PDFTOPDF_Processor::emitFile(FILE *f,ArgOwnership take) // {{{
{
  if (!pdf) {
    return;
  }
  QPDFWriter out(*pdf);
  switch (take) {
  case WillStayAlive:
    out.setOutputFile(f,false);
    break;
  case TakeOwnership:
    out.setOutputFile(f,true);
    break;
  case MustDuplicate:
    error("emitFile with MustDuplicate is not supported");
    return;
  }
  out.write();
}
// }}}

void QPDF_PDFTOPDF_Processor::emitFilename(const char *name) // {{{
{
  if (!pdf) {
    return;
  }
  // special case: name==NULL -> stdout
  QPDFWriter out(*pdf,name);
  out.write();
}
// }}}

  // TODO:
  //   loadPDF();   success?
  //   okToPrint()?
