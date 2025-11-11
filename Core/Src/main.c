/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// flash specification
#define W25Q80 	0XEF13
#define W25Q16 	0XEF14
#define W25Q32 	0XEF15
#define W25Qxx 	0XEF16
#define W25Q128	0XEF17
#define W25Q256 0XEF18

// instruction set, comes from DATASHEET.
// not each specification have such below instruction.
#define W25Qxx_WriteEnable			0x06
#define W25Qxx_WriteDisable			0x04
#define W25Qxx_ReadStatusReg1		0x05
#define W25Qxx_ReadStatusReg2		0x35
#define W25Qxx_ReadStatusReg3		0x15
#define W25Qxx_WriteStatusReg1  	0x01
#define W25Qxx_WriteStatusReg2  	0x31
#define W25Qxx_WriteStatusReg3  	0x11
#define W25Qxx_ReadData				0x03
#define W25Qxx_FastReadData			0x0B
#define W25Qxx_FastReadDual			0x3B
#define W25Qxx_PageProgram			0x02
#define W25Qxx_BlockErase			0xD8
#define W25Qxx_SectorErase			0x20
#define W25Qxx_ChipErase			0xC7
#define W25Qxx_PowerDown			0xB9
#define W25Qxx_ReleasePowerDown		0xAB
#define W25Qxx_DeviceID				0xAB
#define W25Qxx_ManufactDeviceID		0x90
#define W25Qxx_JedecDeviceID		0x9F
#define W25Qxx_Enable4ByteAddr    	0xB7
#define W25Qxx_Exit4ByteAddr      	0xE9
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void W25Qxx_Init(void);
uint16_t W25Qxx_ReadID(void);  	        		//read FLASH_ID
uint8_t W25Qxx_ReadSR(uint8_t regno);   		//read status Register
void W25Qxx_Write_Enable(void);  				//write enable
void W25Qxx_Write_Disable(void);				//write protect
void W25Qxx_Write_NoCheck(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite);
void W25Qxx_Read(uint8_t* pBuffer,uint32_t ReadAddr,uint16_t NumByteToRead);   //read flash
void W25Qxx_Write(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite);//write flash
void W25Qxx_Erase_Sector(uint32_t Dst_Addr);	//sector erase
void W25Qxx_Wait_Busy(void);           			//wait for Idle
uint8_t SPI_ReadWriteByte(uint8_t TxData);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
  * @brief  read W25Qxx ID
  * @param  void
  * @retval uint16_t Temp:
  */
uint16_t W25Qxx_TYPE;							//define W25Qxx type
uint16_t W25Qxx_ReadID(void)
{
	uint16_t Temp = 0;
	HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_RESET);// enable CS,Low level active.
//	SPI_ReadWriteByte(0x90);					//sent read ID cmd
	SPI_ReadWriteByte(W25Qxx_ManufactDeviceID);	//BYTE1=90h,instruction code
	SPI_ReadWriteByte(0x00);					//BYTE2
	SPI_ReadWriteByte(0x00);					//BYTE3
	SPI_ReadWriteByte(0x00);					//BYTE4,return 00h
	Temp|=SPI_ReadWriteByte(0xFF)<<8;			//BYTE5,return efh,high 8bit
	Temp|=SPI_ReadWriteByte(0xFF);				//BYTE6,return 14h
	W25Qxx_TYPE=Temp;
	printf("FLASH SPECIFICATION IS :%x\r\n",W25Qxx_TYPE);	//test
	HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_SET);	// disable CS
	return Temp;
}

/**
  * @brief  W25Qxx Read SR
  * SR1:
  * BIT7  6   5   4   3   2   1   0
  * SPR   RV  TB BP2 BP1 BP0 WEL BUSY
  * SPR:default 0,SR protection bit, to be used with WP
  * TB,BP2,BP1,BP0:FLASH region write protection settings
  * WEL: write enable lock
  * BUSY:busy flag(1,busy;0,idle)
  * default:0x00
  * SR2:
  * BIT7  6   5   4   3   2   1   0
  * SUS   CMP LB3 LB2 LB1 (R) QE  SRP1
  * SR3:
  * BIT7      6    5    4   3   2   1   0
  * HOLD/RST  DRV1 DRV0 (R) (R) WPS ADP ADS
  *
  * @param  regno:SR1~3
  * @retval SR value
  */
