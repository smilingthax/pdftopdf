#include "qpdf_pdftopdf_processor.h"
#include <stdio.h>
#include <stdarg.h>
//#include <assert.h>
#include <stdexcept>
#include <qpdf/QPDFWriter.hh>
#include <qpdf/QUtil.hh>
#include "qpdf_tools.h"
#include "qpdf_xobject.h"

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

PageRect getBoxAsRect(QPDFObjectHandle box);
PageRect getBoxAsRect(QPDFObjectHandle box) // {{{
{
  PageRect ret;

  ret.left=box.getArrayItem(0).getNumericValue();
  ret.top=box.getArrayItem(1).getNumericValue();
  ret.right=box.getArrayItem(2).getNumericValue();
  ret.bottom=box.getArrayItem(3).getNumericValue();

  ret.width=ret.right-ret.left;
  ret.height=ret.bottom-ret.top;

  return ret;
}
// }}}

#if 0
QPDFObjectHandle getRectAsBox(const PageRect &rect);
QPDFObjectHandle getRectAsBox(const PageRect &rect) // {{{
{
  QPDFObjectHandle ret=QPDFObjectHandle::newArray();
  ret.appendItem(QPDFObjectHandle::newReal(rect.left));
  ret.appendItem(QPDFObjectHandle::newReal(rect.top));
  ret.appendItem(QPDFObjectHandle::newReal(rect.right));
  ret.appendItem(QPDFObjectHandle::newReal(rect.bottom));
  return ret;
}
// }}}
#endif

bool QPDF_PDFTOPDF_Processor::setProcess(const ProcessingParameters &param) // {{{
{
  if (!pdf) {
    error("No PDF loaded");
    return false;
  }

// TODO: -left/-right needs to be subtracted from param.nup.width/height

  // make copy
  std::vector<QPDFObjectHandle> pages=pdf->getAllPages();
  const int numPages=pages.size();

  std::map<std::string,QPDFObjectHandle> xobjs;
  std::string content;

  double xpos=param.page.left,
         ypos=param.page.bottom; // for whole page... TODO from position...
  NupState nupstate(param.nup);
  NupPageEdit pe;
  for (int iA=0;iA<numPages;iA++) {
//    QPDFObjectHandle box=getTrimBox(pages[iA]);
    PageRect rect=getBoxAsRect(getTrimBox(pages[iA]));
//    rect.dump();

    bool newPage=nupstate.nextPage(rect.width,rect.height,pe);
//    printf("%d\n",newPage);
    if ( (newPage)&&(!content.empty()) ) {
//      auto npage=makePage(*pdf,xobjs,getRectAsBox(param.page),content);
      auto npage=makePage(*pdf,xobjs,makeBox(0,0,param.page.width,param.page.height),content);
      pdf->addPage(npage,false);

      content.clear();
      xobjs.clear();
    }

// TODO: add frame, possibly already to original page?
    std::string xoname="/X"+QUtil::int_to_string(iA+1);
    xobjs[xoname]=makeXObject(pdf,pages[iA]);
    pdf->removePage(pages[iA]);

// TODO: -left/-right needs to be added back?
    content.append("q\n  ");
    content.append(QUtil::double_to_string(pe.scale)+" 0 0 "+
                   QUtil::double_to_string(pe.scale)+" "+
                   QUtil::double_to_string(pe.xpos+xpos)+" "+
                   QUtil::double_to_string(pe.ypos+ypos)+" cm\n  ");
    content.append(xoname+" Do\n");
    content.append("Q\n");
#if 1
content.append(
  "q 1 w 0.1 G\n "+
  QUtil::double_to_string(pe.sub.left+xpos)+" "+QUtil::double_to_string(pe.sub.top+ypos)+" m  "+
  QUtil::double_to_string(pe.sub.right+xpos)+" "+QUtil::double_to_string(pe.sub.bottom+ypos)+" l "+"S \n "+

  QUtil::double_to_string(pe.sub.right+xpos)+" "+QUtil::double_to_string(pe.sub.top+ypos)+" m  "+
  QUtil::double_to_string(pe.sub.left+xpos)+" "+QUtil::double_to_string(pe.sub.bottom+ypos)+" l "+"S \n "+

  QUtil::double_to_string(pe.sub.left+xpos)+" "+QUtil::double_to_string(pe.sub.top+ypos)+"  "+
  QUtil::double_to_string(pe.sub.right-pe.sub.left)+" "+QUtil::double_to_string(pe.sub.bottom-pe.sub.top)+" re "+"S Q\n");
#endif

//    pe.dump();
  }
  if (!content.empty()) {
//    auto npage=makePage(*pdf,xobjs,getRectAsBox(param.page),content);
    auto npage=makePage(*pdf,xobjs,makeBox(0,0,param.page.width,param.page.height),content);
    pdf->addPage(npage,false);
  }
/*

  ..
  .
*/
error("TODO setProcess");
//  ...

  // we remove stuff now probably defunct  TODO
  pdf->getRoot().removeKey("/PageMode");
  pdf->getRoot().removeKey("/Outlines");
  pdf->getRoot().removeKey("/OpenAction");
  pdf->getRoot().removeKey("/PageLabels");

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
    out.setOutputFile("temp file",f,false);
    break;
  case TakeOwnership:
    out.setOutputFile("temp file",f,true);
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
