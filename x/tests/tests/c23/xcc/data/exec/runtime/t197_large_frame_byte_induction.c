static unsigned int
exercise_large_frame(int seed)
{
    unsigned char bytes[320];
    unsigned int sum = 0;
    unsigned int i;

    for (i = 0; i < sizeof(bytes); ++i)
        bytes[i] = (unsigned char)(seed + i * 3u);
    for (i = 0; i < sizeof(bytes); ++i)
        sum = (unsigned int)(sum + bytes[i]);
    return sum;
}

int
main(void)
{
    unsigned int a = exercise_large_frame(7);
    unsigned int b = exercise_large_frame(19);

    if (a != 39136u)
        return 1;
    if (b != 39904u)
        return 2;
    return 0;
}
