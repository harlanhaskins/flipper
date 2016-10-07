#define __private_include__
#include <platform/atsam4s16b.h>

#define BOARD_OSCOUNT (CKGR_MOR_MOSCXTST(0x8))
#define BOARD_PLLBR (CKGR_PLLBR_MULB(0x0f) | CKGR_PLLBR_PLLBCOUNT(0x1) | CKGR_PLLBR_DIVB(0x3))
#define BOARD_MCKR (PMC_MCKR_PRES_CLK_1 | PMC_MCKR_CSS_PLLB_CLK)

#define F_CPU 64000000

void system_init(void) {

	/* Allow the reset pin to reset the device. */
	RSTC -> RSTC_MR |= RSTC_MR_URSTEN;

	/* Initialize the main oscillator. */
	if (!(PMC -> CKGR_MOR & CKGR_MOR_MOSCSEL)) {
		PMC -> CKGR_MOR = CKGR_MOR_KEY(0x37) | BOARD_OSCOUNT | CKGR_MOR_MOSCRCEN | CKGR_MOR_MOSCXTEN;
		while (!(PMC -> PMC_SR & PMC_SR_MOSCXTS));
	}

	/* Configure the clock generator to accept a 3-20MHz oscillator. */
	PMC -> CKGR_MOR = CKGR_MOR_KEY(0x37) | BOARD_OSCOUNT | CKGR_MOR_MOSCRCEN | CKGR_MOR_MOSCXTEN | CKGR_MOR_MOSCSEL;
	while (!(PMC -> PMC_SR & PMC_SR_MOSCSELS));
	PMC -> PMC_MCKR = (PMC -> PMC_MCKR & ~(uint32_t)PMC_MCKR_CSS_Msk) | PMC_MCKR_CSS_MAIN_CLK;
	while (!(PMC -> PMC_SR & PMC_SR_MCKRDY));

	/* Use three wait states for accesses to flash memory. */
	EFC -> EEFC_FMR = (3 << 8);

	/* Initialize PLLA */
	//PMC -> CKGR_PLLAR = BOARD_PLLAR;
	//while (!(PMC -> PMC_SR & PMC_SR_LOCKA));

	/* Initialize PLLB */
	PMC -> CKGR_PLLBR = BOARD_PLLBR;
	while (!(PMC -> PMC_SR & PMC_SR_LOCKB));

	//Use PLLB for the USB clock.
	//REG_PMC_USB = PMC_USB_USBS | PMC_USB_USBDIV(1);

	/* Configure the PLLA for USB. */
	// PMC -> CKGR_PLLAR = (1<<29) | CKGR_PLLAR_DIVA(1) | CKGR_PLLAR_MULA(7) | CKGR_PLLAR_PLLACOUNT_Msk;
	// while(!(PMC -> PMC_SR & PMC_SR_LOCKA));
	// /* Use PLLA for the USB clock. */
	// PMC -> PMC_USB = PMC_USB_USBDIV(1);

	/* Switch to the fast clock. */
	PMC -> PMC_MCKR = (BOARD_MCKR & ~PMC_MCKR_CSS_Msk) | PMC_MCKR_CSS_MAIN_CLK;
	while (!(PMC -> PMC_SR & PMC_SR_MCKRDY));

	PMC -> PMC_MCKR = BOARD_MCKR;
	while (!(PMC -> PMC_SR & PMC_SR_MCKRDY));

}

volatile void delay_ms() {
	uint64_t counter = 1000000;
	while (counter --) __asm__("nop");
}

void system_task(void) {

	PMC_EnablePeripheral(ID_USART0);
	const Pin usart0_pins[] = { PIO_PA5A_RXD0 | PIO_PA6A_TXD0, PIOA, ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT };
	PIO_Configure(usart0_pins, PIO_LISTSIZE(usart0_pins));
	USART_Configure(USART0, USART_MODE_ASYNCHRONOUS, 115200, BOARD_MCK);
	USART_SetTransmitterEnabled(USART0, 1);

	PIOA -> PIO_PER |= (1 << 8);
	PIOA -> PIO_OER |= (1 << 8);

	while (1) {
		USART_WriteBuffer(USART0, "Hello world!", 11);
		PIOA -> PIO_SODR |= (1 << 8);
		PIOA -> PIO_CODR &= ~(1 << 8);
		delay_ms();
		PIOA -> PIO_SODR &= ~(1 << 8);
		PIOA -> PIO_CODR |= (1 << 8);
		delay_ms();
	}

}

void system_deinit(void) {

}