uint8_t W25Qxx_ReadSR(uint8_t regno)
{
	uint8_t byte=0,command=0;
    switch(regno)
    {
        case 1:
            command=W25Qxx_ReadStatusReg1;    // read SR1,0x05
            break;
        case 2:
            command=W25Qxx_ReadStatusReg2;    // read SR2
            break;
        case 3:
            command=W25Qxx_ReadStatusReg3;    // read SR3
            break;
        default:
            command=W25Qxx_ReadStatusReg1;
            break;
    }
	HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_RESET);// enable CS
	SPI_ReadWriteByte(command);            	// BYTE1=05h,sent read SR CMD
	byte=SPI_ReadWriteByte(0Xff);          	// BYTE2,return SR1
	printf("FLASH SR1 :%d\r\n",byte);		// test
	HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_SET);	// disable CS
	return byte;
}


/**
  * @brief  W25Qxx write enable
  * 		set WEL bit
  * @param  void
  * @retval void
  */
void W25Qxx_Write_Enable(void)
{
	HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_RESET);	//enable CS
    SPI_ReadWriteByte(W25Qxx_WriteEnable);   													//sent write enable CMD
	HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_SET);		//disable CS
}

/**
  * @brief  W25Qxx write disable
  * 		reset WEL bit
  * @param  void
  * @retval void
  */
void W25Qxx_Write_Disable(void)
{
	HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_RESET);    // disable CS
    SPI_ReadWriteByte(W25Qxx_WriteDisable);  													// sent write disable CMD
	HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_SET);      // enable CS
}

/**
  * @brief  read SPI FLASH
  * 		Read data of specified length from the specified address.
  * @param  pBuffer:Data storage area
  * 		ReadAddr:Starting address(24bit)
  * 		NumByteToRead:Number of bytes to be read(max 65535)
  * @retval void
  */
void W25Qxx_Read(uint8_t* pBuffer,uint32_t ReadAddr,uint16_t NumByteToRead)
{
 	uint16_t i;
	HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_RESET);	// enable CS
    SPI_ReadWriteByte(W25Qxx_ReadData);      													// sent read CMD
    if(W25Qxx_TYPE==W25Q256)                													// if W25Q256 addr is 4 bytes，sent up to 8 bit.
    {
        SPI_ReadWriteByte((uint8_t)((ReadAddr)>>24));
    }
    SPI_ReadWriteByte((uint8_t)((ReadAddr)>>16));   											// sent 24bit addr
    SPI_ReadWriteByte((uint8_t)((ReadAddr)>>8));
    SPI_ReadWriteByte((uint8_t)ReadAddr);
    for(i=0;i<NumByteToRead;i++)
	{
        pBuffer[i]=SPI_ReadWriteByte(0XFF);    													// Circular reading
    }
	HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_SET);		// disable CS
}


/**
  * @brief  W25Qxx write one page
  * 		SPI writes less than 256 bytes of data within one page (0~65535)
  * 		Write up to 256 bytes of data starting at the specified address
  * @param  pBuffer:Data storage area
  * 		WriteAddr:Starting address(24bit)
  * 		NumByteToWrite:The bytes to be written(max 256),The size should not exceed the remaining bytes of the page.
  * @retval void
  */
void W25Qxx_Write_Page(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite)
{
 	uint16_t i;
    W25Qxx_Write_Enable();                  													// SET WEL
	HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_RESET);	// enable CS
    SPI_ReadWriteByte(W25Qxx_PageProgram);														// sent write page CMD
    if(W25Qxx_TYPE==W25Q256)                													// If W25Q256, the address is 4 bytes, and the highest 8 bits need to be sent.
    {
        SPI_ReadWriteByte((uint8_t)((WriteAddr)>>24));
    }
    SPI_ReadWriteByte((uint8_t)((WriteAddr)>>16)); 												// sent 24bit addr
    SPI_ReadWriteByte((uint8_t)((WriteAddr)>>8));
    SPI_ReadWriteByte((uint8_t)WriteAddr);
    for(i=0;i<NumByteToWrite;i++)SPI_ReadWriteByte(pBuffer[i]);									// Circular writing
	HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_SET);    	// disable CS
	W25Qxx_Wait_Busy();					   														// Waiting for write completion
}


