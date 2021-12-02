
#include "stm32l0xx.h"

#include "bitmap_text.h"
#include "canvas.h"
#include "sc88stpro_menu.h"

#include <stddef.h>
#include <stdio.h>

static __IO uint32_t Tick;

void send_a_line(void);
void sharp_update_screen(void);
void sharp_clear(void);

/* assumes registers have reset values */
void SystemClock_Config(void) {
    /* at system reset, MSI at 2.1MHz is system clock source */
    /* MSI configuration and activation */
    SET_BIT(RCC->CR, RCC_CR_MSION);
    while(RCC_CR_MSIRDY != READ_BIT(RCC->CR, RCC_CR_MSIRDY)) { };
    MODIFY_REG(RCC->ICSCR, RCC_ICSCR_MSIRANGE, RCC_ICSCR_MSIRANGE_5); // 2.097 MHz
    
    /* System clock activation */
    MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE, RCC_CFGR_HPRE_DIV1); // AHB PRESC
    //	MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_MSI); // Select MSI
    //	while (RCC_CFGR_SWS_MSI != READ_BIT(RCC->CFGR, RCC_CFGR_SWS)) { };
    
    SET_BIT(RCC->CR, RCC_CR_HSION);
    MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_HSI); // Select HSI
    while (RCC_CFGR_SWS_HSI != READ_BIT(RCC->CFGR, RCC_CFGR_SWS)) { };
    
    /* Set APB1 & APB2 prescaler */
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, RCC_CFGR_PPRE1_DIV1);
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2, RCC_CFGR_PPRE2_DIV1);
}

void setup_GPIO_for_USART2(void) {
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
    /* PA2 for USART2_TX */
    MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODE2, 0x2UL << GPIO_MODER_MODE2_Pos);
    CLEAR_BIT(GPIOA->OTYPER, GPIO_OTYPER_OT_2);
    MODIFY_REG(GPIOA->OSPEEDR, GPIO_OSPEEDER_OSPEED2, 0x3UL << GPIO_OSPEEDER_OSPEED2_Pos);
    MODIFY_REG(GPIOA->PUPDR, GPIO_PUPDR_PUPD2, 0x0UL << GPIO_PUPDR_PUPD2_Pos);
    MODIFY_REG(GPIOA->AFR[0], GPIO_AFRL_AFSEL2, 0x4UL << GPIO_AFRL_AFSEL2_Pos);
    
    /* PA3 for USART2_RX */
    MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODE3, 0x2UL << GPIO_MODER_MODE3_Pos);
    CLEAR_BIT(GPIOA->OTYPER, GPIO_OTYPER_OT_3);
    MODIFY_REG(GPIOA->OSPEEDR, GPIO_OSPEEDER_OSPEED3, 0x3UL << GPIO_OSPEEDER_OSPEED3_Pos);
    MODIFY_REG(GPIOA->PUPDR, GPIO_PUPDR_PUPD3, 0x0UL << GPIO_PUPDR_PUPD3_Pos);
    MODIFY_REG(GPIOA->AFR[0], GPIO_AFRL_AFSEL3, 0x4UL << GPIO_AFRL_AFSEL3_Pos);
}

char buf_err_msg[64] = {0};

uint32_t send = 0;
uint32_t allow_sending = 0;
char msg[] = "nom\n";

