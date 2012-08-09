#include "qpdf_pdftopdf_processor.h"
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <stdexcept>
#include <qpdf/QPDFWriter.hh>
#include <qpdf/QUtil.hh>
#include "qpdf_tools.h"
#include "qpdf_xobject.h"
#include "qpdf_pdftopdf.h"

// Use: content.append(debug_box(pe.sub,xpos,ypos));
static std::string debug_box(const PageRect &box,float xshift,float yshift) // {{{ 
{
  return std::string("q 1 w 0.1 G\n ")+
         QUtil::double_to_string(box.left+xshift)+" "+QUtil::double_to_string(box.top+yshift)+" m  "+
         QUtil::double_to_string(box.right+xshift)+" "+QUtil::double_to_string(box.bottom+yshift)+" l "+"S \n "+

         QUtil::double_to_string(box.right+xshift)+" "+QUtil::double_to_string(box.top+yshift)+" m  "+
         QUtil::double_to_string(box.left+xshift)+" "+QUtil::double_to_string(box.bottom+yshift)+" l "+"S \n "+

         QUtil::double_to_string(box.left+xshift)+" "+QUtil::double_to_string(box.top+yshift)+"  "+
         QUtil::double_to_string(box.right-box.left)+" "+QUtil::double_to_string(box.bottom-box.top)+" re "+"S Q\n";
}
// }}}

QPDF_PDFTOPDF_PageHandle::QPDF_PDFTOPDF_PageHandle(QPDFObjectHandle page,int orig_no) // {{{
  : page(page),no(orig_no)
{
}
// }}}

QPDF_PDFTOPDF_PageHandle::QPDF_PDFTOPDF_PageHandle(QPDF *pdf,float width,float height) // {{{
  : no(0)
{
  assert(pdf);
  page=QPDFObjectHandle::parse(
    "<<"
    "  /Type /Page"
    "  /Resources <<"
    "    /XObject null "
    "  >>"
    "  /MediaBox null "
    "  /Contents null "
    ">>");
  page.replaceKey("/MediaBox",makeBox(0,0,width,height));
  page.replaceKey("/Contents",QPDFObjectHandle::newStream(pdf));
  // xobjects: later (in get())
  content.assign("q\n");  // TODO? different/not needed

//  page=pdf->makeIndirectObject(page); // stores *pdf 
}
// }}}

PageRect QPDF_PDFTOPDF_PageHandle::getRect() const // {{{
{
  page.assertInitialized();
  return getBoxAsRect(getTrimBox(page));
}
// }}}

bool QPDF_PDFTOPDF_PageHandle::isExisting() const // {{{
{
  page.assertInitialized();
  return content.empty();
}
// }}}

QPDFObjectHandle QPDF_PDFTOPDF_PageHandle::get() // {{{
{
  QPDFObjectHandle ret=page;
  if (!isExisting()) { // finish up page
    page.getKey("/Resources").replaceKey("/XObject",QPDFObjectHandle::newDictionary(xobjs));
    content.append("Q\n");
    page.getKey("/Contents").replaceStreamData(content,QPDFObjectHandle::newNull(),QPDFObjectHandle::newNull());
  }
  page=QPDFObjectHandle(); // i.e. uninitialized
  return ret;
}
// }}}

// TODO? for non-existing (either drop comment or facility to create split streams needed)
void QPDF_PDFTOPDF_PageHandle::add_border_rect(const PageRect &rect,BorderType border) // {{{
{
  assert(isExisting());
  static const char *pre="%pdftopdf q\n"
                         "q\n",
                    *post="%pdftopdf Q\n"
                          "Q\n";

  std::string boxcmd="q 1 w 0.1 G \n 10 10 100 100 re S \nQ\n";

// if (!isExisting()) {
//   // TODO: only after 
//   return;
// }

  std::vector<QPDFObjectHandle> cntnt=page.getPageContents();
  if (cntnt.size()>=3) {
    // check cntnt.front() and cntnt.back()
    PointerHolder<Buffer> pbuf_f=cntnt.front().getStreamData();
    PointerHolder<Buffer> pbuf_b=cntnt.back().getStreamData();
    if ( (pbuf_f->getSize()>12)&&(strncmp((char *)pbuf_f->getBuffer(),pre,12)==0)&& 
         (pbuf_b->getSize()>12)&&(strncmp((char *)pbuf_b->getBuffer(),post,12)==0) ) {
      // leave "stm1" alone, replace stm2
      cntnt.back().replaceStreamData(std::string(post)+boxcmd,QPDFObjectHandle::newNull(),QPDFObjectHandle::newNull());
      boxcmd.clear();
    }
  }
  if (!boxcmd.empty()) {
    assert(page.getOwningQPDF()); // existing pages are always indirect
    QPDFObjectHandle stm1=QPDFObjectHandle::newStream(page.getOwningQPDF(),pre),
                     stm2=QPDFObjectHandle::newStream(page.getOwningQPDF(),std::string(post)+boxcmd);

    page.addPageContents(stm1,true); // before
    page.addPageContents(stm2,false); // after
  }
}
// }}}

