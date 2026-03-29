/**
 * Hash table utilities for storing and retrieving items by integer or string keys.
 *
 * This module defines a general-purpose hash table (hTab) and associated
 * functions for insertion, deletion, iteration, and lookups. Each entry
 * stores a hashed key, the original pointer key, and a payload value.
 *
 * Hash tables are commonly used in the compiler to manage symbol tables,
 * configuration maps, or any case where fast indexed access is useful.
 *
 * Copyright (C) 1998, Sandeep Dutta <sandeep.dutta@usa.net>
 * Released under the GNU General Public License v2 or later.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    /* Delete behavior flags */
    typedef enum
    {
        DELETE_CHAIN = 1,
        DELETE_ITEM
    } DELETE_ACTION;

    /* Hash table item node */
    typedef struct hashtItem
    {
        int key;                /* Hash value (integer) */
        void *pkey;             /* Original key (owned by table) */
        void *item;             /* Value associated with key */
        struct hashtItem *next; /* Next item in bucket */
    } hashtItem;

    /* Hash table structure */
    typedef struct hTab
    {
        int size;            /* Number of buckets */
        int minKey;          /* Minimum possible key (for iteration) */
        int maxKey;          /* Maximum possible key (for iteration) */
        hashtItem **table;   /* Array of bucket lists */
        int currKey;         /* Current key for iteration */
        hashtItem *currItem; /* Current item in current bucket */
        int nItems;          /* Total items in table */
    } hTab;

    /* API for creating, managing, and querying hash tables */
    hTab *newHashTable(int size);
    void hTabAddItem(hTab **table, int key, void *item);
    void hTabAddItemLong(hTab **table, int key, void *pkey, void *item);
    void *hTabFindByKey(hTab *table, int key, const void *pkey, int (*compare)(const void *, const void *));
    int hTabDeleteByKey(hTab **table, int key, const void *pkey, int (*compare)(const void *, const void *));
    void hTabDeleteItem(hTab **table, int key, const void *item, DELETE_ACTION action, int (*compare)(const void *, const void *));
    int hTabIsInTable(hTab *table, int key, void *item, int (*compare)(void *, void *));
    void *hTabFirstItem(hTab *table, int *key);
    void *hTabNextItem(hTab *table, int *key);
    hTab *hTabFromTable(hTab *src);
    int isHtabsEqual(hTab *a, hTab *b, int (*compare)(void *, void *));
    hashtItem *hTabSearch(hTab *table, int key);
    void *hTabItemWithKey(hTab *table, int key);
    void hTabAddItemIfNotP(hTab **table, int key, void *item);
    void hTabDeleteAll(hTab *table);
    void *hTabFirstItemWK(hTab *table, int weak_key);
    void *hTabNextItemWK(hTab *table);
    void hTabClearAll(hTab *table);
    int hTabMaxKey(hTab *table);
    void *hTabFindItem(hTab *table, int key, void *item, int (*compare)(void *, void *));

    /* API for string-keyed hashtables (shash) */
    void shash_add(hTab **table, const char *key, const char *value);
    const char *shash_find(hTab *table, const char *key);

#ifdef __cplusplus
}
#endif
