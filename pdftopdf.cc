// Copyright (c) 2012 Tobias Hoffmann
//
// Copyright (c) 2006-2011, BBR Inc.  All rights reserved.
// MIT Licensed.

// TODO: check ppd==NULL (?)

#include <stdio.h>
#include <assert.h>
#include <cups/cups.h>
#include <cups/ppd.h>
#include <memory>

#include "pdftopdf_processor.h"
#include "pdftopdf_jcl.h"

#include <stdarg.h>
static void error(const char *fmt,...) // {{{
{
  va_list ap;
  va_start(ap,fmt);

  fputs("Error: ",stderr);
  vfprintf(stderr,fmt,ap);
  fputs("\n",stderr);

  va_end(ap);
}
// }}}

// namespace {}

struct PrinterFeatures {
  PrinterFeatures() : canCollate(false)
  {}
// TODO: ppd->manual_copies

  bool canCollate;
//  bool 
};

PrinterFeatures getPrinterFeatures(ppd_file_t *ppd)
{
  PrinterFeatures ret;
  if (!ppd) {
    return ret;
  }
/*

*/
  return ret;
}

void setFinalPPD(ppd_file_t *ppd,const ProcessingParameters &param)
{
  if ( (param.booklet==BOOKLET_ON)&&(ppdFindOption(ppd,"Duplex")) ) {
    // TODO: elsewhere, better
    ppdMarkOption(ppd,"Duplex","DuplexTumble");
    // TODO? sides=two-sided-short-edge
  }

  // for compatibility
  if ( (param.setDuplex)&&(ppdFindOption(ppd,"Duplex")!=NULL) ) {
    ppdMarkOption(ppd,"Duplex","True");
    ppdMarkOption(ppd,"Duplex","On");
  }

  // we do it, printer should not
  ppd_choice_t *choice;
  if ( (choice=ppdFindMarkedChoice(ppd,"MirrorPrint")) != NULL) {
    choice->marked=0;
  }

  // hardware can't collate the way we want it. Thus we collate; printer shall not
  if (param.unsetCollate) {
    ppdMarkOption(ppd,"Collate","False");
  }
/*
  ...
*/
}

// for choice, only overwrites ret if found in ppd
static bool ppdGetInt(ppd_file_t *ppd,const char *name,int *ret) // {{{
{
  assert(ret);
  ppd_choice_t *choice=ppdFindMarkedChoice(ppd,name); // !ppd is ok.
  if (choice) {
    *ret=atoi(choice->choice);
    return true;
  }
  return false;
}
// }}}

static bool optGetInt(const char *name,int num_options,cups_option_t *options,int *ret) // {{{
{
  assert(ret);
  const char *val=cupsGetOption(name,num_options,options);
  if (val) {
    *ret=atoi(val);
    return true;
  }
  return false;
}
// }}}

static bool optGetFloat(const char *name,int num_options,cups_option_t *options,float *ret) // {{{
{
  assert(ret);
  const char *val=cupsGetOption(name,num_options,options);
  if (val) {
    *ret=atof(val);
    return true;
  }
  return false;
}
// }}}

static bool is_false(const char *value) // {{{
{
  if (!value) {
    return false;
  }
  return (strcasecmp(value,"no")==0)||
         (strcasecmp(value,"off")==0)||
         (strcasecmp(value,"false")==0);
}
// }}}

static bool is_true(const char *value) // {{{
{
  if (!value) {
    return false;
  }
  return (strcasecmp(value,"yes")==0)||
         (strcasecmp(value,"on")==0)||
         (strcasecmp(value,"true")==0);
}
// }}}

static bool ppdGetDuplex(ppd_file_t *ppd) // {{{
{
  return ppdIsMarked(ppd,"Duplex","DuplexNoTumble")||
         ppdIsMarked(ppd,"Duplex","DuplexTumble")||
         ppdIsMarked(ppd,"JCLDuplex","DuplexNoTumble")||
         ppdIsMarked(ppd,"JCLDuplex","DuplexTumble")||
         ppdIsMarked(ppd,"EFDuplex","DuplexNoTumble")||
         ppdIsMarked(ppd,"EFDuplex","DuplexTumble")||
         ppdIsMarked(ppd,"KD03Duplex","DuplexNoTumble")||
         ppdIsMarked(ppd,"KD03Duplex","DuplexTumble");
}
// }}}