void _midi_send(const void * __buf, size_t __nbytes) {
    unsigned char * buf = (unsigned char *)__buf;
    if (__nbytes >=12) {
        snprintf(buf_err_msg, 64, "%02hhx %02hhx %02hhx %02hhx %02hhx\n%02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10], buf[11]);
    }
    // todo add sending
}

void sleep(uint32_t milliseconds) {
    const uint32_t now = Tick;
    while (Tick < (now + milliseconds)) { };
}

/* the device properties */
#if 1
#define SCREEN_WIDTH 400
#define SCREEN_HEIGHT 240
#else
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 400
#endif

#define SCREEN_PIXELS_PER_BYTE 8
/* for rendering */
#define BUFFER_PITCH (50) /* length of row in bytes */
#define BUFFER_ROWS 240
#define BUFFER_PITCH_INT16 (25)

uint8_t screenbuffer[BUFFER_PITCH * BUFFER_ROWS] = {0};
uint32_t frame_start = 0;
int main(void)
{
    SystemClock_Config();
    
    SystemCoreClockUpdate(); // a CMSIS function
    //uint32_t cval = SystemCoreClock;
    
    /* LED on PA5 */
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
    MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODE5, 0x1UL << GPIO_MODER_MODE5_Pos);
    CLEAR_BIT(GPIOA->OTYPER, GPIO_OTYPER_OT_5);
    MODIFY_REG(GPIOA->OSPEEDR, GPIO_OSPEEDER_OSPEED5, 0x0UL << GPIO_OSPEEDER_OSPEED5_Pos);
    CLEAR_BIT(GPIOA->PUPDR, GPIO_PUPDR_PUPD5);
    SET_BIT(GPIOA->BSRR, GPIO_BSRR_BR_5);
    
    /* SCK on PB3, MOSI on PB5, CS on PA10 */
    RCC->IOPENR |= RCC_IOPENR_GPIOBEN;
    MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODE3, 0x2UL << GPIO_MODER_MODE3_Pos);
    CLEAR_BIT(GPIOB->OTYPER, GPIO_OTYPER_OT_3);
    MODIFY_REG(GPIOB->OSPEEDR, GPIO_OSPEEDER_OSPEED3, 0x3UL << GPIO_OSPEEDER_OSPEED3_Pos);
    MODIFY_REG(GPIOB->PUPDR, GPIO_PUPDR_PUPD3, 0x0UL << GPIO_PUPDR_PUPD3_Pos);
    MODIFY_REG(GPIOB->AFR[0], GPIO_AFRL_AFSEL3, 0x0UL << GPIO_AFRL_AFSEL3_Pos);
    
    MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODE5, 0x2UL << GPIO_MODER_MODE5_Pos);
    CLEAR_BIT(GPIOB->OTYPER, GPIO_OTYPER_OT_5);
    MODIFY_REG(GPIOB->OSPEEDR, GPIO_OSPEEDER_OSPEED5, 0x3UL << GPIO_OSPEEDER_OSPEED5_Pos);
    MODIFY_REG(GPIOB->PUPDR, GPIO_PUPDR_PUPD5, 0x0UL << GPIO_PUPDR_PUPD5_Pos);
    MODIFY_REG(GPIOB->AFR[0], GPIO_AFRL_AFSEL5, 0x0UL << GPIO_AFRL_AFSEL5_Pos);
    
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
    MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODE10, 0x1UL << GPIO_MODER_MODE10_Pos);
    CLEAR_BIT(GPIOA->OTYPER, GPIO_OTYPER_OT_4);
    MODIFY_REG(GPIOA->OSPEEDR, GPIO_OSPEEDER_OSPEED4, 0x5UL << GPIO_OSPEEDER_OSPEED4_Pos);
    MODIFY_REG(GPIOA->PUPDR, GPIO_PUPDR_PUPD4, 0x0UL << GPIO_PUPDR_PUPD4_Pos);
    
    /* SPI Source, TX only */
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    CLEAR_BIT(SPI1->CR1, SPI_CR1_SPE); // disable before changing serial clock baud rate
    MODIFY_REG(SPI1->CR1, SPI_CR1_BR, 0x0UL << SPI_CR1_BR_Pos);
    /* CPHA = 0, CPOL = 0
     * BIDIMODE = 0, RXONLY = 0, CRCEN = 0, CRCEN = 0
     * must set SSI = 1 to allow master to transmit. maybe SSM too?
     * CR2 SSOE = 0 */
    SPI1->CR1 |=  SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_LSBFIRST
    | SPI_CR1_MSTR | SPI_CR1_SPE;
    
    GPIOA->BSRR |= GPIO_BSRR_BR_10;
    //	SPI_CR1_DFF
    
    /* Set systick to 1ms, enable SysTick_Handler */
    SysTick_Config(SystemCoreClock/1000);
    
    /* application setup */
    struct canvas1bit canvas = {
        .pix_w = SCREEN_WIDTH,
        .pix_h = SCREEN_HEIGHT,
        .pitch = BUFFER_PITCH,
        .buf = (uint8_t *)screenbuffer,
        ._size = sizeof(screenbuffer),
        .rotated = (SCREEN_WIDTH < SCREEN_HEIGHT ? 1 : 0)};
    canvas_clear(&canvas);
    struct kbd_input kio = {0};
    
    unsigned going_up = 0;
    int count = 1;
    while (1) {
        /* get input */
        // TODO
        
        /* timing */
        const uint32_t elapsed = (Tick - frame_start);
        frame_start = Tick;
        unsigned fps = 1000 / elapsed;
        
        menu_main(&kio, &canvas, fps, buf_err_msg);
        if (going_up) {
            if (count > 100) going_up = 0;
            count++;
        }
        if (!going_up) {
            count--;
            if (count < 1) going_up = 1;
        }
        canvas_clear(&canvas);
        canvas_draw_rect(count, 102, count, 102, 1, &canvas);
        
        /* render */
        sharp_update_screen();
        GPIOA->ODR ^= GPIO_ODR_OD5;
    }
}

