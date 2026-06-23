unsigned char b64_chr[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

int main(void) {
    if (sizeof(b64_chr) != 27) return 1;
    if (b64_chr[0] != 'A') return 2;
    if (b64_chr[25] != 'Z') return 3;
    if (b64_chr[26] != 0) return 4;
    return 0;
}
