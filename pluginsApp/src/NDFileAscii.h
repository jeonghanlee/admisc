/*
 * NDFileAscii.h
 * ASCII file writer, whose main purpose is to create textual representation of
 * the data arriving from Thorlabs CCS.
 *
 * Hinko Kocevar
 * March 18, 2015
 */

#ifndef DRV_NDFileASCII_H
#define DRV_NDFileASCII_H

#include "NDPluginFile.h"

/** Writes NDArrays in the ASCII file format. */

class NDFileAscii : public NDPluginFile {
public:
    NDFileAscii(const char *portName, int queueSize, int blockingCallbacks,
                 const char *NDArrayPort, int NDArrayAddr,
                 int priority, int stackSize);

    /* The methods that this class implements */
    virtual asynStatus openFile(const char *fileName, NDFileOpenMode_t openMode, NDArray *pArray);
    virtual asynStatus readFile(NDArray **pArray);
    virtual asynStatus writeFile(NDArray *pArray);
    virtual asynStatus closeFile();

private:
    FILE *outFile;
};

#define NUM_NDFILE_ASCII_PARAMS 0

#endif /* DRV_NDFileASCII_H */