static bool ppdDefaultOrder(ppd_file_t *ppd) // {{{
{
  ppd_choice_t *choice;
  ppd_attr_t *attr;
  const char *val=NULL;

  // Figure out the right default output order from the PPD file...
  if ( (choice=ppdFindMarkedChoice(ppd,"OutputOrder")) != NULL) {
    val=choice->choice;
  } else if (  ( (choice=ppdFindMarkedChoice(ppd,"OutputBin")) != NULL)&&
               ( (attr=ppdFindAttr(ppd,"PageStackOrder",choice->choice)) != NULL)  ) {
    val=attr->value;
  } else if ( (attr=ppdFindAttr(ppd,"DefaultOutputOrder",0)) != NULL) {
    val=attr->value;
  }
  return (val)&&(strcasecmp(val,"Reverse")==0);
}
// }}}

static bool optGetCollate(int num_options,cups_option_t *options) // {{{
{
  if (is_true(cupsGetOption("Collate",num_options,options))) {
    return true;
  }

  const char *val=NULL;
  if ( (val=cupsGetOption("multiple-document-handling",num_options,options)) != NULL) {
   /* This IPP attribute is unnecessarily complicated:
    *   single-document, separate-documents-collated-copies, single-document-new-sheet:
    *      -> collate (true)
    *   separate-documents-uncollated-copies:
    *      -> can be uncollated (false)
    */
    return (strcasecmp(val,"separate-documents-uncollated-copies")!=0);
  }
  return false;
}
// }}}

static bool parsePosition(const char *value,Position &xpos,Position &ypos) // {{{
{
  // ['center','top','left','right','top-left','top-right','bottom','bottom-left','bottom-right']
  xpos=Position::CENTER;
  ypos=Position::CENTER;
  int next=0;
  if (strcasecmp(value,"center")==0) {
    return true;
  } else if (strncasecmp(value,"top",3)==0) {
    ypos=Position::TOP;
    next=3;
  } else if (strncasecmp(value,"bottom",6)==0) {
    ypos=Position::BOTTOM;
    next=6;
  }
  if (next) {
    if (value[next]==0) {
      return true;
    } else if (value[next]!='-') {
      return false;
    }
    value+=next+1;
  }
  if (strcasecmp(value,"left")==0) {
    xpos=Position::LEFT;
  } else if (strcasecmp(value,"right")==0) {
    xpos=Position::RIGHT;
  } else {
    return false;
  }
  return true;
}
// }}}

#include <ctype.h>
static void parseRanges(const char *range,IntervalSet &ret) // {{{
{
  ret.clear();
  if (!range) {
    ret.add(1); // everything
    ret.finish();
    return;
  }

  int lower,upper;
  while (*range) {
    if (*range=='-') {
      range++;
      upper=strtol(range,(char **)&range,10);
      ret.add(1,upper+1);
    } else {
      lower=strtol(range,(char **)&range,10);
      if (*range=='-') {
        range++;
        if (!isdigit(*range)) {
          ret.add(lower);
        } else {
          upper=strtol(range,(char **)&range,10);
          ret.add(lower,upper+1);
        }
      } else {
        ret.add(lower,lower+1);
      }
    }

    if (*range!=',') {
      break;
    }
    range++;
  }
  ret.finish();
}
// }}}

static bool parseBorder(const char *val,BorderType &ret) // {{{
{
  assert(val);
  if (strcasecmp(val,"none")==0) {
    ret=BorderType::NONE;
  } else if (strcasecmp(val,"single")==0) {
    ret=BorderType::ONE_THIN;
  } else if (strcasecmp(val,"single-thick")==0) {
    ret=BorderType::ONE_THICK;
  } else if (strcasecmp(val,"double")==0) {
    ret=BorderType::TWO_THIN;
  } else if (strcasecmp(val,"double-thick")==0) {
    ret=BorderType::TWO_THICK;
  } else {
    return false;
  }
  return true;
}
// }}}

