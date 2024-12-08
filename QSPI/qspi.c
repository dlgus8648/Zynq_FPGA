#include "xparameters.h"
#include "xqspips.h"
#include "xil_printf.h"
#define QSPI_DEVICE_ID XPAR_XQSPIPS_0_DEVICE_ID

//FLASH operation CMD define
#define CMD_WRITE_STATUS 0x01
#define CMD_WRITE 0x02
#define CMD_READ 0x03
#define CMD_READ_STATUS 0x05
#define CMD_RITE_ENABLE 0x06
#define CMD_FAST_READ 0x0B
#define CMD_DUAL_READ 0x3B
#define CMD_QUAD_READ 0x6B
#define CMD_BULK_ERASE 0xC7
#define CMD_SEC_ERASE 0xD8
#define CMD_READ_ID 0x9F

#define COMMAND_OFFSET 0 /* FLASH instruction */
#define ADDRESS_1_OFFSET 1 /* MSB byte of address to read or write */
#define ADDRESS_2_OFFSET 2 /* Middle byte of address to read or write */
#define ADDRESS_3_OFFSET 3 /* LSB byte of address to read or write */
#define DATA_OFFSET 4 /* Start of Data for Read/Write */
#define DUMMY_OFFSET 4 /* Dummy byte offset for fast, dual and quad reads*/
#define DUMMY_SIZE 1 /* Number of dummy bytes for fast, dual and quad
reads */
#define RD_ID_SIZE 4 /* Read ID command + 3 bytes ID response */
#define BULK_ERASE_SIZE 1 /* Bulk Erase command size */
#define SEC_ERASE_SIZE 4 /* Sector Erase command + Sector address */
#define OVERHEAD_SIZE 4

#define SECTOR_SIZE 0x10000
#define NUM_SECTORS 0x100
#define NUM_PAGES 0x10000
#define PAGE_SIZE 256
#define PAGE_COUNT 16

#define TEST_ADDRESS 0x0
#define MAX_DATA (PAGE_COUNT * PAGE_SIZE)

void FlashErase(XQspiPs *QspiPtr, u32 Address, u32 ByteCount);
void FlashWrite(XQspiPs *QspiPtr, u32 Address, u32 ByteCount, u8 Command);
void FlashRead(XQspiPs *QspiPtr, u32 Address, u32 ByteCount, u8 Command);
int FlashReadID(void);
void FlashQuadEnable(XQspiPs *QspiPtr);
int QspiFlash_test_function(XQspiPs *QspiInstancePtr, u16 QspiDeviceId);

static XQspiPs QspiInstance;

u8 ReadBuffer[MAX_DATA + DATA_OFFSET + DUMMY_SIZE];
u8 WriteBuffer[PAGE_SIZE + DATA_OFFSET];

int main(void)
{
	int Status;
	xil_printf("/******this demo test qspi flash base function**********/\n");

	Status = QspiFlash_test_function(&QspiInstance, QSPI_DEVICE_ID);
	if (Status != XST_SUCCESS)
	{
		xil_printf("qspi est Failed\n");
		return XST_FAILURE;
	}
	xil_printf("test qspi flash successed\n");
	return XST_SUCCESS;
}

