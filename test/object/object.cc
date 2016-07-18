#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "global.hh"
#include "anc/cmpi.hh"
#include "data/datampiio.hh"
#define private public
#define protected public
#include "object/object.hh"
#undef private
#undef protected

using namespace testing;
using namespace PIOL;

class ObjectTest : public Test
{
    protected :
    std::shared_ptr<ExSeisPIOL> piol;
    Comm::MPIOpt opt;
    ObjectTest()
    {
        opt.initMPI = false;
        piol = std::make_shared<ExSeisPIOL>(opt);
    }
};

//FakeObject to test the constructor of the abstract Interface class
struct FakeObject : public Obj::Interface
{
    FakeObject(std::shared_ptr<ExSeisPIOL> piol_, const std::string name_, std::shared_ptr<Data::Interface> data_)
               : Obj::Interface(piol_, name_, data_) { }
    FakeObject(std::shared_ptr<ExSeisPIOL> piol_, const std::string name_, const Data::Opt & dataOpt)
               : Obj::Interface(piol_, name_, dataOpt) { }
    size_t getFileSz()
    {
        return 0U;
    }
    void readHO(uchar * ho) { }
};

//In this test we pass the MPI-IO Data Options class.
//We do not use a valid name as we are not interested in the result
TEST_F(ObjectTest, InterfaceConstructor)
{
    std::string name = "!£$%^&*()<>?:@~}{fakefile1234567890";
    Data::MPIIOOpt dataOpt;
    FakeObject fake(piol, name, dataOpt);
    EXPECT_NE(nullptr, fake.data);
    EXPECT_EQ(piol, fake.piol);
    EXPECT_EQ(name, fake.name);
}

//In this test we pass the wrong Data Options class.
//We pass the base class instead of MPIIOOpt (the default class)
TEST_F(ObjectTest, BadInterfaceConstructor)
{
    std::string name = "!£$%^&*()<>?:@~}{fakefile1234567890";
    Data::Opt dataOpt;

    FakeObject fake(piol, name, dataOpt);
    EXPECT_EQ(nullptr, fake.data);
    EXPECT_EQ(piol, fake.piol);
    EXPECT_EQ(name, fake.name);
}
