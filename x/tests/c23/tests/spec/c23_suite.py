"""
C23 feature catalog and source generators for the compatibility suite.
"""

from __future__ import annotations

from textwrap import dedent


def source(text: str) -> str:
    return dedent(text).lstrip()


def c_source(description: str, headers: list[str], body: str) -> str:
    include_block = "\n".join(f"#include <{header}>" for header in headers)
    return source(
        f"""\
        //
        // {description}
        // Generated from tests/spec/c23_suite.py.
        //
        // This file is part of the C23 compatibility suite.
        //

        {include_block}

        {body}
        """
    )


def run_feature(
    feature_id: str,
    category: str,
    title: str,
    headers: list[str],
    body: str,
    references: list[str],
    support: str = "required",
    extra_files: dict[str, str] | None = None,
    cflags: list[str] | None = None,
) -> dict[str, object]:
    return {
        "id": feature_id,
        "category": category,
        "title": title,
        "kind": "run",
        "support": support,
        "references": references,
        "allowed_stdout": [
            f"OK {feature_id}\n",
            f"NOT-CLAIMED {feature_id}\n" if support != "required" else "",
        ],
        "files": {"main.c": c_source(title, headers, body), **(extra_files or {})},
        "cflags": cflags or [],
    }


def compile_feature(
    feature_id: str,
    category: str,
    title: str,
    headers: list[str],
    body: str,
    references: list[str],
    support: str = "required",
    extra_files: dict[str, str] | None = None,
    cflags: list[str] | None = None,
) -> dict[str, object]:
    return {
        "id": feature_id,
        "category": category,
        "title": title,
        "kind": "compile",
        "support": support,
        "references": references,
        "files": {"main.c": c_source(title, headers, body), **(extra_files or {})},
        "cflags": cflags or [],
    }


def negative_compile_feature(
    feature_id: str,
    category: str,
    title: str,
    headers: list[str],
    body: str,
    references: list[str],
    support: str = "required",
    extra_files: dict[str, str] | None = None,
    cflags: list[str] | None = None,
) -> dict[str, object]:
    return {
        "id": feature_id,
        "category": category,
        "title": title,
        "kind": "negative-compile",
        "support": support,
        "references": references,
        "files": {"main.c": c_source(title, headers, body), **(extra_files or {})},
        "cflags": cflags or [],
    }


