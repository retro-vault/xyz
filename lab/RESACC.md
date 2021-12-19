# Resource Accounting

Every system object in xyz is a resource. Examples of resources are threads, memory blocks, synchronization objects, etc.
The operating system keeps books of all allocated resources. 

  > Each resource has an owner. The owner is typically a process that
  > created the resource. For example if a process allocates a memory
  > block it becomes the owner of that block.

Resources of same type are kept in a single linked lists. Each resource has a `sysobj_t` header that looks like this, followed by resource data.

~~~cpp
typedef struct sysobj_s {
    union {
        list_item_t hdr;
        void* next;                     /* binary compatible with list_item_t */
    };
    void* owner;                        /* owners' id */
} sysobj_t;
~~~

## Cleaning up resources

When a process dies it is the job of the operating system to release all resources that this process allocated and did not free. 

