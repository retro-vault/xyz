# C23 Feature Catalog

This catalog is generated from `tests/spec/c23_suite.py`.

| Feature | Category | Kind | Support | Notes |
| --- | --- | --- | --- | --- |
| `stdc_version_202311` | `core-language` | `run` | `required` | N3220 Annex M.2; N3220 6.10.10.2 |
| `keywords_core` | `core-language` | `run` | `required` | N3220 Annex M.2; N3220 6.4.1; N3220 6.7.2; N3220 6.7.6 |
| `static_assert_single_argument` | `core-language` | `run` | `required` | N3220 Annex M.2; N3220 6.7.11 |
| `empty_parameter_list_is_void` | `core-language` | `run` | `required` | N3220 Annex M.2; N3220 6.7.7.4 |
| `labels_before_declarations` | `core-language` | `run` | `required` | N3220 Annex M.2; N3220 6.8 |
| `empty_initializer` | `initialization` | `run` | `required` | N3220 Annex M.2; N3220 6.7.10 |
| `compound_literal_storage_class` | `initialization` | `run` | `required` | N3220 Annex M.2; N3220 6.5.3.6 |
| `constexpr_objects` | `initialization` | `run` | `required` | N3220 Annex M.2; N3220 6.7.2; N3220 6.6 |
| `typeof_and_typeof_unqual` | `types` | `run` | `required` | N3220 Annex M.2; N3220 6.5.1.1 |
| `auto_type_inference` | `types` | `run` | `required` | N3220 Annex M.2; N2891; N3220 6.7.2 |
| `unnamed_parameters_in_definition` | `functions` | `run` | `required` | N3220 Annex M.2; N3006; N3220 6.9.1 |
| `bare_ellipsis_va_start` | `functions` | `run` | `required` | N3220 Annex M.2; N3220 6.7.7.4; N3220 7.16.1.4 |
| `vm_types_mandatory` | `functions` | `run` | `required` | N3220 Annex M.2; N2992; N3220 6.7.7 |
| `unicode_identifiers` | `lexical` | `run` | `required` | N3220 Annex M.2; N2836; N3220 6.4.2.1 |
| `nullptr_and_nullptr_t` | `types` | `run` | `required` | N3220 Annex M.2; N3042; N3220 6.3.2.4; N3220 7.21 |
| `bit_precise_integers` | `types` | `run` | `required` | N3220 Annex M.2; N3035; N3220 6.2.6.2 |
| `bitint_literal_suffixes` | `types` | `run` | `required` | N3220 6.4.4.2; N3035 |
| `enhanced_enums_large_values` | `types` | `run` | `required` | N3220 Annex M.2; N3029; N3220 6.7.3.3 |
| `fixed_underlying_enums` | `types` | `run` | `required` | N3220 Annex M.2; N3030; N3220 6.7.3.3 |
| `tag_redeclaration_same_scope` | `types` | `run` | `required` | N3220 Annex M.2; N3037; N3220 6.2.7; N3220 6.7.3 |
| `kandr_definition_removed` | `removed-or-incompatible` | `negative-compile` | `required` | N3220 Annex M.2; N3220 6.9.1 |
| `integer_model_probe` | `implementation-model` | `run` | `required` | N3220 Annex M.2; N3220 6.2.6.2 |
| `ptrdiff_width_floor` | `implementation-model` | `run` | `required` | N3220 Annex M.2; N3220 7.21 |
| `has_c_attribute_operator` | `attributes` | `run` | `required` | N3220 Annex M.1; N3220 6.10.2 |
| `attr_maybe_unused` | `attributes` | `run` | `required` | N3220 Annex M.2; N3220 6.7.13.2 |
| `attr_deprecated` | `attributes` | `compile` | `required` | N3220 Annex M.2; N3220 6.7.13.1 |
| `attr_fallthrough` | `attributes` | `run` | `required` | N3220 Annex M.2; N3220 6.7.13.3 |
| `attr_nodiscard` | `attributes` | `run` | `required` | N3220 Annex M.2; N3220 6.7.13.4 |
| `attr_noreturn` | `attributes` | `run` | `required` | N3220 Annex M.2; N3220 6.7.13.5 |
| `attr_reproducible` | `attributes` | `run` | `required` | N3220 Annex M.2; N3220 6.7.13.8 |
| `attr_unsequenced` | `attributes` | `run` | `required` | N3220 Annex M.2; N3220 6.7.13.8 |
| `va_opt` | `preprocessor` | `run` | `required` | N3220 Annex M.2; N3220 6.10.5 |
| `has_include` | `preprocessor` | `run` | `required` | N3220 Annex M.2; N3220 6.10.2 |
| `elifdef_elifndef` | `preprocessor` | `run` | `required` | N3220 Annex M.2; N3220 6.10.2 |
| `warning_directive` | `preprocessor` | `compile` | `required` | N3220 Annex M.2; N3220 6.10.8 |
| `embed` | `preprocessor` | `run` | `required` | N3220 Annex M.2; N3017; N3220 6.10.10 |
| `binary_integer_literals` | `lexical` | `run` | `required` | N3220 Annex M.2; N3220 6.4.4.2 |
| `digit_separators` | `lexical` | `run` | `required` | N3220 Annex M.2; N3220 6.4.4.2 |
| `char8_t_and_u8_char` | `unicode` | `run` | `required` | N3220 Annex M.2; N3220 6.4.4.4; N3220 7.30 |
| `utf_string_encodings` | `unicode` | `run` | `required` | N3220 Annex M.2; N3220 6.4.5 |
| `mbrtoc8_c8rtomb` | `unicode` | `run` | `required` | N3220 Annex M.2; N3031; N3220 7.30.1 |
| `mixed_string_concatenation` | `unicode` | `run` | `required` | N3220 Annex M.2; N3220 6.4.5 |
| `basic_character_set_additions` | `lexical` | `run` | `required` | N3220 Annex M.2; N3220 5.2.1 |
| `header_version_macros` | `library` | `run` | `required` | N3220 Annex M.2; N3220 Clause 7 |
| `qualifier_preserving_library_functions` | `library` | `run` | `required` | N3220 Annex M.2; N3020; N3220 7.24.5; N3220 7.31.5 |
| `stdbit_header` | `library` | `run` | `required` | N3220 Annex M.2; N3001; N3220 7.18 |
| `stdckdint_header` | `library` | `run` | `required` | N3220 Annex M.2; N3220 7.20 |
| `memset_explicit` | `library` | `run` | `required` | N3220 Annex M.2; N3220 7.24.6.1 |
| `memalignment` | `library` | `run` | `required` | N3220 Annex M.2; N3044; N3220 7.24.3 |
| `free_sized_and_free_aligned_sized` | `library` | `run` | `required` | N3220 Annex M.2; N3220 7.24.3 |
| `unreachable_macro` | `library` | `run` | `required` | N3220 Annex M.2; N2826; N3220 7.21 |
| `memccpy` | `library` | `run` | `required` | N3220 Annex M.2; N3220 7.24.2.1 |
| `strdup_strndup` | `library` | `run` | `required` | N3220 Annex M.2; N3220 7.24.6 |
| `gmtime_r_localtime_r` | `time` | `run` | `required` | N3220 Annex M.2; N3220 7.29.3 |
| `strftime_extended_month_formats` | `time` | `run` | `required` | N3220 Annex M.2; N3220 7.29.3.5 |
| `timespec_getres` | `time` | `run` | `required` | N3220 Annex M.2; N3220 7.29.2.6; N3220 7.29.2.7 |
| `timegm` | `time` | `run` | `required` | N3220 Annex M.2; N3220 7.29.3 |
| `call_once_mandated` | `threads` | `run` | `required` | N3220 Annex M.2; N3220 7.28.2.1 |
| `printf_scanf_binary_and_width_modifiers` | `io` | `run` | `required` | N3220 Annex M.2; N3016; N3220 7.23.6.1; N3220 7.23.6.2; N3220 7.22.1 |
| `iec60559_binary_types` | `iec-60559` | `run` | `conditional` | N3220 Annex M.2; N3220 5.2.5.3.3 |
| `iec60559_decimal_types` | `iec-60559` | `run` | `conditional` | N3220 Annex M.2; N3220 5.2.5.3.4; N3220 6.4.4.3 |
| `iec60559_fromfp` | `iec-60559` | `run` | `conditional` | N3220 Annex M.2; N3220 7.12.9.10 |
| `iec60559_minmax` | `iec-60559` | `run` | `conditional` | N3220 Annex M.2; N3220 7.12.12 |
