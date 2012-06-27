#ifndef QPDF_PDFTOPDF_PROCESSOR_H
#define QPDF_PDFTOPDF_PROCESSOR_H

#include "pdftopdf_processor.h"
#include <qpdf/QPDF.hh>

class QPDF_PDFTOPDF_Processor : public PDFTOPDF_Processor {
public:
  QPDF_PDFTOPDF_Processor();
  ~QPDF_PDFTOPDF_Processor();

  bool loadFile(FILE *f,ArgOwnership take=WillStayAlive);
  bool loadFilename(const char *name);

  bool setProcess(const ProcessingParameters &param);
  void emitFile(FILE *dst);

private:
  void closeFile();
  void error(const char *fmt,...);
private:
  QPDF *pdf; // TODO: std::unique_ptr
};

#endif
