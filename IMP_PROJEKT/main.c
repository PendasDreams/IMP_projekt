#include "MK60D10.h"
#include "Letters.h"



#define BTN_SW2 0x400     // bit 10
#define BTN_SW3 0x1000    // bit 12
#define BTN_SW4 0x8000000 // bit 27
#define BTN_SW5 0x4000000 // bit 26
#define BTN_SW6 0x800     // bit 11

#define BOTH_DISPLAYS 16

// bit-level registers
#define GPIO_PIN_MASK	0x1Fu
#define GPIO_PIN(x)		(((1)<<(x & GPIO_PIN_MASK)))


//delay constants
#define	tdelay1			100
#define tdelay2 		1




//  MCU peripherals configuration
void system_config() {
    // Porta pins for column activators of 74HC154
    uint32_t colPins[] = {8, 10, 6, 11};
    // Porta pins for row selectors of 74HC154
    uint32_t rowPins[] = {26, 24, 9, 25, 28, 7, 27, 29};

    // Turn on all port clocks for PORTA and PORTE
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTE_MASK;

    // Configure PORTA pins for GPIO functionality
    for (int i = 0; i < sizeof(colPins) / sizeof(colPins[0]); i++) {
        PORTA->PCR[colPins[i]] = PORT_PCR_MUX(0x01);
    }

    // Configure PORTA pins for GPIO functionality
    for (int i = 0; i < sizeof(rowPins) / sizeof(rowPins[0]); i++) {
        PORTA->PCR[rowPins[i]] = PORT_PCR_MUX(0x01);
    }

    // Configure PORTE pin for GPIO functionality
    PORTE->PCR[28] = PORT_PCR_MUX(0x01);

    // Set corresponding PTA port pins as outputs
    PTA->PDDR = GPIO_PDDR_PDD(0x3F000FC0);

    // Set corresponding PTE port pins as outputs
    PTE->PDDR = GPIO_PDDR_PDD(GPIO_PIN(28));
}

// delay funcion
void delay(int t1, int t2)
{
	int i, j;

	for(i=0; i<t1; i++) {
		for(j=0; j<t2; j++);
	}
}


// Conversion of the provided column number into control signals for a 4-to-16 decoder.
void column_select(unsigned int col_num)
{
    // Define an array to store the corresponding pin numbers
    uint32_t col_pins[] = {8, 10, 6, 11};

    // Initialize the result and col_sel array
    unsigned int result = col_num;
    unsigned int col_sel[4];

    // Calculate col_sel values
    for (int i = 0; i < 4; i++) {
        col_sel[i] = result % 2;
        result /= 2;
    }

    // Configure the pins based on col_sel values
    for (int i = 0; i < 4; i++) {
        uint32_t pin = col_pins[i];
        uint32_t pin_mask = GPIO_PIN(pin);

        if (col_sel[i] == 0) {
            PTA->PDOR &= ~GPIO_PDOR_PDO(pin_mask);
        } else {
            PTA->PDOR |= GPIO_PDOR_PDO(pin_mask);
        }
    }
}


void row_select(unsigned int rowNum) {
    // Array for values of individual pins
    uint32_t rowValues[] = {
        GPIO_PIN(26),  // R0
        GPIO_PIN(24),  // R1
        GPIO_PIN(9),   // R2
        GPIO_PIN(25),  // R3
        GPIO_PIN(28),  // R4
        GPIO_PIN(7),   // R5
        GPIO_PIN(27),  // R6
        GPIO_PIN(29)   // R7
    };

    // Check if rowNum is within the valid range
    if (rowNum >= 0 && rowNum < sizeof(rowValues) / sizeof(rowValues[0])) {
        // Set all pins to LOW
        for (int i = 0; i < sizeof(rowValues) / sizeof(rowValues[0]); i++) {
            PTA->PDOR &= ~rowValues[i];
        }

        // Set the corresponding pin to HIGH
        PTA->PDOR |= rowValues[rowNum];
    }
}




void select_spot(unsigned int x, unsigned int y)
{
    // Check if the x and y coordinates are within the valid range for the displays.
    if (x >= 0 && x < BOTH_DISPLAYS && y >= 0 && y < BOTH_DISPLAYS) {
        // If the coordinates are valid, select the corresponding column and row.
        column_select(x);
        row_select(y);
    }

    // Introduce a delay with parameters tdelay1 and tdelay2.
    delay(tdelay1, tdelay2);
}

void clear()
{
	PTA->PDOR = 0x00;
}

