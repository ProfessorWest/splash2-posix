# splash2-posix
A POSIX version SPLASH-2 that just works.

There are numerous copies of SPLASH-2 in the wild.  Most copies are dated and do not compile without some refactoring of the code and selecting the type of threading mechanism.

In today's world POSIX threads (pthreads) are mature as well accepted.  SPLASH-2 was originally written before pthread's wide acceptance.  This repository is to migrate SPLASH-2 to pthreads, removing any need for extraneous tooling (like m4) that was present in the original SPLASH-2.

##Goals
1. Migrate SPLASH-2 apps and kernels to gcc with POSIX.
2. Address all warnings from gcc.
3. Add compatibility with other compilers (clange/icc.)
4. Increase computation capability to match current architectures.
