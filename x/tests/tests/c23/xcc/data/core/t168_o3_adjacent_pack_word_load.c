unsigned int load16(unsigned char *p) {
    return (unsigned int)p[4] | ((unsigned int)p[5] << 8);
}
