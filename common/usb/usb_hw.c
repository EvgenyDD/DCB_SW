#include "usb_hw.h"
#include "platform.h"
#include "usb_bsp.h"
#include "usb_dcd_int.h"
#include "usbd_desc.h"
#include "usbd_usr.h"

static USB_OTG_CORE_HANDLE USB_OTG_dev;

#if defined(USE_STM322xG_EVAL)
// SYSCLK = 120 MHz
#define count_us 40
// SYSCLK = 168 MHz
#define count_us 55
#else /* defined (USE_STM3210C_EVAL) */
// SYSCLK = 72 MHz
#define count_us 12
#endif
void USB_OTG_BSP_uDelay(const uint32_t usec)
{
	volatile uint32_t count = count_us * usec;
	do
	{
		if(--count == 0) return;
	} while(1);
}

void USB_OTG_BSP_mDelay(const uint32_t msec) { delay_ms(msec); }

void usb_init(void)
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_OTG_HS, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_OTG2_FS);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_OTG2_FS);

	USBD_Init(&USB_OTG_dev, USB_OTG_HS_CORE_ID, &usbd_usr_cb, &usb_class_cb, &USR_cb);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	NVIC_InitStructure.NVIC_IRQChannel = OTG_HS_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1 + 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	NVIC_InitStructure.NVIC_IRQChannel = OTG_HS_EP1_OUT_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1 + 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	NVIC_InitStructure.NVIC_IRQChannel = OTG_HS_EP1_IN_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	DCD_DevConnect(&USB_OTG_dev);
}

void usb_disconnect(void)
{
	DCD_DevDisconnect(&USB_OTG_dev);
	USB_OTG_StopDevice(&USB_OTG_dev);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

bool usb_is_configured(void) { return USB_OTG_dev.dev.device_status == USB_OTG_CONFIGURED; }

void OTG_HS_WKUP_IRQHandler(void)
{
	EXTI_ClearITPendingBit(EXTI_Line20);
}

void OTG_HS_IRQHandler(void)
{
	USBD_OTG_ISR_Handler(&USB_OTG_dev);
}

void OTG_HS_EP1_IN_IRQHandler(void)
{
	USBD_OTG_EP1IN_ISR_Handler(&USB_OTG_dev);
}

void OTG_HS_EP1_OUT_IRQHandler(void)
{
	USBD_OTG_EP1OUT_ISR_Handler(&USB_OTG_dev);
}

void USBD_USR_Init(void) {}
void USBD_USR_DeviceReset(uint8_t speed) {}
void USBD_USR_DeviceConfigured(void) {}
void USBD_USR_DeviceSuspended(void) {}

void USBD_USR_DeviceResumed(void) {}
void USBD_USR_DeviceConnected(void) {}
void USBD_USR_DeviceDisconnected(void) {}

USBD_Usr_cb_TypeDef USR_cb = {
	USBD_USR_Init,
	USBD_USR_DeviceReset,
	USBD_USR_DeviceConfigured,
	USBD_USR_DeviceSuspended,
	USBD_USR_DeviceResumed,
	USBD_USR_DeviceConnected,
	USBD_USR_DeviceDisconnected,
};