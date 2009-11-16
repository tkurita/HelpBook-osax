#include <Carbon/Carbon.h>
#include <ApplicationServices/ApplicationServices.h>

OSErr registerHelpBookHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon);
OSErr showHelpBookHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon);
OSErr versionHandler(const AppleEvent *ev, AppleEvent *reply, long refcon);