/**
  * @brief  write SPI FLASH with no check
  * 		The data in the address range to be written must be all 0XFF,
  * 		otherwise the data written at non-0XFF will fail.
  * 		with the function to turn page automatically.
  * 		Start writing the specified length of data at the specified address.
  * @param  pBuffer:data buffer
  * 		WriteAddr:start write add(24bit)
  * 		NumByteToWrite:bytes to be written (max 65535)
  * @retval void
  */
void W25Qxx_Write_NoCheck(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite)
{
	uint16_t pageremain;
	pageremain=256-WriteAddr%256; 								// The bytes remaining on a single page
	if(NumByteToWrite<=pageremain)pageremain=NumByteToWrite;	// No more than 256 bytes
	while(1)
	{
		W25Qxx_Write_Page(pBuffer,WriteAddr,pageremain);
		if(NumByteToWrite==pageremain)break;					// Finished writing.
	 	else 													//NumByteToWrite>page remain
		{
			pBuffer+=pageremain;
			WriteAddr+=pageremain;

			NumByteToWrite-=pageremain;			  				// Subtract the bytes that have already been written
			if(NumByteToWrite>256)pageremain=256; 				// 256 bytes can be written at a time
			else pageremain=NumByteToWrite; 	  				// Not enough for 256 bytes
		}
	};
}

/**
  * @brief  write SPI FLASH
  * 		Start writing the specified length of data at the specified address,
  * 		include the function with erase operation.
  * @param  pBuffer:data buffer
  * 		WriteAddr:start write add(24bit)
  * 		NumByteToWrite:bytes to be written (max 65535)
  * @retval void
  */
uint8_t W25Qxx_BUFFER[4096];
void W25Qxx_Write(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite)
{
	uint32_t secpos;
	uint16_t secoff;
	uint16_t secremain;
 	uint16_t i;
	uint8_t* W25Qxx_BUF;
   	W25Qxx_BUF=W25Qxx_BUFFER;
 	secpos=WriteAddr/4096;										// sector address
	secoff=WriteAddr%4096;										// offset within the sector
	secremain=4096-secoff;										// sector remaining space
// 	printf("ad:%X,nb:%X\r\n",WriteAddr,NumByteToWrite);			// test
 	if(NumByteToWrite<=secremain)secremain=NumByteToWrite;		// No more than 4096 bytes
	while(1)
	{
		W25Qxx_Read(W25Qxx_BUF,secpos*4096,4096);				// Read out the content of the entire sector;
		for(i=0;i<secremain;i++)								// Check data
		{
			if(W25Qxx_BUF[secoff+i]!=0XFF)break;				// need to be wiped out
		}
		if(i<secremain)											// need to be erase
		{
			W25Qxx_Erase_Sector(secpos);						// erase this sector
			for(i=0;i<secremain;i++)	   						// copy
			{
				W25Qxx_BUF[i+secoff]=pBuffer[i];
			}
			W25Qxx_Write_NoCheck(W25Qxx_BUF,secpos*4096,4096);	// write cover sector

		}else W25Qxx_Write_NoCheck(pBuffer,WriteAddr,secremain);// Write the already erased, write directly into the remaining interval of the sector.
		if(NumByteToWrite==secremain)break;						// Written up
		else													// Unfinished;
		{
			secpos++;											// Sector address increment 1
			secoff=0;											// The offset is 0

		   	pBuffer+=secremain;									// pointer offset
			WriteAddr+=secremain;								// Write address offset
		   	NumByteToWrite-=secremain;							// Decreasing number of bytes
			if(NumByteToWrite>4096)secremain=4096;				// The next sector is still not finished;
			else secremain=NumByteToWrite;						// The next sector can be written now.
		}
	};
}

/**
  * @brief  Erase a sector
  * @param  Dst_Addr:Sector address, set according to actual capacity.
  * 		Minimum time to erase a sector is 150ms.
  * @retval void
  */
