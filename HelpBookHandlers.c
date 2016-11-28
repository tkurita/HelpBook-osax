#include "HelpBookHandlers.h"
#include <sys/param.h>
#include "AEUtils/AEUtils.h"
#include "HelpBookOSAXConsntatns.h"
#include <syslog.h>

#define useLog 0

#pragma mark main routines
OSStatus CopyMainBundleRef(CFURLRef *bundleURL_p, CFBundleRef *bundleRef_p)
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
		CFTypeRef keys[n_val];
		CFTypeRef values[n_val];
		CFDictionaryGetKeysAndValues(recover_plist, keys, values);
		for (int n = 0; n < n_val; n++) {
			CFDictionaryAddValue(current_plist, keys[n], values[n]); // if key present, value is not changed.
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

OSStatus registerHelpBook(const AppleEvent *ev, CFStringRef *bookName)
{	
	OSStatus err = noErr;
	CFBundleRef bundle = NULL;
	CFURLRef bundle_url = NULL;
#if useLog
    syslog(LOG_NOTICE, "start registerHelpBook");
#endif
    bundle_url = CFURLCreateWithEvent(ev, keyDirectObject, (OSErr *)&err);
    
	switch (err) {
        case -1701: 
			err = CopyMainBundleRef(&bundle_url, &bundle);
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
    CFShow(bundle_url);
#endif
    err = AHRegisterHelpBookWithURL(bundle_url);
	
	if (err != noErr) {
		Boolean try_recover = false;
		getBoolValue(ev, kReccoverParam,  &try_recover);
		if (try_recover) {
			if (!bundle) {
				bundle = CFBundleCreate(NULL, bundle_url);
			}
			if (recoverInfoPlist(bundle, bundle_url)){
				CFRelease(bundle); bundle = NULL; // to notify the bundle was updated.
				err = AHRegisterHelpBookWithURL(bundle_url);
				if (err != noErr) {
					err = registerHBAfterRecoveringErr;
					goto bail;
				}
			}
        } else {
            goto bail;
        }
		if (err != noErr) goto bail;
    } else {
#if useLog
        syslog(LOG_NOTICE, "success to AHRegisterHelpBookWithURL");
#endif
    }
	
	if (bundle == NULL) {
		bundle = CFBundleCreate(NULL, bundle_url);
	}
	
	*bookName = CFBundleGetValueForInfoDictionaryKey(bundle, CFSTR("CFBundleHelpBookName"));
	if (*bookName) CFRetain(*bookName);
bail:
	safeRelease(bundle);
	safeRelease(bundle_url);
	return err;
}


OSErr setupErrorString(const AppleEvent *ev, AppleEvent *reply, OSErr err)
{
	CFURLRef file_url = NULL;
	CFStringRef path = NULL;
	CFStringRef msg = NULL;
	CFStringRef template = NULL;
	
	if (err == noErr) return err;
	switch (err) {
        case -50: /* paramErr : error in user parameter list.
                     defoined in MacErrors.h */
			template = CFSTR("Failed to register HelpBook for \n\"%@\".");
			err = regiesrHBErr;
			break;
		case registerHBAfterRecoveringErr:
			template = CFSTR("Succeeded in recovering Info.plist but failed to register HelpBook for \n\"%@\" .\n\nYou may need to relaunch the application.");
			break;
        case NoCFBundleHelpBookName:
            template = CFSTR("No CFBundleHelpBookName in Info.plist of \"%@\".");
            break;
		default:
			goto bail;
			break;
	}
	
	OSErr err_url = noErr;
	file_url = CFURLCreateWithEvent(ev, keyDirectObject, &err_url);
	if (file_url) {
		path = CFURLCopyFileSystemPath(file_url, kCFURLPOSIXPathStyle);
	}
	if ((err_url == noErr) && path) {
		msg = CFStringCreateWithFormat(NULL, NULL, template, path);
		putStringToEvent(reply, keyErrorString, msg, kCFStringEncodingUTF8);
		CFRelease(msg);
	}
	safeRelease(path);
	safeRelease(file_url);
bail:
	return err;
}

#pragma mark functions to install AppleEvent Managers

#define STR(s) _STR(s)
#define _STR(s) #s 

OSErr HelpBookOSAXVersionHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon)
{
	OSErr err;
	err = putStringToEvent(reply, keyAEResult, CFSTR(STR(HelpBook_OSAX_VERSION)), kCFStringEncodingUnicode);
	return err;
}

OSErr registerHelpBookHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon)
{
#if useLog
	showAEDesc(ev);
#endif
	OSStatus err = noErr;
	CFStringRef bookName = NULL;
	err = registerHelpBook(ev, &bookName);

	err = setupErrorString(ev, reply, err);
	
    if (err != noErr) goto bail;
    
    if (bookName) {
        err = putStringToEvent(reply, keyAEResult, bookName, typeUnicodeText);
    } else {
        putMissingValueToReply(reply);
        err = setupErrorString(ev, reply, NoCFBundleHelpBookName);
	}
bail:
	safeRelease(bookName);
	return err;
}

OSErr showHelpBookHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon)
{
#if useLog
	showAEDesc(ev);
#endif
	OSStatus err = noErr;
	CFStringRef bookName = NULL;
	err = registerHelpBook(ev, &bookName);
	
	err = setupErrorString(ev, reply, err);
	
    if (err != noErr) goto bail;
    
    if (bookName) {
        putStringToEvent(reply, keyAEResult, bookName, typeUnicodeText);
        err = AHGotoPage(bookName, NULL, NULL);
        if (err != noErr) goto bail;
    } else {
        putMissingValueToReply(reply);
        err = setupErrorString(ev, reply, NoCFBundleHelpBookName);
    }
bail:
	safeRelease(bookName);
	return err;

}