uint8_t mode_select_and_line_num[2] = {0x1, 0};
volatile uint32_t zero = 0;
uint32_t tx_in_progress = 0;
uint32_t invert_the_screen = 0;

const uint32_t cs_delay = 5;

void sharp_update_screen(void) {
    if (tx_in_progress) return;
    tx_in_progress = 1;
    
    if (invert_the_screen) { mode_select_and_line_num[0] = 0x3; }
    else                   { mode_select_and_line_num[0] = 0x1; }
    
    /* CS high */
    GPIOA->BSRR |= GPIO_BSRR_BS_10;
    for (size_t i = 0; i < cs_delay; i++) zero = 0;
    
    /* this sending as uint16_t for speed??? idk test it. bc sending as bytes is less bookkeeping */
    uint8_t * line_buf = screenbuffer;
    for (size_t iy = 0; iy < BUFFER_ROWS; iy++) {
        mode_select_and_line_num[1] = iy + 1; /* a tricky screen detail */
        
        while ((SPI_SR_TXE != READ_BIT(SPI1->SR, SPI_SR_TXE))) { }
        SPI1->DR = mode_select_and_line_num[0];
        while ((SPI_SR_TXE != READ_BIT(SPI1->SR, SPI_SR_TXE))) { }
        SPI1->DR = mode_select_and_line_num[1];
        
        line_buf = screenbuffer + iy * BUFFER_PITCH;
        for (size_t i = 0; i < BUFFER_PITCH; i++) {
            while ((SPI_SR_TXE != READ_BIT(SPI1->SR, SPI_SR_TXE))) { }
            SPI1->DR = line_buf[i];
        }
    }
    while ((SPI_SR_TXE != READ_BIT(SPI1->SR, SPI_SR_TXE))) { }
    SPI1->DR = zero;
    while ((SPI_SR_TXE != READ_BIT(SPI1->SR, SPI_SR_TXE))) { }
    SPI1->DR = zero;
    
    /* CS low */
    for (size_t i = 0; i < cs_delay; i++) zero = 0;
    GPIOA->BSRR |= GPIO_BSRR_BR_10;
    
    tx_in_progress = 0;
}

void sharp_clear(void) {
    if (tx_in_progress) return;
    
    tx_in_progress = 1;
    
    /* CS high */
    GPIOA->BSRR |= GPIO_BSRR_BS_10;
    for (size_t i = 0; i < cs_delay; i++) zero = 0;
    
    mode_select_and_line_num[0] = 0x4;
    mode_select_and_line_num[1] = 0;
    
    /* send them */
    while ((SPI_SR_TXE != READ_BIT(SPI1->SR, SPI_SR_TXE))) { }
    SPI1->DR = *(uint16_t *)mode_select_and_line_num;
    
    /* CS low */
    for (size_t i = 0; i < cs_delay; i++) zero = 0;
    GPIOA->BSRR |= GPIO_BSRR_BR_10;
    
    tx_in_progress = 0;
}

void toggle_LD2(void) {
    GPIOA->ODR ^= GPIO_ODR_OD5;
}

