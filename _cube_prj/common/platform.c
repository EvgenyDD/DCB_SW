#include "platform.h"

extern SDRAM_HandleTypeDef hsdram1;

#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)
#define WRITE_READ_ADDR     ((uint32_t)0x0000)

static void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram, FMC_SDRAM_CommandTypeDef *Command)
{
	__IO uint32_t tmpmrd = 0;

	Command->CommandMode = FMC_SDRAM_CMD_CLK_ENABLE; // Configure a clock configuration enable command
	Command->CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
	Command->AutoRefreshNumber = 1;
	Command->ModeRegisterDefinition = 0;

	HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT);

	HAL_Delay(1); // Insert 100 us minimum delay

	Command->CommandMode = FMC_SDRAM_CMD_PALL; //  Configure a PALL (precharge all) command
	Command->CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
	Command->AutoRefreshNumber = 1;
	Command->ModeRegisterDefinition = 0;

	HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT);

	Command->CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE; // Configure a Auto-Refresh command
	Command->CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
	Command->AutoRefreshNumber = 16;
	Command->ModeRegisterDefinition = 0;

	HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT);

	tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_1 | // Program the external memory mode register
			 SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL |
			 SDRAM_MODEREG_CAS_LATENCY_2 |
			 SDRAM_MODEREG_OPERATING_MODE_STANDARD |
			 SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

	Command->CommandMode = FMC_SDRAM_CMD_LOAD_MODE;
	Command->CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
	Command->AutoRefreshNumber = 1;
	Command->ModeRegisterDefinition = tmpmrd;

	HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT); //
	HAL_SDRAM_ProgramRefreshRate(hsdram, REFRESH_COUNT);   // Set the device refresh rate
}

void platform_sdram_init(void)
{
	FMC_SDRAM_CommandTypeDef command;
	SDRAM_Initialization_Sequence(&hsdram1, &command);
}