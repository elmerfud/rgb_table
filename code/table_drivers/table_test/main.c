#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <termios.h>
#include <unistd.h>

#define BAUD 500000

#define WIDTH   16
#define HEIGHT  8

static int serial_fd;

static uint8_t table[WIDTH][HEIGHT][3];
static uint8_t flat_table[WIDTH*HEIGHT*3+6];

inline int f(int n, int x)
{
    return x*((x/n+1)%2)+(x+(n-2*(x-n*(x/n))-1))*((x/n)%2);
}

inline void set_led(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    flat_table[f(WIDTH, y)*3+6] = r;
    flat_table[f(WIDTH, y)*3+7] = g;
    flat_table[f(WIDTH, y)*3+8] = b;
}
                                

void send(void);

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

int init_serial(void)
{
    serial_fd = serialport_init("/dev/ttyUSB0", BAUD);

    return 0;
}

void send(void)
{
    int x,y;

    // send out start byte
    flat_table[0] = 'A';
    flat_table[1] = 'd';
    flat_table[2] = 'a';
    flat_table[3] = (WIDTH*HEIGHT-1) >> 8;
    flat_table[4] = (WIDTH*HEIGHT-1) & 0xff;
    flat_table[5] = flat_table[3] ^ flat_table[4] ^ 0x55;

    int count = 0;
    // send out table data
    for (y=0; y<HEIGHT; y++)
    {
        for (x=0; x<WIDTH; x++)
        {
            set_led(x, count, table[x][y][0], table[x][y][1], table[x][y][2]);
            count++;
        }
    }

    int i, err, bytesToGo, bytesSent, totalBytesSent = 0;
    tcdrain(serial_fd);
    for (bytesSent=0, bytesToGo=sizeof(flat_table); bytesToGo > 0;) {
        if ((i = write(serial_fd, &flat_table[bytesSent], bytesToGo)) > 0) {
            bytesToGo -= i;
            bytesSent += i;
        }
    }

    usleep(25000);
}

void set_all(uint8_t r, uint8_t g, uint8_t b)
{
    int x,y;
    for (x=0; x<WIDTH; x++)
    {
        for (y=0; y<HEIGHT; y++)
        {
            table[x][y][0] = r;
            table[x][y][1] = g;
            table[x][y][2] = b;
        }
    }

    send();
}

void clear(void)
{
    int x,y;
    for (x=0; x<WIDTH; x++)
    {
        for (y=0; y<HEIGHT; y++)
        {
            table[x][y][0] = 0;
            table[x][y][1] = 0;
            table[x][y][2] = 0;
        }
    }

    send();
}

void walk(void)
{
    clear();

    int x,y;
    for (y=0; y<HEIGHT; y++)
    {
        for (x=0; x<WIDTH; x++)
        {
            table[x][y][0] = 254;
            table[x][y][1] = 254;
            table[x][y][2] = 254;

            send();

            //sleep(.25);
        }
    }
}

void loop_rows(void)
{
    clear();

    int x,y;
    for (y=0; y<HEIGHT; y++)
    {
        clear();

        for (x=0; x<WIDTH; x++)
        {
            table[x][y][0] = 254;
            table[x][y][1] = 0;
            table[x][y][2] = 0;
        }

        send();

        sleep(.5);
    }
}

void loop_cols(void)
{
    clear();

    int x,y;
    for (x=0; x<WIDTH; x++)
    {
        clear();

        for (y=0; y<HEIGHT; y++)
        {
            table[x][y][0] = 254;
            table[x][y][1] = 0;
            table[x][y][2] = 0;
        }

        send();

        sleep(.5);
    }
}

void fade(void)
{
    clear();

    int fade=0;
    for (fade=0; fade<255; fade++)
    {
        printf("%d\n", fade);
        set_all(fade,fade,fade);
        sleep(.2);
    }
    for (fade=254; fade>-1; fade--)
    {
        printf("%d\n", fade);
        set_all(fade, fade,fade);
        sleep(.2);
    }
}
void set_row(int y, int r, int g, int b)
{
    printf("%d\n", g);
    int x;
    for (x=0; x<WIDTH; x++)
    {
        table[x][y][0] = r;
        table[x][y][1] = g;
        table[x][y][2] = b;
    }
}

void row_gradient_green(void)
{

    int y;
    for (y=0; y<HEIGHT; y++)
    {
        //set_row(y,0, 0,(int)(255.0*(float)y/(float)HEIGHT));
        set_row(y, 0,(int)(255.0*(float)y/(float)HEIGHT),0);
        //set_row(y, (int)(255.0*(float)y/(float)HEIGHT),0,0);
        //set_row(y, (int)(255.0*(float)y/(float)HEIGHT),(int)(255.0*(float)y/(float)HEIGHT),0);
    }

    send();
}

void set_col(int x, int r, int g, int b)
{
    printf("%d\n", g);
    int y;
    for (y=0; y<HEIGHT; y++)
    {
        table[x][y][0] = r;
        table[x][y][1] = g;
        table[x][y][2] = b;
    }
}

void col_gradient_green(void)
{

    int x;
    for (x=0; x<WIDTH; x++)
    {
        set_col(x,0, 0,(int)(255.0*(float)x/(float)WIDTH));
        //set_col(x, 0,(int)(255.0*(float)x/(float)WIDTH),0);
        //set_col(x, (int)(255.0*(float)x/(float)WIDTH),0,0);
    }

    send();
}

int main(void)
{
    if (init_serial())
    {
        printf("Could not open serial port!\n");
        return 1;
    }

    int count = 0;

    int SLEEP = 3;

    //sleep(5);

    while (1)
    {
        printf("set all\n");
        set_all(15,15,15);
        sleep(1);
        //continue;
        printf("row green\n");
        row_gradient_green();
        sleep(1);
        //continue;
        printf("set all\n");
        set_all(10,10,10);
        sleep(2);

        printf("Walk\n");
        walk();

        printf("Set all red\n");
        set_all(254,0,0);
        sleep(SLEEP);

        printf("Set all green\n");
        set_all(0,254,0);
        sleep(SLEEP);

        printf("Set all blue\n");
        set_all(0,0,254);
        sleep(SLEEP);

        printf("Set all red/blue\n");
        set_all(254,0,254);
        sleep(SLEEP);

        printf("Set all red/green\n");
        set_all(254,254,0);
        sleep(SLEEP);

        printf("Set all green/blue\n");
        set_all(0,254,254);
        sleep(SLEEP);

        printf("Set all white\n");
        set_all(254,254,254);
        sleep(SLEEP);

        printf("Loop cols\n");
        loop_cols();

        printf("Loop rows\n");
        loop_rows();

        printf("Fade\n");
        clear();
        fade();

        printf("Clear\n");
        clear();
        sleep(SLEEP);
    }

    return 0;
}
