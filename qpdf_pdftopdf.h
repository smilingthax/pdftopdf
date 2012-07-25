#ifndef QPDF_PDFTOPDF_H
#define QPDF_PDFTOPDF_H

#include <qpdf/QPDFObjectHandle.hh>
#include "pptypes.h"

// helper functions

PageRect getBoxAsRect(QPDFObjectHandle box);
QPDFObjectHandle getRectAsBox(const PageRect &rect);

#endif
