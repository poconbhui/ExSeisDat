#ifndef PIOLWRAPTESTSREADDIRECTWRAPTESTS_INCLUDE_GUARD
#define PIOLWRAPTESTSREADDIRECTWRAPTESTS_INCLUDE_GUARD

#include "mockexseis.hh"
#include "mockparam.hh"
#include <memory>

void test_PIOL_File_ReadDirect(
  std::shared_ptr<PIOL::ExSeis*> piol,
  std::shared_ptr<PIOL::File::Param*> param);

#endif  // PIOLWRAPTESTSREADDIRECTWRAPTESTS_INCLUDE_GUARD
