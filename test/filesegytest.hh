#include <iconv.h>
#include <string.h>
#include <memory>
#include <typeinfo>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "tglobal.hh"
#include "anc/cmpi.hh"
#include "data/datampiio.hh"
#include "object/objsegy.hh"
#include "share/units.hh"
#include "share/segy.hh"
#include "share/datatype.hh"

#define private public
#define protected public
#include "file/filesegy.hh"
#undef private
#undef protected

namespace PIOL { namespace File {
extern int16_t deScale(const geom_t val);
}}
using PIOL::File::deScale;
using PIOL::File::grid_t;
using PIOL::File::coord_t;
using PIOL::File::TraceParam;

using namespace testing;
using namespace PIOL;

enum Hdr : size_t
{
    Increment  = 3216U,
    NumSample  = 3220U,
    Type       = 3224U,
    Sort       = 3228U,
    Units      = 3254U,
    SEGYFormat = 3500U,
    FixedTrace = 3502U,
    Extensions = 3504U,
};

enum TrHdr : size_t
{
    SeqNum      = 0U,
    SeqFNum     = 4U,
    ORF         = 8U,
    TORF        = 12U,
    RcvElv      = 40U,
    SurfElvSrc  = 44U,
    SrcDpthSurf = 48U,
    DtmElvRcv   = 52U,
    DtmElvSrc   = 56U,
    WtrDepSrc   = 60U,
    WtrDepRcv   = 64U,
    ScaleElev   = 68U,
    ScaleCoord  = 70U,
    xSrc        = 72U,
    ySrc        = 76U,
    xRcv        = 80U,
    yRcv        = 84U,
    xCMP        = 180U,
    yCMP        = 184U,
    il          = 188U,
    xl          = 192U
};

class MockObj : public Obj::Interface
{
    public :
    MockObj(std::shared_ptr<ExSeisPIOL> piol_, const std::string name_, std::shared_ptr<Data::Interface> data_)
               : Obj::Interface(piol_, name_, data_) {}
    MOCK_CONST_METHOD0(getFileSz, size_t(void));
    MOCK_CONST_METHOD1(readHO, void(uchar *));
    MOCK_CONST_METHOD1(setFileSz, void(csize_t));
    MOCK_CONST_METHOD1(writeHO, void(const uchar *));
#warning TODO: Separate out groups of functions to separate files
#warning Not covered yet beyond sz=1
    MOCK_CONST_METHOD4(readDOMD, void(csize_t, csize_t, csize_t, uchar *));
    MOCK_CONST_METHOD4(writeDOMD, void(csize_t, csize_t, csize_t, const uchar *));

#warning Not covered yet.
    MOCK_CONST_METHOD4(readDODF, void(csize_t, csize_t, csize_t, uchar *));
    MOCK_CONST_METHOD4(writeDODF, void(csize_t, csize_t, csize_t, const uchar *));
};

class FileSEGYTest : public Test
{
    public :
    std::shared_ptr<ExSeisPIOL> piol;
    std::shared_ptr<MockObj> mock;
    const File::SEGYOpt fileSegyOpt;
    const Obj::SEGYOpt objSegyOpt;
    Data::MPIIOOpt dataOpt;
    Comm::MPIOpt opt;
    bool testEBCDIC;
    std::string testString = {"This is a string for testing EBCDIC conversion etc."};
    File::Interface * file;
    std::vector<uchar> tr;
    size_t nt = 40U;
    size_t ns = 200U;
    geom_t inc = 10;
    csize_t format = 5;
    std::vector<uchar> ho;

    FileSEGYTest()
    {
        testEBCDIC = false;
        file = nullptr;
        opt.initMPI = false;
        piol = std::make_shared<ExSeisPIOL>(opt);

        ho.resize(SEGSz::getHOSz());
    }
    ~FileSEGYTest()
    {
        if (file != nullptr)
            delete file;
    }
/*    void testHO(bool testEBCDIC)
    {
        makeMock(testEBCDIC);
        file = new File::SEGY(piol, notFile, fileSegyOpt, mock);
        piol->isErr();
    }*/
    void testTr()
    {
        makeMockSEGY<false>();
        initTrBlock();
    }
    template <bool WRITE = true>
    void makeSEGY(std::string name)
    {
        if (file != nullptr)
            delete file;

        if (WRITE)
            dataOpt.mode = MPI_MODE_UNIQUE_OPEN | MPI_MODE_CREATE | MPI_MODE_RDWR |
                           MPI_MODE_DELETE_ON_CLOSE | MPI_MODE_EXCL;

        file = new File::SEGY(piol, name, fileSegyOpt, objSegyOpt, dataOpt);
        piol->isErr();

        if (WRITE)
            writeHO<false>();
    }