void QPDF_PDFTOPDF_PageHandle::add_subpage(const std::shared_ptr<PDFTOPDF_PageHandle> &sub,float xpos,float ypos,float scale) // {{{
{
  auto qsub=dynamic_cast<QPDF_PDFTOPDF_PageHandle *>(sub.get());
  assert(qsub);

  std::string xoname="/X"+QUtil::int_to_string((qsub->no!=-1)?qsub->no:++no);
  xobjs[xoname]=makeXObject(qsub->page.getOwningQPDF(),qsub->page); // trick: should be the same as page->getOwningQPDF() [only after made indirect]

// TODO: -left/-right needs to be added back? (here?)
  content.append("q\n  ");
  content.append(QUtil::double_to_string(scale)+" 0 0 "+
                 QUtil::double_to_string(scale)+" "+
                 QUtil::double_to_string(xpos)+" "+
                 QUtil::double_to_string(ypos)+" cm\n  ");
  content.append(xoname+" Do\n");
  content.append("Q\n");
}
// }}}

void QPDF_PDFTOPDF_PageHandle::debug(const PageRect &rect,float xpos,float ypos) // {{{
{
  assert(!isExisting());
  content.append(debug_box(rect,xpos,ypos));
}
// }}}


void QPDF_PDFTOPDF_Processor::closeFile() // {{{
{
  pdf.reset();
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
    pdf.reset(new QPDF);
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
  start();
  return true;
}
// }}}

bool QPDF_PDFTOPDF_Processor::loadFilename(const char *name) // {{{
{
  closeFile();
  try {
    pdf.reset(new QPDF);
    pdf->processFile(name);
  } catch (const std::exception &e) {
    error("loadFilename failed: %s",e.what());
    return false;
  }
  start();
  return true;
}
// }}}


void QPDF_PDFTOPDF_Processor::start() // {{{
{
  assert(pdf);

  orig_pages=pdf->getAllPages();

  // remove them (just unlink, data still there)
  const int len=orig_pages.size();
  for (int iA=0;iA<len;iA++) {
    pdf->removePage(orig_pages[iA]);
  }

  // we remove stuff that becomes defunct (probably)  TODO
  pdf->getRoot().removeKey("/PageMode");
  pdf->getRoot().removeKey("/Outlines");
  pdf->getRoot().removeKey("/OpenAction");
  pdf->getRoot().removeKey("/PageLabels");
}
// }}}

bool QPDF_PDFTOPDF_Processor::check_print_permissions() // {{{
{
  if (!pdf) {
    error("No PDF loaded");
    return false;
  }
  return pdf->allowPrintHighRes() || pdf->allowPrintLowRes(); // from legacy pdftopdf
}
// }}}


std::vector<std::shared_ptr<PDFTOPDF_PageHandle>> QPDF_PDFTOPDF_Processor::get_pages() // {{{
{
  std::vector<std::shared_ptr<PDFTOPDF_PageHandle>> ret;
  if (!pdf) {
    error("No PDF loaded");
    assert(0);
    return ret;
  }
  const int len=orig_pages.size();
  ret.reserve(len);
  for (int iA=0;iA<len;iA++) {
    ret.push_back(std::shared_ptr<PDFTOPDF_PageHandle>(new QPDF_PDFTOPDF_PageHandle(orig_pages[iA],iA+1)));
  }
  return ret;
}
// }}}

std::shared_ptr<PDFTOPDF_PageHandle> QPDF_PDFTOPDF_Processor::new_page(float width,float height) // {{{
{
  if (!pdf) {
    error("No PDF loaded");
    assert(0);
    return std::shared_ptr<PDFTOPDF_PageHandle>();
  }
  return std::shared_ptr<QPDF_PDFTOPDF_PageHandle>(new QPDF_PDFTOPDF_PageHandle(pdf.get(),width,height));
  // return std::make_shared<QPDF_PDFTOPDF_PageHandle>(pdf.get(),width,height);  // problem: make_shared not friend
}
// }}}

void QPDF_PDFTOPDF_Processor::add_page(std::shared_ptr<PDFTOPDF_PageHandle> page) // {{{
{
  assert(pdf);
  auto qpage=dynamic_cast<QPDF_PDFTOPDF_PageHandle *>(page.get());
  if (qpage) {
    pdf->addPage(qpage->get(),false);
  }
}
// }}}

#if 0
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
#endif

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
