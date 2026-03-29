// test-options.cpp
#include <gtest/gtest.h>

#include "options.h"

using namespace xas;

TEST(OptionsTest, ParsesAllFlagsAndFiles)
{
    // Simulate: xas -o out.o -c -g -Iinc -DDEBUG=1 -v -m pasmo in1.s in2.s
    char *argv[] = {
        (char *)"xas",
        (char *)"-o", (char *)"out.o",
        (char *)"-c",
        (char *)"-g",
        (char *)"-Iinc",
        (char *)"-DDEBUG=1",
        (char *)"-v",
        (char *)"-m", (char *)"pasmo",
        (char *)"in1.s",
        (char *)"in2.s"};
    int argc = sizeof(argv) / sizeof(argv[0]);

    options opts = parse_args(argc, argv);

    EXPECT_EQ(opts.output_file, "out.o");
    EXPECT_TRUE(opts.compile_only);
    EXPECT_TRUE(opts.debug);
    ASSERT_EQ(opts.include_dirs.size(), 1u);
    EXPECT_EQ(opts.include_dirs[0], "inc");
    ASSERT_EQ(opts.defines.size(), 1u);
    EXPECT_EQ(opts.defines[0], "DEBUG=1");
    EXPECT_TRUE(opts.verbose);
    EXPECT_EQ(opts.masm, "pasmo");
    ASSERT_EQ(opts.input_files.size(), 2u);
    EXPECT_EQ(opts.input_files[0], "in1.s");
    EXPECT_EQ(opts.input_files[1], "in2.s");
}

TEST(OptionsTest, DefaultValuesWhenMinimal)
{
    // Simulate: xas foo.s
    char *argv[] = {
        (char *)"xas",
        (char *)"foo.s"};
    int argc = sizeof(argv) / sizeof(argv[0]);

    options opts = parse_args(argc, argv);

    EXPECT_EQ(opts.output_file, "");
    EXPECT_FALSE(opts.compile_only);
    EXPECT_FALSE(opts.debug);
    EXPECT_TRUE(opts.include_dirs.empty());
    EXPECT_TRUE(opts.defines.empty());
    EXPECT_FALSE(opts.verbose);
    EXPECT_EQ(opts.masm, "");
    ASSERT_EQ(opts.input_files.size(), 1u);
    EXPECT_EQ(opts.input_files[0], "foo.s");
}
