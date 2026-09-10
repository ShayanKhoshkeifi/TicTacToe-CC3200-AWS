#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// CC3200 SDK includes
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_ints.h"
#include "hw_common_reg.h"
#include "spi.h"
#include "prcm.h"
#include "gpio.h"
#include "pin.h"
#include "interrupt.h"
#include "timer.h"
#include "uart.h"
#include "utils.h"
#include "rom_map.h"
#include "uart_if.h"
#include "i2c_if.h"
#include "pin_mux_config.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"

// AWS/Network includes
#include "simplelink.h"
#include "common.h"
#include "gpio_if.h"
#include "utils/network_utils.h"

#if defined(ccs) || defined(gcc)
extern void (* const g_pfnVectors[])(void);
#endif

// -------------------------------------------------------
// AWS Configuration - same as Lab 4
// -------------------------------------------------------
#define DATE                2
#define MONTH               6
#define YEAR                2026
#define HOUR                18
#define MINUTE              30
#define SECOND              0

#define APPLICATION_NAME      "SSL"
#define APPLICATION_VERSION   "SQ24"
#define SERVER_NAME           "a3fwocwm27pe7-ats.iot.us-east-2.amazonaws.com"
#define GOOGLE_DST_PORT       8443

#define POSTHEADER "POST /things/CC3200_Thing/shadow HTTP/1.1\r\n"
#define HOSTHEADER "Host: a3fwocwm27pe7-ats.iot.us-east-2.amazonaws.com\r\n"
#define CHEADER "Connection: Keep-Alive\r\n"
#define CTHEADER "Content-Type: application/json\r\n"
#define CLHEADER1 "Content-Length: "
#define CLHEADER2 "\r\n\r\n"

// -------------------------------------------------------
// IR and Game defines
// -------------------------------------------------------
#define IR_GPIO_BASE    GPIOA0_BASE
#define IR_GPIO_PIN     0x1

#define IR_KEY_1      0x45
#define IR_KEY_2      0x46
#define IR_KEY_3      0x47
#define IR_KEY_4      0x44
#define IR_KEY_5      0x40
#define IR_KEY_6      0x43
#define IR_KEY_7      0x07
#define IR_KEY_8      0x15
#define IR_KEY_9      0x09
#define IR_KEY_STAR   0x16
#define IR_KEY_HASH   0x0D
#define IR_KEY_OK     0x1C

#define MARGIN      4
#define CELL_SIZE   38
#define LINE_W      4

#define CELL_ORIGIN_X(col) (MARGIN + (col) * (CELL_SIZE + LINE_W))
#define CELL_ORIGIN_Y(row) (MARGIN + (row) * (CELL_SIZE + LINE_W))
#define CELL_CENTER_X(col) (CELL_ORIGIN_X(col) + CELL_SIZE/2)
#define CELL_CENTER_Y(row) (CELL_ORIGIN_Y(row) + CELL_SIZE/2)

#define COLOR_BG     0x0000
#define COLOR_WT     0xFFFF
#define COLOR_LINE   0x4A69
#define COLOR_X      0xFBC0
#define COLOR_O      0xAEFC
#define COLOR_WIN    0x07E0

#define BMA222_ADDR  0x18
#define SHAKE_THRESH 100

// -------------------------------------------------------
// Game state
// -------------------------------------------------------
static int board[3][3];
static int current_player = 1;
static int game_over = 0;

// -------------------------------------------------------
// Draw string helper
// -------------------------------------------------------
void drawString(int x, int y, const char *str, unsigned int color,
                unsigned int bg, unsigned char size)
{
    int i = 0;
    while(str[i] != '\0')
    {
        drawChar(x + (i * 6 * size), y, str[i], color, bg, size);
        i++;
    }
}

// -------------------------------------------------------
// Grid and board drawing
// -------------------------------------------------------
void draw_grid(void)
{
    fillScreen(COLOR_BG);

    fillRect(MARGIN + CELL_SIZE, MARGIN,
             LINE_W, 3*CELL_SIZE + 2*LINE_W, COLOR_LINE);
    fillRect(MARGIN + 2*CELL_SIZE + LINE_W, MARGIN,
             LINE_W, 3*CELL_SIZE + 2*LINE_W, COLOR_LINE);
    fillRect(MARGIN, MARGIN + CELL_SIZE,
             3*CELL_SIZE + 2*LINE_W, LINE_W, COLOR_LINE);
    fillRect(MARGIN, MARGIN + 2*CELL_SIZE + LINE_W,
             3*CELL_SIZE + 2*LINE_W, LINE_W, COLOR_LINE);
}

