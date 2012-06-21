#ifndef PPTYPES_H_
#define PPTYPES_H_

#include <cmath> // NAN

// namespace PPTypes {}   TODO?

enum Axis { X, Y };
enum Position { CENTER=0, LEFT=-1, RIGHT=1, TOP=1, BOTTOM=-1 }; // PS order
enum Rotation { ROT_0, ROT_90, ROT_180, ROT_270 };  // CCW

void Position_dump(Position pos);
void Position_dump(Position pos,Axis axis);

struct PageRect {
  PageRect() : top(NAN),left(NAN),right(NAN),bottom(NAN),width(NAN),height(NAN) {}
  float top,left,right,bottom; // i.e. margins
  float width,height;

  void rotate(Rotation r);
  void set(const PageRect &rhs); // only for rhs.* != NAN
  void dump() const;
};

#endif
