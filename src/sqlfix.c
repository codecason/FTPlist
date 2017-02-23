/*
  File: sqlfix.c
  Project: FTP Crawler
  Author: Peter Salanki (peter@trops.net ICQ: 54915721)
  License: GPL
*/
#include <ftpcrawler.h>

char *STR_replace_c (char *source,char *old,char *new,char *dest);
#define	OUT_OF_MEMORY(p)	if (p == NULL) { \
				  fprintf(stderr,"OPUS: Out of memory\n"); \
				  exit(EXIT_FAILURE); }

char *STR_replace_c (char *source,char *old,char *new,char *dest)

{
  int		i=0, j=0;
  int		lold, lnew, lsource;
  int		tempwork=0;
  int		templen=1024;
  char	*tempdest;

  /* create a temp work area if source = dest */
  if (source == dest) {
    tempwork = 1;
    tempdest = calloc(templen, sizeof(char));
    OUT_OF_MEMORY(tempdest);
  }
  else {
    /* since dest is different from source, do work in dest */
    tempdest = dest;
  }

  lold = strlen(old);
  lnew = strlen(new);
  lsource = strlen (source);
  while (i < (lsource - lold + 1)) {
    /* if temparea used and too small, increase its size */
    if (tempwork && ((j + lnew) >= templen)) {
      templen += 256;
      tempdest = realloc(tempdest, templen * sizeof(char));
      OUT_OF_MEMORY(tempdest);
    }
    if (strncmp (&source[i], old, lold)) {
      tempdest[j++] = source[i++];
    }
    else {
      strcpy (&tempdest[j], new);
      j = j + lnew;
      i = i + lold;
    }
  }

  /* if temp work area used and too small, increase its size */
  if (tempwork && ((j + strlen(&source[i])) >= templen)) {
    templen += 256;
    tempdest = realloc(tempdest, templen * sizeof(char));
    OUT_OF_MEMORY(tempdest);
  }
  tempdest[j] = 0;
  strcat (&tempdest[j], &source[i]);

  /* if a temp work area was used, copy to final dest */
  if (tempwork) {
    strcpy(dest, tempdest);
    free(tempdest);
  }
  return (dest);
}

void sqlfix(char from[], char to[]) {

  STR_replace_c (from, "\'", "\\'", to);
}
