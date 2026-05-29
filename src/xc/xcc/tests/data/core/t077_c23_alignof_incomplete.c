// C23: alignof an incomplete array equals alignof the element type.
static_assert(alignof(int[]) == alignof(int), "array align == element align");
static_assert(alignof(char[]) == alignof(char), "char array align");
static_assert(alignof(long[]) == alignof(long), "long array align");

int x;
