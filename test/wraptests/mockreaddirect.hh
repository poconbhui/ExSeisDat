#ifndef PIOLWRAPTESTSMOCKREADDIRECT_HEADER_GUARD
#define PIOLWRAPTESTSMOCKREADDIRECT_HEADER_GUARD

#include "cppfileapi.hh"

#include "gmock/gmock.h"

namespace PIOL {
namespace File {

class MockReadDirect;
::testing::StrictMock<MockReadDirect>& mockReadDirect();

class MockReadDirect {
  public:
    MOCK_METHOD3(
      ctor,
      void(
        ReadDirect*, std::shared_ptr<ExSeisPIOL> piol, const std::string name));

    MOCK_METHOD2(ctor, void(ReadDirect*, std::shared_ptr<ReadInterface> file));

    MOCK_METHOD1(dtor, void(ReadDirect*));

    MOCK_CONST_METHOD1(readText, const std::string&(const ReadDirect*));

    MOCK_CONST_METHOD1(readNs, size_t(const ReadDirect*));

    MOCK_CONST_METHOD1(readNt, size_t(const ReadDirect*));

    MOCK_CONST_METHOD1(readInc, geom_t(const ReadDirect*));

    MOCK_CONST_METHOD5(
      readTrace,
      void(
        const ReadDirect*,
        const size_t offset,
        const size_t sz,
        trace_t* trace,
        Param* prm));

    MOCK_CONST_METHOD4(
      readParam,
      void(
        const ReadDirect*, const size_t offset, const size_t sz, Param* prm));

    MOCK_CONST_METHOD5(
      readTraceNonContiguous,
      void(
        const ReadDirect*,
        const size_t sz,
        const size_t* offset,
        trace_t* trace,
        Param* prm));

    MOCK_CONST_METHOD5(
      readTraceNonMonotonic,
      void(
        const ReadDirect*,
        const size_t sz,
        const size_t* offset,
        trace_t* trace,
        Param* prm));

    MOCK_CONST_METHOD4(
      readParamNonContiguous,
      void(
        const ReadDirect*, const size_t sz, const size_t* offset, Param* prm));
};

}  // namespace File
}  // namespace PIOL

#endif  // PIOLWRAPTESTSMOCKREADDIRECT_HEADER_GUARD