void draw_X(int row, int col)
{
    int x0 = CELL_ORIGIN_X(col) + 6;
    int y0 = CELL_ORIGIN_Y(row) + 6;
    int x1 = CELL_ORIGIN_X(col) + CELL_SIZE - 6;
    int y1 = CELL_ORIGIN_Y(row) + CELL_SIZE - 6;
    int i;
    for(i = -2; i <= 2; i++)
    {
        drawLine(x0, y0+i, x1, y1+i, COLOR_X);
        drawLine(x0+i, y0, x1+i, y1, COLOR_X);
        drawLine(x0, y1+i, x1, y0+i, COLOR_X);
        drawLine(x0+i, y1, x1+i, y0, COLOR_X);
    }
}

void draw_O(int row, int col)
{
    int cx = CELL_CENTER_X(col);
    int cy = CELL_CENTER_Y(row);
    int r;
    for(r = 12; r <= 15; r++)
        drawCircle(cx, cy, r, COLOR_O);
}

void render_board(void)
{
    int row, col;
    draw_grid();
    for(row = 0; row < 3; row++)
        for(col = 0; col < 3; col++)
        {
            if(board[row][col] == 1)      draw_X(row, col);
            else if(board[row][col] == 2) draw_O(row, col);
        }
}

void draw_turn(int player)
{
    fillRect(0, 120, 128, 8, COLOR_BG);
    if(player == 1)
        drawString(2, 120, "Player X turn", COLOR_X, COLOR_BG, 1);
    else
        drawString(2, 120, "Player O turn", COLOR_O, COLOR_BG, 1);
}

void draw_result(int winner)
{
    fillScreen(COLOR_BG);
    if(winner == 1)
        drawString(20, 50, "X WINS!", COLOR_X, COLOR_BG, 2);
    else if(winner == 2)
        drawString(20, 50, "O WINS!", COLOR_O, COLOR_BG, 2);
    else
        drawString(20, 50, "DRAW!", COLOR_LINE, COLOR_BG, 2);

    drawString(5, 80, "Press * to reset", COLOR_WT, COLOR_BG, 1);
}

// -------------------------------------------------------
// Game logic
// -------------------------------------------------------
void reset_game(void)
{
    int r, c;
    for(r = 0; r < 3; r++)
        for(c = 0; c < 3; c++)
            board[r][c] = 0;
    current_player = 1;
    game_over = 0;
    render_board();
    draw_turn(1);
    Report("Game reset\r\n");
}

int check_winner(void)
{
    int p;
    for(p = 1; p <= 2; p++)
    {
        int r, c;
        for(r = 0; r < 3; r++)
            if(board[r][0]==p && board[r][1]==p && board[r][2]==p)
                return p;
        for(c = 0; c < 3; c++)
            if(board[0][c]==p && board[1][c]==p && board[2][c]==p)
                return p;
        if(board[0][0]==p && board[1][1]==p && board[2][2]==p) return p;
        if(board[0][2]==p && board[1][1]==p && board[2][0]==p) return p;
    }

    int r, c;
    for(r = 0; r < 3; r++)
        for(c = 0; c < 3; c++)
            if(board[r][c] == 0) return 0;

    return 3;
}

int ir_to_cell(uint8_t cmd)
{
    switch(cmd)
    {
        case IR_KEY_1: return 1;
        case IR_KEY_2: return 2;
        case IR_KEY_3: return 3;
        case IR_KEY_4: return 4;
        case IR_KEY_5: return 5;
        case IR_KEY_6: return 6;
        case IR_KEY_7: return 7;
        case IR_KEY_8: return 8;
        case IR_KEY_9: return 9;
        default: return -1;
    }
}

void cell_to_rowcol(int cell, int *row, int *col)
{
    *row = (cell - 1) / 3;
    *col = (cell - 1) % 3;
}

// -------------------------------------------------------
// IR decoder
// -------------------------------------------------------
static int decode_ir(void)
{
    uint32_t i;
    uint32_t raw = 0;
    uint32_t count = 0;

    while(MAP_GPIOPinRead(IR_GPIO_BASE, IR_GPIO_PIN) != 0);

    count = 0;
    while(MAP_GPIOPinRead(IR_GPIO_BASE, IR_GPIO_PIN) == 0)
    {
        count++;
        if(count > 1000000) return -1;
    }
    if(count < 5000) return -1;

    while(MAP_GPIOPinRead(IR_GPIO_BASE, IR_GPIO_PIN) != 0);

    for(i = 0; i < 32; i++)
    {
        while(MAP_GPIOPinRead(IR_GPIO_BASE, IR_GPIO_PIN) != 0);
        while(MAP_GPIOPinRead(IR_GPIO_BASE, IR_GPIO_PIN) == 0);

        uint32_t high_count = 0;
        while(MAP_GPIOPinRead(IR_GPIO_BASE, IR_GPIO_PIN) != 0)
        {
            high_count++;
            if(high_count > 50000) break;
        }

        raw >>= 1;
        if(high_count > 1500)
            raw |= 0x80000000;
    }

    uint8_t cmd     = (raw >> 16) & 0xFF;
    uint8_t cmd_inv = (raw >> 24) & 0xFF;

    if((cmd ^ cmd_inv) != 0xFF)
        return -1;

    return (int)cmd;
}

