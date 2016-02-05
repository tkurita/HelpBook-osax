#include <Carbon/Carbon.h>
#include <ApplicationServices/ApplicationServices.h>

#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_5
typedef SInt32                          SRefCon;
#endif

OSErr registerHelpBookHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon);
OSErr showHelpBookHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon);
OSErr HelpBookOSAXVersionHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon);