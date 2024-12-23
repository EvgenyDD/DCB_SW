/* @file    usbd_core.c
 * @author  MCD Application Team
 * @version V1.2.1
 * @date    17-March-2018
 * @brief   This file provides all the USBD core functions.
 */
#include "usbd_core.h"
#include "usb_bsp.h"
#include "usb_dcd_int.h"
#include "usbd_ioreq.h"
#include "usbd_req.h"

static uint8_t USBD_SetupStage(USB_OTG_CORE_HANDLE *pdev);
static uint8_t USBD_DataOutStage(USB_OTG_CORE_HANDLE *pdev, uint8_t epnum);
static uint8_t USBD_DataInStage(USB_OTG_CORE_HANDLE *pdev, uint8_t epnum);
static uint8_t USBD_SOF(USB_OTG_CORE_HANDLE *pdev);
static uint8_t USBD_Reset(USB_OTG_CORE_HANDLE *pdev);
static uint8_t USBD_Suspend(USB_OTG_CORE_HANDLE *pdev);
static uint8_t USBD_Resume(USB_OTG_CORE_HANDLE *pdev);
static uint8_t USBD_DevConnected(USB_OTG_CORE_HANDLE *pdev) { return 0; }
static uint8_t USBD_DevDisconnected(USB_OTG_CORE_HANDLE *pdev) { return 0; }
static uint8_t USBD_IsoINIncomplete(USB_OTG_CORE_HANDLE *pdev);
static uint8_t USBD_IsoOUTIncomplete(USB_OTG_CORE_HANDLE *pdev);
static uint8_t USBD_RunTestMode(USB_OTG_CORE_HANDLE *pdev);

__IO USB_OTG_DCTL_TypeDef SET_TEST_MODE;

USBD_DCD_INT_cb_TypeDef USBD_DCD_INT_cb = {
	USBD_DataOutStage,
	USBD_DataInStage,
	USBD_SetupStage,
	USBD_SOF,
	USBD_Reset,
	USBD_Suspend,
	USBD_Resume,
	USBD_IsoINIncomplete,
	USBD_IsoOUTIncomplete,
	USBD_DevConnected,
	USBD_DevDisconnected,
};

USBD_DCD_INT_cb_TypeDef *USBD_DCD_INT_fops = &USBD_DCD_INT_cb;

/**
 * @brief  USBD_Init
 *         Initializes the device stack and load the class driver
 * @param  pdev: device instance
 * @param  core_address: USB OTG core ID
 * @param  class_cb: Class callback structure address
 * @param  usr_cb: User callback structure address
 */
void USBD_Init(USB_OTG_CORE_HANDLE *pdev, USB_OTG_CORE_ID_TypeDef coreID, USBD_DEVICE *pDevice, USBD_Class_cb_TypeDef *class_cb, USBD_Usr_cb_TypeDef *usr_cb)
{
	USBD_DeInit(pdev);
	pdev->dev.class_cb = class_cb;
	pdev->dev.usr_cb = usr_cb;
	pdev->dev.usr_device = pDevice;
	DCD_Init(pdev, coreID);
	pdev->dev.usr_cb->Init();
}

/**
 * @brief  USBD_DeInit
 *         Re-Initialize the device library
 * @param  pdev: device instance
 * @retval status: status
 */
USBD_Status USBD_DeInit(USB_OTG_CORE_HANDLE *pdev) { return USBD_OK; }

/**
 * @brief  USBD_SetupStage
 *         Handle the setup stage
 * @param  pdev: device instance
 * @retval status
 */
static uint8_t USBD_SetupStage(USB_OTG_CORE_HANDLE *pdev)
{
	USB_SETUP_REQ req;
	USBD_ParseSetupRequest(pdev, &req);

	switch(req.bmRequest & USB_REQ_TYPE_MASK)
	{
	case USB_REQ_TYPE_STANDARD:
	case USB_REQ_TYPE_CLASS:
		switch(req.bmRequest & 0x1F)
		{
		case USB_REQ_RECIPIENT_DEVICE: USBD_StdDevReq(pdev, &req); break;
		case USB_REQ_RECIPIENT_INTERFACE: USBD_StdItfReq(pdev, &req); break;
		case USB_REQ_RECIPIENT_ENDPOINT: USBD_StdEPReq(pdev, &req); break;
		default: DCD_EP_Stall(pdev, req.bmRequest & 0x80); break;
		}
		break;

	case USB_REQ_TYPE_VENDOR: USBD_VendDevReq(pdev, &req); break;
	default: break;
	}
	return USBD_OK;
}

