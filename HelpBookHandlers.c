#include "HelpBookHandlers.h"
#include <sys/param.h>
#include "AEUtils/AEUtils.h"
#include "HelpBookOSAXConsntatns.h"

#define useLog 0

UInt32 gAdditionReferenceCount = 0;

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
OSErr versionHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
{
	OSErr err;
	CFBundleRef	bundle = CFBundleGetBundleWithIdentifier(BUNDLE_ID);
	CFDictionaryRef info = CFBundleGetInfoDictionary(bundle);
	
	CFStringRef vers = CFDictionaryGetValue(info, CFSTR("CFBundleShortVersionString"));
	err = putStringToEvent(reply, keyAEResult, vers, kCFStringEncodingUnicode);
	return err;
}

OSStatus CopyMainBundleRef(FSRef *ref_p, CFURLRef *bundleURL_p, CFBundleRef *bundleRef_p)
{
	OSStatus err = noErr;
	
	*bundleRef_p = CFBundleGetMainBundle();
	if (*bundleRef_p == NULL) {
		err = fnfErr; 
		printf("error at CFBundleGetMainBundle\n");
		goto bail;
	}
		
	*bundleURL_p = CFBundleCopyBundleURL(*bundleRef_p);// 2
	if (*bundleURL_p == NULL) {
		err = fnfErr; 
		//printf("error at CFBundleCopyBundleURL\n");
		goto bail;
	}
	
	if (!CFURLGetFSRef(*bundleURL_p, ref_p)) {
		err = fnfErr;
		//printf("error at CFURLGetFSRef\n");
	}
bail:
	return err;
}

void WritePropertyListToFile(CFPropertyListRef propertyList,
							   CFURLRef fileURL ) {
	CFDataRef xmlData;
	Boolean status;
	SInt32 errorCode;
	
	// Convert the property list into XML data.
	xmlData = CFPropertyListCreateXMLData( kCFAllocatorDefault, propertyList );
	
	// Write the XML data to the file.
	status = CFURLWriteDataAndPropertiesToResource(
													fileURL,                  // URL to use
													xmlData,                  // data to write
													NULL,
													&errorCode);
	
	CFRelease(xmlData);
}

CFPropertyListRef CFPropertyListCreateFromFile(CFURLRef fileURL) {
	CFPropertyListRef propertyList = NULL;
	CFStringRef       errorString;
	CFDataRef         resourceData = NULL;
	Boolean           status;
	SInt32            errorCode;
	
	// Read the XML file.
	status = CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault,
													  fileURL,
													  &resourceData,            // place to put file data
													  NULL,
													  NULL,
													  &errorCode);
	if (!status) goto bail;
	// Reconstitute the dictionary using the XML data.
	propertyList = CFPropertyListCreateFromXMLData(kCFAllocatorDefault,
												   resourceData,
												   kCFPropertyListMutableContainers,
												   &errorString);
	
bail:
	safeRelease( resourceData );
	return propertyList;
}

Boolean recoverInfoPlist(CFBundleRef bundle, CFURLRef bundleURL)
{
	Boolean result = false;
	CFURLRef recover_file_url = NULL;
	CFMutableDictionaryRef recover_plist = NULL;
	CFURLRef infoPlist_url = NULL;
	CFMutableDictionaryRef current_plist = NULL;
	
	recover_file_url = CFBundleCopyResourceURL(bundle,
											   CFSTR("recover-Info"),
											   CFSTR("plist"),
											   NULL);
	if (!recover_file_url) goto bail;
	
	recover_plist = (CFMutableDictionaryRef)CFPropertyListCreateFromFile(recover_file_url);
	if (!recover_plist) goto bail;
	
	infoPlist_url = CFURLCreateCopyAppendingPathComponent(NULL,
														   bundleURL,
														   CFSTR("Contents/Info.plist"),
														   false);
	current_plist = (CFMutableDictionaryRef)CFPropertyListCreateFromFile(infoPlist_url);
	
	if (!current_plist) {
		current_plist = recover_plist;
		CFRetain(current_plist);
	} else {
		CFIndex n_val = CFDictionaryGetCount(recover_plist);
		CFTypeRef *keys;
		CFTypeRef *values;
		CFDictionaryGetKeysAndValues(recover_plist, keys, values);
		for (int n = 0; n < n_val; n++) {
			CFDictionaryAddValue(current_plist, keys[n], values[n]);
		}
	}
	WritePropertyListToFile(current_plist, infoPlist_url);
	CFRelease(current_plist);
	result = true;
bail:
	safeRelease(infoPlist_url);
	safeRelease(recover_file_url);
	return result;
}

