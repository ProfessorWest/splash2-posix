#ifndef _GRAV_H_
#define _GRAV_H_

void hackgrav(bodyptr p, long ProcessId);
void gravsub(void *p, long ProcessId);
void hackwalk(long ProcessId);
void walksub(void *n, real dsq, long ProcessId);
cbool subdivp(register nodeptr p, real dsq, long ProcessId);

#endif
