static char *version = "@(!--#) @(#) autoserial.c, sversion 0.1.0, fversion 017, 18-december-2025";

/*
 *  autoserial.c
 *
 *  a serial terminal command
 *
 *  has two modes of operation
 *
 *  the first is working like the cu command to connect to /dev/ttySnnn and /dev/ttyUSBnnn devices
 *
 *  the second to connect to a remote host and port (the so TCP to COM bridge server)
 *
 *  I shall call these modes "LOCAL_MODE" and "BRIDGE_MODE" respectively
 *
 */

/**********************************************************************/

/*
 *  Links:
 *    https://stackoverflow.com/questions/13474923/read-dev-ttyusb0-with-a-c-program-on-linux
 *
 *  Notes:
 *
 */

/**********************************************************************/

/*
 *  includes
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <termios.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

/**********************************************************************/

/*
 *  defines
 */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define LOCAL_MODE  0
#define BRIDGE_MODE 1

#define DEFAULT_MODE LOCAL_MODE

#define DEFAULT_ESCAPE_CHAR '^'

#define DEFAULT_DRAIN_PERIOD 0

#define MAX_DRAINING_DOTS 17

#define DEFAULT_LOCAL_PORT "/dev/ttyUSB0"
#define DEFAULT_LOCAL_BAUD "9600"

#define DEFAULT_BRIDGE_HOST "localhost"
#define DEFAULT_BRIDGE_TCP  "8089"

#define POLL_TIMEOUT 10

#define DEVTTY "/dev/tty"

#define BUFFER_SIZE 8192

/**********************************************************************/

/*
 *  global variables
 */

char			*g_progname;

int			debug;

struct termios		original_dev_tty_options;
struct termios		dev_tty_options;

struct termios		original_local_port_options;
struct termios		local_port_options;

/**********************************************************************/

void usage()
{
	fprintf(stderr, "%s: usage %s [ --debug ] [ -d millseconds ] [ -e escape_character ] [ serial_port [ baud_rate ] ]\n", g_progname, g_progname);
	fprintf(stderr, "%s: usage %s [ --debug ] [ -d millseconds ] [ -e escape_character ] [ bridge_host [ tcp_port# ] ]\n", g_progname, g_progname);

	return;
}

/**********************************************************************/

char *basename(char *s)
{
	char    *bn;

	bn = s;

	while (*s != '\0') {
		if (*s == '/') {
			if (*(s+1) != '\0') {
				bn = s+1;
			}
		}

		s++;
	}

	return bn;
}

/**********************************************************************/

int alldigits(char *s)
{
	int	retval;

	retval = TRUE;

	while (*s != '\0') {
		if (! isdigit(*s)) {
			retval = FALSE;
			break;
		}
	}

	return retval;
}

/**********************************************************************/

speed_t baud2int(char *s)
{
	int	baud;

	if (strcmp(s, "9600") == 0) {
		baud = B9600;
	} else if (strcmp(s, "115200") == 0) {
		baud = B115200;
	} else {
		baud = -1;
	}

	return baud;
}

/**********************************************************************/

void writedev(int devfd, char *s)
{
	char	buf[BUFFER_SIZE];
	int	lens;
	int	i;

	lens = strlen(s);

	if (lens > 0) {
		for (i = 0; i < lens; i++) {
			buf[i] = s[i];
		}

		
		write(devfd, buf, lens);
	}

	return;
}

/**********************************************************************/

void cleanup(int dev_tty)
{
	tcsetattr(dev_tty, TCSAFLUSH, &original_dev_tty_options);

	return;
}

/**********************************************************************/

void drain(
	int lp,
	int drain_period,
	int dev_tty
	)
{
	struct pollfd		portfd[1];
	unsigned char		obuf[BUFFER_SIZE];
	int			dotcount;
	int			pollretcode;
	int			n;

	writedev(dev_tty, "<<Draining...");

	dotcount = 0;

	while (TRUE) {
		portfd[0].fd      = lp;
		portfd[0].events  = POLLIN;

		pollretcode = poll(portfd, (nfds_t)1, drain_period);

		if (pollretcode < 0) {
			cleanup(dev_tty);
			fprintf(stderr, "%s: call to local port poll during drain gave a negative return code of %d\n", g_progname, pollretcode);
			exit(2);
		}

		if (pollretcode == 0) {
			break;
		}

		if (pollretcode > 0) {
			n = read(lp, obuf, BUFFER_SIZE);

			if (n < 0) {
				cleanup(dev_tty);
				fprintf(stderr, "%s: local port poll said there was data during drain but error when trying to read the data\n", g_progname);
				exit(2);
			}

			if (n == 0) {
				cleanup(dev_tty);
				fprintf(stderr, "%s: local port poll said there was data but read returned no data\n", g_progname);
				exit(2);
			}

			dotcount++;

			if (dotcount < MAX_DRAINING_DOTS) {
				writedev(dev_tty, ".");
			}
		}
	}

	writedev(dev_tty, "done>>\r\n");

	return;
}

