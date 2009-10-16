#include "HelpBookHandlers.h"
#include <sys/param.h>
#include "AEUtils/AEUtils.h"

#define useLog 0

extern UInt32 gAdditionReferenceCount;

#pragma mark functions for debugging
void showFSRefPath(const FSRef *ref_p)
{
	CFURLRef url = CFURLCreateFromFSRef( kCFAllocatorDefault, ref_p );
	CFStringRef cfString = NULL;
	if ( url != NULL )
	{
		cfString = CFURLCopyFileSystemPath( url, kCFURLHFSPathStyle );
		CFRelease( url );
	}
	CFShow(cfString);
	CFRelease(cfString);
}

#pragma mark main routines
OSStatus getMainBundleRef(FSRef *ref_p, CFBundleRef *bundleRef_p)
{
	CFURLRef myBundleURL= NULL;
	OSStatus err = noErr;
	
	*bundleRef_p = CFBundleGetMainBundle();
	if (*bundleRef_p == NULL) {
		err = fnfErr; 
		printf("error at CFBundleGetMainBundle\n");
		goto bail;
	}
		
	myBundleURL = CFBundleCopyBundleURL(*bundleRef_p);// 2
	if (myBundleURL == NULL) {
		err = fnfErr; 
		//printf("error at CFBundleCopyBundleURL\n");
		goto bail;
	}
#if useLog
	CFShow(myBundleURL);
#endif
	
	if (!CFURLGetFSRef(myBundleURL, ref_p)) {
		err = fnfErr;
		//printf("error at CFURLGetFSRef\n");
	}
bail:
	CFRelease(myBundleURL);
	return err;
}

OSErr registerHelpBook(const AppleEvent *ev, CFStringRef *bookName)
{	
	OSErr err = noErr;
	FSRef bundleFSRef;
	CFBundleRef bundleRef = NULL;
	
	err = getFSRef(ev, keyDirectObject, &bundleFSRef);
	
	switch (err) {
        case -1701: 
			err = getMainBundleRef(&bundleFSRef, &bundleRef);
			if (err == noErr) {
				CFRetain(bundleRef);
				break;
			}
        case noErr:
            break;	
        default:
			goto bail;
    }
	
#if useLog	
	showFSRefPath(&bundleFSRef);
#endif	
	err = AHRegisterHelpBook(&bundleFSRef);
	
	if (err != noErr) {
		goto bail;
	}
	
	if (bundleRef == NULL) {
		CFURLRef urlRef = CFURLCreateFromFSRef(NULL, &bundleFSRef);
		bundleRef = CFBundleCreate(NULL,urlRef);
		CFRelease(urlRef);
	}
	
	CFStringRef tmpBookName = NULL;
	tmpBookName = CFBundleGetValueForInfoDictionaryKey(bundleRef, CFSTR("CFBundleHelpBookName"));
	// get rule: owner of bookName is not myself. don't release
	*bookName = CFStringCreateCopy(NULL, tmpBookName);
bail:
	safeRelease(bundleRef);
	
	return err;
}

/* replaced by putStringToReply
OSErr setupReplyForString(CFStringRef inText, AppleEvent *reply)
{
	OSErr err = noErr;
	char buffer[MAXPATHLEN+1];
	CFStringGetCString(inText, buffer, MAXPATHLEN+1, kCFStringEncodingUTF8);
	AEDesc resultDesc;
	err=AECreateDesc (typeUTF8Text, buffer, strlen(buffer), &resultDesc);
	
	if (err != noErr) goto bail;
	
	err=AEPutParamDesc(reply, keyAEResult,&resultDesc);
	if (err != noErr) {
		AEDisposeDesc(&resultDesc);
	}
	
bail:
		return err;
}
*/

#pragma mark functions to install AppleEvent Managers
OSErr registerHelpBookHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon)
{	
	++gAdditionReferenceCount;
#if useLog
	showAEDesc(ev);
#endif
	OSErr err = noErr;
	CFStringRef bookName = NULL;
	err = registerHelpBook(ev, &bookName);
	if (err != noErr) goto bail;
	
	if (bookName != NULL) {
		//err = setupReplyForString(bookName, reply);
		err = putStringToReply(bookName, typeUnicodeText, reply);
	}
	
bail:
		
	safeRelease(bookName);
	--gAdditionReferenceCount;
	return err;
}

OSErr showHelpBookHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon)
{
	++gAdditionReferenceCount;
#if useLog
	showAEDesc(ev);
#endif
	OSErr err = noErr;
	CFStringRef bookName = NULL;
	err = registerHelpBook(ev, &bookName);
	if (err != noErr) goto bail;
	
	if (bookName != NULL) {
		err = AHGotoPage(bookName, NULL, NULL);
		if (err != noErr) goto bail;
		//err = setupReplyForString(bookName, reply);
		err = putStringToReply(bookName, typeUnicodeText, reply);
	}
	
bail:
		
	safeRelease(bookName);
	--gAdditionReferenceCount;
	return err;

}