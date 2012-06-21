#ifndef NUP_H_
#define NUP_H_

#include "pptypes.h"
#include <utility>

enum BorderType { NONE=0, ONE_THIN=2, ONE_THICK=3, TWO_THIN=4, TWO_THICK=5,
                  ONE=0x02, TWO=0x04, THICK=0x01};
void BorderType_dump(BorderType border);

struct NupParameters {
  NupParameters() 
    : nupX(1),nupY(1),
      width(NAN),height(NAN),
      first(X),
      xstart(CENTER),ystart(CENTER),
      border(NONE),
      xalign(CENTER),yalign(CENTER)
  {}

  // --- "calculated" parameters ---
  int nupX,nupY;
  float width,height;
  // landscape?

  // --- other settings ---
  // ordering
  Axis first;
  Position xstart,ystart;

  BorderType border;

  Position xalign,yalign;

  static bool possible(int nup); // TODO?  float in_ratio,float out_ratio
  static float calculate(int nup, float in_ratio, float out_ratio,NupParameters &ret); // returns "quality", 1 is best

  void dump() const;
};

struct NupPageEdit {
  // required transformation: first translate, then scale
  float xpos,ypos;  // TODO:  already given by sub.left,sub.bottom    [but for rotation?]
  float scale; // uniform

// ? "landscape"  e.g. to rotate labels

  // for border, clip, ...
  // also stores in_width/in_height, unscaled!
  // everything in "outer"-page coordinates
  PageRect sub;

  BorderType border; // just copied, for convenience // TODO?

  void dump() const;
};

class NupState {
public:
  NupState(const NupParameters &param);

  // will overwrite ret with the new parameters
  // returns true, if a new output page should be started first
  bool nextPage(float in_width,float in_height,NupPageEdit &ret);

private:
  std::pair<int,int> convert_order(int subpage) const;
  void calculate_edit(int subx,int suby,NupPageEdit &ret) const;
private:
  NupParameters param;

  int in_pages,out_pages;
  int nup; // max. per page (==nupX*nupY)
  int subpage; // on the current output-page
};

// TODO? elsewhere
// parsing functions for cups parameters (will not calculate nupX,nupY!)
bool parseNupLayout(const char *val,NupParameters &ret); // lrtb, btlr, ...
bool parseNupBorder(const char *val,NupParameters &ret); // none,single,...,double-thick

#endif
