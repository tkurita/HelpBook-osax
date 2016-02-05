#include "HelpBookHandlers.h"
#include "HelpBookOSAXConsntatns.h"

extern UInt32 gAdditionReferenceCount;
CFBundleRef gAdditionBundle;

// =============================================================================
// == Entry points.

static OSErr InstallMyEventHandlers();
static void RemoveMyEventHandlers();

OSErr SAInitialize(CFBundleRef theBundle)
{
#if useLog
	printf("start SAInitialize\n");
#endif	
	gAdditionBundle = theBundle;  // no retain needed.
	
	return InstallMyEventHandlers();
}

void SATerminate()
{
	RemoveMyEventHandlers();
}

Boolean SAIsBusy()
{
	return gAdditionReferenceCount != 0;
}

struct AEEventHandlerInfo {
	FourCharCode			evClass, evID;
	AEEventHandlerProcPtr	proc;
};
typedef struct AEEventHandlerInfo AEEventHandlerInfo;

static const AEEventHandlerInfo gEventInfo[] = {
	{ kHelpBookSuite, kRegisterHelpBookEvent, registerHelpBookHandler },
	{ kHelpBookSuite, kShowHelpBookEvent, showHelpBookHandler },
	{ kHelpBookSuite, kVersionEvent, HelpBookOSAXVersionHandler },
};

#define kEventHandlerCount  (sizeof(gEventInfo) / sizeof(AEEventHandlerInfo))

static AEEventHandlerUPP gEventUPPs[kEventHandlerCount];

static OSErr InstallMyEventHandlers()
{
	OSErr		err;
	size_t		i;
	
	for (i = 0; i < kEventHandlerCount; ++i) {
		if ((gEventUPPs[i] = NewAEEventHandlerUPP(gEventInfo[i].proc)) != NULL)
			err = AEInstallEventHandler(gEventInfo[i].evClass, gEventInfo[i].evID, gEventUPPs[i], 0, true);
		else
			err = memFullErr;
		
		if (err != noErr) {
			SATerminate();  // Call the termination function ourselves, because the loader won't once we fail.
			return err;
		}
	}
	
	return noErr; 
}

static void RemoveMyEventHandlers()
{
	OSErr		err;
	size_t		i;
	//printf("start RemoveMyEventHandlers\n");
	for (i = 0; i < kEventHandlerCount; ++i) {
		//printf("%i\n",i);
		if (gEventUPPs[i] &&
			(err = AERemoveEventHandler(gEventInfo[i].evClass, gEventInfo[i].evID, gEventUPPs[i], true)) == noErr)
		{
			DisposeAEEventHandlerUPP(gEventUPPs[i]);
			gEventUPPs[i] = NULL;
		}
	}
	//printf("end RemoveMyEventHandlers\n");
}
#if !__LP64__
int main(int argc, char *argv[]) // for debugging
{
	//AHGotoPage(CFSTR("KeyValueDictionary Help"), NULL, NULL);
	InstallMyEventHandlers();
    RunApplicationEventLoop();
	RemoveMyEventHandlers();
	
	return 0;
}
#endif
