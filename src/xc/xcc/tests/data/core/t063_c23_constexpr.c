// C23: constexpr at file scope — object with constant initializer, implicitly const.
constexpr int LIMIT = 32;
int arr[LIMIT];

int get_limit(void) { return LIMIT; }