    template <bool WRITE = true, bool makeCall = true>
    void makeMockSEGY()
    {
        if (file != nullptr)
            delete file;
        if (mock != nullptr)
            mock.reset();
        mock = std::make_shared<MockObj>(piol, notFile, nullptr);
        Mock::AllowLeak(mock.get());

        ho.resize(SEGSz::getHOSz());
        if (!WRITE)
        {
            EXPECT_CALL(*mock, getFileSz()).Times(Exactly(1)).WillOnce(Return(SEGSz::getHOSz() + nt*SEGSz::getDOSz(ns)));
            initReadHOMock(testEBCDIC);
        }
        else
            EXPECT_CALL(*mock, getFileSz()).Times(Exactly(1)).WillOnce(Return(0U));

        file = new File::SEGY(piol, notFile, fileSegyOpt, mock);

        if (WRITE && makeCall)
        {
            piol->isErr();
            writeHO<true>();
        }
    }

    void initReadHOMock(bool testEBCDIC)
    {
        if (testEBCDIC)
        {
            // Create an EBCDID string to convert back to ASCII in the test
            size_t tsz = testString.size();
            size_t tsz2 = tsz;
            char * t = &testString[0];
            char * newText = reinterpret_cast<char *>(ho.data());
            iconv_t toAsc = iconv_open("EBCDICUS//", "ASCII//");
            ::iconv(toAsc, &t, &tsz, &newText, &tsz2);
            iconv_close(toAsc);
        }
        else
        {
            for (size_t i = 0; i < testString.size(); i++)
                ho[i] = testString[i];
        }
        for (size_t i = testString.size(); i < SEGSz::getTextSz(); i++)
            ho[i] = ho[i % testString.size()];

        ho[NumSample] = (ns >> 8) & 0xFF;
        ho[NumSample+1] = ns & 0xFF;
        ho[Increment+1] = int(inc);
        ho[Type+1] = format;
        EXPECT_CALL(*mock, readHO(_)).Times(Exactly(1)).WillOnce(SetArrayArgument<0>(ho.begin(), ho.end()));
    }

    void initTrBlock()
    {
        tr.resize(nt * SEGSz::getMDSz());
        for (size_t i = 0; i < nt; i++)
        {
            uchar * md = &tr[i * SEGSz::getMDSz()];
            getBigEndian(ilNum(i), &md[il]);
            getBigEndian(xlNum(i), &md[xl]);

            int16_t scale;
            int16_t scal1 = deScale(xNum(i));
            int16_t scal2 = deScale(yNum(i));

            if (scal1 > 1 || scal2 > 1)
                scale = std::max(scal1, scal2);
            else
                scale = std::min(scal1, scal2);

            getBigEndian(scale, &md[ScaleCoord]);
            getBigEndian(int32_t(std::lround(xNum(i)/scale)), &md[xSrc]);
            getBigEndian(int32_t(std::lround(yNum(i)/scale)), &md[ySrc]);
        }
    }

    void initReadTrMock(size_t ns, size_t offset)
    {
        std::vector<uchar>::iterator iter = tr.begin() + offset*SEGSz::getMDSz();
        EXPECT_CALL(*mock.get(), readDOMD(offset, ns, 1U, _))
                    .Times(Exactly(2))
                    .WillRepeatedly(SetArrayArgument<3>(iter, iter + SEGSz::getMDSz()));

        grid_t line;
        file->readGridPoint(File::Grid::Line, offset, 1U, &line);
        ASSERT_EQ(ilNum(offset), line.first);
        ASSERT_EQ(xlNum(offset), line.second);

        coord_t src;
        file->readCoordPoint(File::Coord::Src, offset, 1U, &src);
        ASSERT_DOUBLE_EQ(xNum(offset), src.first);
        ASSERT_DOUBLE_EQ(yNum(offset), src.second);
    }

    void initReadTrHdrsMock(size_t ns, size_t nt)
    {
        EXPECT_CALL(*mock.get(), readDOMD(0, ns, nt, _))
                    .Times(Exactly(2))
                    .WillRepeatedly(SetArrayArgument<3>(tr.begin(), tr.end()));

        std::vector<File::grid_t> line(nt);
        file->readGridPoint(File::Grid::Line, 0U, nt, line.data());

        std::vector<File::coord_t> src(nt);
        file->readCoordPoint(File::Coord::Src, 0U, nt, src.data());

        for (size_t i = 0; i < nt; i++)
        {
            ASSERT_EQ(ilNum(i), line[i].first);
            ASSERT_EQ(xlNum(i), line[i].second);

            ASSERT_DOUBLE_EQ(xNum(i), src[i].first);
            ASSERT_DOUBLE_EQ(yNum(i), src[i].second);
        }
    }

/*    void makeWriteSEGY()
    {
        mock = std::make_shared<MockObj>(piol, notFile, nullptr);
        Mock::AllowLeak(mock.get());

        EXPECT_CALL(*mock, getFileSz()).Times(Exactly(1)).WillOnce(Return(0U));

        file = new File::SEGY(piol, notFile, fileSegyOpt, mock);
        piol->isErr();
    }*/

