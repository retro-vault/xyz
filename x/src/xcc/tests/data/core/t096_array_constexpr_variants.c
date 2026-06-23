enum { BASE = 7, PAD = 5 };

#define EXTRA 3
#define SCALE(x) ((x) * 2)

static unsigned char arr_enum[BASE];
static unsigned char arr_sizeof[sizeof(int) + 1];
static unsigned char arr_macro[SCALE(EXTRA) + 1];
static unsigned char arr_nested[((BASE + EXTRA) * 2) - 1];
static unsigned char arr_combo[(sizeof(arr_enum) + BASE + EXTRA) / 2];

int main(void) {
    return sizeof(arr_enum) + sizeof(arr_sizeof) + sizeof(arr_macro) +
           sizeof(arr_nested) + sizeof(arr_combo);
}
