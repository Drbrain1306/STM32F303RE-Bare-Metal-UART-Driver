
#include <stdint.h>
#include "main.h"

//GPIOA Register base address - 0x48000000 - PA2, PA3
#define GPIOA_MODER 0x48000000 //Use bit fields in library header
#define GPIOA_OSPEEDR 0x48000008 //Port x configuration I/O pin y (y = 15 to 0)/ 2bit per pin
//x0: Low speed
//01: Medium speed
//11: High speed
#define GPIOA_PUPDR 0x4800000C //Port x configuration I/O pin y (y = 15 to 0)
//00: No pull-up, pull-down
//01: Pull-up
//10: Pull-down
#define GPIOA_AFRL 0x48000020 //AFRy[3:0]: Alternate function selection for port x pin y (y = 0..7)

//RCC Register base address - 0x40021000
#define RCC_AHBENR 0x40021014 //Enable bit 17 for GPIOAEN
#define RCC_APB1ENR 0x4002101C //Enable bit 17 for USART2

//USART2 Register base address - 0x40004400
#define USART_BRR 0x4000440C
#define USART_CR1 0x40004400
#define USART_CR2 0x40004404
#define USART_ISR 0x4000441C // TXE, TC, RXNE
#define USART_TDR 0x40004428
#define USART_RDR 0x40004424

#define F_CPU_DEFAULT 8000000UL // 8 MHz internal HSI clock
#define UART_BAUD_RATE 9600UL

//Function to transmit a byte
void uart2_write_char_byte(uint8_t ch){
	//Addressing UART ISR and TDR
	volatile uint32_t *const pUSART_ISR = (uint32_t*) USART_ISR;
	volatile uint32_t *const pUSART_TDR = (uint32_t*) USART_TDR;

	while(!(*pUSART_ISR & (1U << 7))); //Wait until the transmit data register is empty

	*pUSART_TDR = ((uint32_t)ch & 0xFF); //Copies 8bit char into TDR, 0xFF masks 31:8
}

//Function to transmit a string
void uart2_write_string(const char *str){
	while(*str){
		uart2_write_char_byte((uint8_t)*str);
		str++;
	}

	//Addressing UART ISR
	volatile uint32_t *const pUSART_ISR = (uint32_t*) USART_ISR;

	while(!(*pUSART_ISR & (1U << 6))); //Wait until transmission is complete
}

//Function for reading a byte
uint8_t uart2_read_char(void)
{
    volatile uint32_t *const pUSART_ISR = (uint32_t*) USART_ISR;
    volatile uint32_t *const pUSART_RDR = (uint32_t*) USART_RDR;

    // Wait until received data is ready to be read
    while(!(*pUSART_ISR & (1U << 5)));

    // Read and return data, hardware then clears RXNE flag
    return (uint8_t)(*pUSART_RDR & 0xFF); //0xFF masks off bits 31:8
}


int main(void)
{
	//Addressing RCC registers
	volatile uint32_t *const pRCC_AHBENR = (uint32_t*) RCC_AHBENR;
	volatile uint32_t *const pRCC_APB1ENR = (uint32_t*) RCC_APB1ENR;

	//Initializing RCC registers
	*pRCC_AHBENR |= (1U << 17);
	*pRCC_APB1ENR |= (1U << 17);

	//Addressing GPIOA MODER
	volatile GPIOx_MODER_t *const pGPIOA_MODER = (GPIOx_MODER_t*) GPIOA_MODER;

	//Setting mode of PA2 and PA3 to Alternate function
	pGPIOA_MODER->moder2 = 2;
	pGPIOA_MODER->moder3 = 2;

	//Addressing AFRL register for PA2 and PA3
	volatile uint32_t *const pGPIOA_AFRL = (uint32_t*) GPIOA_AFRL;

	//Clearing the AFRy register, then Assigning AFR7 - 0111
	*pGPIOA_AFRL &= ~((15U << (2 * 4)) | (15U << (3 * 4)));
	*pGPIOA_AFRL |= ((7U << (2 * 4)) | (7U << (3 * 4)));

	//Addressing pupdr registers
	volatile GPIOx_PUPDR_t *const pGPIOA_PUPDR = (GPIOx_PUPDR_t*) GPIOA_PUPDR;

	//Setting pupdr to pull up mode - 01
	pGPIOA_PUPDR->pupdr2 = 1;
	pGPIOA_PUPDR->pupdr3 = 1;

	//Addressing UART2 baud-rate
	volatile uint32_t *const pUSART_BRR = (uint32_t*) USART_BRR;

	//Calculating the baud rate value
	*pUSART_BRR = ((F_CPU_DEFAULT + UART_BAUD_RATE / 2U) / UART_BAUD_RATE);

	//Addressing control registers in USART2
	volatile uint32_t *const pUSART_CR1 = (uint32_t*) USART_CR1;
	volatile uint32_t *const pUSART_CR2 = (uint32_t*) USART_CR2;

	//Enabling (In CR1) Word length(8 data bits), RE, TE, Parity(0), UE; (In CR2) stop bit (1)
	*pUSART_CR1 &= ~((1U << 28) | (1U << 12) | (1U << 10));
	*pUSART_CR1 |= ((1U << 0) | (1U << 2) | (1U << 3));
	*pUSART_CR2 &= ~(3 << 12);

	//Send initializing message over the UART
	uart2_write_string("Hey UAR2, it's Clinton\r\n");

	while (1){
		uint8_t received_byte = uart2_read_char(); //Confirms and proves the working ability of full
	    uart2_write_char_byte(received_byte); //duplex lines in uart transmission
	}
}
