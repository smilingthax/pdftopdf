#include "pptypes.h"
#include <utility>
#include <stdio.h>
#include <assert.h>

void Position_dump(Position pos) // {{{
{
  static const char *pstr[3]={"Left/Bottom","Center","Right/Top"};
  if ( (pos<LEFT)||(pos>RIGHT) ) {
    printf("(bad position: %d)",pos);
  } else {
    fputs(pstr[pos+1],stdout);
  }
}
// }}}

void Position_dump(Position pos,Axis axis) // {{{
{
  assert( (axis==Axis::X)||(axis==Axis::Y) );
  if ( (pos<LEFT)||(pos>RIGHT) ) {
    printf("(bad position: %d)",pos);
    return;
  }
  if (axis==Axis::X) {
    static const char *pxstr[3]={"Left","Center","Right"};
    fputs(pxstr[pos+1],stdout);
  } else {
    static const char *pystr[3]={"Bottom","Center","Top"};
    fputs(pystr[pos+1],stdout);
  }
}
// }}}

void PageRect::rotate(Rotation r) // {{{
{
  if (r>=ROT_180) {
    std::swap(top,bottom);
    std::swap(left,right);
  }
  if ( (r==ROT_90)||(r==ROT_270) ) {
    const float tmp=bottom;
    bottom=left;
    left=top;
    top=right;
    right=tmp;
    // TODO? std::swap(width,height);
  }
}
// }}}

void PageRect::set(const PageRect &rhs) // {{{
{
  if (!isnan(rhs.top)) top=rhs.top;
  if (!isnan(rhs.left)) left=rhs.left;
  if (!isnan(rhs.right)) right=rhs.right;
  if (!isnan(rhs.bottom)) bottom=rhs.bottom;
}
// }}}

void PageRect::dump() const // {{{
{
  printf("top: %f, left: %f, right: %f, bottom: %f\n"
         "width: %f, height: %f\n",
         top,left,right,bottom,
         width,height);
}
// }}}

