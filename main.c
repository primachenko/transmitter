#include "main.h"

void delay(int millis) {
    while (millis-- > 0) {
        volatile int x = 5971;
        while (x-- > 0) {
            __asm("nop");
        }
    }
}

#define FSYNC   (GPIO_Pin_0)
#define SCLK    (GPIO_Pin_1)
#define SDAT    (GPIO_Pin_2)
#define PSELECT (GPIO_Pin_3)
#define FSELECT (GPIO_Pin_4)
#define LED     (GPIO_Pin_13)

enum
{
    AD9834_CR_DB0 = (1 << 0),
    AD9834_CR_DB1 = (1 << 1),
    AD9834_CR_DB2 = (1 << 2),
    AD9834_CR_DB3 = (1 << 3),
    AD9834_CR_DB4 = (1 << 4),
    AD9834_CR_DB5 = (1 << 5),
    AD9834_CR_DB6 = (1 << 6),
    AD9834_CR_DB7 = (1 << 7),
    AD9834_CR_DB8 = (1 << 8),
    AD9834_CR_DB9 = (1 << 9),
    AD9834_CR_DB10 = (1 << 10),
    AD9834_CR_DB11 = (1 << 11),
    AD9834_CR_DB12 = (1 << 12),
    AD9834_CR_DB13 = (1 << 13),
    AD9834_CR_DB14 = (1 << 14),
    AD9834_CR_DB15 = (1 << 15),
} AD9834_CR_DB_e;

enum
{
    AD9834_CR_B28 = AD9834_CR_DB13,
    AD9834_CR_HLB = AD9834_CR_DB12,
    AD9834_CR_FSEL = AD9834_CR_DB11,
    AD9834_CR_PSEL = AD9834_CR_DB10,
    AD9834_CR_PIN_SW = AD9834_CR_DB9,
    AD9834_CR_RESET = AD9834_CR_DB8,
    AD9834_CR_SLEEP1 = AD9834_CR_DB7,
    AD9834_CR_SLEEP12 = AD9834_CR_DB6,
    AD9834_CR_OPBITEN = AD9834_CR_DB5,
    AD9834_CR_SING_PIB = AD9834_CR_DB4,
    AD9834_CR_DIV2 = AD9834_CR_DB3,
    AD9834_CR_MODE = AD9834_CR_DB1,
} AD9834_CR_e;

int initDDS()
{
    GPIO_SetBits(GPIOA, FSYNC);
    GPIO_SetBits(GPIOA, SCLK);
    // delay(100);
    return 0;
}

int pulseClock()
{
    GPIO_ResetBits(GPIOA, SCLK);
    // delay(5);
    GPIO_SetBits(GPIOA, SCLK);
    // delay(5);
    return 0;
}

int shift16(uint16_t data)
{
    GPIO_ResetBits(GPIOA, FSYNC);
    delay(2);
    for (int i = 0; i < 16; ++i)
    {
        if (data & 0x8000)
        {
          GPIO_SetBits(GPIOA, SDAT);
        }
        else
        {
          GPIO_ResetBits(GPIOA, SDAT);
        }
        data <<= 1;
        data &= 0xFFFF;
        pulseClock();
    }
    GPIO_SetBits(GPIOA, FSYNC);
    return 0;
}

enum
{
    FREQ0,
    FREQ1
};

enum
{
    PHASE0,
    PHASE1,
};

#define PI (3.14)

static inline double deg2rad(double deg)
{
    return deg * (PI/180);
}

int setPhase(int regNo, double phaseDeg)
{
    uint16_t regPhase = (uint16_t)(deg2rad(phaseDeg) * 4096 / (2 * PI));
    regPhase &= 0x0FFF;
    if (PHASE0 == regNo)
    {
        regPhase |= 0xC000;
    }
    else if (PHASE1 == regNo)
    {
        regPhase |= 0xE000;
    }
    shift16(regPhase);
    return 0;
}

int setFreq(int regNo, double hz)
{
    double factor = 3.57913941333;
    uint32_t reg = (uint32_t) (hz * factor);
    uint16_t regLo = reg & 0x3FFF;
    uint16_t regHi = reg >> 14;
    if (0 == regNo)
    {
        regLo |= 0x4000;
        regHi |= 0x4000;
    }
    else if (1 == regNo)
    {
        regLo |= 0x8000;
        regHi |= 0x8000;
    }
    shift16(regLo);
    shift16(regHi);
    return 0;
}

