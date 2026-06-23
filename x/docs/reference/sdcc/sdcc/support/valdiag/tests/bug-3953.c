/* bug-3953.c

   A lack of diagnostics on lost qualifiers on assignments when the pointer on the right side originated from array decay.
 */

#pragma std_c23

#ifdef TEST1
struct S1 {
  char m[64];
};

char *str_from_struct(const struct S1 *pcs)
{
  // valid because of array to pointer decay
  static_assert(_Generic(pcs->m,
                         const char *: 1,
                         default: 0));

  // invalid: array to pointer decay does not remove const
  char *m = pcs->m; // constraint violation, no warning /* WARNING */
  m = pcs->m;       // constraint violation, no warning /* WARNING */
  return pcs->m;    // constraint violation, no warning /* WARNING */
}

struct S2 {
  const char m[64];
};

char *str_from_struct2(struct S2 *pcs)
{
  // valid because of array to pointer decay
  static_assert(_Generic(pcs->m,
                         const char *: 1,
                         default: 0));

  // invalid: array to pointer decay does not remove const
  char *m = pcs->m; // constraint violation, no warning /* WARNING */
  m = pcs->m;       // constraint violation, no warning /* WARNING */
  return pcs->m;    // constraint violation, no warning /* WARNING */
}

char *str_from_str(const char *pcc)
{
  static_assert(_Generic(pcc,
                         const char *: 1,
                         default: 0));

  // invalid: violates constraint on assignment
  char *m = pcc; // pointer target lost const qualifier /* WARNING */
  m = pcc;       // pointer target lost const qualifier /* WARNING */
  return pcc;    // pointer target lost const qualifier /* WARNING */
}

#endif

