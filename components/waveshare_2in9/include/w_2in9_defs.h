#ifndef W_2IN9_DEFS
#define W_2IN9_DEFS

typedef struct 
{
    uint8_t reset_pin;
    uint8_t dc_pin;
    uint8_t cs_pin;
    uint8_t busy_pin;
    uint16_t width;
    uint16_t height;

    xSemaphoreHandle spi_mutx;
}w_2in9;

#define E_PAPER_LUT_REGISTER        0x32
#define E_PAPER_RAM_WRITE           0x24
#define E_PAPER_SECOND_RAM_WRITE    0x26
#define E_PAPER_START_DISP_REG      0x22
#define E_PAPER_VCOM_OTP_SELECTION  0x37
#define E_PAPER_VCOM_REG            0x2c
#define E_PAPER_SOFTWARE_RESET      0x12

#define E_PAPER_START_DISP          0xc7
#define E_PAPER_ACTIVATE            0x20

#endif