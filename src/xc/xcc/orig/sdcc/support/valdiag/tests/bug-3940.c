/* bug-3940.c

   An issue resulting in missing qualifiers in the result of a ternary operator with
   differently-qualified second and third operand.
 */

#ifdef TEST1
int i;
_Optional int *poi_2;

void d (_Optional int *poi)
{
  poi_2 = i ? &i : poi; // Bug was observable via (invalid) "pointer target lost _Optional qualifier" warning here
}
#endif

#ifdef TEST2
int i;
const int *poi_2;

void d (const int *poi)
{
  poi_2 = i ? &i : poi; // Bug was observable via (invalid) "pointer target lost const qualifier" warning here
}
#endif

#ifdef TEST3
int *i;
int *const *poi_2;

void d (int *const *poi)
{
  poi_2 = i ? &i : poi; // Bug was observable via (invalid) "pointer target lost const qualifier" warning here
}
#endif

