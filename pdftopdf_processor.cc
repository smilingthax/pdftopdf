#include "pdftopdf_processor.h"
#include "qpdf_pdftopdf_processor.h"
#include <stdio.h>
#include <assert.h>

void ProcessingParameters::dump() const // {{{
{
  printf("jobId: %d, nupCopies: %d\n",
         jobId,numCopies);
  printf("user: %s, title: %s\n",
         (user)?user:"(null)",(title)?title:"(null)");
  printf("fitplot: %s\n",
         (fitplot)?"true":"false");

  page.dump();

  printf("Rotation(CCW): ");
  Rotation_dump(orientation);
  printf("\n");

  printf("duplex: %s\n",
         (duplex)?"true":"false");

  printf("Border: ");
  BorderType_dump(border);
  printf("\n");

  nup.dump();

  printf("reverse: %s\n",
         (reverse)?"true":"false");

  printf("evenPages: %s, oddPages: %s\n",
         (evenPages)?"true":"false",
         (oddPages)?"true":"false");

  printf("page range: ");
  pageRange.dump();

  printf("mirror: %s\n",
         (mirror)?"true":"false");

  printf("Position: ");
  Position_dump(xpos,Axis::X);
  printf("/");
  Position_dump(ypos,Axis::Y);
  printf("\n");

  printf("collate: %s\n",
         (collate)?"true":"false");
/*
  // std::string pageLabel; // or NULL?  must stay/dup!
  ...
  ...

  ??? shuffle 
*/
  printf("evenDuplex: %s\n",
         (evenDuplex)?"true":"false");

  printf("emitJCL: %s\n",
         (emitJCL)?"true":"false");
  printf("deviceCopies: %d\n",
         deviceCopies);
  printf("setDuplex: %s\n",
         (setDuplex)?"true":"false");
  printf("unsetCollate: %s\n",
         (unsetCollate)?"true":"false");
}
// }}}

PDFTOPDF_Processor *PDFTOPDF_Factory::processor()
{
  return new QPDF_PDFTOPDF_Processor();
}

