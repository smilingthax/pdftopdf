#include "qpdf_pdftopdf.h"

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

#include "qpdf_tools.h"

QPDFObjectHandle getRectAsBox(const PageRect &rect) // {{{
{
  return makeBox(rect.left,rect.top,rect.right,rect.bottom);
}
// }}}