void W25Qxx_Erase_Sector(uint32_t Dst_Addr)
{
//	printf("fe:%x\r\n",Dst_Addr);								// monitor flash erasing, used for test
 	Dst_Addr*=4096;
    W25Qxx_Write_Enable();                  					// SET WEL
    W25Qxx_Wait_Busy();
  	HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_RESET);
    SPI_ReadWriteByte(W25Qxx_SectorErase);   					// Send erase sector CMD
    if(W25Qxx_TYPE==W25Q256)                					// If W25Q256, the address is 4 bytes, the highest 8 bits need to be sent.
    {
        SPI_ReadWriteByte((uint8_t)((Dst_Addr)>>24));
    }
    SPI_ReadWriteByte((uint8_t)((Dst_Addr)>>16));  				// sent 24bit addr
    SPI_ReadWriteByte((uint8_t)((Dst_Addr)>>8));
    SPI_ReadWriteByte((uint8_t)Dst_Addr);

	HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_SET);
    W25Qxx_Wait_Busy();   				    					// Waiting for erase completion
}

/**
  * @brief  Wait for idle
  * @param  void
  * @retval void
  */
void W25Qxx_Wait_Busy(void)
{
	while((W25Qxx_ReadSR(1)&0x01)==0x01);   					// Wait for BUSY bit to clear
}

/**
  * @brief  write CMD into flash and then return a value,in block mode
  * 		either a byte is written or a byte is returned.
  * @param  TxData: bytes written
  * @retval Rxdata: bytes to be return
  */
uint8_t SPI_ReadWriteByte(uint8_t TxData)
{
	uint8_t Rxdata;
    HAL_SPI_TransmitReceive(&hspi2,&TxData,&Rxdata,1, 1000);
 	return Rxdata;          		    						// Return the received data
}

/**
 * @brief	SPI send data of specified length
 * @param	send_buf:send buffer start address
 * 			size: the bytes to be sent
 * @retval	HAL_OK
 */
//static HAL_StatusTypeDef SPI_Transmit(uint8_t* send_buf, uint16_t size)
//{
//    return HAL_SPI_Transmit(&hspi2, send_buf, size, 100);
//}

/**
 * @brief   SPI receive a specified bytes
 * @param   recv_buf:receive data buffer start address
 * 			size:the byte to receive
 * @retval  HAL_OK
 */
//static HAL_StatusTypeDef SPI_Receive(uint8_t* recv_buf, uint16_t size)
//{
//   return HAL_SPI_Receive(&hspi2, recv_buf, size, 100);
//}

/**
 * @brief   SPI transmit data while receiving a specified length of data.
 * @param   send_buf:sent buffer start address
 * @param   recv_buf:receive buffer start address
 * @param   size:the bytes to be sent/received
 * @retval  HAL_OK
 */
//static HAL_StatusTypeDef SPI_TransmitReceive(uint8_t* send_buf, uint8_t* recv_buf, uint16_t size)
//{
//   return HAL_SPI_TransmitReceive(&hspi2, send_buf, recv_buf, size, 100);
//}

/**
 * @brief   read Flash ID
 * @param   none
 * @retval  device_id
 */
//uint16_t W25Qxx_ReadID(void)
//{
//    uint8_t recv_buf[2] = {0};    //recv_buf[0]存放Manufacture ID, recv_buf[1]存放Device ID
//    uint16_t device_id = 0;
//    uint8_t send_data[4] = {ManufactDeviceID_CMD,0x00,0x00,0x00};
//    HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_RESET);
//    if (HAL_OK == SPI_Transmit(send_data, 4))
//    {
//        if (HAL_OK == SPI_Receive(recv_buf, 2))
//        {
//            device_id = (recv_buf[0] << 8) | recv_buf[1];
//        }
//    }
//    HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_SET);
//    return device_id;
//}

/**
 * @brief     read W25Qxx SR
 * @param     reg: SR1 or SR2
 * @retval    0
 */
//uint8_t W25Qxx_ReadSR(uint8_t reg)
//{
//    uint8_t result = 0;
//    uint8_t send_buf[4] = {0x00,0x00,0x00,0x00};
//    switch(reg)
//    {
//        case 1:
//            send_buf[0] = READ_STATU_REGISTER_1;
//        case 2:
//            send_buf[0] = READ_STATU_REGISTER_2;
//        case 0:
//        default:
//            send_buf[0] = READ_STATU_REGISTER_1;
//    }

