#include "mockgetminmax.hh"

namespace PIOL {
namespace File {

void getMinMax(
  ExSeisPIOL* piol,
  size_t offset,
  size_t sz,
  Meta m1,
  Meta m2,
  const Param* prm,
  CoordElem* minmax)
{
    return mockGetMinMax().getMinMax(piol, offset, sz, m1, m2, prm, minmax);
}

}  // namespace File
}  // namespace PIOL