// -------------------------------------------------------
// Accelerometer shake detection
// -------------------------------------------------------
int detect_shake(void)
{
    unsigned char reg;
    unsigned char data;
    signed char x, y, z;

    reg = 0x03;
    I2C_IF_Write(BMA222_ADDR, &reg, 1, 0);
    I2C_IF_Read(BMA222_ADDR, &data, 1);
    x = (signed char)data;

    reg = 0x05;
    I2C_IF_Write(BMA222_ADDR, &reg, 1, 0);
    I2C_IF_Read(BMA222_ADDR, &data, 1);
    y = (signed char)data;

    reg = 0x07;
    I2C_IF_Write(BMA222_ADDR, &reg, 1, 0);
    I2C_IF_Read(BMA222_ADDR, &data, 1);
    z = (signed char)data;

    if(x > SHAKE_THRESH || x < -SHAKE_THRESH ||
       y > SHAKE_THRESH || y < -SHAKE_THRESH ||
       z > SHAKE_THRESH || z < -SHAKE_THRESH)
        return 1;

    return 0;
}

// -------------------------------------------------------
// AWS HTTP POST
// -------------------------------------------------------
static int http_post(int iTLSSockID, int winner)
{
    char acSendBuff[512];
    char acRecvbuff[1460];
    char cCLLength[200];
    char* pcBufHeaders;
    char data[200];
    int lRetVal = 0;

    // Build the result message
    if(winner == 1)
        sprintf(data, "{\"state\":{\"desired\":{\"result\":\"Player X wins!\"}}}\r\n\r\n");
    else if(winner == 2)
        sprintf(data, "{\"state\":{\"desired\":{\"result\":\"Player O wins!\"}}}\r\n\r\n");
    else
        sprintf(data, "{\"state\":{\"desired\":{\"result\":\"Draw!\"}}}\r\n\r\n");

    int dataLength = strlen(data);

    pcBufHeaders = acSendBuff;
    strcpy(pcBufHeaders, POSTHEADER);
    pcBufHeaders += strlen(POSTHEADER);
    strcpy(pcBufHeaders, HOSTHEADER);
    pcBufHeaders += strlen(HOSTHEADER);
    strcpy(pcBufHeaders, CHEADER);
    pcBufHeaders += strlen(CHEADER);
    strcpy(pcBufHeaders, CTHEADER);
    pcBufHeaders += strlen(CTHEADER);
    strcpy(pcBufHeaders, CLHEADER1);
    pcBufHeaders += strlen(CLHEADER1);
    sprintf(cCLLength, "%d", dataLength);
    strcpy(pcBufHeaders, cCLLength);
    pcBufHeaders += strlen(cCLLength);
    strcpy(pcBufHeaders, CLHEADER2);
    pcBufHeaders += strlen(CLHEADER2);
    strcpy(pcBufHeaders, data);
    pcBufHeaders += strlen(data);

    UART_PRINT(acSendBuff);

    lRetVal = sl_Send(iTLSSockID, acSendBuff, strlen(acSendBuff), 0);
    if(lRetVal < 0)
    {
        UART_PRINT("POST failed. Error: %i\n\r", lRetVal);
        sl_Close(iTLSSockID);
        return lRetVal;
    }

    lRetVal = sl_Recv(iTLSSockID, &acRecvbuff[0], sizeof(acRecvbuff), 0);
    if(lRetVal < 0)
    {
        UART_PRINT("Receive failed. Error: %i\n\r", lRetVal);
        return lRetVal;
    }
    else
    {
        acRecvbuff[lRetVal+1] = '\0';
        UART_PRINT(acRecvbuff);
        UART_PRINT("\n\r\n\r");
    }

    return 0;
}

// -------------------------------------------------------
// Hardware init
// -------------------------------------------------------
static void BoardInit(void)
{
#ifndef USE_TIRTOS
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#endif
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);
    PRCMCC3200MCUInit();
    PinMuxConfig();
}

