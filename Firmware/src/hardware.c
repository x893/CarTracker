#include "hardware.h"

#if defined( UTRACKER_3 )
void Led_On(void)
{
	pinLow(LED_PIN);
}
void Led_Off(void)
{
	pinHigh(LED_PIN);
}
#endif

#if defined( UTRACKER_4 )
void Led1_On(void)
{
	pinHigh(LED1_PIN);
	pinLow (LED2_PIN);
}
void Led2_On(void)
{
	pinLow (LED1_PIN);
	pinHigh(LED2_PIN);
}
void Leds_Off(void)
{
	pinLow (LED1_PIN);
	pinLow (LED2_PIN);
}
#endif

#if defined( STM32F2XX ) || defined( STM32F4XX )

void pinMode(u8 pin, PinMode_Type mode)
{
	GPIO_TypeDef * GPIOx = GPIO_PORT(pin);
	u8 pin2 = (pin & 0x0F) * 2;
	u32 mask = ~(0x03 << pin2);
	pin &= 0x0F;
	
	GPIOx->MODER	= (GPIOx->MODER   & mask) | ((mode & 0x3) << pin2);
	mode >>= 2;
	GPIOx->PUPDR	= (GPIOx->PUPDR   & mask) | ((mode & 0x3) << pin2);
	mode >>= 2;
	GPIOx->OSPEEDR	= (GPIOx->OSPEEDR & mask) | ((mode & 0x3) << pin2);
	mode >>= 2;
	GPIOx->OTYPER	= (GPIOx->OTYPER  & ~(1 << pin)) | ((mode & 0x1)  << pin);
}

void pinLow(u8 pin)
{
	GPIO_TypeDef * GPIOx = GPIO_PORT(pin);
	GPIOx->BSRRH = (1 << (pin & 0xF));
}

void pinHigh(u8 pin)
{
	GPIO_TypeDef * GPIOx = GPIO_PORT(pin);
	GPIOx->BSRRL = (1 << (pin & 0xF));
}

void pinRemap(uint8_t pin, uint8_t GPIO_AF)
{
	GPIO_TypeDef * GPIOx = GPIO_PORT(pin);
	uint8_t pin2 = (pin & 0x7) * 4;
	pin = (pin & 0x0F) >> 3;
	GPIOx->AFR[pin] = (GPIOx->AFR[pin] & ~((uint32_t)0xF << pin2)) | ((uint32_t)GPIO_AF << pin2);
}

void pinToggle(u8 pin)
{
	GPIO_TypeDef * GPIOx = GPIO_PORT(pin);
	uint16_t mask = (1 << (pin & 0xF));
	if (GPIOx->ODR & mask)
		GPIOx->BSRRH = mask;
	else
		GPIOx->BSRRL = mask;
}

#elif	defined( STM32F10X_HD )	|| \
		defined( STM32F10X_MD )	|| \
		defined( STM32F10X_MD_VL )

void pinMode(uint8_t pin, PinMode_Type mode)
{
	__IO uint32_t * crx;
	uint32_t mask;
	uint8_t pin2;
	GPIO_TypeDef * port = GPIO_PORT(pin);
	
	crx = (pin & 0x08) ? &(port->CRH) : &(port->CRL);
	pin2 = (pin & 0x07) << 2;
	mask = ~(0x0F << pin2);
	*crx = (*crx & mask) | ((mode & 0x0F) << pin2);
	if (mode == GPIO_Mode_IPD)
		port->BRR  = (1 << pin);
	else if (mode == GPIO_Mode_IPU)
		port->BSRR = (1 << pin);
}

void pinLow(u8 pin)
{
	GPIO_TypeDef * port = GPIO_PORT(pin);
	port->BRR = (1 << (pin & 0xF));
}

void pinHigh(u8 pin)
{
	GPIO_TypeDef * port = GPIO_PORT(pin);
	port->BSRR = (1 << (pin & 0xF));
}

#else
	#error "CPU not define"
#endif
/*
uint8_t pinRead(u8 pin)
{
	GPIO_TypeDef *port = GPIO_PORT(pin);
	return (port->IDR & (1 << (pin & 0xF))) ? SET : RESET;
}
*/
