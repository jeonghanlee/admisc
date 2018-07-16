#ifndef NDPluginImaging_H
#define NDPluginImaging_H

#include <epicsTypes.h>
#include "NDPluginDriver.h"

#define NDPluginImagingDummy1String     "NDIMAGING_DUMMY_1"    /* (asynFloat64Array, r/o) dummy array */
#define NDPluginImagingDummy2String     "NDIMAGING_DUMMY_2"    /* (asynFloat64,      r/o) dummy scalar */

/** Does curve fitting to Gaussian model.
  */
class epicsShareClass NDPluginImaging : public NDPluginDriver {
public:
    NDPluginImaging(const char *portName, int queueSize, int blockingCallbacks,
					const char *NDArrayPort, int NDArrayAddr, int maxPeaks,
					int maxBuffers, size_t maxMemory,
					int priority, int stackSize);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);

    template <typename epicsType> asynStatus doComputeImagingT(NDArray *pArray);
    asynStatus doComputeImaging(NDArray *pArray);

protected:
    int NDPluginImagingDummy1;
#define FIRST_NDPLUGIN_IMAGING_PARAM NDPluginImagingDummy1
    int NDPluginImagingDummy2;

    #define LAST_NDPLUGIN_IMAGING_PARAM NDPluginImagingDummy2

private:
    size_t points;
    double *dataSet;

};
#define NUM_NDPLUGIN_IMAGING_PARAMS ((int)(&LAST_NDPLUGIN_IMAGING_PARAM - &FIRST_NDPLUGIN_IMAGING_PARAM + 1))

#endif