void getParameters(ppd_file_t *ppd,int num_options,cups_option_t *options,ProcessingParameters &param) // {{{
{
  // param.numCopies initially from commandline
  if (param.numCopies==1) {
    ppdGetInt(ppd,"Copies",&param.numCopies);
  }
  if (param.numCopies==0) {
    param.numCopies=1;
  }

  const char *val;
  if ( (val=cupsGetOption("fitplot",num_options,options)) == NULL) {
    if ( (val=cupsGetOption("fit-to-page",num_options,options)) == NULL) {
      val=cupsGetOption("ipp-attribute-fidelity",num_options,options);
    }
  }
// TODO?  pstops checks =="true", pdftops !is_false  ... pstops says: fitplot only for PS (i.e. not for PDF, cmp. cgpdftopdf)
  param.fitplot=(val)&&(!is_false(val));

  if ( (ppd)&&(ppd->landscape>0) ) { // direction the printer rotates landscape (90 or -90)
    param.normal_landscape=ROT_90;
  } else {
    param.normal_landscape=ROT_270;
  }

  int ipprot;
  param.orientation=ROT_0;
  if ( (val=cupsGetOption("landscape",num_options,options)) != NULL) {
    if (!is_false(val)) {
      param.orientation=param.normal_landscape;
    }
  } else if (optGetInt("orientation-requested",num_options,options,&ipprot)) {
    /* IPP orientation values are:
     *   3: 0 degrees,  4: 90 degrees,  5: -90 degrees,  6: 180 degrees
     */
    if ( (ipprot<3)||(ipprot>6) ) {
      error("Bad value (%d) for orientation-requested, using 0 degrees",ipprot);
    } else {
      static const Rotation ipp2rot[4]={ROT_0, ROT_90, ROT_270, ROT_180};
      param.orientation=ipp2rot[ipprot-3];
    }
  }

  ppd_size_t *pagesize;
  // param.page default is letter, border 36,18
  if ( (pagesize=ppdPageSize(ppd,0)) != NULL) { // "already rotated"
    param.page.top=pagesize->top;
    param.page.left=pagesize->left;
    param.page.right=pagesize->right;
    param.page.bottom=pagesize->bottom;
    param.page.width=pagesize->width;
    param.page.height=pagesize->length;
  }

  PageRect tmp; // borders (before rotation)
  optGetFloat("page-top",num_options,options,&tmp.top);
  optGetFloat("page-left",num_options,options,&tmp.left);
  optGetFloat("page-right",num_options,options,&tmp.right);
  optGetFloat("page-bottom",num_options,options,&tmp.bottom);
  tmp.rotate(param.orientation);

  // NaN stays NaN
  tmp.right=param.page.width-tmp.right;
  tmp.top=param.page.height-tmp.top;
  param.page.set(tmp); // replace values, where tmp.* != NaN

  if (ppdGetDuplex(ppd)) {
    param.duplex=true;
  } else if (is_true(cupsGetOption("Duplex",num_options,options))) {
    param.duplex=true;
    param.setDuplex=true;
  } else if ( (val=cupsGetOption("sides",num_options,options)) != NULL) {
    if ( (strcasecmp(val,"two-sided-long-edge")==0)||
         (strcasecmp(val,"two-sided-short-edge")==0) ) {
      param.duplex=true;
      param.setDuplex=true;
    }
  }

  // default nup is 1
  int nup=1;
  if (optGetInt("number-up",num_options,options,&nup)) {
    if (!NupParameters::possible(nup)) {
      error("Unsupported number-up value %d, using number-up=1!",nup);
      nup=1;
    }
// TODO   ;  TODO? nup enabled? ... fitplot
//    NupParameters::calculate(nup,param.nup);
    NupParameters::preset(nup,param.nup);
  }

  if ( (val=cupsGetOption("number-up-layout",num_options,options)) != NULL) {
    if (!parseNupLayout(val,param.nup)) {
      error("Unsupported number-up-layout %s, using number-up-layout=lrtb!",val);
      param.nup.first=Axis::X;
      param.nup.xstart=Position::LEFT;
      param.nup.ystart=Position::TOP;
    }
  }

  if ( (val=cupsGetOption("page-border",num_options,options)) != NULL) {
    if (!parseBorder(val,param.border)) {
      error("Unsupported page-border value %s, using page-border=none!",val);
      param.border=BorderType::NONE;
    }
  }

  if ( (val=cupsGetOption("OutputOrder",num_options,options)) != NULL) {
    param.reverse=(strcasecmp(val,"Reverse")==0);
  } else if (ppd) {
    param.reverse=ppdDefaultOrder(ppd);
  }

  // TODO: pageLabel  (not used)
  // param.pageLabel=cupsGetOption("page-label",num_options,options);  // strdup?

  if ( (val=cupsGetOption("page-set",num_options,options)) != NULL) {
    if (strcasecmp(val,"even")==0) {
      param.oddPages=false;
    } else if (strcasecmp(val,"odd")==0) {
      param.evenPages=false;
    } else if (strcasecmp(val,"all")!=0) {
      error("Unsupported page-set value %s, using page-set=all!",val);
    }
  }

  if ( (val=cupsGetOption("page-ranges",num_options,options)) != NULL) {
    parseRanges(val,param.pageRange);
  }

  ppd_choice_t *choice;
  if ( (choice=ppdFindMarkedChoice(ppd,"MirrorPrint")) != NULL) {
    val=choice->choice;
  } else {
    val=cupsGetOption("mirror",num_options,options);
  }
  param.mirror=is_true(val);

  if ( (val=cupsGetOption("emit-jcl",num_options,options)) != NULL) {
    param.emitJCL=!is_false(val)&&(strcmp(val,"0")!=0);
  }

  param.booklet=BookletMode::BOOKLET_OFF;
  if ( (val=cupsGetOption("booklet",num_options,options)) != NULL) {
    if (strcasecmp(val,"shuffle-only")==0) {
      param.booklet=BookletMode::BOOKLET_JUSTSHUFFLE;
    } else if (is_true(val)) {
      param.booklet=BookletMode::BOOKLET_ON;
    } else if (!is_false(val)) {
      error("Unsupported booklet value %s, using booklet=off!",val);
    }
  }
  param.bookSignature=-1;
  if (optGetInt("booklet-signature",num_options,options,&param.bookSignature)) {
    if (param.bookSignature==0) {
      error("Unsupported booklet-signature value, using booklet-signature=-1 (all)!",val);
      param.bookSignature=-1;
    }
  }

  if ( (val=cupsGetOption("position",num_options,options)) != NULL) {
    if (!parsePosition(val,param.xpos,param.ypos)) {
      error("Unrecognized position value %s, using position=center!",val);
      param.xpos=Position::CENTER;
      param.ypos=Position::CENTER;
    }
  }

  param.collate=optGetCollate(num_options,options);
  // FIXME? pdftopdf also considers if ppdCollate is set (only when cupsGetOption is /not/ given) [and if is_true overrides param.collate=true]  -- pstops does not

/*
  // TODO: scaling
  // TODO: natural-scaling

  scaling


  if ((val = cupsGetOption("scaling",num_options,options)) != 0) {
    scaling = atoi(val) * 0.01;
    fitplot = gTrue;
  } else if (fitplot) {
    scaling = 1.0;
  }
  if ((val = cupsGetOption("natural-scaling",num_options,options)) != 0) {
    naturalScaling = atoi(val) * 0.01;
  }

bool checkFeature(const char *feature, int num_options, cups_option_t *options) // {{{
{
  const char *val;
  ppd_attr_t *attr;

  return ( (val=cupsGetOption(feature,num_options,options)) != NULL && is_true(val)) ||
         ( (attr=ppdFindAttr(ppd,feature,0)) != NULL && is_true(attr->val));
}
// }}}
*/

  // make pages a multiple of two (only considered when duplex is on).
  // FIXME? pdftopdf also supports it as cmdline option (via checkFeature())
  ppd_attr_t *attr;
  if ( (attr=ppdFindAttr(ppd,"cupsEvenDuplex",0)) != NULL) {
    param.evenDuplex=is_true(attr->value);
  }

  // TODO? pdftopdf* ?
  // TODO?! pdftopdfAutoRotate
}
// }}}

