#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <math.h>

#include "main.h"
#include "draw.h"
#include "table.h"
#include "serial.h"

static int serial_fd;

unsigned char flat_table[TABLE_HEIGHT*TABLE_WIDTH*3+6];

inline int f(int n, int x)
{
    return x*((x/n+1)%2)+(x+(n-2*(x-n*(x/n))-1))*((x/n)%2);
}

inline void set_led(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{   
    flat_table[f(TABLE_WIDTH, y)*3+6] = (uint8_t)(r * (pow((float)r / 255.0, 2.8)));
    flat_table[f(TABLE_WIDTH, y)*3+7] = (uint8_t)(g * (pow((float)g / 255.0, 2.8)));
    flat_table[f(TABLE_WIDTH, y)*3+8] = (uint8_t)(b * (pow((float)b / 255.0, 2.8)));
}

static int serialport_init(const char* serialport, int baud)
{
    struct termios toptions;
    int fd;

    fd = open(serialport, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd == -1)  {
        perror("init_serialport: Unable to open port ");
        return -1;
    }

    if (tcgetattr(fd, &toptions) < 0) {
        perror("init_serialport: Couldn't get term attributes");
        return -1;
    }
    speed_t brate = baud; // let you override switch below if needed
    switch(baud) {
        case 115200:
            brate=B115200;
            break;
        case 230400:
            brate=B230400;
            break;
        case 500000:
            brate=B500000;
            break;
    }

    cfsetispeed(&toptions, brate);
    cfsetospeed(&toptions, brate);

    toptions.c_iflag       = INPCK;
    toptions.c_lflag       = 0;
    toptions.c_oflag       = 0;
    toptions.c_cflag       = CREAD | CS8 | CLOCAL;
    toptions.c_cc[ VMIN ]  = 0;
    toptions.c_cc[ VTIME ] = 0;

    if( tcsetattr(fd, TCSANOW, &toptions) < 0) {
        perror("init_serialport: Couldn't set term attributes");
        return -1;
    }

    return fd;
}

int init_serial( void )
{
    serial_fd = serialport_init("/dev/ttyACM0", BAUD);
        
    return 0;
}
/*
void send_serial_fpga( void )
{
   unsigned char data = 0;

    if (lights[0].state) data += 1;
    if (lights[1].state) data += 2;
    if (lights[2].state) data += 4;
    if (lights[3].state) data += 8;

    if (lights[4].state) data += 16;
    if (lights[5].state) data += 32;
    if (lights[6].state) data += 64;
    if (lights[7].state) data += 128;

    ftdi_write_data(&ftdic, &data, 1);

}
*/
void send_serial_table( void )
{
    static int x, y = 0;

    // send out start data
    flat_table[0] = 'A';
    flat_table[1] = 'd';
    flat_table[2] = 'a';
    flat_table[3] = (uint8_t)(TABLE_WIDTH*TABLE_HEIGHT-1) >> 8;
    flat_table[4] = (uint8_t)(TABLE_WIDTH*TABLE_HEIGHT-1) & 0xff;
    flat_table[5] = flat_table[3] ^ flat_table[4] ^ 0x55;

    // index into the flat_table array
    int index = 0;

    // flatten table into a 1D array 
    for (y=0; y<TABLE_HEIGHT; y++)
    {
        for (x=0; x<TABLE_WIDTH; x++)
        {
            set_led(x, index, table[x][y].r, table[x][y].g, table[x][y].b);
            index++;
        }
    }

    int i, bytesToGo, bytesSent = 0;
    tcdrain(serial_fd);
    for (bytesSent=0, bytesToGo=sizeof(flat_table); bytesToGo > 0;) {
        if ((i = write(serial_fd, &flat_table[bytesSent], bytesToGo)) > 0) {
            bytesToGo -= i;
            bytesSent += i;
        }
    }

}
