
#ifndef LIBMILLWEED_MISC_H
#define LIBMILLWEED_MISC_H


#define BUFFSZ     4096
#define SMBUFFSZ   256

#ifndef DISABLE_COLOR
#define COFF       "\x1b[0;m"
#define CRED       "\x1b[0;31m"
#define CGREEN     "\x1b[0;32m"
#define CPINK      "\x1b[0;35m"
#define CBLUE      "\x1b[0;36m"
#define CDARKBLUE  "\x1b[0;34m"
#define CBOLD      "\x1b[0;37m"
#define CYELLOW    "\x1b[0;33;02m"
#else
#define COFF
#define CRED
#define CGREEN
#define CPINK
#define CBLUE
#define CDARKBLUE
#define CBOLD
#define CYELLOW
#endif

#define VERBOSITY_NORMAL      1
#define VERBOSITY_ERROR_MAJOR 2
#define VERBOSITY_VERBOSE     3
#define VERBOSITY_ERROR_MINOR 3
#define VERBOSITY_DEBUG       4

extern int verbosity;

#define report(verb, args...)  (verb <= verbosity? fprintf(verb > VERBOSITY_NORMAL? stderr : stdout, ## args): 0)

//for STANDARD MESSAGES, not EXTRA DEBUGGING INFO
#define mess(args...) report(VERBOSITY_NORMAL, ## args)

//for MAJOR ERRORS, not MINOR ERRORS
#define error(fmt, args...) report(VERBOSITY_ERROR_MAJOR, "[" CRED "error" COFF "] " fmt, ## args)

//for MINOR ERRORS
#define error2(fmt, args...) report(VERBOSITY_ERROR_MINOR, "[" CRED "error2" COFF "][" __FILE__ "] " fmt, ## args)

//for VERBOSE MESSAGES, not STANDARD MESSAGES or DEBUGGING MESSAGES
#define mess2(fmt, args...) report(VERBOSITY_VERBOSE, "[" CBOLD "mess2" COFF "] " fmt, ## args)

//for EXTRA DEBUGGING INFO, not ERRORS or VERBOSE MESSAGES
#define debug(fmt, args...) report(VERBOSITY_DEBUG, "[debug][" __FILE__ "] " fmt, ## args)


#endif