OSErr registerHelpBook(const AppleEvent *ev, CFStringRef *bookName)
{	
	OSErr err = noErr;
	FSRef bundle_ref;
	CFBundleRef bundle = NULL;
	CFURLRef bundle_url = NULL;
	
	err = getFSRef(ev, keyDirectObject, &bundle_ref);
	
	switch (err) {
        case -1701: 
			err = CopyMainBundleRef(&bundle_ref, &bundle_url, &bundle);
			if (err == noErr) {
				CFRetain(bundle);
				break;
			}
        case noErr:
            break;	
        default:
			goto bail;
    }
	
#if useLog	
	showFSRefPath(&bundle_ref);
#endif
	err = AHRegisterHelpBook(&bundle_ref);
	
	if (err != noErr) {
		Boolean try_recover = false;
		getBoolValue(ev, kReccoverParam,  &try_recover);
		if (try_recover) {
			if (!bundle) {
				bundle_url = CFURLCreateFromFSRef(NULL, &bundle_ref);
				bundle = CFBundleCreate(NULL, bundle_url);
			}
			
			if (recoverInfoPlist(bundle, bundle_url)){
				CFRelease(bundle); bundle = NULL; // to notify the bundle was updated.
				err = AHRegisterHelpBook(&bundle_ref);
			}
		}
		if (err != noErr) goto bail;
	}
	
	if (bundle == NULL) {
		bundle_url = CFURLCreateFromFSRef(NULL, &bundle_ref);
		bundle = CFBundleCreate(NULL, bundle_url);
	}
	
	*bookName = CFBundleGetValueForInfoDictionaryKey(bundle, CFSTR("CFBundleHelpBookName"));
	CFRetain(*bookName);
bail:
	safeRelease(bundle);
	safeRelease(bundle_url);
	return err;
}


#pragma mark functions to install AppleEvent Managers

void setupErrorString(const AppleEvent *ev, AppleEvent *reply, OSErr err)
{
	CFURLRef file_url = NULL;
	CFStringRef path = NULL;
	CFStringRef msg = NULL;
	
	switch (err) {
		case noErr:
			break;
		case -50:
			file_url = CFURLCreateWithEvent(ev, keyDirectObject, &err);
			if (file_url) {
				path = CFURLCopyFileSystemPath(file_url, kCFURLHFSPathStyle);
			}
			if ((err == noErr) && path) {
				msg = CFStringCreateWithFormat(NULL, NULL, CFSTR("Failed to register HelpBook for %@ ."),
											   path);
				putStringToEvent(reply, keyErrorString, msg, kCFStringEncodingUTF8);
				CFRelease(msg);
			}
			safeRelease(path);
			safeRelease(file_url);
			err = 1850;
		default:
			break;
	}
}

OSErr registerHelpBookHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon)
{	
	++gAdditionReferenceCount;
#if useLog
	showAEDesc(ev);
#endif
	OSErr err = noErr;
	CFStringRef bookName = NULL;
	err = registerHelpBook(ev, &bookName);

	setupErrorString(ev, reply, err);
	
	if ((err == noErr) && (bookName != NULL)) {
		err = putStringToEvent(reply, keyAEResult, bookName, typeUnicodeText);
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
	
	setupErrorString(ev, reply, err);
	
	if ((err == noErr) && (bookName != NULL)) {
		err = AHGotoPage(bookName, NULL, NULL);
		if (err != noErr) goto bail;
		err = putStringToEvent(reply, keyAEResult, bookName, typeUnicodeText);
	}
	
bail:
	safeRelease(bookName);
	--gAdditionReferenceCount;
	return err;

}