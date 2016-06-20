#ifndef PIOLFILEFILE_INCLUDE_GUARD
#define PIOLFILEFILE_INCLUDE_GUARD
#include <vector>
#include <array>
#include <cassert>
#include <utility>
#include <string>
#include <iostream>
#include <memory>
#include <mpi.h>
#include "object/object.hh"
#include "comm/comm.hh"
namespace PIOL { 
namespace File {
enum class Md : size_t
{
    Increment,
    NumSample,
    NumTraces,
    Type
};

enum class BlockMd : size_t
{
    xSrc,
    ySrc,
    xRcv,
    yRcv,
    xCDP,
    yCDP,
    iLin,
    xLin,

    Len,
    ERROR
};

enum class Coord : size_t
{
    Src,
    Rcv,
    Cmp,
    Lin,
    Len
};

struct Header
{
    std::string note;
    real inc;
    size_t ns;
    size_t nt;
};

class Interface
{
    protected:
    std::string note;
    real inc;
    size_t ns;
    size_t nt;

    size_t rank;
    size_t numRank;

    std::shared_ptr<Comms::Interface> comm;
    std::unique_ptr<Obj::Interface> obj;

    bool defHOUpdate;
    void checkFileSize()
    {
        if (obj->getFileSz() != obj->getSize(readNt(), readNs()))
        {
            obj->setFileSz(readNt(), readNs());
        }
    }

    public :

    typedef std::pair<real, real> CoordData;
    typedef std::pair<BlockMd, BlockMd> CoordPair;
    typedef std::array<CoordData, static_cast<size_t>(Coord::Len)> CoordArray;

    Interface(std::shared_ptr<Comms::Interface> Comm) : comm(Comm)
    {

    }
    ~Interface(void)
    {
        //TODO:
        if (defHOUpdate)
        {
        }
    }
    Header readHeader()
    {
        Header header;
        header.note = readNote();
        header.ns = readNs();
        header.nt = readNt();
        header.inc = readInc();
        return header;
    }
    virtual std::string readNote()
    {
        return note;
    }
    virtual size_t readNs()
    {
        return ns;
    }
    virtual size_t readNt()
    {
        return nt;
    }
    virtual real readInc()
    {
        return inc;
    }

    virtual void writeHeader(Header header) = 0;

    virtual void writeNote(std::string Note)
    {
        note = Note;
        defHOUpdate = true;
    }
    virtual void writeNs(size_t Ns)
    {
        ns = Ns;
        defHOUpdate = true;
    }
    virtual void writeNt(size_t Nt)
    {
        nt = Nt;
        defHOUpdate = true;
    }
    virtual void writeInc(real Inc)
    {
        inc = Inc;
    }

    virtual void readCoord(size_t offset, CoordPair items, std::vector<CoordData> & data) = 0;
    virtual void readCoord(size_t offset, std::vector<CoordArray> & data) = 0;
    virtual void readCoord(size_t offset, Coord coord, std::vector<CoordData> & data) = 0;
    virtual void writeCoord(size_t offset, Coord coord, std::vector<CoordData> & data) = 0;
    virtual void writeCoord(size_t offset, std::vector<CoordArray> & data) = 0;
    virtual void readTraces(size_t offset, std::vector<real> & data) = 0;
//Random access pattern. Good candidate for collective.
    virtual void readTraces(std::vector<size_t> & offset, std::vector<real> & data) = 0;
    virtual void writeTraces(size_t offset, std::vector<real> & data) = 0;

    inline void readCoord(CoordPair items, std::vector<CoordData> & data)
    {
        readCoord(0U, items, data);
    }
    inline void readCoord(std::vector<CoordArray> & data)
    {
        readCoord(0U, data);
    }
    inline void readCoord(Coord coord, std::vector<CoordData> & data)
    {
        readCoord(0U, coord, data);
    }
    inline void writeCoord(Coord coord, std::vector<CoordData> & data)
    {
        writeCoord(0U, coord, data);
    }
    inline void writeCoord(std::vector<CoordArray> & data)
    {
        writeCoord(0U, data);
    }
    inline void readTraces(std::vector<real> & data)
    {
        readTraces(0U, data);
    }
    inline void writeTraces(std::vector<real> & data)
    {
        writeTraces(0U, data);
    }

    void writeFile(Header & header, std::vector<CoordArray> & coord, std::vector<real> & data)
    {
        assert((coord.size() == data.size() / header.ns) && (header.nt*header.ns == data.size()));
        writeHeader(header);
    }
    void readFile(Header & header, std::vector<CoordArray> & coord, std::vector<real> & data)
    {
        header = readHeader();
    }
};

#ifndef __ICC
constexpr 
#endif
Interface::CoordPair getCoordPair(Coord pair)
{
    switch (pair)
    {
        case Coord::Src :
            return std::make_pair(BlockMd::xSrc, BlockMd::ySrc);
        case Coord::Rcv :
            return std::make_pair(BlockMd::xRcv, BlockMd::yRcv);
        case Coord::Cmp :
            return std::make_pair(BlockMd::xCDP, BlockMd::yCDP);
        case Coord::Lin :
            return std::make_pair(BlockMd::iLin, BlockMd::xLin);
        default : 
            return std::make_pair(BlockMd::ERROR, BlockMd::ERROR);
    }
}
}}
#endif