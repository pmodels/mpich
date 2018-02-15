#ifndef CONNECTSTUFF_H_INCLUDED
#define CONNECTSTUFF_H_INCLUDED

/* handlers */
void startWatchdog(int seconds);
void strokeWatchdog(void);
void installSegvHandler(void);
void installExitHandler(const char *fname);
void indicateConnectSucceeded(void);

/* util */
void msg(const char *fmt, ...);
void printStackTrace(void);
void safeSleep(double seconds);
char *getPortFromFile(const char *fmt, ...);
char *writePortToFile(const char *port, const char *fmt, ...);

#endif /* CONNECTSTUFF_H_INCLUDED */