void calculate(ppd_file_t *ppd,ProcessingParameters &param)
{
  // First step: no device copies, no device collate, no device reverse.

/*
  TODO:
    force collate for duplex and not hw-collate

  TODO: 

  // check collate device
  if (P2PDoc::options.collate && !ppd->manual_copies) {
    if ((choice = ppdFindMarkedChoice(ppd,"Collate")) != NULL &&
       !strcasecmp(choice->choice,"true")) {
      ppd_option_t *opt;

      if ((opt = ppdFindOption(ppd,"Collate")) != NULL &&
        !opt->conflicted) {
	deviceCollate = gTrue;
      } else {
	ppdMarkOption(ppd,"Collate","False");
      }
    }
  }

  // check OutputOrder device
  if (P2PDoc::options.reverse) {
    if (ppdFindOption(ppd,"OutputOrder") != NULL) {
      deviceReverse = gTrue;  // i.e. also param.reverse=false; !!
    }
  }

  // TODO...
  if (!param.duplex) {
    param.evenDuplex=false;
  } else if ( (slow_collate)||(slow_order) ) {
    param.evenDuplex=true;
  }

see also setFinalPPD()
*/
  setFinalPPD(ppd,param);
}

void dump_options(int num_options,cups_option_t *options)
{
  fprintf(stderr,"%d options:\n",num_options);
  for (int iA=0;iA<num_options;iA++) {
    fprintf(stderr,"  %s: %s\n",options[iA].name,options[iA].value);
  }
}

