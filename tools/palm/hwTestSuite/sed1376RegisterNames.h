#ifndef SED1376_REGISTER_NAMES_H
#define SED1376_REGISTER_NAMES_H

/*SED1376 Register Definitions*/

/*Read-Only Configuration Registers*/
#define REV_CODE        0x00
#define DISP_BUFF_SIZE  0x01
#define CFG_READBACK    0x02

/*Clock Configuration Registers*/
#define MEM_CLK         0x04
#define PIXEL_CLK       0x05

/*Look Up Table Registers*/
#define LUT_B_WRITE     0x08
#define LUT_G_WRITE     0x09
#define LUT_R_WRITE     0x0A
#define LUT_WRITE_LOC   0x0B
#define LUT_B_READ      0x0C
#define LUT_G_READ      0x0D
#define LUT_R_READ      0x0E
#define LUT_READ_LOC    0x0F

/*Panel Configuration Registers*/
#define PANEL_TYPE      0x10
#define MOD_RATE        0x11
#define HORIZ_TOTAL     0x12
#define HORIZ_PERIOD    0x14
#define HORIZ_START_0   0x16
#define HORIZ_START_1   0x17
#define VERT_TOTAL_0    0x18
#define VERT_TOTAL_1    0x19
#define VERT_PERIOD_0   0x1C
#define VERT_PERIOD_1   0x1D
#define VERT_START_0    0x1E
#define VERT_START_1    0x1F
#define FPLINE_WIDTH    0x20
#define FPLINE_START_0  0x22
#define FPLINE_START_1  0x23
#define FPFRAME_WIDTH   0x24
#define FPFRAME_START_0 0x26
#define FPFRAME_START_1 0x27
#define DTFD_GCP_INDEX  0x28
#define DTFD_GCP_DATA   0x2C

/*Display Mode Registers*/
#define DISP_MODE       0x70
#define SPECIAL_EFFECT  0x71
#define DISP_ADDR_0     0x74
#define DISP_ADDR_1     0x75
#define DISP_ADDR_2     0x76
#define LINE_SIZE_0     0x78
#define LINE_SIZE_1     0x79

/*Picture-in-Picture Plus (PIP+) Registers*/
#define PIP_ADDR_0      0x7C
#define PIP_ADDR_1      0x7D
#define PIP_ADDR_2      0x7E
#define PIP_LINE_SZ_0   0x80
#define PIP_LINE_SZ_1   0x81
#define PIP_X_START_0   0x84
#define PIP_X_START_1   0x85
#define PIP_Y_START_0   0x88
#define PIP_Y_START_1   0x89
#define PIP_X_END_0     0x8C
#define PIP_X_END_1     0x8D
#define PIP_Y_END_0     0x90
#define PIP_Y_END_1     0x91

/*Miscellaneous Registers*/
#define PWR_SAVE_CFG    0xA0
#define SCRATCH_0       0xA4
#define SCRATCH_1       0xA5

/*General Purpose IO Pins Registers*/
#define GPIO_CONF_0     0xA8
#define GPIO_CONF_1     0xA9
#define GPIO_CONT_0     0xAC
#define GPIO_CONT_1     0xAD

/*PWM Clock and CV Pulse Configuration Registers*/
#define PWM_CONTROL     0xB0
#define PWM_CONFIG      0xB1
#define PWM_LENGTH      0xB2
#define PWM_DUTY_CYCLE  0xB3

#endif
