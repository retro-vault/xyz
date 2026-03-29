/**
 * Stack frame block tree management for local symbol allocation.
 *
 * This module manages the hierarchical block tree used to represent
 * lexical scopes in a function. It supports child-parent relations,
 * symbol attachment, and stack-based allocation of locals.
 *
 * Used during code generation to group variables by block and
 * ensure proper allocation based on lifetime and scope overlap.
 *
 * Copyright (C) 2011 Philipp Klaus Krause <philipp@informatik.uni-frankfurt.de>
 * Released under the GNU General Public License v2 or later.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    /* Clear block tree. To be called after each function. */
    void btree_clear(void);

    /* Add child as a sub-block of parent. */
    void btree_add_child(int parent, int child);

    /* Gives the lowest common ancestor for blocks a and b. */
    int btree_lowest_common_ancestor(int a, int b);

    /* Add symbol to block tree for allocation. */
    void btree_add_symbol(struct symbol *s);

    /* Allocate all previously added symbols on the stack. */
    void btree_alloc(void);

#ifdef __cplusplus
}
#endif
