///
/// Example: sort.cc    {#examples_sort_cc}
/// ================
///
/// @todo DOCUMENT ME - Finish documenting example.
///
/// Sort a file in 9 lines
///

#include "flow.hh"

using namespace PIOL;

int main(void)
{
    auto piol = ExSeis::New();
    Set set(piol, "/ichec/work/exseisdat/*dat/10*/b*", "temp");
    set.sort(PIOL_SORTTYPE_OffLine);
    return 0;
}
