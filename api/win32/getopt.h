#ifndef __GETOPT_H__
#define __GETOPT_H__
#include <razorback/visibility.h>
#ifdef __cplusplus
extern "C" {
#endif

SO_PUBLIC extern int opterr;		/* if error message should be printed */
SO_PUBLIC extern int optind;		/* index into parent argv vector */
SO_PUBLIC extern int optopt;		/* character checked for validity */
SO_PUBLIC extern int optreset;		/* reset getopt */
SO_PUBLIC extern char *optarg;		/* argument associated with option */

struct option
{
  const char *name;
  int has_arg;
  int *flag;
  int val;
};

#define no_argument       0
#define required_argument 1
#define optional_argument 2

SO_PUBLIC int getopt(int, char**, char*);
SO_PUBLIC int getopt_long(int, char**, char*, struct option*, int*);

#ifdef __cplusplus
}
#endif

#endif /* __GETOPT_H__ */
