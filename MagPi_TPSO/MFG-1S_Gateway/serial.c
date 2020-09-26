#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <asm/termbits.h>
#include <unistd.h>
#include "serial.h"
#include "helpers.h"


int open_serial(char *serial_port_device, uint32_t baud)
{
	int serial_port = -1;
	struct termios2 tty;

	serial_port = open(serial_port_device, O_RDWR);

	// Check for errors
	if (serial_port < 0)
	{
	    fprintf(stderr, "error (%s) while opening serial port device %s\n", strerror(errno), serial_port_device);
	    return -1;
	}

	memset(&tty, 0, sizeof(tty));

	// Read in existing settings, and handle any errors
	if(ioctl(serial_port, TCGETS2, &tty) < 0)
	{
		close(serial_port);
		fprintf(stderr, "error from ioctl while reading serial port settings\n");
		return -2;
	}

	// Set non-standard baudrate
	tty.c_cflag &= ~CBAUD;
	tty.c_cflag |= BOTHER;
	tty.c_ispeed = baud; //615384
	tty.c_ospeed = baud; //615384;

	//tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
	tty.c_cflag |= PARENB;  // Set parity bit, enabling parity
	//tty.c_cflag &= ~PARODD;	// Use even parity
	tty.c_cflag |= PARODD;	// Use odd parity
	tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
	//tty.c_cflag |= CSTOPB;  // Set stop field, two stop bits used in communication
	//tty.c_cflag |= CS5; // 5 bits per byte
	//tty.c_cflag |= CS6; // 6 bits per byte
	//tty.c_cflag |= CS7; // 7 bits per byte
	tty.c_cflag |= CS8; // 8 bits per byte (most common)
	tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
	//tty.c_cflag |= CRTSCTS;  // Enable RTS/CTS hardware flow control
	tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
	tty.c_lflag &= ~ICANON;
	tty.c_lflag &= ~ECHO; // Disable echo
	tty.c_lflag &= ~ECHOE; // Disable erasure
	tty.c_lflag &= ~ECHONL; // Disable new-line echo
	tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes
	tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
	// tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT IN LINUX)
	// tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT IN LINUX)
	tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds) between bytes, returning as soon as any data is received.
	tty.c_cc[VMIN] = 0;	// Number of expected received bytes. Read returns after that or if VTIME is reached

	// Save tty settings, also checking for errors
	if(ioctl(serial_port, TCSETS2, &tty) < 0)
	{
		close(serial_port);
		fprintf(stderr, "error from ioctl while writing serial port settings\n");
		return -2;
	}

	return serial_port;
}

uint8_t slip_decode(uint8_t byte, uint8_t *packet)
{
	static uint8_t escape_mode = FALSE;
	uint8_t ret = PACKET_NOT_COMPLETE;
	static uint8_t packet_idx = 0;

	switch (byte)
	{
		case FESC:	//switch into ESCAPE-Mode
			escape_mode = TRUE;
			break;
		case FEND:	// Frame end found
			ret = packet_idx;
			packet_idx = 0;
			break;
		case TFESC:
			if (escape_mode)
			{
				packet[packet_idx] = FESC;
				escape_mode = FALSE;
			}
			else
			{
				packet[packet_idx] = byte;
			}
			if (packet_idx < PACKET_BUFF_LEN)
				packet_idx++;
			break;
		case TFEND:
			if (escape_mode)
			{
				packet[packet_idx] = FEND;
				escape_mode = FALSE;
			}
			else
			{
				packet[packet_idx] = byte;
			}
			if (packet_idx < PACKET_BUFF_LEN)
				packet_idx++;
			break;
		default:
			packet[packet_idx] = byte;
			if (packet_idx < PACKET_BUFF_LEN)
				packet_idx++;
			break;
	}

	return ret;
}

void slip_encode(uint8_t byte_in, uint8_t **pp_buffer_wr)
{
	uint8_t *p_buffer_wr = *pp_buffer_wr;
	switch (byte_in)
	{
		case FESC:
			*p_buffer_wr=FESC;
			p_buffer_wr++;
			*p_buffer_wr=TFESC;
			p_buffer_wr++;
			break;
		case FEND:
			*p_buffer_wr=FESC;
			p_buffer_wr++;
			*p_buffer_wr=TFEND;
			p_buffer_wr++;
			break;
		default:
			*p_buffer_wr = byte_in;
			p_buffer_wr++;
			break;
	}
	*pp_buffer_wr = p_buffer_wr;
}
