#define PRESSED       0
#define NOT_PRESSED   1

void gpio_init();

void led_on();
void led_off();
void led_set(uint32_t level);

void power_on();
void power_off();
void power_set(uint32_t level);

uint32_t button_get();
