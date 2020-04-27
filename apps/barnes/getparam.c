/*************************************************************************/
/*                                                                       */
/*  Copyright (c) 1994 Stanford University                               */
/*                                                                       */
/*  All rights reserved.                                                 */
/*                                                                       */
/*  Permission is given to use, copy, and modify this software for any   */
/*  non-commercial purpose as long as this copyright notice is not       */
/*  removed.  All other uses, including redistribution in whole or in    */
/*  part, are forbidden without prior written permission.                */
/*                                                                       */
/*  This software is provided with absolutely no warranty and no         */
/*  support.                                                             */
/*                                                                       */
/*************************************************************************/

/*
 * GETPARAM.C:
 */

#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
extern pthread_t PThreadTable[];

#define global extern

#include "stdinc.h"

local const char **defaults = NULL;        /* vector of "name=value" strings */

/*
 * INITPARAM: ignore arg vector, remember defaults.
 */

void initparam(const char **defv)
{
   defaults = defv;
}

/*
 * GETPARAM: export version prompts user for value.
 */

char *getparam(const char *name)
{
   long i, leng;
   char *def;
   char buf[128];

   if (defaults == NULL)
      error("getparam: called before initparam\n");
   i = scanbind(defaults, name);
   if (i < 0)
      error("getparam: %s unknown\n", name);
   def = extrvalue(defaults[i]);
   fgets(buf, 128, stdin);
   leng = strlen(buf) + 1;
   if (leng > 1) {
      return (strcpy((char*)malloc(leng), buf));
   }
   else {
      return (def);
   }
}

/*
 * GETIPARAM, ..., GETDPARAM: get long, long, cbool, or double parameters.
 */

long getiparam(const char *name)
{
   const char *val = "";

   for (; *val == '\0';) {
      val = getparam(name);
   }
   return (atoi(val));
}

long getlparam(const char *name)
{
   const char *val;

   for (val = ""; *val == '\0'; )
      val = getparam(name);
   return (atol(val));
}

cbool getbparam(const char *name)
{
   const char *val;

   for (val = ""; *val == '\0'; )
      val = getparam(name);
   if (strchr("tTyY1", *val) != NULL) {
      return (TRUE);
   }
   if (strchr("fFnN0", *val) != NULL) {
      return (FALSE);
   }
   error("getbparam: %s=%s not bool\n", name, val);
   return FALSE;
}

double getdparam(const char *name)
{
   const char *val = "";

   for (; *val == '\0'; ) {
      val = getparam(name);
   }
   return (atof(val));
}



/*
 * SCANBIND: scan binding vector for name, return index.
 */

long scanbind(const char *bvec[], const char *name)
{
   long i;

   for (i = 0; bvec[i] != NULL; i++)
      if (matchname(bvec[i], name))
	 return (i);
   return (-1);
}

/*
 * MATCHNAME: determine if "name=value" matches "name".
 */

cbool matchname(const char *bind, const char *name)
{
   const char *bp, *np;

   bp = bind;
   np = name;
   while (*bp == *np) {
      bp++;
      np++;
   }
   return (*bp == '=' && *np == '\0');
}

/*
 * EXTRVALUE: extract value from name=value string.
 */

char *extrvalue(const char *arg)
{
   char *ap;

   ap = (char *) arg;
   while (*ap != '\0')
      if (*ap++ == '=')
	 return ((char *) ap);
   return (NULL);
}

