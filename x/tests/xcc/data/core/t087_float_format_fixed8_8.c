float a = 10.2f;

float area(float w, float h) {
    return w * h + 1.5f;
}

int main(void) {
    return sizeof(float) + (int)area(2.0f, 3.0f);
}
