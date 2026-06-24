// C23: char8_t — distinct unsigned char type for UTF-8.
char8_t buf[4] = {0xC3, 0xA9, 0x00, 0x00}; // é in UTF-8

int get_first(void) { return (int)buf[0]; }
int get_second(void) { return (int)buf[1]; }
