#include "wraptests.h"
#include "wraptesttools.hh"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "exseiswraptest.hh"
#include "rulewraptests.hh"

extern "C" {

void init_wraptests()
{
    // Initialize GoogleTest

    // Argc and argv.
    int argc = 1;
    char* argv = NULL;

    // const char* needed for string literal, non-const char* needed for
    // google test.
    const char* cargv = "cwraptests";
    argv = new char[strlen(cargv)+1];
    strcpy(argv, cargv);

    // Throw so TestBuilder has something to catch and report.
    testing::GTEST_FLAG(throw_on_failure) = true;
    testing::InitGoogleTest(&argc, &argv);

    // Disable GoogleTest printing exceptions.
    testing::TestEventListeners& listeners =
        testing::UnitTest::GetInstance()->listeners();
    listeners.Release(listeners.default_result_printer());

    // Setup and add the CheckReturnListener
    if(PIOL::checkReturnListener == nullptr)
    {
        PIOL::checkReturnListener = new PIOL::CheckReturnListener;
    }
    listeners.Append(PIOL::checkReturnListener);

    //::testing::FLAGS_gmock_verbose = "info";
    ::testing::FLAGS_gmock_verbose = "error";

    // Add test initializers here
    testing::InSequence s;
    test_PIOL_ExSeis();
    test_PIOL_File_Rule();
}

void wraptest_ok()
{
    PIOL::returnChecker().Call();
}

}