void showField(char field[HEIGHT][MAX], int length, int z)
{
    // Iterate through the columns of the field.
    for (int x = 0; x < length; x++) {
        // Iterate through the rows of the field.
        for (int y = 0; y < HEIGHT; y++) {
            // Calculate the column and row based on the offset 'z', current 'x', and 'y'.
            int column = z - 7 - x;
            int row = 7 - y;

            // Check if the current cell in the field contains '1'.
            if (field[y][x] == '1') {
                // Call a function to select and display a spot on the screen at the calculated column and row.
                select_spot(column, row);
            }
        }
    }

    // Clear the screen after displaying the field.
    clear();
}

void createString(char field[HEIGHT][MAX], char* str, int strLength)
{
    int fieldLength = 0;

    // Table for mapping characters to functions
    void (*charToFunc[256])(char[HEIGHT][MAX], int) = {
        [' '] = add_Space, ['A'] = add_A, ['B'] = add_B, ['C'] = add_C, ['D'] = add_D,
        ['E'] = add_E, ['F'] = add_F, ['G'] = add_G, ['H'] = add_H, ['I'] = add_I,
        ['J'] = add_J, ['K'] = add_K, ['L'] = add_L, ['M'] = add_M, ['N'] = add_N,
        ['O'] = add_O, ['P'] = add_P, ['Q'] = add_Q, ['R'] = add_R, ['S'] = add_S,
        ['T'] = add_T, ['U'] = add_U, ['V'] = add_V, ['W'] = add_W, ['X'] = add_X,
        ['Y'] = add_Y, ['Z'] = add_Z, ['0'] = add_0, ['1'] = add_1, ['2'] = add_2,
        ['3'] = add_3, ['4'] = add_4, ['5'] = add_5, ['6'] = add_6, ['7'] = add_7,
        ['8'] = add_8, ['9'] = add_9,
    };

    for (int i = 0; i < strLength; i++) {
        // Get the corresponding function for the character from the table
        void (*func)(char[HEIGHT][MAX], int) = charToFunc[(unsigned char)str[i]];

        if (func) {
            // Call the function to add the character
            func(field, fieldLength);
        }

        fieldLength += 8;
    }
}



void MCUInit(void)
{
    MCG_C4 |= ( MCG_C4_DMX32_MASK | MCG_C4_DRST_DRS(0x01) );
    SIM_CLKDIV1 |= SIM_CLKDIV1_OUTDIV1(0x00);
    WDOG_STCTRLH &= ~WDOG_STCTRLH_WDOGEN_MASK;
}


void init_button_ports()
{
    uint32_t pins[] = {10, 12, 27, 26, 11};
    for (int i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
        PORTE->PCR[pins[i]] = PORT_PCR_MUX(0x01); // SW2, SW3, SW4, SW5, SW6
    }
}

void display_text(const char *text, int len) {
    int maxLen = len * 8; // Calculate the maximum length for the 'field' array based on 'len'.
    int forLoopLimit = maxLen * 2 + 20; // Determine the limit for the outer loop.
    char field[HEIGHT][MAX]; // Declare a 2D array 'field' to store the display content.
    createString(field, text, len); // Call a function to create the 'field' based on 'text' and 'len'.

    // Outer loop to control the animation sequence.
    for (int i = 0; i < forLoopLimit; i++) {
        // Inner loop for repeating the display content multiple times.
        for (int k = 0; k < 12; k++) {
            showField(field, maxLen, i); // Call a function to display the 'field' content with a given offset 'i'.
        }
    }
}




int main(void) {
    MCUInit();                   // Initialize the microcontroller
    system_config();             // Configure the system settings
    init_button_ports();         // Initialize button ports
    char field[HEIGHT][MAX];      // Declare a character array 'field'
    int maxLen;                  // Declare a variable 'maxLen'
    int forLoopLimit;            // Declare a variable 'forLoopLimit'
    int strLen;                  // Declare a variable 'strLen'

    while (1) {                  // Start an infinite loop
        if (!(GPIOE_PDIR & BTN_SW2)) {
            display_text("XNOVOS14", 8);   // Display "XNOVOS14" when SW2 button is pressed
        } else if (!(GPIOE_PDIR & BTN_SW3)) {
            display_text("PROJEKT1", 8);    // Display "PROJEKT" when SW3 button is pressed
        } else if (!(GPIOE_PDIR & BTN_SW4)) {
            display_text("FIT", 3);         // Display "FIT" when SW4 button is pressed
        } else if (!(GPIOE_PDIR & BTN_SW5)) {
            display_text("IMP", 3);         // Display "IMP" when SW5 button is pressed
        } else if (!(GPIOE_PDIR & BTN_SW6)) {
            display_text("0123456", 7);     // Display "0123456" when SW6 button is pressed
        }
    }

    // Infinite loop, the program will never reach this point
    return 0;
}
