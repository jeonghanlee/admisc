/* NDFileAscii.cpp
 * ASCII file writer, whose main purpose is to create textual representation of
 * the data arriving from Thorlabs CCS.
 *
 * Hinko Kocevar
 * March 18, 2015
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <epicsStdio.h>
#include <iocsh.h>

#include <epicsExport.h>

#include "NDFileAscii.h"

static const char *driverName = "NDFileAscii";

#define MAX_ATTRIBUTE_STRING_SIZE 256

/** Opens a ASCII file.
  * \param[in] fileName The name of the file to open.
  * \param[in] openMode Mask defining how the file should be opened; bits are
  *            NDFileModeRead, NDFileModeWrite, NDFileModeAppend, NDFileModeMultiple
  * \param[in] pArray A pointer to an NDArray; this is used to determine the array and attribute properties.
  */
asynStatus NDFileAscii::openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray) {
    /* We don't support reading yet */
    if (openMode & NDFileModeRead) return(asynError);

    /* We don't support opening an existing file for appending yet */
    if (openMode & NDFileModeAppend) return(asynError);

    /* Create the file. */
    if ((this->outFile = fopen(fileName, "w")) == NULL) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s error opening file %s\n", driverName, __func__, fileName);
        return(asynError);
    }

    return(asynSuccess);
}

/** Writes single NDArray to the ASCII file.
  * \param[in] pArray Pointer to the NDArray to be written
  */
asynStatus NDFileAscii::writeFile(NDArray *pArray) {
    NDArrayInfo info;
    char buf[4096];
    int ret;
    size_t c, r, b;
    size_t nRow, nCol;
    struct tm *tm;
    time_t t;
    int colIdx = 0;
    int rowIdx = 1;
    int rowSkip = 1;
    int colSkip = 1;
    NDAttribute *pAttribute;

    if (! outFile) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s:%s file not opened\n", driverName, __func__);
        return(asynError);
    }

    pAttribute = pArray->pAttributeList->find("Row");
    if (pAttribute) pAttribute->getValue(NDAttrInt32, &rowIdx);
    pAttribute = pArray->pAttributeList->find("Col");
    if (pAttribute) pAttribute->getValue(NDAttrInt32, &colIdx);
    pAttribute = pArray->pAttributeList->find("RowSkip");
    if (pAttribute) pAttribute->getValue(NDAttrInt32, &rowSkip);
    pAttribute = pArray->pAttributeList->find("ColSkip");
    if (pAttribute) pAttribute->getValue(NDAttrInt32, &colSkip);
    //printf("%s: rowIdx = %d, colIdx = %d, rowSkip = %d, colSkip = %d\n", __func__, rowIdx, rowIdx, rowSkip, colSkip);

    pArray->getInfo(&info);
    nRow = pArray->dims[rowIdx].size;
    nCol = pArray->dims[colIdx].size;
	//printf("%s: rows = %d, cols = %d\n", __func__, (int)nRow, (int)nCol);

    t = time(NULL);
    tm = localtime(&t);
    strftime(buf, sizeof(buf), "# Saved on: %a %d %b %Y %H:%M:%S %z\n", tm);
    ret = fwrite(buf, 1, strlen(buf), outFile);
    if (ret < 0) {
        asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: %s\n", driverName,
                __func__, strerror(errno));
        return(asynError);
    }

    for (r = 0; r < nRow; r++) {
    	b = 0;
    	b += sprintf(buf + b, "%ld ", r);
    	for (c = 0; c < nCol; c++) {
    	    switch (pArray->dataType) {
    	        case NDInt8:
    	        	b += sprintf(buf + b, "%d ", *((epicsInt8 *)(pArray->pData) + (r * rowSkip) + (c * colSkip)));
    	            break;
    	        case NDUInt8:
    	        	b += sprintf(buf + b, "%d ", *((epicsUInt8 *)(pArray->pData) + (r * rowSkip) + (c * colSkip)));
    	            break;
    	        case NDInt16:
    	        	b += sprintf(buf + b, "%d ", *((epicsInt16 *)(pArray->pData) + (r * rowSkip) + (c * colSkip)));
    	            break;
    	        case NDUInt16:
    	        	b += sprintf(buf + b, "%d ", *((epicsUInt16 *)(pArray->pData) + (r * rowSkip) + (c * colSkip)));
    	            break;
    	        case NDInt32:
    	        	b += sprintf(buf + b, "%d ", *((epicsInt32 *)(pArray->pData) + (r * rowSkip) + (c * colSkip)));
    	            break;
    	        case NDUInt32:
    	        	b += sprintf(buf + b, "%d ", *((epicsUInt32 *)(pArray->pData) + (r * rowSkip) + (c * colSkip)));
    	            break;
    	        case NDFloat32:
    	        	b += sprintf(buf + b, "%.16f ", *((epicsFloat32 *)(pArray->pData) + (r * rowSkip) + (c * colSkip)));
    	            break;
    	        case NDFloat64:
    	        	b += sprintf(buf + b, "%.16f ", *((epicsFloat64 *)(pArray->pData) + (r * rowSkip) + (c * colSkip)));
    	            break;
    	    }
        }

	    buf[b] = '\0';
		ret = fprintf(outFile, "%s\n", buf);
		if (ret < 0) {
			asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "%s:%s: %s\n", driverName,
					__func__, strerror(errno));
			return(asynError);
		}
    }

    return(asynSuccess);
}

/** Reads single NDArray from a ASCII file; NOT CURRENTLY IMPLEMENTED.
  * \param[in] pArray Pointer to the NDArray to be read
  */
asynStatus NDFileAscii::readFile(NDArray **pArray) {
    return asynError;
}


/** Closes the ASCII file. */
asynStatus NDFileAscii::closeFile() {
    fclose(outFile);
    return(asynSuccess);
}

/** Constructor for NDFileAscii; all parameters are simply passed to NDPluginFile::NDPluginFile.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDFileAscii::NDFileAscii(const char *portName, int queueSize, int blockingCallbacks,
                       const char *NDArrayPort, int NDArrayAddr,
                       int priority, int stackSize)
    /* Invoke the base class constructor.
     * We allocate 2 NDArrays of unlimited size in the NDArray pool.
     * This driver can block (because writing a file can be slow), and it is not multi-device.
     * Set autoconnect to 1.  priority and stacksize can be 0, which will use defaults. */
    : NDPluginFile(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_NDFILE_ASCII_PARAMS,
                   2, 0, asynGenericPointerMask, asynGenericPointerMask,
                   ASYN_CANBLOCK, 1, priority, stackSize),
      outFile(NULL)
{
    //const char *functionName = "NDFileAscii";

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDFileAscii");
    this->supportsMultipleArrays = 0;
}

/* Configuration routine.  Called directly, or from the iocsh  */

extern "C" int NDFileAsciiConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                   const char *NDArrayPort, int NDArrayAddr,
                                   int priority, int stackSize)
{
	NDFileAscii *plugin = new NDFileAscii(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                   priority, stackSize);
	return plugin->start();
}


/* EPICS iocsh shell commands */

static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArray Port",iocshArgString};
static const iocshArg initArg4 = { "NDArray Addr",iocshArgInt};
static const iocshArg initArg5 = { "priority",iocshArgInt};
static const iocshArg initArg6 = { "stack size",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6};
static const iocshFuncDef initFuncDef = {"NDFileAsciiConfigure",7,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDFileAsciiConfigure(args[0].sval, args[1].ival, args[2].ival, args[3].sval, args[4].ival, args[5].ival, args[6].ival);
}

extern "C" void NDFileAsciiRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDFileAsciiRegister);
}
