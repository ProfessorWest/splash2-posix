#ifndef _GETPARAM_H_
#define _GETPARAM_H_

void initparam(const char **defv);
char* getparam(const char *name);
long getiparam(const char *name);
long getlparam(const char *name);
cbool getbparam(const char *name);
double getdparam(const char *name);
long scanbind(const char *bvec[], const char *name);
cbool matchname(const char *bind, const char *name);
char* extrvalue(const char *arg);

#endif
