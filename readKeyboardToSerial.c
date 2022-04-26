// standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdint.h>

// serial communication library, linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

const char *serial_path = "/dev/ttyACM0";

uint32_t getk(void);

int main (int argc, char **argv) {
    // Open the serial prot, the return value is the fd
    int serial_port = open(serial_path, O_RDWR);
    
    // Check for errors
    if (serial_port < 0) {
        printf("Error %i from open: %s\n", errno, strerror(errno));
        return 1;
    }
    sleep(3);
    // Create new termios struct, we call it 'tty' for convention
    // No need for "= {0}" at the end as we'll immediately write the existing
    // config to this struct
    struct termios old_tty, new_tty;

    // Read in existing settings, and handle any error
    // NOTE: This is important! POSIX states that the struct passed to tcsetattr()
    // must have been initialized with a call to tcgetattr() overwise behaviour
    // is undefined
    if(tcgetattr(serial_port, &old_tty) != 0) {
        printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
        close(serial_port);
        return 1;
    }

    // copy the old_tty att to the new_tty att
    new_tty = old_tty;

    // ************* the termios struct *****************
    // struct termios {
	// tcflag_t c_iflag;		/* input mode flags */
	// tcflag_t c_oflag;		/* output mode flags */
	// tcflag_t c_cflag;		/* control mode flags */
	// tcflag_t c_lflag;		/* local mode flags */
	// cc_t c_line;			/* line discipline */
	// cc_t c_cc[NCCS];		/* control characters */
    // };
    // ************* the termios struct *****************

    // setting up the bits
    new_tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)  
    new_tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
    new_tty.c_cflag &= ~CSIZE; // Clear all the size bits, then select 8 bits per byte
    new_tty.c_cflag |= CS8; // 8 bits per byte (most common)
    new_tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
    new_tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
    
    new_tty.c_lflag &= ~ICANON; // Disable the thinking of the data as UTF-8 and look on the data jjust as a stream of bytes
    new_tty.c_lflag &= ~ECHO; // Disable echo
    new_tty.c_lflag &= ~ECHOE; // Disable erasure
    new_tty.c_lflag &= ~ECHONL; // Disable new-line echo

    new_tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP

    new_tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl

    new_tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

    new_tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    new_tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

    new_tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    new_tty.c_cc[VMIN] = 100; // https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/

    // Set in/out baud rate to be 115200
    cfsetispeed(&new_tty, B115200);
    cfsetospeed(&new_tty, B115200);

    // Save tty settings, also checking for error
    if (tcsetattr(serial_port, TCSANOW, &new_tty) != 0) {
        printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
        close(serial_port);
        return 1;
    }

    char* buffer = (char*)malloc(128);
    
    uint32_t c = {0};

    while (c != 27) {
        
        c = getk();
        
        switch (c) {
            case 'w':
                {
                write(serial_port, "w", strlen("w"));
                // usleep(5000);
                ssize_t n = read(serial_port, buffer, 5);
                buffer[n] = '\0';
                printf("arduino msg = %s\n", buffer);
                fflush(stdout);
                }
            break;
            case 's':
                {
                write(serial_port, "s", strlen("s"));
                // usleep(5000);
                ssize_t n = read(serial_port, buffer, 15);
                buffer[n] = '\0';
                printf("arduino msg = %s\n", buffer);
                fflush(stdout);
                }
            break;

            // default:
            //     {
            //     ssize_t n = read(serial_port, buffer, 9);
            //     buffer[n] = '\0';
            //     printf("arduino msg = %s", buffer);
            //     fflush(stdout);
            //     }
            // break;
        }
    }

    if (tcsetattr(serial_port, TCSANOW, &old_tty) != 0) {
        printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
        close(serial_port);
        return 1;
    }

    close(serial_port);

    return 0;
}

unsigned int getk(void) {
    struct termios oldt, newt;
    uint32_t ch = {0};
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VTIME] = 10; // time to wait = 1sec
    newt.c_cc[VMIN] = 0; // minimum bytes to wait for
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    read(STDIN_FILENO, &ch, 4);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}