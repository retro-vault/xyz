int classify_small(unsigned char value);
volatile int small_word_sink;

int matches_one(unsigned char value)
{
    int result = classify_small(value);
    if (result == 1)
        small_word_sink += 3;
    else
        small_word_sink += 5;
    return result;
}

int differs_two(unsigned char value)
{
    int result = classify_small(value);
    if (result != 2)
        small_word_sink += 7;
    else
        small_word_sink += 9;
    return result;
}