//    HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_RESET);
//
//    if (HAL_OK == SPI_Transmit(send_buf, 4))
//    {
//        if (HAL_OK == SPI_Receive(&result, 1))
//        {
//            HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_SET);
//
//            return result;
//        }
//    }

//    HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_SET);
//    return 0;
//}

/**
 * @brief   block waiting for Flash to be idle
 * @param   none
 * @retval  none
 */
//static void W25Qxx_Wait_Busy(void)
//{
//    while((W25Qxx_ReadSR(1) & 0x01) == 0x01); // wait for the BUSY bit to clear
//}

/**
 * @brief   Read SPI FLASH data
 * @param   buffer:  data buffer
 *			start_addr:starting address to be read (32bit)
 * 			nbytes:the bytes to be read (max 65535)
 * @retval  successful return 0, failure return -1
 */
//int W25Qxx_Read(uint8_t* buffer, uint32_t start_addr, uint16_t nbytes)
//{
//    uint8_t cmd = READ_DATA_CMD;
//    start_addr = start_addr << 8;
//    W25Qxx_Wait_Busy();
//    HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_RESET);	// enable CS
//    SPI_Transmit(&cmd, 1);
//    if (HAL_OK == SPI_Transmit((uint8_t*)&start_addr, 3))
//    {
//        if (HAL_OK == SPI_Receive(buffer, nbytes))
//        {
//            HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_SET);
//            return 0;
//        }
//    }
//    HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_SET);
//    return -1;
//}

/**
 * @brief    W25Qxx write enable, set the WEL bit of the S1 register
 * @param    none
 * @retval
 */
//void W25Qxx_Write_Enable(void)
//{
//    uint8_t cmd= WRITE_ENABLE_CMD;
//    HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_RESET);
//    SPI_Transmit(&cmd, 1);
//    HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_SET);
//    W25Qxx_Wait_Busy();
//}

/**
 * @brief    W25Qxx write disable, clear WEL bit
 * @param    none
 * @retval   none
 */
//void W25Qxx_Write_Disable(void)
//{
//    uint8_t cmd = WRITE_DISABLE_CMD;
//    HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_RESET);
//    SPI_Transmit(&cmd, 1);
//    HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_SET);
//    W25Qxx_Wait_Busy();
//}

/**
 * @brief   W25Qxx Erase entire sector
 * @param   sector_addr:sector address set according to actual capacity;
 * @retval  none
 * @note    Blocking mode
 */
//void W25Qxx_Erase_Sector(uint32_t sector_addr)
//{
//    uint8_t cmd = SECTOR_ERASE_CMD;
//    sector_addr *= 4096;		//each block has 16 sectors, each sector is 4KB, and it needs to be converted into a real address
//    sector_addr <<= 8;
//    W25Qxx_Write_Enable();	//erase operation is to write 0xFF, and write enable needs to be turned on.
//    W25Qxx_Wait_Busy();		//waiting for write enable to complete;
//    HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_RESET);	// enable CS
//    SPI_Transmit(&cmd, 1);
//    SPI_Transmit((uint8_t*)&sector_addr, 3);
//    HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_SET);
//    W25Qxx_Wait_Busy();		//waiting for sector erase completion
//}

/**
 * @brief	write page
 * @param	dat:data buffer
 *			WriteAddr:the starting address of the data buffer to be written
 *			nbytes:the bytes to write (0-256)
 * @retval	none
 */
//void W25Qxx_Page_Program(uint8_t* dat, uint32_t WriteAddr, uint16_t nbytes)
//{
//    uint8_t cmd = PAGE_PROGRAM_CMD;
//    WriteAddr <<= 8;
//    W25Qxx_Write_Enable();
//    HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_RESET);	// enable CS
//    SPI_Transmit(&cmd, 1);
//    SPI_Transmit((uint8_t*)&WriteAddr, 3);
//    SPI_Transmit(dat, nbytes);
//    HAL_GPIO_WritePin(W25Qxx_CHIP_SELECT_GPIO_Port, W25Qxx_CHIP_SELECT_Pin, GPIO_PIN_SET);
//    W25Qxx_Wait_Busy();
//}