void initial_setup_enable_USART2(void) {
    allow_sending = 1;
    // turn on the peripheral clock
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    // Before setting M bits in CR1 and BRR, disable USART:
    //   1. CR1 TE bit cleared
    //   2. wait for USART_ISR TC bit to be set
    //   3. clear CR1 UE bit
    USART2->CR1 &= ~(USART_CR1_TE);
    while (USART_ISR_TC != (USART2->ISR & USART_ISR_TC)) { };
    USART2->CR1 &= ~(USART_CR1_UE);
    
    // M[1:0] = 00 1 Start bit, 8 data bits, n stop bits, no parity
    MODIFY_REG(USART2->CR1, USART_CR1_M0, 0x0UL << USART_CR1_M0_Pos); // default
    MODIFY_REG(USART2->CR1, USART_CR1_M1, 0x0UL << USART_CR1_M1_Pos); // default
    // 00, 1 stop bit
    MODIFY_REG(USART2->CR2, USART_CR2_STOP, 0x0UL << USART_CR2_STOP_Pos); // default
    
    /* Baud Rate is described in 21.5.4 of Reference Manual
     BRR max value is fCK/16 for OVER8=0 and fCK/8 for OVER8=1,
     where fCK is the USART clock speed --- defaults to PCLK (APB clock)
     A listing of the peripherals on each APB is in:
     Table 3. STM32L010xx peripheral register boundary addresses
     USART clock source is set in RCC_CCIPR, USART2SEL.
     
     When OVER8=0 BRR=USARTDIV
     OVER8=0, baud rate = fCK / USARTDIV. Thus BRR = fCK / baud-rate
     
     When OVER8=1 see Reference Manual
     OVER8=1, baud rate = 2 * fCK / USARTDIV  - but see ref manual for details */
    MODIFY_REG(USART2->CR1, USART_CR1_OVER8, 0x1UL << USART_CR1_OVER8_Pos); // default
    USART2->BRR = SystemCoreClock / 9600;
    
    USART2->CR1 |= USART_CR1_TE | USART_CR1_UE;
    
    /* poll for idle frame transmission after enable */
    while(USART_ISR_TC != (USART2->ISR & USART_ISR_TC)) {
        // add timeout here for robust application, says the example
    }
    
    USART2->ICR = USART_ICR_TCCF; // setting this clears that flag in ISR
    USART2->CR1 |= USART_CR1_TCIE; // enable interrupt for each byte transmission
    
    NVIC_SetPriority(USART2_IRQn, 0);
    NVIC_EnableIRQ(USART2_IRQn);
}


void USART2_IRQHandler(void) {
    if (USART_ISR_TC == (USART2->ISR & USART_ISR_TC)) {
        if ((send != sizeof(msg))) { // && ('\0' != msg[send])
            /* clear TXE by writing to TDR */
            USART2->TDR = msg[send++];
        } else {
            send = 0;
            USART2->ICR = USART_ICR_TCCF; /* writing 1 clears transfer complete flag */
            toggle_LD2();
        }
    }
}


/******************************************************************************/
/*            Cortex-M0 Plus Processor Exceptions Handlers                         */
/******************************************************************************/

void SysTick_Handler(void) {
    Tick++;
    if (!(Tick % 1000)) {
        //		// begin transmitting string, the rest is handled in the interrupt
        //		if (allow_sending) USART2->TDR = msg[send++];
        //		else toggle_LD2();
        //		GPIOA->ODR ^= GPIO_ODR_OD5;
        //		send_a_line();
        //		clear_screen();
        //		invert_the_screen = !invert_the_screen;
    }
}


void NMI_Handler(void)
{
}


void HardFault_Handler(void)
{
    /* Go to infinite loop when Hard Fault exception occurs */
    while (1)
    {
    }
}

void SVC_Handler(void)
{
}

void PendSV_Handler(void)
{
}

