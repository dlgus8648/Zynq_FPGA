
#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xdevcfg.h"
#include "xparameters.h"
#include "ff.h"

int SD_Init(void);
int Sd_Test_Write(void);
int Sd_Test_Read(void);

int main()
{
	init_platform();
	print("KIM's zynq_sd_card_fatfs-test \r\n");
	SD_Init();
	Sd_Test_Write();
	Sd_Test_Read();
	while (1)
	{

		;

	}
	cleanup_platform();
	return 0;

}

static FATFS fatfs;

int SD_Init()
{
	FRESULT rc;
	rc = f_mount(&fatfs, "", 0);
	if (rc)
	{
		xil_printf("ERROR: f_mount returned %d\r\n", rc);
		return XST_FAILURE;
	}
	return XST_SUCCESS;
}

int Sd_Test_Write()
{
	FIL fil;
	FRESULT rc;
	UINT br;
	const char src_str1[] = "KIM RIHYEIN Test  card write and read. \n";
		rc = f_open(&fil, "test.txt", FA_WRITE | FA_CREATE_NEW);
	if (rc)
	{
		xil_printf("ERROR : f_open returned %d\r\n", rc);
		return XST_FAILURE;
	}
	rc = f_write(&fil, src_str1, sizeof(src_str1), &br); rc = f_sync(&fil);
	rc = f_close(&fil);
}

int Sd_Test_Read()
{
	FIL fil;
	FRESULT rc;
	UINT br;
	const char src_str[4096] = { 0 };
	rc = f_open(&fil, "test.txt", FA_READ);
	if (rc)
	{
		xil_printf("ERROR : f_open returned %d\r\n", rc);
		return XST_FAILURE;
	}
	rc = f_lseek(&fil, 0);
	rc = f_read(&fil, src_str, 4096, &br);
	xil_printf(src_str);
	rc = f_close(&fil);
}
