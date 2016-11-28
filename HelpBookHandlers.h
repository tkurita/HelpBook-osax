#include <Carbon/Carbon.h>
#include <ApplicationServices/ApplicationServices.h>

#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_5
typedef SInt32                          SRefCon;
#endif

enum {
    regiesrHBErr            = 1850,  /*Failed to register HelpBook*/
    registerHBAfterRecoveringErr = 1851, /*Succeeded in recovering Info.plist 
                                          but failed to register HelpBook*/
    NoCFBundleHelpBookName = 1852 /*No CFBundleHelpBookName in Info.plist*/
};

OSErr registerHelpBookHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon);
OSErr showHelpBookHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon);
OSErr HelpBookOSAXVersionHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon);
