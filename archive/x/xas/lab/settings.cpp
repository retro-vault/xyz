
// Supported assembler dialects
enum class dialect
{
    zilog, // Zilog official syntax
    sdas   // SDCC's -mz80 assembler (based on asxxxx dialect)
};
// Configurable settings for each assembler dialect
struct dialect_settings
{
    dialect dialect;                   // The assembler dialect being used
    bool case_insensitive;             // Are instructions/registers case-insensitive? (e.g., LD A,B = ld a,b)
    bool case_sensitive_labels;        // Are labels case-sensitive? (e.g., Loop != loop)
    bool dot_prefixed_directives;      // Are directives prefixed with a dot? (e.g., .org vs ORG)
    bool use_square_brackets;          // Use square brackets for memory operands? (e.g., LD A,[HL] vs LD A,(HL))
    char comment_char;                 // Character used for comments (e.g., ; or //)
    std::string equ_directive;         // Directive for defining constants (e.g., EQU, DEFC)
    std::string data_byte_directive;   // Directive for defining byte data (e.g., DB, DEFB)
    std::string data_word_directive;   // Directive for defining word data (e.g., DW, DEFW)
    bool require_label_colon;          // Are colons required after labels? (e.g., loop: vs loop)
    bool end_stops_assembly;           // Does the .end directive stop assembly? (e.g., Brass vs TASM)
    bool support_macros;               // Does the dialect support macro definitions?
    bool support_undocumented_opcodes; // Are undocumented Z80 opcodes supported? (e.g., SLL, IXH)

    // Additional syntax behavior
    bool hash_prefix_for_immediate; // Accepts '#' before immediate values (e.g., LD A,#10)
    bool h_suffix_for_hex;          // Accepts 'h' suffix for hex (e.g., 0FFh)
    bool displacement_prefix_first; // Displacement before register (e.g., 2(ix) vs [ix+2])
};
static asm_settings current_settings;

asm_settings::asm_settings(asm_dialect d) : dialect(d)
{
    switch (d)
    {
    case asm_dialect::zilog:
        case_insensitive = true;
        case_sensitive_labels = false;
        dot_prefixed_directives = false;
        use_square_brackets = false;
        comment_char = ';';
        equ_directive = "EQU";
        data_byte_directive = "DB";
        data_word_directive = "DW";
        require_label_colon = true;
        end_stops_assembly = false;
        support_macros = false;
        support_undocumented_opcodes = false;
        hash_prefix_for_immediate = false;
        h_suffix_for_hex = false;
        displacement_prefix_first = false;
        break;

    case asm_dialect::pasmo:
        case_insensitive = true;
        case_sensitive_labels = false;
        dot_prefixed_directives = false;
        use_square_brackets = true;
        comment_char = ';';
        equ_directive = "EQU";
        data_byte_directive = "DEFB";
        data_word_directive = "DEFW";
        require_label_colon = false;
        end_stops_assembly = true;
        support_macros = true;
        support_undocumented_opcodes = true;
        hash_prefix_for_immediate = false;
        h_suffix_for_hex = true;
        displacement_prefix_first = false;
        break;

    case asm_dialect::tasm:
        case_insensitive = true;
        case_sensitive_labels = false;
        dot_prefixed_directives = true;
        use_square_brackets = false;
        comment_char = ';';
        equ_directive = "EQU";
        data_byte_directive = "DB";
        data_word_directive = "DW";
        require_label_colon = true;
        end_stops_assembly = false;
        support_macros = true;
        support_undocumented_opcodes = false;
        hash_prefix_for_immediate = true;
        h_suffix_for_hex = false;
        displacement_prefix_first = true;
        break;

    case asm_dialect::sdcc:
        case_insensitive = true;
        case_sensitive_labels = false;
        dot_prefixed_directives = true;
        use_square_brackets = true;
        comment_char = ';';
        equ_directive = "EQU";
        data_byte_directive = "DB";
        data_word_directive = "DW";
        require_label_colon = true;
        end_stops_assembly = true;
        support_macros = true;
        support_undocumented_opcodes = false;
        hash_prefix_for_immediate = true;
        h_suffix_for_hex = false;
        displacement_prefix_first = true;
        break;
    }
}

const asm_settings &get_asm_settings()
{
    return current_settings;
}
