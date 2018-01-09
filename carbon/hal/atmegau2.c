/* This is done in a separate compilation unit so that the atmegau2 module index enumerations are correct. */

#define __private_include__
#include <flipper/carbon.h>
#include <flipper/atmegau2/modules.h>

int carbon_select_atmegau2(struct _lf_device *device) {
	_button.index = _button_id;
	_gpio.index = _gpio_id;
	_led.index = _led_id;
	_uart0.index = _uart0_id;
	_wdt.index = _wdt_id;
	return lf_success;
}
