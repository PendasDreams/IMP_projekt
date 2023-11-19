#include "MK60D10.h"
#include "letters.h"



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
void SystemConfig() {
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
    // Pole pro hodnoty jednotlivých pinů
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

    // Zkontrolujte, zda rowNum nevychází mimo rozsah
    if (rowNum >= 0 && rowNum < sizeof(rowValues) / sizeof(rowValues[0])) {
        // Nastavte všechny piny na LOW
        for (int i = 0; i < sizeof(rowValues) / sizeof(rowValues[0]); i++) {
            PTA->PDOR &= ~rowValues[i];
        }

        // Nastavte odpovídající pin na HIGH
        PTA->PDOR |= rowValues[rowNum];
    }
}



void select_spot(unsigned int x, unsigned int y)
{
	if (x >= 0 && x < BOTH_DISPLAYS && y >= 0 && y < BOTH_DISPLAYS) {
		column_select(x);
		row_select(y);
	}

	delay(tdelay1, tdelay2);
}

void clear()
{
	PTA->PDOR = 0x00;
}

void showField(char field[HEIGHT][MAX], int length, int z)
{
    for (int x = 0; x < length; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            int column = z - 7 - x;
            int row = 7 - y;

            if (field[y][x] == '1') {
                select_spot(column, row);
            }
        }
    }

    clear();
}

void createString(char field[HEIGHT][MAX], char* str, int strLength)
{
	int fieldLength = 0;

	// Tabulka pro mapování znaků na funkce
	void (*charToFunc[256])(char[HEIGHT][MAX], int) = {
		[' '] = addSpace, ['A'] = addA, ['B'] = addB, ['C'] = addC, ['D'] = addD,
		['E'] = addE, ['F'] = addF, ['G'] = addG, ['H'] = addH, ['I'] = addI,
		['J'] = addJ, ['K'] = addK, ['L'] = addL, ['M'] = addM, ['N'] = addN,
		['O'] = addO, ['P'] = addP, ['Q'] = addQ, ['R'] = addR, ['S'] = addS,
		['T'] = addT, ['U'] = addU, ['V'] = addV, ['W'] = addW, ['X'] = addX,
		['Y'] = addY, ['Z'] = addZ, ['0'] = add0, ['1'] = add1, ['2'] = add2,
		['3'] = add3, ['4'] = add4, ['5'] = add5, ['6'] = add6, ['7'] = add7,
		['8'] = add8, ['9'] = add9,
	};

	for (int i = 0; i < strLength; i++) {
		// Získání odpovídající funkce pro znak z tabulky
		void (*func)(char[HEIGHT][MAX], int) = charToFunc[(unsigned char)str[i]];

		if (func) {
			// Volání funkce pro přidání znaku
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


void initButtonPorts()
{
    uint32_t pins[] = {10, 12, 27, 26, 11};
    for (int i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
        PORTE->PCR[pins[i]] = PORT_PCR_MUX(0x01); // SW2, SW3, SW4, SW5, SW6
    }
}

void displayText(const char *text, int len) {
    int maxLen = len * 8;
    int forLoopLimit = maxLen * 2 + 20;
    char field[HEIGHT][MAX];
    createString(field, text, len);

    for (int i = 0; i < forLoopLimit; i++) {
        for (int k = 0; k < 12; k++) {
            showField(field, maxLen, i);
        }
    }
}



int main(void) {
    MCUInit();
    SystemConfig();
    initButtonPorts();
    char field[HEIGHT][MAX];
    int maxLen;
    int forLoopLimit;
    int strLen;

    while (1) {
        if (!(GPIOE_PDIR & BTN_SW2)) {
            displayText("XNOVOS14", 8);
        } else if (!(GPIOE_PDIR & BTN_SW3)) {
            displayText("PROJEKT", 7);
        } else if (!(GPIOE_PDIR & BTN_SW5)) {
            displayText("IMP", 3);
        } else if (!(GPIOE_PDIR & BTN_SW4)) {
            displayText("FIT", 3);
        } else if (!(GPIOE_PDIR & BTN_SW6)) {
            displayText("VUT", 3);
        }
    }

    /* Never leave main */
    return 0;
}
