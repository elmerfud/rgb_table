#include <stdio.h>
#include <stdlib.h>
#include <ftdi.h>

#include "main.h"
#include "draw.h"
#include "table.h"
#include "serial.h"

struct ftdi_context ftdic;

unsigned char flat_table[TABLE_HEIGHT*TABLE_WIDTH*3+6];

inline int f(int n, int x)
{
    return x*((x/n+1)%2)+(x+(n-2*(x-n*(x/n))-1))*((x/n)%2);
}

inline void set_led(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    flat_table[f(TABLE_WIDTH, y)*3+6] = r;
    flat_table[f(TABLE_WIDTH, y)*3+7] = g;
    flat_table[f(TABLE_WIDTH, y)*3+8] = b;
}

int init_serial( void )
{
    if (ftdi_init(&ftdic) < 0)
    {   
        fprintf(stderr, "ftdi_init failed\n");
        return 1;
    }  

    ftdi_set_interface(&ftdic, INTERFACE_ANY);

    int f = ftdi_usb_open(&ftdic, 0x0403, 0x6001);
    if (f < 0)
    {   
        fprintf(stderr, "unable to open ftdi device: %d (%s)\n", f, ftdi_get_error_string(&ftdic));
        return 1;
    }   

    f = ftdi_set_baudrate(&ftdic, BAUD);
    if (f < 0)
    {   
        fprintf(stderr, "unable to set baudrate: %d (%s)\n", f, ftdi_get_error_string(&ftdic));
        return 1;  
    }   
            
    return 0;
}

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

    // send the flat array
    ftdi_write_data(&ftdic, flat_table, TABLE_WIDTH*TABLE_HEIGHT*3+6);

}
