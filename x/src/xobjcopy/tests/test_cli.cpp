//
// xobjcopy CLI tests
//
// MIT License (see: LICENSE)
//
#include <xobjcopy/cli.h>
#include <xobjcopy/errors.h>

TEST(cli_parses_gnu_style_conversion_switches) {
    char prog[] = "xobjcopy";
    char in[] = "input.rel";
    char out[] = "output.o";
    char input_target[] = "--input-target=rel";
    char output_target[] = "--output-target=elf";
    char strip[] = "--strip-debug";
    char* argv[] = {prog, input_target, output_target, strip, in, out};

    auto opts = xobjcopy::cli::parse(6, argv);
    ASSERT_EQ(opts.input_file.string(), "input.rel");
    ASSERT_EQ(opts.output_file.string(), "output.o");
    ASSERT(opts.input_target.has_value());
    ASSERT(opts.output_target.has_value());
    ASSERT_EQ(static_cast<int>(opts.input_target.value()),
              static_cast<int>(xobjcopy::target_kind::rel));
    ASSERT_EQ(static_cast<int>(opts.output_target.value()),
              static_cast<int>(xobjcopy::target_kind::elf));
    ASSERT(opts.strip_debug);
}

TEST(cli_rejects_conflicting_outputs) {
    char prog[] = "xobjcopy";
    char in[] = "input.rel";
    char out1[] = "output1.o";
    char out2[] = "output2.o";
    char dash_o[] = "-o";
    char* argv[] = {prog, dash_o, out1, in, out2};
    ASSERT_THROWS(xobjcopy::cli::parse(5, argv), xobjcopy::usage_error);
}