static void ConfigureSPI(void)
{
    MAP_SPIReset(GSPI_BASE);
    MAP_SPIConfigSetExpClk(GSPI_BASE,
                           MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                           20000000,
                           SPI_MODE_MASTER,
                           SPI_SUB_MODE_0,
                           (SPI_HW_CTRL_CS |
                            SPI_4PIN_MODE |
                            SPI_TURBO_OFF |
                            SPI_CS_ACTIVEHIGH |
                            SPI_WL_8));
    MAP_SPIEnable(GSPI_BASE);
}

static int set_time()
{
    long retVal;
    g_time.tm_day = DATE;
    g_time.tm_mon = MONTH;
    g_time.tm_year = YEAR;
    g_time.tm_hour = HOUR;
    g_time.tm_min = MINUTE;
    g_time.tm_sec = SECOND;
    retVal = sl_DevSet(SL_DEVICE_GENERAL_CONFIGURATION,
                       SL_DEVICE_GENERAL_CONFIGURATION_DATE_TIME,
                       sizeof(SlDateTime), (unsigned char *)(&g_time));
    ASSERT_ON_ERROR(retVal);
    return SUCCESS;
}

// -------------------------------------------------------
// Main
// -------------------------------------------------------
int main(void)
{
    long lRetVal = -1;
    long tlsSocket = -1;

    BoardInit();
    InitTerm();
    ClearTerm();

    UART_PRINT("Connecting to AWS...\r\n");

    // Connect to AWS
    g_app_config.host = SERVER_NAME;
    g_app_config.port = GOOGLE_DST_PORT;
    lRetVal = connectToAccessPoint();
    lRetVal = set_time();
    if(lRetVal < 0)
    {
        UART_PRINT("Unable to set time\n\r");
        LOOP_FOREVER();
    }
    tlsSocket = tls_connect();
    if(tlsSocket < 0)
    {
        ERR_PRINT(tlsSocket);
        LOOP_FOREVER();
    }
    UART_PRINT("Connected to AWS!\r\n");

    // Init SPI and OLED
    MAP_PRCMPeripheralClkEnable(PRCM_GSPI, PRCM_RUN_MODE_CLK);
    MAP_PRCMPeripheralReset(PRCM_GSPI);
    ConfigureSPI();
    Adafruit_Init();

    // Init I2C for accelerometer
    I2C_IF_Open(I2C_MASTER_MODE_FST);

    UART_PRINT("Tic Tac Toe starting...\r\n");

    // Start game
    reset_game();

    while(1)
    {
        // Check accelerometer shake to reset
        if(detect_shake())
        {
            UART_PRINT("Shake detected - resetting game\r\n");
            MAP_UtilsDelay(8000000);
            reset_game();
            continue;
        }

        // Check for IR input
        if(MAP_GPIOPinRead(IR_GPIO_BASE, IR_GPIO_PIN) == 0)
        {
            int cmd = decode_ir();
            if(cmd < 0)
            {
                MAP_UtilsDelay(800000);
                continue;
            }

            Report("CMD: 0x%02X\r\n", (uint8_t)cmd);

            // Reset on * button
            if(cmd == IR_KEY_STAR)
            {
                reset_game();
                continue;
            }

            // If game is over, only reset is allowed
            if(game_over)
            {
                MAP_UtilsDelay(800000);
                continue;
            }

            // Get cell number from IR command
            int cell = ir_to_cell((uint8_t)cmd);
            if(cell < 0)
            {
                MAP_UtilsDelay(800000);
                continue;
            }

            // Convert cell to row/col
            int row, col;
            cell_to_rowcol(cell, &row, &col);

            // Check if cell is already occupied
            if(board[row][col] != 0)
            {
                Report("Cell %d already taken!\r\n", cell);
                MAP_UtilsDelay(800000);
                continue;
            }

            // Place mark
            board[row][col] = current_player;
            render_board();

            // Check for winner
            int result = check_winner();
            if(result > 0)
            {
                game_over = 1;

                if(result == 3)
                    Report("Draw!\r\n");
                else
                    Report("Player %d wins!\r\n", result);

                // Show result on OLED first
                MAP_UtilsDelay(2000000);
                draw_result(result);

                // Then POST to AWS
                Report("Posting result to AWS...\r\n");
                http_post(tlsSocket, result);
            }
            else
            {
                // Switch player
                current_player = (current_player == 1) ? 2 : 1;
                draw_turn(current_player);
            }

            // Debounce
            MAP_UtilsDelay(2000000);
        }
    }

    return 0;
}
