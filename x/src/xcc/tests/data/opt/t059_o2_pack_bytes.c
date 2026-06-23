unsigned f(unsigned char low, unsigned char high) {
    return (unsigned)low | ((int)high << 8);
}
