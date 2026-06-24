// C23 preprocessor: #elifdef, #elifndef, #warning (suppressed), __VA_OPT__.
#define HAVE_X 1

#ifdef HAVE_X
#define RESULT 10
#elifdef HAVE_Y
#define RESULT 20
#else
#define RESULT 30
#endif

#define LOG(...) __VA_OPT__(int _dummy = __VA_ARGS__)

int get_result(void) { return RESULT; }
