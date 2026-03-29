# Assembly Style Guide for yos

## 1. File Header Format
```asm
;;; filename.s
;;;
;;; Description of what this file does
;;;
;;; MIT License (see: LICENSE)
;;; Copyright (C) [YEAR] [AUTHOR]
;;;
;;; YYYY-MM-DD   [INITIALS]
```

## 2. Global Routine Comments
```asm
;;; extern [return_type] routine_name([params]);
;;; [param_name]  [param location/description]
;;; return:       [where return value goes]
;;; affects:      [list of registers modified]
;;; notes:        [optional: important implementation notes]
```

## 3. Local Label Naming
- All local labels must be prefixed with dot: `.label_name`
- Exception: labels that are part of public named areas (e.g., `key_map`)

## 4. Local Subroutine Documentation
For local subroutines (prefixed with dot), if complex:
```asm
;;; .subroutine_name
;;; param:  [description]
;;; return: [where result goes]
;;; affects: [registers used]
```

## 5. End-of-Line Comments
- Align to column 41 if the instruction fits before it
- If can't fit, move comment to a line above with `;` not `;;`
- Use single `;;` for block comments, single `;` for inline

## 6. Utility Functions
All utility/helper functions: `__function_name` (double underscore)
Examples: `__kbd_scan`, `__thread_robin`, `__sys_vec_set`

## 7. Register Aliases
Use consistent naming:
- Pairs: `bc`, `de`, `hl`
- Individual bytes: `a`, `b`, `c`, `d`, `e`, `h`, `l`
- Alternate: `af`, `af'`, `bc'`, `de'`, `hl'`, `ix`, `iy`