int QspiFlash_test_function(XQspiPs *QspiInstancePtr, u16 QspiDeviceId)
{
	int Status;
	u8 *BufferPtr;
	u8 tmp_alue;
	int Count;
	int Page;
	XQspiPs_Config *QspiConfig;

	/* Initialize the QSPI driver so that it's ready to use*/
	QspiConfig = XQspiPs_LookupConfig(QspiDeviceId);
	if (QspiConfig == NULL)return XST_FAILURE;

	Status = XQspiPs_CfgInitialize(QspiInstancePtr, QspiConfig, QspiConfig->BaseAddress);
	if (Status != XST_SUCCESS)return XST_FAILURE;

	/* Perform a self-test to check hardware build*/
	Status = XQspiPs_SelfTest(QspiInstancePtr);
	if (Status != XST_SUCCESS)return XST_FAILURE;
	tmp_alue = 0;
	for (Count = 0; Count < MAX_DATA; Count++)
	{
		tmp_alue++;
		WriteBuffer[DATA_OFFSET + Count] = (u8)(tmp_alue);
	}
	memset(ReadBuffer, 0x00, sizeof(ReadBuffer));

	//SET MANUAL OPERATE AND MANUAL OPERATE CS MODE
	XQspiPs_SetOptions(QspiInstancePtr, XQSPIPS_MANUAL_START_OPTION
		| XQSPIPS_FORCE_SSELECT_OPTION | XQSPIPS_HOLD_B_DRIVE_OPTION);

	// clock fabric
	XQspiPs_SetClkPrescaler(QspiInstancePtr, XQSPIPS_CLK_PRESCALE_8);

	/* Assert the FLASH chip select.*/
	XQspiPs_SetSlaveSelect(QspiInstancePtr);

	FlashReadID();//READ ID
	FlashQuadEnable(QspiInstancePtr);

	//erase entire flash
	xil_printf("flash erase...\n");
	FlashErase(QspiInstancePtr, TEST_ADDRESS, MAX_DATA);
	xil_printf("erase complete\n");
	//write flash
	xil_printf("flash write test data...\n");
	for (Page = 0; Page < PAGE_COUNT; Page++)
	{
		FlashWrite(QspiInstancePtr, (Page * PAGE_SIZE) + TEST_ADDRESS, PAGE_SIZE,
			CMD_WRITE);
	}
	xil_printf("write data complete\n");

	//USE NORMAL READ
	FlashRead(QspiInstancePtr, TEST_ADDRESS, MAX_DATA, CMD_READ);
	xil_printf("flash normal read test data...\n");
	BufferPtr = &ReadBuffer[DATA_OFFSET];

	tmp_alue = 0;
	for (Count = 0; Count < MAX_DATA; Count++)
	{
		tmp_alue++;
		if (BufferPtr[Count] != (u8)(tmp_alue))return XST_FAILURE;
		xil_printf(" %x", BufferPtr[Count]);

	}
	xil_printf("\n read data ok\n");
	//USE FAST_READ
	memset(ReadBuffer, 0x00, sizeof(ReadBuffer));
	xil_printf("flash fast read test data...\n");
	FlashRead(QspiInstancePtr, TEST_ADDRESS, MAX_DATA, CMD_FAST_READ);

	BufferPtr = &ReadBuffer[DATA_OFFSET + DUMMY_SIZE];

	tmp_alue = 0;
	for (Count = 0; Count < MAX_DATA; Count++)
	{
		tmp_alue++;
		if (BufferPtr[Count] != (u8)(tmp_alue))return XST_FAILURE;
		xil_printf(" %x", BufferPtr[Count]);
	}
	xil_printf("\n read data ok\n");

	// USE DUAL_READ MODE READ
	memset(ReadBuffer, 0x00, sizeof(ReadBuffer));
	xil_printf("flash dual read test data...\n");
	FlashRead(QspiInstancePtr, TEST_ADDRESS, MAX_DATA, CMD_DUAL_READ);
	BufferPtr = &ReadBuffer[DATA_OFFSET + DUMMY_SIZE];

	tmp_alue = 0;
	for (Count = 0; Count < MAX_DATA; Count++)
	{
		tmp_alue++;
		if (BufferPtr[Count] != (u8)(tmp_alue))return XST_FAILURE;
		xil_printf(" %x", BufferPtr[Count]);
	}
	xil_printf("\n read data ok\n");
	//USE QUAD_READ
	memset(ReadBuffer, 0x00, sizeof(ReadBuffer));
	xil_printf("flash quad read test data...\n");
	FlashRead(QspiInstancePtr, TEST_ADDRESS, MAX_DATA, CMD_QUAD_READ);

	BufferPtr = &ReadBuffer[DATA_OFFSET + DUMMY_SIZE];
	tmp_alue = 0;
	for (Count = 0; Count < MAX_DATA; Count++)
	{

		tmp_alue++;
		if (BufferPtr[Count] != (u8)(tmp_alue))return XST_FAILURE;
		xil_printf(" %x", BufferPtr[Count]);

	}
	xil_printf("\n read data ok\n");
	return XST_SUCCESS;

}

void FlashWrite(XQspiPs *QspiPtr, u32 Address, u32 ByteCount, u8 Command)
{
	u8 WriteEnableCmd = { CMD_RITE_ENABLE };
	u8 ReadStatusCmd[] = { CMD_READ_STATUS, 0 }; /* must send 2 bytes */
	u8 FlashStatus[2];
	XQspiPs_PolledTransfer(QspiPtr, &WriteEnableCmd, NULL, sizeof(WriteEnableCmd));
	WriteBuffer[COMMAND_OFFSET] = Command;
	WriteBuffer[ADDRESS_1_OFFSET] = (u8)((Address & 0xFF0000) >> 16);
	WriteBuffer[ADDRESS_2_OFFSET] = (u8)((Address & 0xFF00) >> 8);
	WriteBuffer[ADDRESS_3_OFFSET] = (u8)(Address & 0xFF);
	XQspiPs_PolledTransfer(QspiPtr, WriteBuffer, NULL, ByteCount + OVERHEAD_SIZE);
	while (1)
	{
		XQspiPs_PolledTransfer(QspiPtr, ReadStatusCmd,
			FlashStatus, sizeof(ReadStatusCmd));
		if ((FlashStatus[1] & 0x01) == 0) break;
	}
}

