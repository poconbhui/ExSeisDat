#ifndef PIOLCOMMMPI_INCLUDE_GUARD
#define PIOLCOMMMPI_INCLUDE_GUARD
#include "anc/comm.hh"
#include <vector>
#include <functional>
#include <mpi.h>
#include <cassert>
#include "global.hh"

namespace PIOL { namespace Comm {
struct MPIOpt : Opt
{
    MPI_Comm comm = MPI_COMM_WORLD;
    bool initMPI = true;
};

class MPI : public Comm::Interface
{
    private :
    MPI_Comm comm;
    bool init;
    public :
    MPI(const MPIOpt & opt) : comm(opt.comm), init(opt.initMPI)
    {
        if (init)
        {
            //Quote from MPI 3.1 specification: "The version for ISO C accepts the argc and argv that are provided by the arguments to main or NULL"
            int err = MPI_Init(NULL, NULL);
            //PIOL::Block::MPI::printErr(err, NULL, "MPI_Init failure\n");
        }
        int irank;
        int inumRank;
        MPI_Comm_rank(comm, &irank);
        MPI_Comm_size(comm, &inumRank);
        rank = irank;
        numRank = inumRank;
        assert(getRank() < getNumRank());
    }
    ~MPI(void)
    {
        if (init)
        {
            int err = MPI_Finalize();
            //PIOL::Block::MPI::printErr(err, NULL, "MPI_Finalize failure\n");
        }
    }
    MPI_Comm getComm()
    {
        return comm;
    }

    void wait(void)
    {
        MPI_Barrier(comm);
    }
};
}}
#endif