// reads from stdin into temporary file. returns FILE *  or NULL on error 
// TODO? to extra file (also used in pdftoijs, e.g.)
FILE *copy_stdin_to_temp() // {{{
{
  char buf[BUFSIZ];
  int n;

  // FIXME:  what does >buf mean here?
  int fd=cupsTempFd(buf,sizeof(buf));
  if (fd<0) {
    error("Can't create temporary file");
    return NULL;
  }
  // remove name
  unlink(buf);

  // copy stdin to the tmp file
  while ( (n=read(0,buf,BUFSIZ)) > 0) {
    if (write(fd,buf,n) != n) {
      error("Can't copy stdin to temporary file");
      close(fd);
      return NULL;
    }
  }
  if (lseek(fd,0,SEEK_SET) < 0) {
    error("Can't rewind temporary file");
    close(fd);
    return NULL;
  }

  FILE *f;
  if ( (f=fdopen(fd,"rb")) == 0) {
    error("Can't fdopen temporary file");
    close(fd);
    return NULL;
  }
  return f;
}
// }}}

int main(int argc,char **argv)
{
  if ( (argc<6)||(argc>7) ) {
    fprintf(stderr,"Usage: %s job-id user title copies options [file]\n",argv[0]);
#ifdef DEBUG
ProcessingParameters param;
std::unique_ptr<PDFTOPDF_Processor> proc1(PDFTOPDF_Factory::processor());
  param.page.width=595.276; // A4
  param.page.height=841.89;

  param.page.top=param.page.bottom=36.0;
  param.page.right=param.page.left=18.0;
  param.page.right=param.page.width-param.page.right;
  param.page.top=param.page.height-param.page.top;

// param.nup.calculate(4,0.707,0.707,param.nup);
  param.nup.nupX=2;
  param.nup.nupY=2;
/*
*/
//param.nup.yalign=TOP;
param.border=BorderType::ONE;
//param.mirror=true;
//param.reverse=true;
//param.numCopies=3;
if (!proc1->loadFilename("in.pdf")) return 2;
    param.dump();
if (!processPDFTOPDF(*proc1,param)) return 3;
proc1->emitFilename("out.pdf");
#endif
    return 1;
  }

  ProcessingParameters param;

  param.jobId=atoi(argv[1]);
  param.user=argv[2];
  param.title=argv[3];
  param.numCopies=atoi(argv[4]);

  // TODO?! sanity checks

  int num_options=0;
  cups_option_t *options=NULL;
  num_options=cupsParseOptions(argv[5],num_options,&options);

  ppd_file_t *ppd=NULL;
  ppd=ppdOpenFile(getenv("PPD")); // getenv (and thus ppd) may be null. This will not cause problems.
  ppdMarkDefaults(ppd);

  cupsMarkOptions(ppd,num_options,options);

  getParameters(ppd,num_options,options,param);
  calculate(ppd,param);

#ifdef DEBUG
  param.dump();
#endif

  cupsFreeOptions(num_options,options);

  std::unique_ptr<PDFTOPDF_Processor> proc(PDFTOPDF_Factory::processor());

  if (argc==7) {
    if (!proc->loadFilename(argv[6])) {
      ppdClose(ppd);
      return 1;
    }
  } else {
    FILE *f=copy_stdin_to_temp();
    if ( (!f)||
         (!proc->loadFile(f,TakeOwnership)) ) {
      ppdClose(ppd);
      return 1;
    }
  }

/* TODO
  // color management
--- PPD:
    copyPPDLine_(fp_dest, fp_src, "*PPD-Adobe: "); 
    copyPPDLine_(fp_dest, fp_src, "*cupsICCProfile ");  
    copyPPDLine_(fp_dest, fp_src, "*Manufacturer:");
    copyPPDLine_(fp_dest, fp_src, "*ColorDevice:");
    copyPPDLine_(fp_dest, fp_src, "*DefaultColorSpace:");  
  if (cupsICCProfile) {
    proc.addCM(...,...);
  }
*/

  if (!processPDFTOPDF(*proc,param)) {
    ppdClose(ppd);
    return 2;
  }

  emitPreamble(ppd,param); // ppdEmit, JCL stuff

//  proc->emitFile(stdout);
  proc->emitFilename(NULL);

  emitPostamble(ppd,param);
  ppdClose(ppd);

  return 0;
}
