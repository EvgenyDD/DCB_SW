#include "sram.h"
#include "platform.h"

#define SDRAM_MODEREG_BURST_LENGTH_1 ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2 ((uint16_t)0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4 ((uint16_t)0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8 ((uint16_t)0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED ((uint16_t)0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2 ((uint16_t)0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3 ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE ((uint16_t)0x0200)

static void __Delay(__IO uint32_t nCount)
{
	__IO uint32_t index = 0;
	for(index = (100000 * nCount); index != 0; index--)
	{
	}
}

static void SDRAM_Initialization_Sequence(FMC_SDRAMCommandTypeDef *Command)
{
	__IO uint32_t tmpmrd = 0;

	Command->FMC_CommandMode = FMC_Command_Mode_CLK_Enabled; // Configure a clock configuration enable command
	Command->FMC_CommandTarget = FMC_Command_Target_bank1;
	Command->FMC_AutoRefreshNumber = 1;
	Command->FMC_ModeRegisterDefinition = 0;

	FMC_SDRAMCmdConfig(Command);

	delay_ms(1); // Insert 100 us minimum delay

	Command->FMC_CommandMode = FMC_Command_Mode_PALL; //  Configure a PALL (precharge all) command
	Command->FMC_CommandTarget = FMC_Command_Target_bank1;
	Command->FMC_AutoRefreshNumber = 1;
	Command->FMC_ModeRegisterDefinition = 0;

	FMC_SDRAMCmdConfig(Command);

	Command->FMC_CommandMode = FMC_Command_Mode_AutoRefresh; // Configure a Auto-Refresh command
	Command->FMC_CommandTarget = FMC_Command_Target_bank1;
	Command->FMC_AutoRefreshNumber = 16;
	Command->FMC_ModeRegisterDefinition = 0;

	FMC_SDRAMCmdConfig(Command);

	tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_1 | // Program the external memory mode register
			 SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL |
			 SDRAM_MODEREG_CAS_LATENCY_2 |
			 SDRAM_MODEREG_OPERATING_MODE_STANDARD |
			 SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

	Command->FMC_CommandMode = FMC_Command_Mode_LoadMode;
	Command->FMC_CommandTarget = FMC_Command_Target_bank1;
	Command->FMC_AutoRefreshNumber = 1;
	Command->FMC_ModeRegisterDefinition = tmpmrd;

	FMC_SDRAMCmdConfig(Command);
	FMC_SetRefreshCount(500);
}

void sram_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_PinAFConfig(GPIOC, GPIO_PinSource0, GPIO_AF_FMC);

	GPIO_PinAFConfig(GPIOD, GPIO_PinSource0, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource1, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource8, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource9, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource10, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource14, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource15, GPIO_AF_FMC);

	GPIO_PinAFConfig(GPIOE, GPIO_PinSource0, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource1, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource7, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource8, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource9, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource10, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource11, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource12, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource13, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource14, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource15, GPIO_AF_FMC);

	GPIO_PinAFConfig(GPIOF, GPIO_PinSource0, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOF, GPIO_PinSource1, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOF, GPIO_PinSource2, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOF, GPIO_PinSource3, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOF, GPIO_PinSource4, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOF, GPIO_PinSource5, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOF, GPIO_PinSource11, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOF, GPIO_PinSource12, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOF, GPIO_PinSource13, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOF, GPIO_PinSource14, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOF, GPIO_PinSource15, GPIO_AF_FMC);

	GPIO_PinAFConfig(GPIOG, GPIO_PinSource0, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOG, GPIO_PinSource1, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOG, GPIO_PinSource4, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOG, GPIO_PinSource5, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOG, GPIO_PinSource8, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOG, GPIO_PinSource15, GPIO_AF_FMC);

	GPIO_PinAFConfig(GPIOH, GPIO_PinSource2, GPIO_AF_FMC);
	GPIO_PinAFConfig(GPIOH, GPIO_PinSource3, GPIO_AF_FMC);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOF, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_8 | GPIO_Pin_15;
	GPIO_Init(GPIOG, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_Init(GPIOH, &GPIO_InitStructure);

	FMC_SDRAMInitTypeDef FMC_SDRAMInitStructure;
	FMC_SDRAMTimingInitTypeDef FMC_SDRAMTimingInitStructure;

	RCC_AHB3PeriphClockCmd(RCC_AHB3Periph_FMC, ENABLE);

	/* Timing configuration for 90 Mhz of SD clock frequency (168Mhz/2) */
	FMC_SDRAMTimingInitStructure.FMC_LoadToActiveDelay = 2;	   /* TMRD: 2 Clock cycles */
	FMC_SDRAMTimingInitStructure.FMC_ExitSelfRefreshDelay = 6; /* TXSR: min=70ns (7x11.11ns) */
	FMC_SDRAMTimingInitStructure.FMC_SelfRefreshTime = 4;	   /* TRAS: min=42ns (4x11.11ns) max=120k (ns) */
	FMC_SDRAMTimingInitStructure.FMC_RowCycleDelay = 6;		   /* TRC:  min=70 (7x11.11ns) */
	FMC_SDRAMTimingInitStructure.FMC_WriteRecoveryTime = 2;	   /* TWR:  min=1+ 7ns (1+1x11.11ns) */
	FMC_SDRAMTimingInitStructure.FMC_RPDelay = 2;			   /* TRP:  20ns => 2x11.11ns */
	FMC_SDRAMTimingInitStructure.FMC_RCDDelay = 2;			   /* TRCD: 20ns => 2x11.11ns */

	FMC_SDRAMInitStructure.FMC_Bank = FMC_Bank1_SDRAM;
	FMC_SDRAMInitStructure.FMC_ColumnBitsNumber = FMC_ColumnBits_Number_9b;
	FMC_SDRAMInitStructure.FMC_RowBitsNumber = FMC_RowBits_Number_12b;
	FMC_SDRAMInitStructure.FMC_SDMemoryDataWidth = FMC_SDMemory_Width_16b;
	FMC_SDRAMInitStructure.FMC_InternalBankNumber = FMC_InternalBank_Number_4;
	FMC_SDRAMInitStructure.FMC_CASLatency = FMC_CAS_Latency_2;
	FMC_SDRAMInitStructure.FMC_WriteProtection = FMC_Write_Protection_Disable;
	FMC_SDRAMInitStructure.FMC_SDClockPeriod = FMC_SDClock_Period_2;
	FMC_SDRAMInitStructure.FMC_ReadBurst = FMC_Read_Burst_Enable;
	FMC_SDRAMInitStructure.FMC_ReadPipeDelay = FMC_ReadPipe_Delay_0;
	FMC_SDRAMInitStructure.FMC_SDRAMTimingStruct = &FMC_SDRAMTimingInitStructure;

	FMC_SDRAMInit(&FMC_SDRAMInitStructure);

	FMC_SDRAMCommandTypeDef command;
	SDRAM_Initialization_Sequence(&command);
}
