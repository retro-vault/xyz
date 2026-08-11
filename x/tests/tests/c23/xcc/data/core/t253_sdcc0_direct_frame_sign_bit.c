int sign_sink;

int classify_sign(int value)
{
    volatile int checked = value;

    if (checked >= 0)
        sign_sink += 3;
    else
        sign_sink += 5;
    return checked;
}
