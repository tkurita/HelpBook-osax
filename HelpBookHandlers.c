#include "HelpBookHandlers.h"
#include <sys/param.h>

#define useLog 0

extern UInt32 gAdditionReferenceCount;

#pragma mark functions for debugging
void show(CFStringRef formatString, ...) {
    CFStringRef resultString;
    CFDataRef data;
    va_list argList;
	
    va_start(argList, formatString);
    resultString = CFStringCreateWithFormatAndArguments(NULL, NULL, formatString, argList);
    va_end(argList);
	
    data = CFStringCreateExternalRepresentation(NULL, resultString, CFStringGetSystemEncoding(), '?');
	
    if (data != NULL) {
    	printf ("%.*s\n\n", (int)CFDataGetLength(data), CFDataGetBytePtr(data));
    	CFRelease(data);
    }
	
    CFRelease(resultString);
}

void showAEDesc(const AppleEvent *ev)
{
	Handle result;
	OSStatus resultStatus;
	resultStatus = AEPrintDescToHandle(ev,&result);
	printf("%s\n",*result);
	DisposeHandle(result);
}

void showFSRefPath(const FSRef *ref_p)
{
	CFURLRef url = CFURLCreateFromFSRef( kCFAllocatorDefault, ref_p );
	CFStringRef cfString = NULL;
	if ( url != NULL )
	{
		cfString = CFURLCopyFileSystemPath( url, kCFURLHFSPathStyle );
		CFRelease( url );
	}
	show(cfString);
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
	//show(CFSTR("%@"), myBundleURL);
#endif
	
	if (!CFURLGetFSRef(myBundleURL, ref_p)) {
		err = fnfErr;
		//printf("error at CFURLGetFSRef\n");
	}
bail:
	CFRelease(myBundleURL);
	return err;
}

OSStatus utextPathToFSRef(const AppleEvent *ev, FSRef *ref_p, CFURLRef *urlRef_p)
{
	AEDesc givenDesc;
	OSStatus err = AEGetParamDesc(ev, keyDirectObject, typeUnicodeText, &givenDesc);
#if useLog
	showAEDesc(&givenDesc);
#endif
	Size theLength = AEGetDescDataSize(&givenDesc);
	UInt8 *theData = malloc(theLength);
	if (err == noErr) {
		err = AEGetDescData(&givenDesc, theData, theLength);
	}
	
	CFStringRef pathStr = CFStringCreateWithBytes(NULL, theData, theLength, kCFStringEncodingUnicode, false);
	CFURLPathStyle pathStyle;
	if (CFStringHasPrefix(pathStr, CFSTR("/"))) {
		pathStyle = kCFURLPOSIXPathStyle;
	}
	else {
		pathStyle = kCFURLHFSPathStyle;
	}	
	*urlRef_p = CFURLCreateWithFileSystemPath(NULL, pathStr, pathStyle, true);
	
	if (*urlRef_p != NULL) {
		CFURLGetFSRef(*urlRef_p, ref_p);
	}
	else {
		err = errAECoercionFail;
	}
	
	CFRelease(pathStr);
	free(theData);	
	return err;
}

OSStatus getFSRefFromAE(const AppleEvent *ev, FSRef *ref_p)
{
	AEDesc givenDesc;
	OSStatus err = AEGetParamDesc(ev, keyDirectObject, typeFSRef, &givenDesc);
#if useLog
	showAEDesc(&givenDesc);
#endif
	err = AEGetDescData(&givenDesc, ref_p, sizeof(FSRef));
	return err;
}

void safeRelease(CFTypeRef theObj)
{
	if (theObj != NULL) {
		CFRelease(theObj);
	}
}

OSErr registerHelpBook(const AppleEvent *ev, CFStringRef *bookName)
{	
	OSErr err = noErr;
	FSRef bundleFSRef;
	CFURLRef urlRef = NULL;
	CFBundleRef bundleRef = NULL;
	DescType typeCode;
	Size dataSize;
	err = AESizeOfParam(ev, keyDirectObject, &typeCode, &dataSize);
	
	switch (err) {
        case -1701: 
			err = getMainBundleRef(&bundleFSRef, &bundleRef);
			if (err == noErr) CFRetain(bundleRef);
				break;
        case noErr:
			//show(CFSTR("%@"), UTCreateStringForOSType(typeCode));
			switch (typeCode) {
				case typeAlias:
				case typeFSS:
				case typeFileURL:
				case cObjectSpecifier:
					err = getFSRefFromAE(ev, &bundleFSRef);
					break;
				case typeChar:
				case typeUTF8Text:
				case typeUnicodeText:
					err = utextPathToFSRef(ev, &bundleFSRef, &urlRef);
					break;
				default:
					err = errAEWrongDataType;
					goto bail;
			}
			
            break;	
        default:
			goto bail;
    }
	
	if (err != noErr) goto bail;
#if useLog	
	showFSRefPath(&bundleFSRef);
#endif	
	err = AHRegisterHelpBook(&bundleFSRef);
	
	if (err != noErr) {
		goto bail;
	}
	
	if (bundleRef == NULL) {
		if (urlRef == NULL) {
			urlRef = CFURLCreateFromFSRef(NULL, &bundleFSRef);
		}
		bundleRef = CFBundleCreate(NULL,urlRef);
	}
	
	CFStringRef tmpBookName = NULL;
	tmpBookName = CFBundleGetValueForInfoDictionaryKey(bundleRef, CFSTR("CFBundleHelpBookName"));
	// get rule: owner of bookName is not myself. don't release
	*bookName = CFStringCreateCopy(NULL, tmpBookName);
bail:
	safeRelease(bundleRef);
	safeRelease(urlRef);
	
	return err;
}

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

#pragma mark functions to install AppleEvent Managers
OSErr registerHelpBookHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
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
		err = setupReplyForString(bookName, reply);
	}
	
bail:
		
	safeRelease(bookName);
	--gAdditionReferenceCount;
	return err;
}

OSErr showHelpBookHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
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
		err = setupReplyForString(bookName, reply);
	}
	
bail:
		
	safeRelease(bookName);
	--gAdditionReferenceCount;
	return err;

}