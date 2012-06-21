#include <ctype.h>
#include "pdftopdf_processor.h"
#include <cups/ppd.h>

// TODO: -currently changes ppd.
//
static void emitJCLOptions(FILE *fp, ppd_file_t *ppd, int copies)
{
  int section;
  ppd_choice_t **choices;
  int i;
  char buf[1024];
  ppd_attr_t *attr;
  int pdftoopvp = 0;
  int datawritten = 0;

  if (ppd == 0) return;
  if ((attr = ppdFindAttr(ppd,"pdftopdfJCLBegin",NULL)) != NULL) {
    int n = strlen(attr->value);
    pdftoopvp = 1;
    for (i = 0;i < n;i++) {
	if (attr->value[i] == '\r' || attr->value[i] == '\n') {
	    /* skip new line */
	    continue;
	}
	fputc(attr->value[i],fp);
	datawritten = 1;
    }
  }
         
  snprintf(buf,sizeof(buf),"%d",copies);
  if (ppdFindOption(ppd,"Copies") != NULL) {
    ppdMarkOption(ppd,"Copies",buf);
  } else {
    if ((attr = ppdFindAttr(ppd,"pdftopdfJCLCopies",buf)) != NULL) {
      fputs(attr->value,fp);
      datawritten = 1;
    } else if (pdftoopvp) {
      fprintf(fp,"Copies=%d;",copies);
      datawritten = 1;
    }
  }
  for (section = (int)PPD_ORDER_ANY;
      section <= (int)PPD_ORDER_PROLOG;section++) {
    int n;

    n = ppdCollect(ppd,(ppd_section_t)section,&choices);
    for (i = 0;i < n;i++) {
      snprintf(buf,sizeof(buf),"pdftopdfJCL%s",
        ((ppd_option_t *)(choices[i]->option))->keyword);
      if ((attr = ppdFindAttr(ppd,buf,choices[i]->choice)) != NULL) {
	fputs(attr->value,fp);
	datawritten = 1;
      } else if (pdftoopvp) {
	fprintf(fp,"%s=%s;",
	  ((ppd_option_t *)(choices[i]->option))->keyword,
	  choices[i]->choice);
	datawritten = 1;
      }
    }
  }
  if (datawritten) fputc('\n',fp);
}

/* Copied ppd_decode() from CUPS which is not exported to the API; needed in emitPreamble() */
// {{{ static int ppd_decode(char *string) 
static int				/* O - Length of decoded string */
ppd_decode(char *string)		/* I - String to decode */
{
  char	*inptr,				/* Input pointer */
	*outptr;			/* Output pointer */


  inptr  = string;
  outptr = string;

  while (*inptr != '\0')
    if (*inptr == '<' && isxdigit(inptr[1] & 255))
    {
     /*
      * Convert hex to 8-bit values...
      */

      inptr ++;
      while (isxdigit(*inptr & 255))
      {
	if (isalpha(*inptr))
	  *outptr = (tolower(*inptr) - 'a' + 10) << 4;
	else
	  *outptr = (*inptr - '0') << 4;

	inptr ++;

        if (!isxdigit(*inptr & 255))
	  break;

	if (isalpha(*inptr))
	  *outptr |= tolower(*inptr) - 'a' + 10;
	else
	  *outptr |= *inptr - '0';

	inptr ++;
	outptr ++;
      }

      while (*inptr != '>' && *inptr != '\0')
	inptr ++;
      while (*inptr == '>')
	inptr ++;
    }
    else
      *outptr++ = *inptr++;

  *outptr = '\0';

  return ((int)(outptr - string));
}
// }}}

void emitPreamble(ppd_file_t *ppd,const ProcessingParameters &param) // {{{
{
  ppdEmit(ppd,stdout,PPD_ORDER_EXIT);

  if (param.emitJCL) {
    /* pdftopdf only adds JCL to the job if the printer is a native PDF
       printer and the PPD is for this mode, having the "*JCLToPDFInterpreter:"
       keyword. We need to read this keyword manually from the PPD and replace
       the content of ppd->jcl_ps by the value of this keyword, so that
       ppdEmitJCL() actalually adds JCL based on the presence on 
       "*JCLToPDFInterpreter:". */
    ppd_attr_t *attr;
    if ( (attr=ppdFindAttr(ppd,"JCLToPDFInterpreter",NULL)) != NULL) {
      ppd->jcl_ps=strdup(attr->value);
      ppd_decode(ppd->jcl_ps);
    } else {
      ppd->jcl_ps=NULL;
    }
    ppdEmitJCL(ppd,stdout,param.jobId,param.user,param.title);
    emitJCLOptions(stdout,ppd,param.deviceCopies);
  }
}
// }}}

void emitPostamble(ppd_file_t *ppd,const ProcessingParameters &param) // {{{
{
  if (param.emitJCL) { 
    ppdEmitJCLEnd(ppd,stdout);
  }
}
// }}}