/**
 * @brief	write page no check
 * 			The data written in the address range must be ensured to be all 0XFF,
 * 			otherwise the data written will be lost at non-0XFF
 * 			with automatic page turning function,
 * 			write the specified bytes from the specified start address,
 * 			but ensure that the address does not exceed the range.
 * @param	pBuffer:data buffer
 *			WriteAddr:starting address to be written (24 bit)
 *			NumByteToWrite:bytes to write (max 65535)
 * @retval	none
 * 			CHECK OK
 */
//void W25Qxx_Write_NoCheck(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite)
//{
//	uint16_t pageremain;
//	pageremain=256-WriteAddr%256;					// the bytes remaining on a single page
//	if(NumByteToWrite<=pageremain)pageremain=NumByteToWrite;	//no more than 4096 bytes
//	while(1)
//	{
//		W25Qxx_Page_Program(pBuffer,WriteAddr,pageremain);
//		if(NumByteToWrite==pageremain)break;		// Written up
//	 	else //NumByteToWrite>pageremain
//		{
//			pBuffer+=pageremain;
//			WriteAddr+=pageremain;
//			NumByteToWrite-=pageremain;			  	// subtract the bytes written
//			if(NumByteToWrite>256)pageremain=256; 	// 256 bytes can be written at a time
//			else pageremain=NumByteToWrite; 	  	// not enough for 256 bytes
//		}
//	};
//}

/**
 * @brief	write SPI FLASH
 * 			start writing the specified size of data at the specified address.
 * 			with erase function.
 * @param	pBuffer:data buffer
 *			WriteAddr:the starting address of the data buffer to be written
 *			NumByteToWrite:the bytes to write (max 65535)
 * @retval	none
 */
//uint8_t W25Qxx_BUFFER[4096];
//void W25Qxx_Write(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite)
//{
//	uint32_t secpos;
//	uint16_t secoff;
//	uint16_t secremain;
// 	uint16_t i;
//	uint8_t * W25Qxx_BUF;
//  W25Qxx_BUF=W25Qxx_BUFFER;
// 	secpos=WriteAddr/4096;											// sector addr
//	secoff=WriteAddr%4096;											// offset within the sector
//	secremain=4096-secoff;											// remaining space size
// 	//printf("ad:%X,nb:%X\r\n",WriteAddr,NumByteToWrite);			// test
// 	if(NumByteToWrite<=secremain)secremain=NumByteToWrite;			// no more than 4096 bytes
//	while(1)
//	{
//		W25Qxx_Read(W25Qxx_BUF,secpos*4096,4096);					// read the entire sector of memory
//		for(i=0;i<secremain;i++)									// check
//		{
//			if(W25Qxx_BUF[secoff+i]!=0XFF)break;
//		}
//		if(i<secremain)
//		{
//			W25Qxx_Erase_Sector(secpos);							// erase sector
//			for(i=0;i<secremain;i++)	   							// copy
//			{
//				W25Qxx_BUF[i+secoff]=pBuffer[i];
//			}
//			W25Qxx_Write_NoCheck(W25Qxx_BUF,secpos*4096,4096);		// write entire sector

//		}else W25Qxx_Write_NoCheck(pBuffer,WriteAddr,secremain);	// write into addr that has been erased, write the remaining interval of the sector directly.
//		if(NumByteToWrite==secremain)break;							// written up
//		else														// write unfinished;
//		{
//			secpos++;												// sector address auto-increment by 1
//			secoff=0;												// offset bit set 0

//		   	pBuffer+=secremain;  									// pointer offset
//			WriteAddr+=secremain;									// write addr offset
//		   	NumByteToWrite-=secremain;								// decreasing number of bytes
//			if(NumByteToWrite>4096)secremain=4096;					// the next sector is still not finished;
//			else secremain=NumByteToWrite;							// the next sector can be written now.
//		}
//	};
//}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI2_Init();
  MX_USART6_UART_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
int __io_putchar(int ch)
{
	HAL_UART_Transmit(&huart6,(uint8_t*)&ch,1,0xFFFF);
	return ch;
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
