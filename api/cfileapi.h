#ifndef PIOLCFILEAPI_INCLUDE_GUARD
#define PIOLCFILEAPI_INCLUDE_GUARD
#include <mpi.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
//struct PIOLWrapper;
//struct ExSeisFileWrapper;

typedef struct PIOLWrapper * ExSeisHandle;
typedef struct ExSeisFileWrapper * ExSeisFile;

////////// Options structures

typedef struct
{
    int mode;
    MPI_Info info;
    size_t maxSize;
    MPI_Comm fcomm;
} MPIIOOptions;

typedef struct
{
    MPI_Comm comm;
    bool initMPI;
} MPIOptions;

typedef struct
{
    double incFactor;
} SEGYOptions;

/////////////

typedef struct
{
    double first;
    double second;
} ccoord_t;

typedef struct
{
    int64_t first;
    int64_t second;
} cgrid_t;

typedef enum
{
    Src,
    Rcv,
    CMP
} CCoord;

typedef enum
{
    Line
} CGrid;

//Piol
extern ExSeisHandle initPIOL(size_t logLevel, MPIOptions * mpiOpt);
extern void closePIOL(ExSeisHandle piol);
extern size_t getRank(ExSeisHandle piol);
extern size_t getNumRank(ExSeisHandle piol);
extern void isErr(ExSeisHandle piol);

//File Layer
extern ExSeisFile openFile(ExSeisHandle piol, const char * name, SEGYOptions * opt, MPIIOOptions * mpiOpt);
extern void closeFile(ExSeisFile file);

extern const char * readText(ExSeisFile f);
extern size_t readNs(ExSeisFile f);
extern size_t readNt(ExSeisFile f);
extern double readInc(ExSeisFile f);

extern void writeText(ExSeisFile file, const char * text);
extern void writeNs(ExSeisFile file, size_t ns);
extern void writeNt(ExSeisFile f, size_t nt);
extern void writeInc(ExSeisFile f, double inc);

extern void readCoordPoint(ExSeisFile f, CCoord item, size_t offset, size_t sz, ccoord_t * buf);
extern void readGridPoint(ExSeisFile f, CGrid item, size_t offset, size_t sz, cgrid_t * buf);

extern void readTrace(ExSeisFile f, size_t offset, size_t sz, float * trace);
extern void writeTrace(ExSeisFile f, size_t offset, size_t sz, float * trace);
#ifdef __cplusplus
}
#endif
#endif