/**********************************************************************/

int local_connect(
	int	dev_tty,
	char	escape_char,
	int	drain_period,
	char	*local_port,
	char	*local_baud
	)
{
	speed_t			baud;
	int			lp;
	int			exitflag;
	struct pollfd		ttyfd[1];
	struct pollfd		portfd[1];
	unsigned char		ibuf[BUFFER_SIZE];
	unsigned char		obuf[BUFFER_SIZE];
	int			pollretcode;
	int			n;
	int			i;
	unsigned char		c;

	if ((baud = baud2int(local_baud)) == -1) {
		cleanup(dev_tty);
		fprintf(stderr, "%s: unsupported baud rate \"%s\"\n", g_progname, local_baud);
		return 1;
	}

	if ((lp = open(local_port, O_RDWR)) == -1) {
		cleanup(dev_tty);
		fprintf(stderr, "%s: unable to open local port \"%s\"\n", g_progname, local_port);
		return 1;
	}

	tcgetattr(lp, &original_local_port_options);
	tcgetattr(lp, &local_port_options);

	cfmakeraw(&local_port_options);
	cfsetspeed(&local_port_options, baud);

	tcsetattr(lp, TCSANOW, &local_port_options);


	writedev(dev_tty, "<<Connected>>\r\n");

	if (drain_period >= 1) {
		drain(lp, drain_period, dev_tty);
	}

	exitflag = FALSE;

	while (! exitflag) {
		ttyfd[0].fd      = dev_tty;
		ttyfd[0].events  = POLLIN;


		pollretcode = poll(ttyfd, (nfds_t)1, POLL_TIMEOUT);

		if (pollretcode < 0) {
			cleanup(dev_tty);
			fprintf(stderr, "%s: call to tty poll gave a negative return code of %d\n", g_progname, pollretcode);
			exit(2);
		}

		if (pollretcode > 0) {
			n = read(dev_tty, ibuf, BUFFER_SIZE);

			if (n < 0) {
				cleanup(dev_tty);
				fprintf(stderr, "%s: tty poll said there was data but error when trying to read the data\n", g_progname);
				exit(2);
			}

			if (n == 0) {
				cleanup(dev_tty);
				fprintf(stderr, "%s: tty poll said there was data but read returned no data\n", g_progname);
				exit(2);
			}


			if (debug) {
				printf("\r\n*** %d chars read from keyboard\r\n", n);
			}

			for (i = 0; i < n; i++) {
				if (ibuf[i] == escape_char) {
					exitflag = TRUE;
					break;
				}
			}

			/* if exitflag is true then it is time break out now */
			if (exitflag) {
				break;
			}

			/* send the data to the serial port */
			write(lp, ibuf, n);
		}

		portfd[0].fd      = lp;
		portfd[0].events  = POLLIN;

		pollretcode = poll(portfd, (nfds_t)1, POLL_TIMEOUT);

		if (pollretcode < 0) {
			cleanup(dev_tty);
			fprintf(stderr, "%s: call to poll serial port gave a negative return code of %d\n", g_progname, pollretcode);
			exit(2);
		}

		if (pollretcode > 0) {
			n = read(lp, obuf, BUFFER_SIZE);

			if (n < 0) {
				cleanup(dev_tty);
				fprintf(stderr, "%s: serial port poll said there was data but error when trying to read the data\n", g_progname);
				exit(2);
			}

			if (n == 0) {
				cleanup(dev_tty);
				fprintf(stderr, "%s: serial port poll said there was data but read returned no data\n", g_progname);
				exit(2);
			}

			if (debug) {
				printf("\r\n*** %d chars read from serialport\r\n", n);
			}

			for (i = 0; i < n; i++) {
				if ((obuf[i] >= 32) && (obuf[i] <=126) || (obuf[i] == 8) || (obuf[i] == 10) || (obuf[i] == 13) || (obuf[i] == 27)) {
					if (obuf[i] == 27) {
						/* write(dev_tty, "<<ESC>>", 5); */
					} else {
						write(dev_tty, obuf+i, 1);
					}
				}
			}
		}
	}

	writedev(dev_tty, "\r\n<<Exiting>>\r\n");

	tcsetattr(dev_tty, TCSANOW, &original_dev_tty_options);

	tcsetattr(lp, TCSANOW, &original_local_port_options);

	close(dev_tty);

	close(lp);

	return 0;
}
/**********************************************************************/