/**
 * @brief  USBD_DataOutStage
 *         Handle data out stage
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t USBD_DataOutStage(USB_OTG_CORE_HANDLE *pdev, uint8_t epnum)
{
	if(epnum == 0)
	{
		USB_OTG_EP *ep = &pdev->dev.out_ep[0];
		if(pdev->dev.device_state == USB_OTG_EP0_DATA_OUT)
		{
			if(ep->rem_data_len > ep->maxpacket)
			{
				ep->rem_data_len -= ep->maxpacket;
				if(pdev->cfg.dma_enable == 1) ep->xfer_buff += ep->maxpacket; // in slave mode this, is handled by the RxSTSQLvl ISR
				USBD_CtlContinueRx(pdev, ep->xfer_buff, MIN(ep->rem_data_len, ep->maxpacket));
			}
			else
			{
				int sts = 0;
				if((pdev->dev.class_cb->EP0_RxReady != NULL) &&
				   (pdev->dev.device_status == USB_OTG_CONFIGURED))
				{
					sts = pdev->dev.class_cb->EP0_RxReady(pdev);
				}
				if(sts != USBD_POSTPONE)
				{
					if(sts)
					{
						DCD_EP_Stall(pdev, 0x80);
						DCD_EP_Stall(pdev, 0);
						USB_OTG_EP0_OutStart(pdev);
					}
					else
					{
						USBD_CtlSendStatus(pdev);
					}
				}
			}
		}
	}
	else if((pdev->dev.class_cb->DataOut != NULL) &&
			(pdev->dev.device_status == USB_OTG_CONFIGURED))
	{
		pdev->dev.class_cb->DataOut(pdev, epnum);
	}

	else
	{
		/* Do Nothing */
	}
	return USBD_OK;
}

/**
 * @brief  USBD_DataInStage
 *         Handle data in stage
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t USBD_DataInStage(USB_OTG_CORE_HANDLE *pdev, uint8_t epnum)
{
	if(epnum == 0)
	{
		USB_OTG_EP *ep = &pdev->dev.in_ep[0];
		if(pdev->dev.device_state == USB_OTG_EP0_DATA_IN)
		{
			if(ep->rem_data_len > ep->maxpacket)
			{
				ep->rem_data_len -= ep->maxpacket;
				if(pdev->cfg.dma_enable == 1) ep->xfer_buff += ep->maxpacket; // in slave mode this, is handled by the TxFifoEmpty ISR
				USBD_CtlContinueSendData(pdev, ep->xfer_buff, ep->rem_data_len);
				DCD_EP_PrepareRx(pdev, 0, NULL, 0);
			}
			else
			{ /* last packet is MPS multiple, so send ZLP packet */
				if((ep->total_data_len % ep->maxpacket == 0) &&
				   (ep->total_data_len >= ep->maxpacket) &&
				   (ep->total_data_len < ep->ctl_data_len))
				{
					USBD_CtlContinueSendData(pdev, NULL, 0);
					ep->ctl_data_len = 0;
					DCD_EP_PrepareRx(pdev, 0, NULL, 0);
				}
				else
				{
					if((pdev->dev.class_cb->EP0_TxSent != NULL) &&
					   (pdev->dev.device_status == USB_OTG_CONFIGURED))
					{
						pdev->dev.class_cb->EP0_TxSent(pdev);
					}
					USBD_CtlReceiveStatus(pdev);
				}
			}
		}
		if(pdev->dev.test_mode == 1)
		{
			USBD_RunTestMode(pdev);
			pdev->dev.test_mode = 0;
		}
	}
	else if((pdev->dev.class_cb->DataIn != NULL) &&
			(pdev->dev.device_status == USB_OTG_CONFIGURED))
	{
		pdev->dev.class_cb->DataIn(pdev, epnum);
	}
	else
	{
		/* Do Nothing */
	}
	return USBD_OK;
}

/**
 * @brief  USBD_RunTestMode
 *         Launch test mode process
 * @param  pdev: device instance
 * @retval status
 */
static uint8_t USBD_RunTestMode(USB_OTG_CORE_HANDLE *pdev)
{
	USB_OTG_WRITE_REG32(&pdev->regs.DREGS->DCTL, SET_TEST_MODE.d32);
	return USBD_OK;
}

/**
 * @brief  USBD_Reset
 *         Handle Reset event
 * @param  pdev: device instance
 * @retval status
 */