FEATURES: list[dict[str, object]] = [
    run_feature(
        "stdc_version_202311",
        "core-language",
        "__STDC_VERSION__ reports a C23 language mode.",
        ["stdio.h"],
        """
        #if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
        #error This compiler is not in a C23 language mode.
        #endif

        int main(void)
        {
            puts("OK stdc_version_202311");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.10.10.2"],
    ),
    run_feature(
        "keywords_core",
        "core-language",
        "Promoted keywords such as bool, thread_local, alignas, and alignof work as core syntax.",
        ["stdio.h"],
        """
        alignas(32) thread_local int tls_value = 7;
        static_assert(alignof(tls_value) >= 32);

        int main(void)
        {
            bool enabled = true;

            if (!enabled || false)
                return 1;

            if (tls_value != 7)
                return 1;

            puts("OK keywords_core");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.4.1", "N3220 6.7.2", "N3220 6.7.6"],
    ),
    run_feature(
        "static_assert_single_argument",
        "core-language",
        "The one-argument static_assert form is accepted.",
        ["stdio.h"],
        """
        static_assert(sizeof(int) >= 2);

        int main(void)
        {
            puts("OK static_assert_single_argument");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.7.11"],
    ),
    run_feature(
        "empty_parameter_list_is_void",
        "core-language",
        "Function declarations with an empty parameter list behave like (void).",
        ["stdio.h"],
        """
        int sample();
        int sample()
        {
            return 17;
        }

        static_assert(_Generic(&sample, int (*)(void): 1, default: 0));

        int main(void)
        {
            if (sample() != 17)
                return 1;

            puts("OK empty_parameter_list_is_void");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.7.7.4"],
    ),
    run_feature(
        "labels_before_declarations",
        "core-language",
        "Labels may appear before declarations and at the end of compound statements.",
        ["stdio.h"],
        """
        int main(void)
        {
        start:
            int value = 23;
            if (value != 23)
                return 1;

            {
                goto done;
            done:
            }

            puts("OK labels_before_declarations");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.8"],
    ),
    run_feature(
        "empty_initializer",
        "initialization",
        "Objects can be initialized with empty braces.",
        ["stdio.h"],
        """
        struct pair
        {
            int left;
            int right;
        };

        int main(void)
        {
            int values[3] = {};
            struct pair p = {};

            if (values[0] != 0 || values[1] != 0 || values[2] != 0)
                return 1;

            if (p.left != 0 || p.right != 0)
                return 1;

            puts("OK empty_initializer");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.7.10"],
    ),
    run_feature(
        "compound_literal_storage_class",
        "initialization",
        "Compound literals accept storage-class specifiers such as static.",
        ["stdio.h"],
        """
        static int *sample(void)
        {
            return (static int[2]){1, 2};
        }

        int main(void)
        {
            int *first = sample();
            int *second = sample();

            if (first != second)
                return 1;

            if (first[0] != 1 || first[1] != 2)
                return 1;

            first[0] = 9;
            if (sample()[0] != 9)
                return 1;

            puts("OK compound_literal_storage_class");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.5.3.6"],
    ),
    run_feature(
        "constexpr_objects",
        "initialization",
        "constexpr object definitions participate in constant-expression contexts.",
        ["stdio.h"],
        """
        constexpr int answer = 42;
        static_assert(answer == 42);
        int table[answer == 42 ? 1 : -1];

        int main(void)
        {
            if ((int)(sizeof(table) / sizeof(table[0])) != 1)
                return 1;

            puts("OK constexpr_objects");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.7.2", "N3220 6.6"],
    ),
    run_feature(
        "typeof_and_typeof_unqual",
        "types",
        "typeof and typeof_unqual deduce qualified and unqualified forms as specified.",
        ["stdio.h"],
        """
        int main(void)
        {
            const int value = 23;
            static_assert(_Generic((typeof(value) *)0, const int *: 1, default: 0));
            static_assert(_Generic((typeof_unqual(value) *)0, int *: 1, default: 0));

            puts("OK typeof_and_typeof_unqual");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.5.1.1"],
    ),
    run_feature(
        "auto_type_inference",
        "types",
        "The enhanced auto specifier infers object types from initializers.",
        ["stdio.h"],
        """
        int main(void)
        {
            auto whole = 23;
            auto fraction = 1.5;

            static_assert(_Generic(whole, int: 1, default: 0));
            static_assert(_Generic(fraction, double: 1, default: 0));

            if (whole != 23)
                return 1;

            if (fraction <= 1.4 || fraction >= 1.6)
                return 1;

            puts("OK auto_type_inference");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N2891", "N3220 6.7.2"],
    ),
    run_feature(
        "unnamed_parameters_in_definition",
        "functions",
        "Function definitions may omit parameter names.",
        ["stdio.h"],
        """
        int constant_sum(int, int)
        {
            return 23;
        }

        int main(void)
        {
            if (constant_sum(7, 8) != 23)
                return 1;

            puts("OK unnamed_parameters_in_definition");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3006", "N3220 6.9.1"],
    ),
    run_feature(
        "bare_ellipsis_va_start",
        "functions",
        "Variadic functions may use a bare ellipsis and call va_start without a named parameter.",
        ["stdarg.h", "stdio.h"],
        """
        static int sum_three(...)
        {
            va_list ap;
            int total = 0;

            va_start(ap);
            total += va_arg(ap, int);
            total += va_arg(ap, int);
            total += va_arg(ap, int);
            va_end(ap);
            return total;
        }

        int main(void)
        {
            if (sum_three(3, 7, 13) != 23)
                return 1;

            puts("OK bare_ellipsis_va_start");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.7.7.4", "N3220 7.16.1.4"],
    ),
    run_feature(
        "vm_types_mandatory",
        "functions",
        "Variably modified types are supported independently of VLA objects.",
        ["stddef.h", "stdio.h"],
        """
        static int array_sum(size_t count, int (*values)[count])
        {
            int total = 0;
            size_t i = 0;

            for (i = 0; i < count; ++i)
                total += (*values)[i];

            return total;
        }

        int main(void)
        {
            int values[4] = {1, 2, 3, 4};

            if (array_sum(4, &values) != 10)
                return 1;

            puts("OK vm_types_mandatory");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N2992", "N3220 6.7.7"],
    ),
    run_feature(
        "unicode_identifiers",
        "lexical",
        "Identifiers can use Unicode character names that conform to the C23 rules.",
        ["stdio.h"],
        """
        static int \\u03b1\\u03b2\\u03b3(void)
        {
            return 23;
        }

        int main(void)
        {
            if (\\u03b1\\u03b2\\u03b3() != 23)
                return 1;

            puts("OK unicode_identifiers");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N2836", "N3220 6.4.2.1"],
    ),
    run_feature(
        "nullptr_and_nullptr_t",
        "types",
        "The nullptr constant and nullptr_t type behave as null pointer values.",
        ["stdbool.h", "stddef.h", "stdio.h"],
        """
        int main(void)
        {
            nullptr_t np = nullptr;
            int *pointer = np;

            if ((bool)np)
                return 1;

            if (pointer != NULL)
                return 1;

            puts("OK nullptr_and_nullptr_t");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3042", "N3220 6.3.2.4", "N3220 7.21"],
    ),
    run_feature(
        "bit_precise_integers",
        "types",
        "Signed _BitInt and BITINT_MAXWIDTH are available.",
        ["limits.h", "stdio.h"],
        """
        #if !defined(BITINT_MAXWIDTH)
        #error BITINT_MAXWIDTH must be defined in C23.
        #endif

        int main(void)
        {
            _BitInt(9) left = 200;
            _BitInt(9) right = 23;
            _BitInt(10) total = left + right;

            if (BITINT_MAXWIDTH < 9)
                return 1;

            if (total != 223)
                return 1;

            puts("OK bit_precise_integers");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3035", "N3220 6.2.6.2"],
    ),
    run_feature(
        "bitint_literal_suffixes",
        "types",
        "The wb and uwb literal suffixes produce bit-precise integer constants.",
        ["stdio.h"],
        """
        int main(void)
        {
            static_assert(_Generic(3wb, _BitInt(3): 1, default: 0));
            static_assert(_Generic(3uwb, unsigned _BitInt(2): 1, default: 0));

            if ((_BitInt(3))3wb != 3)
                return 1;

            if ((unsigned _BitInt(2))3uwb != 3u)
                return 1;

            puts("OK bitint_literal_suffixes");
            return 0;
        }
        """,
        ["N3220 6.4.4.2", "N3035"],
    ),
    run_feature(
        "enhanced_enums_large_values",
        "types",
        "Normal enumerations can represent values outside the old int-only range.",
        ["stdio.h"],
        """
        enum huge_value
        {
            huge_constant = 1ULL << 40
        };

        int main(void)
        {
            enum huge_value value = huge_constant;

            if ((unsigned long long)value != (1ULL << 40))
                return 1;

            puts("OK enhanced_enums_large_values");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3029", "N3220 6.7.3.3"],
    ),
    run_feature(
        "fixed_underlying_enums",
        "types",
        "Enumerations may specify a fixed underlying type with colon syntax.",
        ["stdio.h"],
        """
        enum tiny : unsigned char
        {
            tiny_constant = 200
        };

        int main(void)
        {
            static_assert(_Generic((enum tiny)tiny_constant, unsigned char: 1, default: 0));
            puts("OK fixed_underlying_enums");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3030", "N3220 6.7.3.3"],
    ),
    run_feature(
        "tag_redeclaration_same_scope",
        "types",
        "A tagged type may be redeclared with the same content in the same scope.",
        ["stdio.h"],
        """
        struct point
        {
            int x;
            int y;
        };

        struct point
        {
            int x;
            int y;
        };

        int main(void)
        {
            struct point p = {2, 3};

            if (p.x + p.y != 5)
                return 1;

            puts("OK tag_redeclaration_same_scope");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3037", "N3220 6.2.7", "N3220 6.7.3"],
    ),
    negative_compile_feature(
        "kandr_definition_removed",
        "removed-or-incompatible",
        "Identifier-list function definitions are no longer supported.",
        ["stdio.h"],
        """
        int sum(a, b)
        int a;
        int b;
        {
            return a + b;
        }

        int main(void)
        {
            printf("%d\\n", sum(20, 3));
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.9.1"],
    ),
    run_feature(
        "integer_model_probe",
        "implementation-model",
        "The suite probes the C23 integer model assumptions that removed legacy sign representations.",
        ["limits.h", "stdio.h", "string.h"],
        """
        int main(void)
        {
            signed char negative_one = -1;
            unsigned char raw = 0;

            memcpy(&raw, &negative_one, sizeof(raw));

            if (raw != UCHAR_MAX)
                return 1;

            puts("OK integer_model_probe");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.2.6.2"],
    ),
    run_feature(
        "ptrdiff_width_floor",
        "implementation-model",
        "PTRDIFF_WIDTH reflects the C23 minimum width rule.",
        ["stddef.h", "stdio.h"],
        """
        static_assert(PTRDIFF_WIDTH >= 16);

        int main(void)
        {
            puts("OK ptrdiff_width_floor");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 7.21"],
    ),
    run_feature(
        "has_c_attribute_operator",
        "attributes",
        "__has_c_attribute exposes the standard attribute revisions.",
        ["stdio.h"],
        """
        #if __has_c_attribute(maybe_unused) < 202106L
        #error maybe_unused should be visible through __has_c_attribute.
        #endif

        #if __has_c_attribute(noreturn) < 202202L
        #error noreturn should be visible through __has_c_attribute.
        #endif

        int main(void)
        {
            puts("OK has_c_attribute_operator");
            return 0;
        }
        """,
        ["N3220 Annex M.1", "N3220 6.10.2"],
    ),
    run_feature(
        "attr_maybe_unused",
        "attributes",
        "The [[maybe_unused]] attribute is accepted.",
        ["stdio.h"],
        """
        int main(void)
        {
            [[maybe_unused]] int value = 23;

            puts("OK attr_maybe_unused");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.7.13.2"],
    ),
    compile_feature(
        "attr_deprecated",
        "attributes",
        "The [[deprecated]] attribute is accepted.",
        ["stdio.h"],
        """
        [[deprecated("legacy interface")]] int old_value(void)
        {
            return 23;
        }

        int main(void)
        {
            return old_value() == 23 ? 0 : 1;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.7.13.1"],
    ),
    run_feature(
        "attr_fallthrough",
        "attributes",
        "The [[fallthrough]] attribute is accepted in switch statements.",
        ["stdio.h"],
        """
        int main(void)
        {
            int value = 1;
            int total = 0;

            switch (value) {
            case 1:
                total += 20;
                [[fallthrough]];
            case 2:
                total += 3;
                break;
            default:
                return 1;
            }

            if (total != 23)
                return 1;

            puts("OK attr_fallthrough");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.7.13.3"],
    ),
    run_feature(
        "attr_nodiscard",
        "attributes",
        "The [[nodiscard]] attribute is accepted on functions.",
        ["stdio.h"],
        """
        [[nodiscard]] static int compute_value(void)
        {
            return 23;
        }

        int main(void)
        {
            if (compute_value() != 23)
                return 1;

            puts("OK attr_nodiscard");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.7.13.4"],
    ),
    run_feature(
        "attr_noreturn",
        "attributes",
        "The [[noreturn]] attribute is accepted on functions.",
        ["setjmp.h", "stdio.h"],
        """
        static jmp_buf jump_buffer;

        [[noreturn]] static void leave_now(void)
        {
            longjmp(jump_buffer, 1);
        }

        int main(void)
        {
            if (setjmp(jump_buffer) == 0) {
                leave_now();
                return 1;
            }

            puts("OK attr_noreturn");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.7.13.5"],
    ),
    run_feature(
        "attr_reproducible",
        "attributes",
        "The [[reproducible]] attribute is accepted on function declarators.",
        ["stdio.h"],
        """
        static int twice(int x) [[reproducible]];
        static int twice(int x)
        {
            return x * 2;
        }

        int main(void)
        {
            if (twice(11) != 22)
                return 1;

            puts("OK attr_reproducible");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.7.13.8"],
    ),
    run_feature(
        "attr_unsequenced",
        "attributes",
        "The [[unsequenced]] attribute is accepted on function declarators.",
        ["stdio.h"],
        """
        static int add_one(int x) [[unsequenced]];
        static int add_one(int x)
        {
            return x + 1;
        }

        int main(void)
        {
            if (add_one(22) != 23)
                return 1;

            puts("OK attr_unsequenced");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.7.13.8"],
    ),
    run_feature(
        "va_opt",
        "preprocessor",
        "The __VA_OPT__ preprocessor feature handles optional commas.",
        ["stdio.h", "string.h"],
        """
        #define APPEND(buffer, format, ...) \\
            snprintf((buffer), sizeof(buffer), (format) __VA_OPT__(,) __VA_ARGS__)

        int main(void)
        {
            char first[32];
            char second[32];

            APPEND(first, "%s", "hello");
            APPEND(second, "%s %d", "value", 23);

            if (strcmp(first, "hello") != 0)
                return 1;

            if (strcmp(second, "value 23") != 0)
                return 1;

            puts("OK va_opt");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.10.5"],
    ),
    run_feature(
        "has_include",
        "preprocessor",
        "The __has_include conditional inclusion operator works in preprocessing expressions.",
        ["stdio.h"],
        """
        #if !__has_include(<stdio.h>)
        #error __has_include should detect stdio.h.
        #endif

        #if __has_include(<this_header_should_not_exist_anywhere.h>)
        #error __has_include must report a missing header as unavailable.
        #endif

        int main(void)
        {
            puts("OK has_include");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.10.2"],
    ),
    run_feature(
        "elifdef_elifndef",
        "preprocessor",
        "The #elifdef and #elifndef directives work as conditional branches.",
        ["stdio.h"],
        """
        #define FEATURE_PRESENT 1

        #if 0
        #error The first branch should not be selected.
        #elifdef FEATURE_PRESENT
        #define BRANCH_VALUE 23
        #elifndef FEATURE_PRESENT
        #define BRANCH_VALUE -1
        #else
        #define BRANCH_VALUE -2
        #endif

        int main(void)
        {
            if (BRANCH_VALUE != 23)
                return 1;

            puts("OK elifdef_elifndef");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.10.2"],
    ),
    compile_feature(
        "warning_directive",
        "preprocessor",
        "The #warning directive is accepted.",
        ["stdio.h"],
        """
        #warning C23 warning directive smoke test.

        int main(void)
        {
            puts("warning directive compiled");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.10.8"],
    ),
    run_feature(
        "embed",
        "preprocessor",
        "The #embed directive and __has_embed operator can embed binary payloads.",
        ["stdio.h"],
        """
        #if __has_embed("embed_payload.txt") != __STDC_EMBED_FOUND__
        #error The embed payload should be discoverable.
        #endif

        static const unsigned char payload[] = {
        #embed "embed_payload.txt"
        };

        int main(void)
        {
            if (sizeof(payload) != 4)
                return 1;

            if (payload[0] != 'C' || payload[1] != '2')
                return 1;

            if (payload[2] != '3' || payload[3] != '\\n')
                return 1;

            puts("OK embed");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3017", "N3220 6.10.10"],
        extra_files={"embed_payload.txt": "C23\n"},
    ),
    run_feature(
        "binary_integer_literals",
        "lexical",
        "Binary integer literals with 0b and 0B prefixes are accepted.",
        ["stdio.h"],
        """
        int main(void)
        {
            unsigned int value = 0b10100101u;

            if (value != 165u)
                return 1;

            puts("OK binary_integer_literals");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.4.4.2"],
    ),
    run_feature(
        "digit_separators",
        "lexical",
        "Digit separators with the single quote character are accepted in integer literals.",
        ["stdio.h"],
        """
        int main(void)
        {
            unsigned long long big = 1'000'000ull;
            unsigned int mask = 0b1010'0101u;

            if (big != 1000000ull)
                return 1;

            if (mask != 165u)
                return 1;

            puts("OK digit_separators");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.4.4.2"],
    ),
    run_feature(
        "char8_t_and_u8_char",
        "unicode",
        "u8 character literals and char8_t are supported.",
        ["stdio.h", "uchar.h"],
        """
        int main(void)
        {
            char8_t text[] = u8"AB";
            static_assert(_Generic((u8'A'), char8_t: 1, default: 0));

            if (sizeof(text) != 3)
                return 1;

            if (text[0] != (char8_t)'A' || text[1] != (char8_t)'B')
                return 1;

            puts("OK char8_t_and_u8_char");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.4.4.4", "N3220 7.30"],
    ),
    run_feature(
        "utf_string_encodings",
        "unicode",
        "u8, u, and U string literals use UTF-8, UTF-16, and UTF-32 code units.",
        ["stdint.h", "stdio.h", "uchar.h"],
        """
        int main(void)
        {
            char8_t utf8_text[] = u8"\\u00A2";
            char16_t utf16_text[] = u"\\u00A2";
            char32_t utf32_text[] = U"\\u00A2";

            if (sizeof(utf8_text) != 3)
                return 1;

            if (utf8_text[0] != 0xC2 || utf8_text[1] != 0xA2 || utf8_text[2] != 0)
                return 1;

            if (utf16_text[0] != 0x00A2 || utf16_text[1] != 0)
                return 1;

            if (utf32_text[0] != 0x000000A2 || utf32_text[1] != 0)
                return 1;

            puts("OK utf_string_encodings");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.4.5"],
    ),
    run_feature(
        "mbrtoc8_c8rtomb",
        "unicode",
        "The mbrtoc8 and c8rtomb functions are available in <uchar.h>.",
        ["limits.h", "stdio.h", "string.h", "uchar.h"],
        """
        int main(void)
        {
            mbstate_t to_c8 = {0};
            mbstate_t to_mb = {0};
            char8_t c8 = 0;
            char buffer[MB_LEN_MAX];
            size_t converted = mbrtoc8(&c8, "A", 1, &to_c8);

            if (converted != 1 || c8 != (char8_t)'A')
                return 1;

            converted = c8rtomb(buffer, c8, &to_mb);
            if (converted != 1 || buffer[0] != 'A')
                return 1;

            puts("OK mbrtoc8_c8rtomb");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3031", "N3220 7.30.1"],
    ),
    run_feature(
        "mixed_string_concatenation",
        "unicode",
        "Mixed wide and narrow literal concatenation is supported.",
        ["stdio.h", "wchar.h"],
        """
        int main(void)
        {
            const wchar_t *text = L"A" "B";

            if (text[0] != L'A' || text[1] != L'B' || text[2] != L'\\0')
                return 1;

            puts("OK mixed_string_concatenation");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 6.4.5"],
    ),
    run_feature(
        "basic_character_set_additions",
        "lexical",
        "The basic source character set additions can appear in character and string literals.",
        ["stdio.h", "string.h"],
        """
        int main(void)
        {
            char text[] = {'@', '$', '`', '\\0'};

            if (strcmp(text, "@$`") != 0)
                return 1;

            puts("OK basic_character_set_additions");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 5.2.1"],
    ),
    run_feature(
        "header_version_macros",
        "library",
        "Changed library headers define their C23 version macros.",
        [
            "inttypes.h",
            "math.h",
            "setjmp.h",
            "stdarg.h",
            "stdatomic.h",
            "stdbit.h",
            "stdckdint.h",
            "stddef.h",
            "stdint.h",
            "stdio.h",
            "stdlib.h",
            "string.h",
            "tgmath.h",
            "time.h",
            "uchar.h",
            "wchar.h",
        ],
        """
        #define CHECK_VERSION(name) \\
            do { \\
                if ((name) != 202311L) \\
                    return 1; \\
            } while (0)

        int main(void)
        {
            CHECK_VERSION(__STDC_VERSION_MATH_H__);
            CHECK_VERSION(__STDC_VERSION_SETJMP_H__);
            CHECK_VERSION(__STDC_VERSION_STDARG_H__);
            CHECK_VERSION(__STDC_VERSION_STDATOMIC_H__);
            CHECK_VERSION(__STDC_VERSION_STDBIT_H__);
            CHECK_VERSION(__STDC_VERSION_STDCKDINT_H__);
            CHECK_VERSION(__STDC_VERSION_STDDEF_H__);
            CHECK_VERSION(__STDC_VERSION_STDINT_H__);
            CHECK_VERSION(__STDC_VERSION_STDIO_H__);
            CHECK_VERSION(__STDC_VERSION_STDLIB_H__);
            CHECK_VERSION(__STDC_VERSION_STRING_H__);
            CHECK_VERSION(__STDC_VERSION_TGMATH_H__);
            CHECK_VERSION(__STDC_VERSION_TIME_H__);
            CHECK_VERSION(__STDC_VERSION_UCHAR_H__);
            CHECK_VERSION(__STDC_VERSION_WCHAR_H__);

            puts("OK header_version_macros");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 Clause 7"],
    ),
    run_feature(
        "qualifier_preserving_library_functions",
        "library",
        "Qualifier-preserving search functions retain const qualification in their return types.",
        ["stdio.h", "string.h", "wchar.h"],
        """
        int main(void)
        {
            const char *const_text = "abc";
            char text[] = "abc";

            static_assert(_Generic(strchr(const_text, 'b'), const char *: 1, default: 0));
            static_assert(_Generic(strchr(text, 'b'), char *: 1, default: 0));
            static_assert(_Generic(memchr(const_text, 'b', 3), const void *: 1, default: 0));
            static_assert(_Generic(wcschr(L"abc", L'b'), const wchar_t *: 1, default: 0));

            puts("OK qualifier_preserving_library_functions");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3020", "N3220 7.24.5", "N3220 7.31.5"],
    ),
    run_feature(
        "stdbit_header",
        "library",
        "The new <stdbit.h> header exposes C23 bit utilities.",
        ["limits.h", "stdbit.h", "stdio.h"],
        """
        int main(void)
        {
            unsigned int value = 0b1011000u;

            if (stdc_count_ones(value) != 3)
                return 1;

            if (stdc_bit_width(value) != 7u)
                return 1;

            if (stdc_has_single_bit(8u) != 1)
                return 1;

            if (stdc_bit_floor(10u) != 8u)
                return 1;

            if (stdc_bit_ceil(10u) != 16u)
                return 1;

            if (stdc_rotate_left(1u, 3) != 8u)
                return 1;

            puts("OK stdbit_header");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3001", "N3220 7.18"],
    ),
    run_feature(
        "stdckdint_header",
        "library",
        "The new <stdckdint.h> header exposes checked integer operations.",
        ["limits.h", "stdckdint.h", "stdio.h"],
        """
        int main(void)
        {
            int result = 0;

            if (ckd_add(&result, 20, 3))
                return 1;

            if (result != 23)
                return 1;

            if (!ckd_add(&result, INT_MAX, 1))
                return 1;

            puts("OK stdckdint_header");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 7.20"],
    ),
    run_feature(
        "memset_explicit",
        "library",
        "The memset_explicit function is available.",
        ["stdio.h", "string.h"],
        """
        int main(void)
        {
            unsigned char buffer[6] = "secret";
            size_t i = 0;

            memset_explicit(buffer, 0xA5, sizeof(buffer));

            for (i = 0; i < sizeof(buffer); ++i) {
                if (buffer[i] != 0xA5)
                    return 1;
            }

            puts("OK memset_explicit");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 7.24.6.1"],
    ),
    run_feature(
        "memalignment",
        "library",
        "The memalignment function reports pointer alignment.",
        ["stdalign.h", "stdio.h", "stdlib.h"],
        """
        int main(void)
        {
            alignas(64) unsigned char buffer[64];
            size_t alignment = memalignment(buffer);

            if (alignment < 64)
                return 1;

            puts("OK memalignment");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3044", "N3220 7.24.3"],
    ),
    run_feature(
        "free_sized_and_free_aligned_sized",
        "library",
        "free_sized and free_aligned_sized are available in <stdlib.h>.",
        ["stdio.h", "stdlib.h"],
        """
        int main(void)
        {
            void *plain = malloc(64);
            void *aligned = aligned_alloc(32, 64);

            if (plain == NULL || aligned == NULL)
                return 1;

            free_sized(plain, 64);
            free_aligned_sized(aligned, 32, 64);

            puts("OK free_sized_and_free_aligned_sized");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 7.24.3"],
    ),
    run_feature(
        "unreachable_macro",
        "library",
        "The unreachable feature is available through <stddef.h>.",
        ["stddef.h", "stdio.h"],
        """
        int main(void)
        {
            if (0)
                unreachable();

            puts("OK unreachable_macro");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N2826", "N3220 7.21"],
    ),
    run_feature(
        "memccpy",
        "library",
        "The memccpy function is available through <string.h>.",
        ["stdio.h", "string.h"],
        """
        int main(void)
        {
            char dst[8] = {0};
            const char src[] = "abcd";
            void *stop = memccpy(dst, src, 'c', sizeof(src));

            if (stop != &dst[3])
                return 1;

            if (memcmp(dst, "abc", 3) != 0)
                return 1;

            puts("OK memccpy");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 7.24.2.1"],
    ),
    run_feature(
        "strdup_strndup",
        "library",
        "The strdup and strndup functions are available.",
        ["stdio.h", "stdlib.h", "string.h"],
        """
        int main(void)
        {
            char *full = strdup("hello");
            char *part = strndup("hello", 3);
            int failed = 0;

            if (full == NULL || part == NULL)
                failed = 1;

            if (!failed && strcmp(full, "hello") != 0)
                failed = 1;

            if (!failed && strcmp(part, "hel") != 0)
                failed = 1;

            free(full);
            free(part);

            if (failed)
                return 1;

            puts("OK strdup_strndup");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 7.24.6"],
    ),
    run_feature(
        "gmtime_r_localtime_r",
        "time",
        "The gmtime_r and localtime_r functions are available.",
        ["stdio.h", "time.h"],
        """
        int main(void)
        {
            time_t epoch = 0;
            struct tm utc_result;
            struct tm local_result;

            if (gmtime_r(&epoch, &utc_result) == NULL)
                return 1;

            if (localtime_r(&epoch, &local_result) == NULL)
                return 1;

            if (utc_result.tm_year != 70 || utc_result.tm_mon != 0 || utc_result.tm_mday != 1)
                return 1;

            if (mktime(&local_result) == (time_t)-1)
                return 1;

            puts("OK gmtime_r_localtime_r");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 7.29.3"],
    ),
    run_feature(
        "strftime_extended_month_formats",
        "time",
        "Extended month-name formats added through POSIX harmonization are available.",
        ["locale.h", "stdio.h", "string.h", "time.h"],
        """
        int main(void)
        {
            struct tm value = {0};
            char full_month[32];
            char alt_month[32];

            if (setlocale(LC_TIME, "C") == NULL)
                return 1;

            value.tm_year = 124;
            value.tm_mon = 2;
            value.tm_mday = 1;

            if (strftime(full_month, sizeof(full_month), "%B", &value) == 0)
                return 1;

            if (strftime(alt_month, sizeof(alt_month), "%OB", &value) == 0)
                return 1;

            if (strcmp(full_month, "March") != 0)
                return 1;

            if (strcmp(alt_month, "March") != 0)
                return 1;

            puts("OK strftime_extended_month_formats");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 7.29.3.5"],
    ),
    run_feature(
        "timespec_getres",
        "time",
        "timespec_getres and the additional time-base hooks are available.",
        ["stdio.h", "time.h"],
        """
        int main(void)
        {
            struct timespec resolution = {0};
            struct timespec current = {0};

            if (timespec_getres(&resolution, TIME_UTC) != TIME_UTC)
                return 1;

            if (resolution.tv_nsec < 0 || resolution.tv_nsec >= 1000000000L)
                return 1;

            if (timespec_get(&current, TIME_UTC) != TIME_UTC)
                return 1;

            puts("OK timespec_getres");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 7.29.2.6", "N3220 7.29.2.7"],
    ),
    run_feature(
        "timegm",
        "time",
        "The timegm function is available.",
        ["stdio.h", "time.h"],
        """
        int main(void)
        {
            struct tm value = {0};
            time_t result;

            value.tm_year = 70;
            value.tm_mon = 0;
            value.tm_mday = 1;
            result = timegm(&value);

            if (result != 0)
                return 1;

            puts("OK timegm");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 7.29.3"],
    ),
    run_feature(
        "call_once_mandated",
        "threads",
        "call_once support is mandated and usable.",
        ["stdio.h", "threads.h"],
        """
        static once_flag global_once = ONCE_FLAG_INIT;
        static int calls = 0;

        static void initialize_once(void)
        {
            ++calls;
        }

        int main(void)
        {
            call_once(&global_once, initialize_once);
            call_once(&global_once, initialize_once);

            if (calls != 1)
                return 1;

            puts("OK call_once_mandated");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3220 7.28.2.1"],
    ),
    run_feature(
        "printf_scanf_binary_and_width_modifiers",
        "io",
        "Binary formatting and the wN length modifiers are available for printf and scanf.",
        ["inttypes.h", "stdint.h", "stdio.h", "string.h"],
        """
        int main(void)
        {
            char buffer[64];
            unsigned int parsed_binary = 0;
            int8_t parsed_small = 0;

            if (snprintf(buffer, sizeof(buffer), "%#" PRIb8 " %w8d", (uint8_t)10, (int8_t)-5) < 0)
                return 1;

            if (strcmp(buffer, "0b1010 -5") != 0)
                return 1;

            if (sscanf("1010 -5", "%b %w8d", &parsed_binary, &parsed_small) != 2)
                return 1;

            if (parsed_binary != 10u || parsed_small != -5)
                return 1;

            puts("OK printf_scanf_binary_and_width_modifiers");
            return 0;
        }
        """,
        ["N3220 Annex M.2", "N3016", "N3220 7.23.6.1", "N3220 7.23.6.2", "N3220 7.22.1"],
    ),
    run_feature(
        "iec60559_binary_types",
        "iec-60559",
        "Binary floating-point TS integration exposes _FloatN types when claimed.",
        ["stdio.h"],
        """
        #ifdef __STDC_IEC_60559_TYPES__
        int main(void)
        {
            _Float32 left = (_Float32)1.5;
            _Float64 right = (_Float64)2.25;
            double total = (double)(left + (_Float32)right);

            if (total <= 3.74 || total >= 3.76)
                return 1;

            puts("OK iec60559_binary_types");
            return 0;
        }
        #else
        int main(void)
        {
            puts("NOT-CLAIMED iec60559_binary_types");
            return 0;
        }
        #endif
        """,
        ["N3220 Annex M.2", "N3220 5.2.5.3.3"],
        support="conditional",
    ),
    run_feature(
        "iec60559_decimal_types",
        "iec-60559",
        "Decimal floating-point TS integration exposes _DecimalN types when claimed.",
        ["stdio.h"],
        """
        #ifdef __STDC_IEC_60559_DFP__
        int main(void)
        {
            _Decimal64 left = 1.20dd;
            _Decimal64 right = 2.30dd;
            _Decimal64 total = left + right;

            if (total != 3.50dd)
                return 1;

            puts("OK iec60559_decimal_types");
            return 0;
        }
        #else
        int main(void)
        {
            puts("NOT-CLAIMED iec60559_decimal_types");
            return 0;
        }
        #endif
        """,
        ["N3220 Annex M.2", "N3220 5.2.5.3.4", "N3220 6.4.4.3"],
        support="conditional",
    ),
    run_feature(
        "iec60559_fromfp",
        "iec-60559",
        "The fromfp and ufromfp functions are available when IEC 60559 binary floating-point is claimed.",
        ["limits.h", "math.h", "stdio.h"],
        """
        #ifdef __STDC_IEC_60559_BFP__
        int main(void)
        {
            if (fromfp(2.25, FP_INT_DOWNWARD, INT_WIDTH) != 2)
                return 1;

            if (ufromfp(3.75, FP_INT_TOWARDZERO, UINT_WIDTH) != 3u)
                return 1;

            puts("OK iec60559_fromfp");
            return 0;
        }
        #else
        int main(void)
        {
            puts("NOT-CLAIMED iec60559_fromfp");
            return 0;
        }
        #endif
        """,
        ["N3220 Annex M.2", "N3220 7.12.9.10"],
        support="conditional",
    ),
    run_feature(
        "iec60559_minmax",
        "iec-60559",
        "The new IEC 60559 minimum and maximum functions are available when claimed.",
        ["math.h", "stdio.h"],
        """
        #ifdef __STDC_IEC_60559_BFP__
        int main(void)
        {
            if (fmaximum(1.0, 2.0) != 2.0)
                return 1;

            if (fminimum(1.0, 2.0) != 1.0)
                return 1;

            puts("OK iec60559_minmax");
            return 0;
        }
        #else
        int main(void)
        {
            puts("NOT-CLAIMED iec60559_minmax");
            return 0;
        }
        #endif
        """,
        ["N3220 Annex M.2", "N3220 7.12.12"],
        support="conditional",
    ),
]


for feature in FEATURES:
    if feature["kind"] == "run":
        allowed_stdout = [item for item in feature["allowed_stdout"] if item]
        feature["allowed_stdout"] = allowed_stdout
