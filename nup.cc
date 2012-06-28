#include "nup.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <utility>

void NupParameters::dump() const // {{{
{
  printf("NupX: %d, NupY: %d\n"
         "width: %f, height: %f\n",
         nupX,nupY,
         width,height);

  int opos=-1,fpos=-1,spos=-1;
  if (xstart==Position::LEFT) { // or Bottom
    fpos=0;
  } else if (xstart==Position::RIGHT) { // or Top
    fpos=1;
  }
  if (ystart==Position::LEFT) { // or Bottom
    spos=0;
  } else if (ystart==Position::RIGHT) { // or Top
    spos=1;
  }
  if (first==Axis::X) {
    printf("First Axis: X\n");
    opos=0;
  } else if (first==Axis::Y) {
    printf("First Axis: Y\n");
    opos=2;
    std::swap(fpos,spos);
  }

  if ( (opos==-1)||(fpos==-1)||(spos==-1) ) {
    printf("Bad Spec: %d; start: %d, %d\n\n",
           first,xstart,ystart);
  } else {
    static const char *order[4]={"lr","rl","bt","tb"};
    printf("Order: %s%s\n",
           order[opos+fpos],order[(opos+2)%4+spos]);
  }

  fputs("Alignment: ",stdout);
  Position_dump(xalign,Axis::X);
  fputs("/",stdout); 
  Position_dump(yalign,Axis::Y);
  fputs("\n",stdout);
}
// }}}

bool NupParameters::possible(int nup) // {{{
{
  // 1 2 3 4 6 8 9 10 12 14 15
  return (nup>=1)&&(nup<=16)&&
         ( (nup!=5)||(nup!=7)||(nup!=11)||(nup!=13) );
}
// }}}


NupState::NupState(const NupParameters &param) // {{{
  : param(param),
    in_pages(0),out_pages(0),
    nup(param.nupX*param.nupY),
    subpage(nup)
{
  assert( (param.nupX>0)&&(param.nupY>0) );
}
// }}}

void NupPageEdit::dump() const // {{{
{
  printf("xpos: %f, ypos: %f, scale: %f\n",
         xpos,ypos,scale);
  sub.dump();
}
// }}}

std::pair<int,int> NupState::convert_order(int subpage) const // {{{
{
  int subx,suby;
  if (param.first==Axis::X) {
    subx=subpage%param.nupX;
    suby=subpage/param.nupX;
  } else {
    subx=subpage/param.nupY;
    suby=subpage%param.nupY;
  }

  subx=(param.nupX-1)*(param.xstart+1)/2-param.xstart*subx;
  suby=(param.nupY-1)*(param.ystart+1)/2-param.ystart*suby;

  return std::make_pair(subx,suby);
}
// }}}

static inline float lin(Position pos,float size) // {{{
{
  if (pos==-1) return 0;
  else if (pos==0) return size/2;
  else if (pos==1) return size;
  return size*(pos+1)/2;
}
// }}}

void NupState::calculate_edit(int subx,int suby,NupPageEdit &ret) const // {{{
{
  // dimensions of a "nup cell"
  const float width=param.width/param.nupX,
              height=param.height/param.nupY;

  // first calculate only for bottom-left corner
  ret.xpos=subx*width;
  ret.ypos=suby*height;

  const float scalex=width/ret.sub.width,
              scaley=height/ret.sub.height;
  float subwidth=ret.sub.width*scaley,
        subheight=ret.sub.height*scalex;

  // TODO?  if ( (!fitPlot)&&(ret.scale>1) ) ret.scale=1.0;
  if (scalex>scaley) {
    ret.scale=scaley;
    subheight=height;
    ret.xpos+=lin(param.xalign,width-subwidth);
  } else {
    ret.scale=scalex;
    subwidth=width;
    ret.ypos+=lin(param.yalign,height-subheight);
  }

  ret.sub.left=ret.xpos;
  ret.sub.bottom=ret.ypos;
  ret.sub.right=ret.sub.left+subwidth;
  ret.sub.top=ret.sub.bottom+subheight;
}
// }}}

bool NupState::nextPage(float in_width,float in_height,NupPageEdit &ret) // {{{
{
  in_pages++;
  subpage++;
  if (subpage>=nup) {
    subpage=0;
    out_pages++;
  }

  ret.sub.width=in_width;
  ret.sub.height=in_height;

  auto sub=convert_order(subpage);
  calculate_edit(sub.first,sub.second,ret);

  return (subpage==0);
}
// }}}


static std::pair<Axis,Position> parsePosition(char a,char b) // {{{ returns ,CENTER(0) on invalid
{
  a|=0x20; // make lowercase
  b|=0x20;
  if ( (a=='l')&&(b=='r') ) {
    return std::make_pair(Axis::X,Position::LEFT);
  } else if ( (a=='r')&&(b=='l') ) {
    return std::make_pair(Axis::X,Position::RIGHT);
  } else if ( (a=='t')&&(b=='b') ) {
    return std::make_pair(Axis::Y,Position::TOP);
  } else if ( (a=='b')&&(b=='t') ) {
    return std::make_pair(Axis::Y,Position::BOTTOM);
  } 
  return std::make_pair(Axis::X,Position::CENTER);
}
// }}}

bool parseNupLayout(const char *val,NupParameters &ret) // {{{
{
  assert(val);
  auto pos0=parsePosition(val[0],val[1]);
  if (pos0.second==CENTER) {
    return false;
  }
  auto pos1=parsePosition(val[2],val[3]);
  if ( (pos1.second==CENTER)||(pos0.first==pos1.first) ) {
    return false;
  }

  ret.first=pos0.first;
  if (ret.first==Axis::X) {
    ret.xstart=pos0.second;
    ret.ystart=pos1.second;
  } else {
    ret.xstart=pos1.second;
    ret.ystart=pos0.second;
  }

  return (val[4]==0); // everything seen?
}
// }}}

