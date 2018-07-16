#ifndef NDPluginSource_H
#define NDPluginSource_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "NDPluginDriver.h"

#define NDPluginSourceExtValueString	"EXT_VALUE"  /* (asynFloat64,        w) External value */
#define NDPluginSourceSourceString		"SOURCE"     /* (asynOctet,        r/w) The PV for the external value */
#define NDPluginSourceIndexString		"INDEX"      /* (asynInt32,        r/w) Current index */
#define NDPluginSourceLengthString		"LENGTH"     /* (asynInt32,        r/w) Total length */

/** Does read scalar input and stored into NDArray.
  */
class NDPluginSource : public NDPluginDriver {
public:
    NDPluginSource(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);

    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);

protected:
    int NDPluginSourceExtValue;
    #define FIRST_NDPLUGIN_SOURCE_PARAM NDPluginSourceExtValue
    int NDPluginSourceSource;
    int NDPluginSourceIndex;
    int NDPluginSourceLength;

	#define LAST_NDPLUGIN_SOURCE_PARAM NDPluginSourceLength

private:
    NDArray *pArray;
    bool doRun;

};

#define NUM_NDPLUGIN_SOURCE_PARAMS ((int)(&LAST_NDPLUGIN_SOURCE_PARAM - &FIRST_NDPLUGIN_SOURCE_PARAM + 1))
    
#endif /* NDPluginSource_H */