    template <bool MOCK = true>
    void writeHO()
    {
        if (MOCK)
        {
            size_t fsz = SEGSz::getHOSz() + nt*SEGSz::getDOSz(ns);
            EXPECT_CALL(*mock, setFileSz(fsz)).Times(Exactly(1));

            for (size_t i = 0U; i < std::min(testString.size(), SEGSz::getTextSz()); i++)
                ho[i] = testString[i];
            ho[NumSample+1] = ns;
            ho[Increment+1] = inc;
            ho[Type+1] = format;
            ho[3255U] = 1;
            ho[3500U] = 1;
            ho[3503U] = 1;
            ho[3505U] = 0;

            EXPECT_CALL(*mock, writeHO(_)).Times(Exactly(1)).WillOnce(check0(ho.data(), SEGSz::getHOSz()));
        }

        file->writeNt(nt);
        piol->isErr();

        file->writeNs(ns);
        piol->isErr();

        file->writeInc(geom_t(inc*SI::Micro));
        piol->isErr();

        file->writeText(testString);
        piol->isErr();
    }

    void writeTrHdrGridTest(size_t offset)
    {
        std::vector<uchar> tr(SEGSz::getMDSz());
        getBigEndian(ilNum(offset), tr.data()+il);
        getBigEndian(xlNum(offset), tr.data()+xl);
        getBigEndian<int16_t>(1, &tr[ScaleCoord]);

        EXPECT_CALL(*mock, writeDOMD(offset, ns, 1U, _)).Times(Exactly(1))
                                                        .WillOnce(check3(tr.data(), SEGSz::getMDSz()));

        TraceParam prm;
        prm.line = {ilNum(offset), xlNum(offset)};
        file->writeTraceParam(offset, 1U, &prm);
    }

    void initWriteTrHdrCoord(std::pair<size_t, size_t> item, std::pair<int32_t, int32_t> val, int16_t scal,
                                       size_t offset, std::vector<uchar> * tr)
    {
        getBigEndian(scal, tr->data()+70U);
        getBigEndian(val.first, tr->data()+item.first);
        getBigEndian(val.second, tr->data()+item.second);
        EXPECT_CALL(*mock, writeDOMD(offset, ns, 1U, _)).Times(Exactly(1))
                                                        .WillOnce(check3(tr->data(), SEGSz::getMDSz()));
    }

    template <bool MOCK = true>
    void readTraceTest(csize_t offset)
    {
        if (MOCK && mock == nullptr)
        {
            std::cerr << "Using Mock when not initialised: LOC: " << __LINE__ << std::endl;
            return;
        }
        std::vector<uchar> buf;
        if (MOCK)
        {
            if (nt * ns)
            {
                buf.resize(nt * ns * sizeof(float));
                for (size_t i = 0U; i < nt * ns; i++)
                {
                    float val = getPattern(SEGSz::getDODFLoc(offset, ns)
                                          + i + (i / SEGSz::getDFSz(ns)) * SEGSz::getDOSz(ns));
                    getBigEndian(toint(val), &buf[i*sizeof(float)]);
                }
                EXPECT_CALL(*mock, readDODF(offset, ns, nt, _))
                            .Times(Exactly(1)).WillOnce(SetArrayArgument<3>(buf.begin(), buf.end()));
            }
        }

        std::vector<float> bufnew(nt * ns);
        file->readTrace(offset, nt, bufnew.data());
        for (size_t i = 0U; i < nt * ns; i++)
            ASSERT_EQ(bufnew[i], float(getPattern(SEGSz::getDODFLoc(offset, ns)
                                                  + i + (i / SEGSz::getDFSz(ns)) * SEGSz::getDOSz(ns))));
    }
    template <bool MOCK = true>
    void writeTraceTest(csize_t offset)
    {
        if (MOCK && mock == nullptr)
        {
            std::cerr << "Using Mock when not initialised: LOC: " << __LINE__ << std::endl;
            return;
        }
        std::vector<uchar> buf;
        if (MOCK)
        {
            if (nt * ns)
            {
                buf.resize(nt * ns * sizeof(float));
                for (size_t i = 0U; i < nt * ns; i++)
                {
                    float val = getPattern(SEGSz::getDODFLoc(offset, ns)
                                          + i + (i / SEGSz::getDFSz(ns)) * SEGSz::getDOSz(ns));
                    getBigEndian(toint(val), &buf[i*sizeof(float)]);
                }
                EXPECT_CALL(*mock, writeDODF(offset, ns, nt, _))
                            .Times(Exactly(1)).WillOnce(check3(buf.data(), nt * SEGSz::getDFSz(ns)));
            }
        }

        std::vector<float> bufnew(nt * ns);
        for (size_t i = 0U; i < nt * ns; i++)
            bufnew[i] = float(getPattern(SEGSz::getDODFLoc(offset, ns)
                                                  + i + (i / SEGSz::getDFSz(ns)) * SEGSz::getDOSz(ns)));
        file->writeTrace(offset, nt, bufnew.data());

        if (MOCK == false)
            readTraceTest<MOCK>(offset);
    }
};
typedef FileSEGYTest FileSEGYWrite;
typedef FileSEGYTest FileSEGYRead;
typedef FileSEGYTest FileSEGYIntegRead;
typedef FileSEGYTest FileSEGYIntegWrite;

