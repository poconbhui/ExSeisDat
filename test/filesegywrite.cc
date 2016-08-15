#include "filesegytest.hh"
TEST_F(FileSEGYWrite, FileWriteHO)
{
    makeMockSEGY<true>();
}

TEST_F(FileSEGYWrite, FileWriteHOLongString)
{
    //Extend the string beyond the text boundary
    //Extended text should be dropped in write call
    csize_t sz = testString.size();
    csize_t extendSz = 3400U - sz;
    testString.resize(sz + extendSz);
    for (size_t i = 3200U; i < sz+extendSz; i++)
        testString[i] = uchar(0x7F);

    makeMockSEGY<true>();
}

TEST_F(FileSEGYWrite, FileWriteHOEmptyString)
{
    testString.resize(0);
    makeMockSEGY<true>();
}

TEST_F(FileSEGYWrite, FileWriteTrHdrGrid)
{
    makeMockSEGY<true>();
    for (size_t i = 0; i < nt; i++)
        writeTrHdrGridTest(i);
}

TEST_F(FileSEGYWrite, FileWriteTrHdrCoord1)
{
    makeMockSEGY<true>();
    std::vector<uchar> tr(SEGSz::getMDSz());
    initWriteTrHdrCoord({xCMP, yCMP}, {160010, 240022}, -100, 10, &tr);

    TraceParam prm;
    prm.cmp = {1600.1, 2400.22};
    file->writeTraceParam(10U, 1U, &prm);
}

TEST_F(FileSEGYWrite, FileWriteTrHdrCoord2)
{
    makeMockSEGY<true>();
    std::vector<uchar> tr(SEGSz::getMDSz());
    initWriteTrHdrCoord({xSrc, ySrc}, {1600100, 3400222}, -1000, 10, &tr);

    TraceParam prm;
    prm.src = {1600.1000, 3400.2220};
    file->writeTraceParam(10U, 1U, &prm);
}

TEST_F(FileSEGYWrite, FileWriteTrHdrCoord3)
{
    makeMockSEGY<true>();
    std::vector<uchar> tr(SEGSz::getMDSz());
    initWriteTrHdrCoord({xSrc, ySrc}, {1623001001,   34002220}, -10000, 10, &tr);

    TraceParam prm;
    prm.src = {162300.10009, 3400.22201};
    file->writeTraceParam(10U, 1U, &prm);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// DEATH TESTS ////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
typedef FileSEGYWrite FileSEGYDeath;

TEST_F(FileSEGYDeath, FileWriteAPIBadns)
{
    ns = 0x470000;
    makeMockSEGY<true, false>();
    std::cout << "here1\n";
    file->writeNs(ns);
    std::cout << "here2\n";
    mock.reset();
    std::cout << "here3\n";
    EXPECT_EXIT(piol->isErr(), ExitedWithCode(EXIT_FAILURE), ".*8 3 Fatal Error in PIOL. . Dumping Log 0");
}

#ifdef NT_LIMITS
TEST_F(FileSEGYDeath, FileWriteAPIBadnt)
{
    nt = NT_LIMITS + 1;
    makeMockSEGY<true, false>();
    file->writeNt(nt);

    mock.reset();
    EXPECT_EXIT(piol->isErr(), ExitedWithCode(EXIT_FAILURE), ".*8 3 Fatal Error in PIOL. . Dumping Log 0");
}
#endif

TEST_F(FileSEGYDeath, FileWriteAPIBadinc)
{
    inc = geom_t(1)/geom_t(0);
    makeMockSEGY<true, false>();
    file->writeInc(inc);

    mock.reset();
    EXPECT_EXIT(piol->isErr(), ExitedWithCode(EXIT_FAILURE), ".*8 3 Fatal Error in PIOL. . Dumping Log 0");
}
