////////////////////////////////////////////////////////////////////////////////
/// @file
/// @author Cathal O Broin - cathal@ichec.ie - first commit
/// @copyright TBD. Do not distribute
/// @date December 2016
/// @brief
/// @details Functions etc for C++ API
////////////////////////////////////////////////////////////////////////////////

#include "cppfileapi.hh"
#include "data/datampiio.hh"
#include "file/filesegy.hh"
#include "global.hh"
#include "object/objsegy.hh"

namespace PIOL {

ExSeis::ExSeis(const Verbosity maxLevel, MPI_Comm comm) :
    ExSeisPIOL(maxLevel, Comm::MPI::Opt{comm})
{
}

ExSeis::~ExSeis() = default;

size_t ExSeis::getRank(void) const
{
    return comm->getRank();
}

size_t ExSeis::getNumRank(void) const
{
    return comm->getNumRank();
}

void ExSeis::barrier(void) const
{
    comm->barrier();
}

size_t ExSeis::max(size_t n) const
{
    return comm->max(n);
}

void ExSeis::isErr(const std::string& msg) const
{
    ExSeisPIOL::isErr(msg);
}

namespace File {
ReadDirect::ReadDirect(std::shared_ptr<ExSeisPIOL> piol, const std::string name)
{
    const File::ReadSEGY::Opt f;
    const Obj::SEGY::Opt o;
    const Data::MPIIO::Opt d;
    auto data = std::make_shared<Data::MPIIO>(piol, name, d, FileMode::Read);
    auto obj = std::make_shared<Obj::SEGY>(piol, name, o, data, FileMode::Read);
    file     = std::make_shared<File::ReadSEGY>(piol, name, f, obj);
}

ReadDirect::ReadDirect(std::shared_ptr<ReadInterface> file_) : file(file_) {}

WriteDirect::WriteDirect(
  std::shared_ptr<ExSeisPIOL> piol, const std::string name)
{
    const File::WriteSEGY::Opt f;
    const Obj::SEGY::Opt o;
    const Data::MPIIO::Opt d;
    auto data = std::make_shared<Data::MPIIO>(piol, name, d, FileMode::Write);
    auto obj =
      std::make_shared<Obj::SEGY>(piol, name, o, data, FileMode::Write);
    file = std::make_shared<File::WriteSEGY>(piol, name, f, obj);
}

WriteDirect::WriteDirect(std::shared_ptr<WriteInterface> file_) : file(file_) {}

ReadDirect::~ReadDirect()   = default;
WriteDirect::~WriteDirect() = default;

const std::string& ReadDirect::readText(void) const
{
    return file->readText();
}

size_t ReadDirect::readNs(void) const
{
    return file->readNs();
}

size_t ReadDirect::readNt(void) const
{
    return file->readNt();
}

geom_t ReadDirect::readInc(void) const
{
    return file->readInc();
}

void ReadDirect::readParam(
  const size_t offset, const size_t sz, Param* prm) const
{
    file->readParam(offset, sz, prm);
}

void WriteDirect::writeParam(
  const size_t offset, const size_t sz, const Param* prm)
{
    file->writeParam(offset, sz, prm);
}

void ReadDirect::readTrace(
  const size_t offset, const size_t sz, trace_t* trace, Param* prm) const
{
    file->readTrace(offset, sz, trace, prm);
}

void WriteDirect::writeTrace(
  const size_t offset, const size_t sz, trace_t* trace, const Param* prm)
{
    file->writeTrace(offset, sz, trace, prm);
}

void ReadDirect::readTraceNonContiguous(
  const size_t sz, const size_t* offset, trace_t* trace, Param* prm) const
{
    file->readTraceNonContiguous(sz, offset, trace, prm);
}

void ReadDirect::readTraceNonMonotonic(
  const size_t sz, const size_t* offset, trace_t* trace, Param* prm) const
{
    file->readTraceNonMonotonic(sz, offset, trace, prm);
}

ReadModel::ReadModel(std::shared_ptr<ExSeisPIOL> piol, const std::string name) :
    ReadDirect(
      piol,
      name,
      Data::MPIIO::Opt(),
      Obj::SEGY::Opt(),
      File::ReadSEGYModel::Opt())
{
}

std::vector<trace_t> ReadModel::readModel(
  size_t gOffset, size_t numGather, Uniray<size_t, llint, llint>& gather)
{
    return std::dynamic_pointer_cast<File::Model3dInterface>(file)->readModel(
      gOffset, numGather, gather);
}

void WriteDirect::writeTraceNonContiguous(
  const size_t sz, const size_t* offset, trace_t* trace, const Param* prm)
{
    file->writeTraceNonContiguous(sz, offset, trace, prm);
}

void ReadDirect::readParamNonContiguous(
  const size_t sz, const size_t* offset, Param* prm) const
{
    file->readParamNonContiguous(sz, offset, prm);
}

void WriteDirect::writeParamNonContiguous(
  const size_t sz, const size_t* offset, const Param* prm)
{
    file->writeParamNonContiguous(sz, offset, prm);
}

void WriteDirect::writeText(const std::string text_)
{
    file->writeText(text_);
}

void WriteDirect::writeNs(const size_t ns_)
{
    file->writeNs(ns_);
}

void WriteDirect::writeNt(const size_t nt_)
{
    file->writeNt(nt_);
}

void WriteDirect::writeInc(const geom_t inc_)
{
    file->writeInc(inc_);
}

}  // namespace File
}  // namespace PIOL