int bridge_connect(
	int	dev_tty,
	char	escape_char,
	int	drain_period,
	char	*bridge_host,
	char	*bridge_tcp
	)
{
	int			clientsocket;
	struct sockaddr_in	serv_addr;
	int			exitflag;
	struct pollfd		ttyfd[1];
	struct pollfd		sockfd[1];
	unsigned char		ibuf[BUFFER_SIZE];
	unsigned char		obuf[BUFFER_SIZE];
	int			pollretcode;
	int			n;
	int			i;
	unsigned char		c;

        if ((clientsocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		cleanup(dev_tty);
                fprintf(stderr, "%s: unable to create a TCP (stream) based socket\n", g_progname);
                exit(2);
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port   = htons(atoi(bridge_tcp));

        if (inet_pton(AF_INET, bridge_host, &serv_addr.sin_addr) <= 0) {
		cleanup(dev_tty);
                fprintf(stderr, "%s: invalid bridge host IP address \"%s\"\n", g_progname, bridge_host);
                exit(2);
        }

        if (connect(clientsocket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		cleanup(dev_tty);
                fprintf(stderr, "%s: connection to %s:%s failed\n", g_progname, bridge_host, bridge_tcp);
                exit(2);
        }

	writedev(dev_tty, "<<Connected>>\r\n");

	exitflag = FALSE;

	while (! exitflag) {
		ttyfd[0].fd      = dev_tty;
		ttyfd[0].events  = POLLIN;


		pollretcode = poll(ttyfd, (nfds_t)1, POLL_TIMEOUT);

		if (pollretcode < 0) {
			cleanup(dev_tty);
			fprintf(stderr, "%s: call to tty poll gave a negative return code of %d\n", g_progname, pollretcode);
			exit(2);
		}

		if (pollretcode > 0) {
			n = read(dev_tty, ibuf, BUFFER_SIZE);

			if (n < 0) {
				cleanup(dev_tty);
				fprintf(stderr, "%s: tty poll said there was data but error when trying to read the data\n", g_progname);
				exit(2);
			}

			if (n == 0) {
				cleanup(dev_tty);
				fprintf(stderr, "%s: tty poll said there was data but read returned no data\n", g_progname);
				exit(2);
			}


			if (debug) {
				printf("\r\n*** %d chars read from keyboard\r\n", n);
			}

			for (i = 0; i < n; i++) {
				if (ibuf[i] == escape_char) {
					exitflag = TRUE;
					break;
				}
			}

			/* if exitflag is true then it is time break out now */
			if (exitflag) {
				break;
			}

			/* send the data to the serial port */
			send(clientsocket, ibuf, n, 0);
		}

		sockfd[0].fd      = clientsocket;
		sockfd[0].events  = POLLIN;

		pollretcode = poll(sockfd, (nfds_t)1, POLL_TIMEOUT);

		if (pollretcode < 0) {
			cleanup(dev_tty);
			fprintf(stderr, "%s: call to poll TCP port gave a negative return code of %d\n", g_progname, pollretcode);
			exit(2);
		}

		if (pollretcode > 0) {
			n = read(clientsocket, obuf, BUFFER_SIZE);

			if (n < 0) {
				cleanup(dev_tty);
				fprintf(stderr, "%s: TCP port poll said there was data but error when trying to read the data\n", g_progname);
				exit(2);
			}

			if (n == 0) {
				cleanup(dev_tty);
				fprintf(stderr, "%s: TCP port poll said there was data but read returned no data\n", g_progname);
				exit(2);
			}

			if (debug) {
				printf("\r\n*** %d chars read from TCP port\r\n", n);
			}

			for (i = 0; i < n; i++) {
				if ((obuf[i] >= 32) && (obuf[i] <=126) || (obuf[i] == 8) || (obuf[i] == 10) || (obuf[i] == 13)) {
					write(dev_tty, obuf+i, 1);
				}
			}
		}
	}

	writedev(dev_tty, "\r\n<<Exiting>>\r\n");

	tcsetattr(dev_tty, TCSANOW, &original_dev_tty_options);

	close(dev_tty);

	close(clientsocket);

	return 0;
}


/**********************************************************************/

/*
 *  Main
 */

/* function */
int main(int argc, char **argv)
/*
	int     argc;
	char    *argv[];
*/
{
	/* local declarations */
	int			mode;
	char			escape_char;
	int			drain_period;
	char			*local_port;
	char			*local_baud;
	char			*bridge_host;
	char			*bridge_tcp;
	int			arg;
	int			pos_arg_counter;
	int			dev_tty;

	/* set debug to zero (no debugging) */
	debug = 0;

	/* set the program name */
	g_progname = basename(argv[0]);

	/* if help command line option specified print usage message and exit */
	if (argc == 2) {
		if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0)) {
			usage();
			exit(2);
		}

		if ((strcmp(argv[1], "-v") == 0) || (strcmp(argv[1], "--version") == 0)) {
			fprintf(stderr, "%s: version: %s\n", g_progname, version);
			exit(2);
		}
	}

	/* set default values */
	escape_char  = DEFAULT_ESCAPE_CHAR;
	drain_period = DEFAULT_DRAIN_PERIOD;
	mode         = DEFAULT_MODE;
	local_port   = DEFAULT_LOCAL_PORT;
	local_baud   = DEFAULT_LOCAL_BAUD;
	bridge_host  = DEFAULT_BRIDGE_HOST;
	bridge_tcp   = DEFAULT_BRIDGE_TCP;

	/* process command line arguments */
	arg = 1;
	pos_arg_counter = 0;

	while (arg < argc) {
		if (strcmp(argv[arg], "--debug") == 0) {
			debug = 1;
		} else if (strcmp(argv[arg], "-e") == 0) {
			arg++;

			if (arg >= argc) {
				fprintf(stderr, "%s: expected escape character after the \"-e\" command line option\n", g_progname);
				exit(1);
			}

			if (strlen(argv[arg]) != 1) {
				fprintf(stderr, "%s: escape character option must be a single character after the \"-b\" command line option\n", g_progname);
				exit(1);
			}

			escape_char = argv[arg][0];
		} else if (strcmp(argv[arg], "-d") == 0) {
			arg++;

			if (arg >= argc) {
				fprintf(stderr, "%s: expected drain period in milliseconds after the \"-d\" command line option\n", g_progname);
				exit(1);
			}

			drain_period = atoi(argv[arg]);

			if (drain_period < 1) {
				fprintf(stderr, "%s: drain period option must be an integer >= 1 after the \"-d\" command line option\n", g_progname);
				exit(1);
			}
		} else if (argv[arg][0] == '-') {
			fprintf(stderr, "%s: unrecognised command line option \"%s\"\n", g_progname, argv[arg]);
			usage();
			exit(1);
		} else {
			pos_arg_counter++;

			switch (pos_arg_counter) {
				case 1:
					if (argv[arg][0] == '/') {
						mode = LOCAL_MODE;
						local_port = argv[arg];
					} else {
						mode = BRIDGE_MODE;
						bridge_host = argv[arg];
					}
					break;
				case 2:
					if (mode == LOCAL_MODE) {
						local_baud = argv[arg];
					} else if (mode == BRIDGE_MODE) {
						bridge_tcp = argv[arg];
					}
					break;
				default:
					fprintf(stderr, "%s: too many positional command line arguments\n", g_progname);
					usage();
					exit(2);
			}
		}

		arg++;
	}

	/* send debug output summarising command lne argument values */
	if (debug) {
		printf("Mode ...............: ");
		switch (mode) {
			case LOCAL_MODE:
				printf("local");
				break;
			case BRIDGE_MODE:
				printf("bridge");
				break;
			default:
				fprintf(stderr, "\n%s: internal error: mode=%d which is out of range\n", g_progname, mode);
				exit(2);
		}
		putchar('\n');
		printf("Escape char ........: \"%c\"\n", escape_char);
		printf("Drain period .......: %d\n", drain_period);
		printf("Local port..........: \"%s\"\n", local_port);
		printf("Local baud..........: \"%s\"\n", local_baud);
		printf("Bridge host ........: \"%s\"\n", bridge_host);
		printf("Bridge TCP port# ...: \"%s\"\n", bridge_tcp);
	}

	if ((dev_tty = open(DEVTTY, O_RDWR)) == -1) {
		fprintf(stderr, "%s: unable to open %s\n", g_progname, DEVTTY);
		exit(1);
	}

	tcgetattr(dev_tty, &original_dev_tty_options);
	tcgetattr(dev_tty, &dev_tty_options);
	cfmakeraw(&dev_tty_options);
	tcsetattr(dev_tty, TCSANOW, &dev_tty_options);

	if (mode == LOCAL_MODE) {
		return local_connect(dev_tty, escape_char, drain_period, local_port, local_baud);
	} else if (mode == BRIDGE_MODE) {
		return bridge_connect(dev_tty, escape_char, drain_period, bridge_host, bridge_tcp);
	} else {
		cleanup(dev_tty);
		fprintf(stderr, "%s: mode %d is invalid (or maybe not supported yet)\n", g_progname, mode);
		exit(2);
	}

	/* control should not get here! */
	cleanup(dev_tty);
	fprintf(stderr, "%s: logic error - control should not fall off the end of the main)() function!\n", g_progname);
	exit(2);
}

/**********************************************************************/

/* end of file: autoserial.c */