void FlashRead(XQspiPs *QspiPtr, u32 Address, u32 ByteCount, u8 Command)
{
	WriteBuffer[COMMAND_OFFSET] = Command;
	WriteBuffer[ADDRESS_1_OFFSET] = (u8)((Address & 0xFF0000) >> 16);
	WriteBuffer[ADDRESS_2_OFFSET] = (u8)((Address & 0xFF00) >> 8);
	WriteBuffer[ADDRESS_3_OFFSET] = (u8)(Address & 0xFF);
	if ((Command == CMD_FAST_READ) || (Command == CMD_DUAL_READ) || (Command
		== CMD_QUAD_READ))ByteCount += DUMMY_SIZE;
	XQspiPs_PolledTransfer(QspiPtr, WriteBuffer, ReadBuffer, ByteCount +
		OVERHEAD_SIZE);
}

void FlashErase(XQspiPs *QspiPtr, u32 Address, u32 ByteCount)
{
	u8 WriteEnableCmd = { CMD_RITE_ENABLE };
	u8 ReadStatusCmd[] = { CMD_READ_STATUS, 0 }; /* must send 2 bytes */
	u8 FlashStatus[2];
	int Sector;

	if (ByteCount == (NUM_SECTORS * SECTOR_SIZE))
	{
		XQspiPs_PolledTransfer(QspiPtr, &WriteEnableCmd,
			NULL, sizeof(WriteEnableCmd));
		WriteBuffer[COMMAND_OFFSET] = CMD_BULK_ERASE;
		XQspiPs_PolledTransfer(QspiPtr, WriteBuffer, NULL, BULK_ERASE_SIZE);
			while (1)
			{
				XQspiPs_PolledTransfer(QspiPtr,
					ReadStatusCmd, FlashStatus, sizeof(ReadStatusCmd));
				if ((FlashStatus[1] & 0x01) == 0)break;
			}
		return;
	}

	for (Sector = 0; Sector < ((ByteCount / SECTOR_SIZE) + 1); Sector++)
	{
		XQspiPs_PolledTransfer(QspiPtr, &WriteEnableCmd,
			NULL, sizeof(WriteEnableCmd));
		WriteBuffer[COMMAND_OFFSET] = CMD_SEC_ERASE;
		WriteBuffer[ADDRESS_1_OFFSET] = (u8)(Address >> 16);
		WriteBuffer[ADDRESS_2_OFFSET] = (u8)(Address >> 8);
		WriteBuffer[ADDRESS_3_OFFSET] = (u8)(Address & 0xFF);
		XQspiPs_PolledTransfer(QspiPtr, WriteBuffer, NULL, SEC_ERASE_SIZE);

		while (1)
		{
			XQspiPs_PolledTransfer(QspiPtr,
				ReadStatusCmd, FlashStatus, sizeof(ReadStatusCmd));
			if ((FlashStatus[1] & 0x01) == 0)break;
		}

		Address += SECTOR_SIZE;

	}

}

int FlashReadID(void)
{
	int Status;
	/* Read ID in Auto mode.*/
	WriteBuffer[COMMAND_OFFSET] = CMD_READ_ID;
	WriteBuffer[ADDRESS_1_OFFSET] = 0x23; /* 3 dummy bytes */
	WriteBuffer[ADDRESS_2_OFFSET] = 0x08;
	WriteBuffer[ADDRESS_3_OFFSET] = 0x09;
	Status = XQspiPs_PolledTransfer(&QspiInstance, WriteBuffer, ReadBuffer, RD_ID_SIZE);
		if (Status != XST_SUCCESS)return XST_FAILURE;
	xil_printf("QSPI Flash ID=0x%x 0x%x 0x%x\n", ReadBuffer[1],
		ReadBuffer[2], ReadBuffer[3]);
	return XST_SUCCESS;
}

void FlashQuadEnable(XQspiPs *QspiPtr)
{
	u8 WriteEnableCmd = { CMD_RITE_ENABLE };
	u8 ReadStatusCmd[] = { CMD_READ_STATUS, 0 };
	u8 QuadEnableCmd[] = { CMD_WRITE_STATUS, 0 };
	u8 FlashStatus[2];

	if (ReadBuffer[1] == 0x9D)
	{
		XQspiPs_PolledTransfer(QspiPtr,
			ReadStatusCmd, FlashStatus, sizeof(ReadStatusCmd));
		QuadEnableCmd[1] = FlashStatus[1] | 1 << 6;
		XQspiPs_PolledTransfer(QspiPtr, &WriteEnableCmd,
			NULL, sizeof(WriteEnableCmd));
		XQspiPs_PolledTransfer(QspiPtr, QuadEnableCmd,
			NULL, sizeof(QuadEnableCmd));
	}
}
