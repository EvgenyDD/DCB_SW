#ifndef __USB_CONF__H__
#define __USB_CONF__H__

#include "stm32f4xx.h"

#define USBD_SELF_POWERED

#define USE_USB_OTG_HS
#define USB_OTG_HS_CORE
#define USE_EMBEDDED_PHY

#define USB_VCP_DISABLE_VBUS
#define USB_VCP_DISABLE_ID

#ifdef USB_OTG_HS_CORE
#define RX_FIFO_HS_SIZE 512
#define TX0_FIFO_HS_SIZE 64
#define TX1_FIFO_HS_SIZE 372
#define TX2_FIFO_HS_SIZE 64
#define TX3_FIFO_HS_SIZE 0
#define TX4_FIFO_HS_SIZE 0
#define TX5_FIFO_HS_SIZE 0

#define USB_OTG_EMBEDDED_PHY_ENABLED
/* wakeup is working only when HS core is configured in FS mode */
// #define USB_OTG_HS_LOW_PWR_MGMT_SUPPORT

#define USB_OTG_HS_DEDICATED_EP1_ENABLED
#endif

#define USE_DEVICE_MODE

#define __ALIGN_BEGIN
#define __ALIGN_END
#define __packed __attribute__((__packed__))

#endif /* __USB_CONF__H__ */
