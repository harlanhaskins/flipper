#define __private_include__
#include <flipper/uart0.h>
#include <flipper/atmegau2/atmegau2.h>
#include <flipper/atmegau2/megausb.h>

uint8_t uart0_buffer[256];
uint8_t idx = 0;

int uart0_configure(uint8_t baud, uint8_t interrupts) {

	UBRR1L = baud;
	UCSR1A &= ~(1 << U2X1);
	/* 8n1 */
	UCSR1C = (1 << UCSZ11) | (1 << UCSZ10);
	/* Enable the receiver, transmitter, and receiver interrupt. */
	UCSR1B = (1 << RXEN1) | (1 << TXEN1);

	if (interrupts) {
		UCSR1B |= (1 << RXCIE1);
	} else {
		UCSR1B &= ~(1 << RXCIE1);
	}

	/* Enable the FSI line as an input. */
	FSI_DDR &= ~(1 << FSI_PIN);
	return lf_success;
}

int uart0_ready(void) {
	return (UCSR1A & (1 << RXC1));
}

int uart0_push(void *source, lf_size_t length) {
	while (length --) {
		uint8_t timeout = UDFNUML + LF_UART_TIMEOUT_MS;
		while (!(UCSR1A & (1 << UDRE1))) {
			lf_assert(UDFNUML != timeout, failure, E_UART0_PUSH_TIMEOUT, "Timeout occurred while pushing to uart0.");
		}
		UDR1 = *(uint8_t *)(source ++);
	}
	return lf_success;
failure:
	return lf_error;
}

int uart0_pull(void *destination, lf_size_t length) {
	/* Drain the buffered data. */
	if (idx) {
		if (length > idx) {
			memcpy(destination, uart0_buffer, idx);
			destination += idx;
			length -= idx;
			idx = 0;
		} else {
			memcpy(destination, uart0_buffer, length);
			memmove(uart0_buffer, uart0_buffer + length, idx - length);
			idx -= length;
			return lf_success;
		}
	}
	while (length--) {
		uint8_t timeout = UDFNUML + LF_UART_TIMEOUT_MS;
		while (!(UCSR1A & (1 << RXC1))) {
			lf_assert(UDFNUML != timeout, failure, E_UART0_PULL_TIMEOUT, "Timeout occurred while pulling from uart0.");
		}
		*(uint8_t *)(destination ++) = UDR1;
	}
	return lf_success;
failure:
	return lf_error;
}

ISR(USART1_RX_vect) {
	if (FSI_IN & (1 << FSI_PIN)) {
		uart0_buffer[idx++] = UDR1;
	} else {
		usb_debug_putchar(UDR1);
	}
}