static uint8_t USBD_Reset(USB_OTG_CORE_HANDLE *pdev)
{
	DCD_EP_Open(pdev, 0x00, USB_OTG_MAX_EP0_SIZE, EP_TYPE_CTRL); // Open EP0 OUT
	DCD_EP_Open(pdev, 0x80, USB_OTG_MAX_EP0_SIZE, EP_TYPE_CTRL); // Open EP0 IN
	/* Upon Reset call usr callback */
	pdev->dev.device_status = USB_OTG_DEFAULT;
	pdev->dev.usr_cb->DeviceReset(pdev->cfg.speed);
	return USBD_OK;
}

/**
 * @brief  USBD_Resume
 *         Handle Resume event
 * @param  pdev: device instance
 * @retval status
 */
static uint8_t USBD_Resume(USB_OTG_CORE_HANDLE *pdev)
{
	/* Upon Resume call usr callback */
	pdev->dev.usr_cb->DeviceResumed();
	pdev->dev.device_status = pdev->dev.device_old_status;
	pdev->dev.device_status = USB_OTG_CONFIGURED;
	return USBD_OK;
}

/**
 * @brief  USBD_Suspend
 *         Handle Suspend event
 * @param  pdev: device instance
 * @retval status
 */

static uint8_t USBD_Suspend(USB_OTG_CORE_HANDLE *pdev)
{
	pdev->dev.device_old_status = pdev->dev.device_status;
	pdev->dev.device_status = USB_OTG_SUSPENDED;
	/* Upon Resume call usr callback */
	pdev->dev.usr_cb->DeviceSuspended();
	return USBD_OK;
}

/**
 * @brief  USBD_SOF
 *         Handle SOF event
 * @param  pdev: device instance
 * @retval status
 */
static uint8_t USBD_SOF(USB_OTG_CORE_HANDLE *pdev)
{
	if(pdev->dev.class_cb->SOF) pdev->dev.class_cb->SOF(pdev);
	return USBD_OK;
}

/**
 * @brief  USBD_SetCfg
 *        Configure device and start the interface
 * @param  pdev: device instance
 * @param  cfgidx: configuration index
 * @retval status
 */

USBD_Status USBD_SetCfg(USB_OTG_CORE_HANDLE *pdev, uint8_t cfgidx)
{
	pdev->dev.class_cb->Init(pdev, cfgidx);
	pdev->dev.usr_cb->DeviceConfigured();
	return USBD_OK;
}

/**
 * @brief  USBD_ClrCfg
 *         Clear current configuration
 * @param  pdev: device instance
 * @param  cfgidx: configuration index
 * @retval status: USBD_Status
 */
USBD_Status USBD_ClrCfg(USB_OTG_CORE_HANDLE *pdev, uint8_t cfgidx)
{
	pdev->dev.class_cb->DeInit(pdev, cfgidx);
	return USBD_OK;
}

/**
 * @brief  USBD_IsoINIncomplete
 *         Handle iso in incomplete event
 * @param  pdev: device instance
 * @retval status
 */
static uint8_t USBD_IsoINIncomplete(USB_OTG_CORE_HANDLE *pdev)
{
	pdev->dev.class_cb->IsoINIncomplete(pdev);
	return USBD_OK;
}

/**
 * @brief  USBD_IsoOUTIncomplete
 *         Handle iso out incomplete event
 * @param  pdev: device instance
 * @retval status
 */
static uint8_t USBD_IsoOUTIncomplete(USB_OTG_CORE_HANDLE *pdev)
{
	pdev->dev.class_cb->IsoOUTIncomplete(pdev);
	return USBD_OK;
}

#ifdef VBUS_SENSING_ENABLED
/**
 * @brief  USBD_DevConnected
 *         Handle device connection event
 * @param  pdev: device instance
 * @retval status
 */
static uint8_t USBD_DevConnected(USB_OTG_CORE_HANDLE *pdev)
{
	pdev->dev.usr_cb->DeviceConnected();
	pdev->dev.connection_status = 1;
	return USBD_OK;
}

/**
 * @brief  USBD_DevDisconnected
 *         Handle device disconnection event
 * @param  pdev: device instance
 * @retval status
 */
static uint8_t USBD_DevDisconnected(USB_OTG_CORE_HANDLE *pdev)
{
	pdev->dev.usr_cb->DeviceDisconnected();
	pdev->dev.class_cb->DeInit(pdev, 0);
	pdev->dev.connection_status = 0;
	return USBD_OK;
}
#endif