# Resource Accounting

Every system object in the **xyz os** is a resource. Examples of resources are: threads, memory blocks, synchronization objects, etc.
The operating system keeps books of all allocated resources. 

 > Resource accounting makes it possible not only to automatically 
 > clean thread resources, but also to monitor resource usage.

## Linked lists

Resources of same type are kept in linked lists. Generic functions 
to manage linked lists are inside `list.c`. These functions work
on any data structure that has a header, binary compatible with
the `list_item_t` structure.

~~~cpp
typedef struct list_item_s
{
    void *next;         /* next list el. */
} list_item_t;
~~~

Linked link functions don't care about the rest of the structure.

## Derivation of system object from list item

Besides being a member of a linked list, each systemm resource 
also has an owner. The owner is typically a process that created 
the resource. For example if a process allocates a memory block then 
it becomes the owner of that memory block.

This is why each system resource has an enriched header, which is
binary compatible with the `sysobj_t` structure, but also an extension
of a `list_item_t`. In object oriented terms we would think of these
as derived classes.

~~~cpp
typedef struct sysobj_s {
    union {
        list_item_t hdr;
        void* next;                     
    };
    void* owner;                        /* owners' id */
} sysobj_t;
~~~

Additional functionality for managing system objects is and the `sysobj.c` file.

# Cleaning up resources

When a process dies it is the job of the operating system to release all resources that this process allocated (and has not freed!). 
But resource cleansing operation does different things depending 
on resource type. Killing a thread is obviously different to 
releasing a memory block. 

Hence forceach resource there exists a destructor function; 
and the kill function calls them all in correct order.


...to be continued...