//uint32_t line = 1;
//uint32_t col = 0;
///* carefully check for uint8 or uint16 consistency in the ENTIRE function */
//void send_a_line(void) {
//	if (tx_in_progress) return;
//
//	tx_in_progress = 1;
//
//	if (invert_the_screen) { mode_select_and_line_num[0] = 0x3; }
//	else                   { mode_select_and_line_num[0] = 0x1; }
//
//	/* CS high */
//	GPIOA->BSRR |= GPIO_BSRR_BS_10;
//	for (size_t i = 0; i < cs_delay; i++) zero = 0;
//
//	if (line > 240) {
//		line = 1;
//	}
//	mode_select_and_line_num[1] = line++;
//
////	mode_select_and_line_num[1] = xorshift32();
//	if (mode_select_and_line_num[1] > 239) mode_select_and_line_num[1] >>= 1;
//	for (size_t i = 0; i < 32; i++) screen_line_buf[i] = xorshift32();
//
//	/* send them */
//	while ((SPI_SR_TXE != READ_BIT(SPI1->SR, SPI_SR_TXE))) { }
//	SPI1->DR = *(uint16_t *)mode_select_and_line_num;
//	for (size_t i = 0; i < screen_line_buf_len; i++) {
//		while ((SPI_SR_TXE != READ_BIT(SPI1->SR, SPI_SR_TXE))) { }
//		SPI1->DR = screen_line_buf[i];
//	}
//	while ((SPI_SR_TXE != READ_BIT(SPI1->SR, SPI_SR_TXE))) { }
//	SPI1->DR = zero;
//
//	/* CS low */
//	for (size_t i = 0; i < cs_delay; i++) zero = 0;
//	GPIOA->BSRR |= GPIO_BSRR_BR_10;
//	GPIOA->ODR ^= GPIO_ODR_OD5;
//	tx_in_progress = 0;
//}

//	// setup PA8 for MCO. Following "steps" in 8.3.2
//	MODIFY_REG(GPIOA->AFR[1], GPIO_AFRH_AFSEL8, 0x0UL << GPIO_AFRH_AFSEL8_Pos);
//	CLEAR_BIT(GPIOA->OTYPER, GPIO_OTYPER_OT_8);
//	MODIFY_REG(GPIOA->PUPDR, GPIO_PUPDR_PUPD8_Msk, 0x0UL << GPIO_PUPDR_PUPD8_Pos);
//	MODIFY_REG(GPIOA->OSPEEDR, GPIO_OSPEEDER_OSPEED8, 0x3UL << GPIO_OSPEEDER_OSPEED8_Pos);
//	MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODE8, 0x2UL << GPIO_MODER_MODE8_Pos);
//
//	// disable MCO output, set prescaler, enable MCO
//	MODIFY_REG(RCC->CFGR, RCC_CFGR_MCOSEL, 0x0UL << RCC_CFGR_MCOSEL_Pos);
//	MODIFY_REG(RCC->CFGR, RCC_CFGR_MCOPRE, 0x0UL << RCC_CFGR_MCOPRE_Pos);
//	MODIFY_REG(RCC->CFGR, RCC_CFGR_MCOSEL, 0x1UL << RCC_CFGR_MCOSEL_Pos);

//	MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODE14, 0x1UL << GPIO_MODER_MODE14_Pos);
//	CLEAR_BIT(GPIOA->OTYPER, GPIO_OTYPER_OT_14);
//	MODIFY_REG(GPIOA->OSPEEDR, GPIO_OSPEEDER_OSPEED14, 0x0UL << GPIO_OSPEEDER_OSPEED14_Pos);
//	CLEAR_BIT(GPIOA->PUPDR, GPIO_PUPDR_PUPD14);
//	SET_BIT(GPIOA->BSRR, GPIO_BSRR_BR_14);

//	/* First time DMA setup, memory to SPI TX */
//	RCC->AHBENR |= RCC_AHBENR_DMA1EN;
//	CLEAR_BIT(DMA1_Channel3->CCR, DMA_CCR_EN);
//	/* channel 3 to SPI1_TX */
//	MODIFY_REG(DMA1_CSELR->CSELR, DMA_CSELR_C3S, 0x1UL << DMA_CSELR_C3S_Pos);
//	/* 16bit transfers, LSB,  */
//	DMA1_Channel3->CCR |= DMA_CCR_MSIZE_1 | DMA_CCR_PSIZE_1 | DMA_CCR_MINC
//			| DMA_CCR_DIR | DMA_CCR_TCIE;
//	DMA1_Channel3->CPAR = (uint32_t)SPI1->DR;
//	NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0);
//	NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