void SetSysClockToHSE(void)
{
    ErrorStatus HSEStartUpStatus;
    RCC_DeInit();
    RCC_HSEConfig(RCC_HSE_ON);
    HSEStartUpStatus = RCC_WaitForHSEStartUp();
 
    if (HSEStartUpStatus == SUCCESS)
    {
        RCC_HCLKConfig(RCC_SYSCLK_Div1);
        RCC_PCLK2Config(RCC_HCLK_Div1);
        RCC_PCLK1Config(RCC_HCLK_Div1);
        RCC_SYSCLKConfig(RCC_SYSCLKSource_HSE);
        while (RCC_GetSYSCLKSource() != 0x04);
    }
    else
    {
        while (1);
    }
}

#define PREAMBLE_SEQUENCE (0xAB) // 10101011b
#define HIGH_LOW_SELECT (0x01)   // 00000001b

#define FREQ_SWITCH
// #define PHASE_SWITCH

#define HW_SWITH
// #define SW_SWITH

#if defined FREQ_SWITCH && defined PHASE_SWITCH
#error "there should be a freq switch or phase switch, one thing"
#endif

#if defined HW_SWITH && defined SW_SWITH
#error "only one switching method must be defined"
#endif

#define SWITCH_PERIOD (3)
#define FREQ1_VAL (1500.0)
#define FREQ2_VAL (1000.0)
#define PHASE1_VAL (0)
#define PHASE2_VAL (180)

// #define LED_INDICATION

void modulate_high()
{
    /* reg1 select */
#ifdef LED_INDICATION
    GPIO_SetBits(GPIOC, GPIO_Pin_13);
#endif
#if defined FREQ_SWITCH && defined HW_SWITH
    GPIO_SetBits(GPIOA, FSELECT);
#elif defined FREQ_SWITCH && defined SW_SWITH
    shift16(AD9834_CR_B28);
#elif defined PHASE_SWITCH && defined HW_SWITH
    GPIO_SetBits(GPIOA, PSELECT);
#elif defined PHASE_SWITCH && defined SW_SWITH
    shift16(AD9834_CR_B28 | AD9834_CR_PSEL);
#endif
    delay(SWITCH_PERIOD);
}

void moduleate_low()
{
    /* reg0 select */
#ifdef LED_INDICATION
    GPIO_ResetBits(GPIOC, GPIO_Pin_13);
#endif
#if defined FREQ_SWITCH && defined HW_SWITH
    GPIO_ResetBits(GPIOA, FSELECT);
#elif defined FREQ_SWITCH && defined SW_SWITH
    shift16(AD9834_CR_B28 | AD9834_CR_FSEL);
#elif defined PHASE_SWITCH && defined HW_SWITH
    GPIO_ResetBits(GPIOA, PSELECT);
#elif defined PHASE_SWITCH && defined SW_SWITH
    shift16(AD9834_CR_B28 | AD9834_CR_PSEL);
#endif
    delay(SWITCH_PERIOD);
}

uint8_t data[] = "hello";
// {
//     0x68, // 'h'
//     0x65, // 'e'
//     0x6c, // 'l'
//     0x6c, // 'l'
//     0x6f, // 'o'
//     0x00  // '\0'
// };

void send_byte(uint8_t byte)
{
    for (int j = 7; j >= 0; --j)
    {
        if ((byte >> j) & HIGH_LOW_SELECT)
            modulate_high();
        else
            moduleate_low();
    }
}

void send_data(uint8_t * data, uint32_t size)
{
    for (int i = 0; i < size; ++i)
    {
        send_byte(data[i]);
    }
}

void send_preamble()
{
    send_byte(PREAMBLE_SEQUENCE);
}

int main(void)
{
    SetSysClockToHSE();

    GPIO_InitTypeDef  GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_ResetBits(GPIOA, GPIO_Pin_All);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_ResetBits(GPIOB, GPIO_Pin_All);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_ResetBits(GPIOC, GPIO_Pin_All);

    initDDS();
    shift16(AD9834_CR_B28 | AD9834_CR_RESET);
#ifdef FREQ_SWITCH
    setFreq(0, FREQ1_VAL);
    setFreq(1, FREQ2_VAL);
#elif PHASE_SWITCH
    setPhase(0, PHASE1_VAL);
    setPhase(1, PHASE2_VAL);
#endif
#ifdef HW_SWITH
    shift16(AD9834_CR_B28 | AD9834_CR_PIN_SW);
#elif SW_SWITH
    shift16(AD9834_CR_B28);
#endif
    while(1)
    {
        send_preamble();
        send_data(data, sizeof(data));
    }
}
