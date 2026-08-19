/* A function used only by a static initializer is live and address-taken. */
struct service_api {
    void (*first)(void);
    void (*second)(void);
};

static void retained_first(void)
{
}

static void retained_second(void)
{
}

static void genuinely_unused(void)
{
}

static struct service_api service = {
    .first = retained_first,
    .second = retained_second,
};

void call_service(void)
{
    service.first();
    service.second();
}
