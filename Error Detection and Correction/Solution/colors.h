#ifndef _COLORS_
#define _COLORS_

/* FOREGROUND */
#define RST  string("\x1B[0m")
#define KRED  string("\x1B[31m")
#define KGRN  string("\x1B[32m")
#define KYEL  string("\x1B[33m")
#define KBLU  string("\x1B[34m")
#define KMAG  string("\x1B[35m")
#define KCYN  string("\x1B[36m")
#define KWHT  string("\x1B[37m")

#define FRED(x) KRED+x+RST
#define FGRN(x) KGRN+x+RST
#define FYEL(x) KYEL+x+RST
#define FBLU(x) KBLU+x+RST
#define FMAG(x) KMAG+x+RST
#define FCYN(x) KCYN+x+RST
#define FWHT(x) KWHT+x+RST

#define BOLD(x) string("\x1B[1m")+x+RST
#define UNDL(x) string("\x1B[4m")+x+RST

#endif  /* _COLORS_ */