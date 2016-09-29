#include <stdio.h>
#define __private_include__
#include <flipper.h>
#include <platform/posix.h>
#include <platform/fvm.h>
#include <platform/atsam4s16b.h>

#define v(i) atoi(argv[i])

#define SOH 0x01
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define ETB 0x17
#define CAN 0x18
#define XLEN 128

struct __attribute__((__packed__)) _xpacket {
	uint8_t header;
	uint8_t number;
	uint8_t _number;
	uint8_t data[XLEN];
	uint16_t checksum;
};

uint8_t applet[] = {
	0x09, 0x48, 0x0A, 0x49, 0x0A, 0x4A, 0x02, 0xE0,
	0x08, 0xC9, 0x08, 0xC0, 0x01, 0x3A, 0x00, 0x2A,
	0xFA, 0xD1, 0x04, 0x48, 0x00, 0x28, 0x01, 0xD1,
	0x01, 0x48, 0x85, 0x46, 0x70, 0x47, 0x00, 0xBF,
	0x00, 0x00, 0x02, 0x20, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x34, 0x00, 0x00, 0x20,
	0x80, 0x00, 0x00, 0x00
};

#define _APPLET IRAM_ADDR
#define _APPLET_DESTINATION _APPLET + 0x28
#define USERSPACE _APPLET + sizeof(applet)

void sam_ba_jump(uint32_t address) {
	char buffer[11];
	sprintf(buffer, "G%08X#", address);
	uart.push(buffer, sizeof(buffer) - 1);
}

void sam_ba_transfer(uint32_t address) {
	char buffer[12];
	sprintf(buffer, "S%08X,#", address);
	uart.push(buffer, sizeof(buffer) - 1);
}

void sam_ba_write_word(uint32_t destination, uint32_t word) {
	char buffer[20];
	sprintf(buffer, "W%08X,%08X#", destination, word);
	uart.push(buffer, sizeof(buffer) - 1);
}

void sam_ba_write_fcr0(uint8_t command, uint32_t page) {
	sam_ba_write_word(0x400E0A04U, (EEFC_FCR_FKEY(0x5A) | EEFC_FCR_FARG(page) | EEFC_FCR_FCMD(command)));
}

int sam_ba_copy(uint32_t destination, void *source, uint32_t length) {
	sam_ba_transfer(destination);
	/* Calculate the number of packets needed to perform the transfer. */
	int packets = lf_ceiling(length, XLEN);
	uint8_t pno = 1;
	void *_source = source;
	uint8_t retries = 0;
	while (packets --) {
		uint32_t _len = XLEN;
		if (length < _len) {
			_len = length;
		}
		/* Construct the base packet. */
		struct _xpacket packet = { SOH, pno, ~pno, {0}, 0x00 };
		/* Copy the data into the packet. */
		memcpy(packet.data, _source + ((pno - 1) * XLEN), _len);
		/* Perform the checksum. */
		packet.checksum = little(lf_checksum(packet.data, sizeof(packet.data)));
		/* Transfer the packet. */
		uart.push(&packet, sizeof(struct _xpacket));
		/* Increment the source. */
		_source += XLEN;
		/* Decrement the length. */
		length -= _len;
		/* Increment the packet number. */
		pno ++;
	}
	/* Send end of transmission. */
	uart.put(EOT);
	return lf_success;
}

void *load_page_data(FILE *firmware, size_t size) {
	size_t pages = lf_ceiling(size, IFLASH_PAGE_SIZE);
	uint8_t *raw = (uint8_t *)malloc(pages * IFLASH_PAGE_SIZE);
	for (int i = 0; i < pages; i ++) {
		for (int j = 0; j < IFLASH_PAGE_SIZE; j ++) {
			uint8_t c = fgetc(firmware);
			if (!feof(firmware)) {
				raw[((i * IFLASH_PAGE_SIZE) + j)] = c;
			} else {
				raw[((i * IFLASH_PAGE_SIZE) + j)] = 0;
			}
		}

	}
	fclose(firmware);
	return raw;
}

int main(int argc, char *argv[]) {

	/* Ensure the correct argument count. */
	if (argc < 2) {
		fprintf(stderr, "Please provide a path to the firmware.\n");
		return EXIT_FAILURE;
	}

	/* Open the firmware image. */
	FILE *firmware = fopen(argv[1], "rb");
	if (!firmware) {
		fprintf(stderr, "The file being opened, '%s', does not exist.\n", argv[1]);
		return EXIT_FAILURE;
	}

	/* Attach to a Flipper device. */
	flipper.attach();

	/* Determine the size of the file. */
	fseek(firmware, 0L, SEEK_END);
	size_t firmware_size = ftell(firmware);
	fseek(firmware, 0L, SEEK_SET);

	/* Enter DFU mode. */
	cpu.dfu();
	for (int i = 0; i < 3; i ++) {
		/* Send the synchronization character. */
		uart.put('#');
		char ack[3];
		/* Check for acknowledgement. */
		uart.pull(ack, sizeof(ack));
		if (!memcmp(ack, (char []){ 0x0a, 0x0d, 0x3e }, sizeof(ack))) {
			fprintf(stderr, KGRN "Successfully entered update mode.\n" KNRM);
			goto connected;
		}
	}
	/* If no acknowledgement was received, throw and error. */
	fprintf(stderr, KRED "Failed to enter update mode.\n");
	return EXIT_FAILURE;

connected:

	/* Set normal mode. */
	uart.push("N#", 2);

	/* Move the copy applet into RAM. */
	int _e = sam_ba_copy(IRAM_ADDR, applet, sizeof(applet));
	if (_e < lf_success) {
		return EXIT_FAILURE;
	}
	printf(KGRN "Successfully uploaded applet.\n" KNRM);

	/* Obtain a linear buffer of the firmware precalculated by page. */
	void *pagedata = load_page_data(firmware, firmware_size);

	/* Calculate the number of pages to send. */
	lf_size_t pages = lf_ceiling(firmware_size, IFLASH_PAGE_SIZE);
	/* Send the firmware, page by page. */
	printf("Uploading page ");
	for (lf_size_t page = 0; page < pages; page ++) {
		char buf[32];
		/* Print the page count. */
		sprintf(buf, "%i / %u.", page + 1, pages);
		printf("%s", buf);
		fflush(stdout);
		/* Copy the page. */
		sam_ba_copy(USERSPACE, (void *)(pagedata + (page * IFLASH_PAGE_SIZE)), IFLASH_PAGE_SIZE);
		/* Write the destination. */
		sam_ba_write_word(IRAM_ADDR + _APPLET_DESTINATION, (uint32_t)(IFLASH_ADDR + (page * IFLASH_PAGE_SIZE)));
		/* Jump to the copy applet to move the page into the flash write buffer. */
		sam_ba_jump(_APPLET);
		/* Erase and write the page into flash. */
		sam_ba_write_fcr0(0x03, page);
		/* Clear the progress message. */
		if (page != pages - 1) for (int j = 0; j < strlen(buf); j ++) printf("\b");
		fflush(stdout);
	}
	/* Free the memory allocated to hold the page data. */
	free(pagedata);

	/* Reset the CPU. */
	cpu.reset();

    return EXIT_SUCCESS;
}
