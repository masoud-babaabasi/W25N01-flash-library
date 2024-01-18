/*********************************************************************************************************
*
* File                : W25nxx_SPI_Dual.h
* Hardware Environment: 
* Build Environment   : RealView MDK-ARM  Version: 5.32
* Version             : V1.0
* By                  : M.Babaabasi
*
*
*********************************************************************************************************/

#ifndef __W25NXX_SPI_H
#define __W25NXX_SPI_H
#include "main.h"   
 // This library is designed by Masoud Babaabasi May 2022
////////////////////////////////////////////////////////////////////////////////// 
// this library is especificly designed for W25N01XX (128MB) with SPI interface

//define the SPI Handle type
#define W25_SPI	hspi2

// define CS pin 
#define W25_CS1_PIN		SPI2_CS1_Pin
#define W25_CS1_PORT	SPI2_CS1_GPIO_Port

#define W25_CS2_PIN		SPI2_CS2_Pin
#define W25_CS2_PORT	SPI2_CS2_GPIO_Port

//Instruction list
#define W25NXX_RESET 										0xFF
#define W25NXX_WriteEnable							0x06 
#define W25NXX_WriteDisable							0x04 
#define W25NXX_ReadStatusReg						0x05 
#define W25NXX_WriteStatusReg 				  0x01
#define W25NXX_StatusReg1 							0xA0
#define W25NXX_StatusReg2 							0xB0
#define W25NXX_StatusReg3 							0xC0
#define W25NXX_LoadProgramData					0x32 
#define W25NXX_LoadRandomProgramData		0x84
#define W25NXX_ProgramExec							0x10
#define W25NXX_PageRead									0x13
#define W25NXX_FastRead									0x0B
#define W25NXX_BlockErase								0xD8 
#define W25NXX_JedecDeviceID						0x9F 

#define W25NXX_TIMOUT										500
#define W25NXX_QUAD_TIMEOUT 						3000
	
#define W25N01_PageSize									(2048 ) 
#define W25N01_BlockSize								(64)
#define W25N01_BlockNum									(1024 * 2)
#define	W25N01_EraseSize								(64 * 2048) //128KB	
#define W25N01_BackUpAdd								( 0 )

#define W25NXX_SectorSize									W25N01_PageSize
#define W25NXX_SectorNum									((W25N01_BlockNum * W25N01_BlockSize * W25N01_PageSize)/W25NXX_SectorSize)
#define W25NXX_BlkSecNum									(W25N01_PageSize / W25NXX_SectorSize)
#define W25NXX_EraseSecNum								(W25N01_EraseSize / W25NXX_SectorSize)

//#define W25NXX_CS1_Enable()			HAL_GPIO_WritePin(W25_CS1_PORT, W25_CS1_PIN, GPIO_PIN_RESET)
//#define W25NXX_CS1_Disable()		HAL_GPIO_WritePin(W25_CS1_PORT, W25_CS1_PIN, GPIO_PIN_SET)

void W25NXX_CS_Enable(uint8_t chip); // chip = 1 OR 2
void W25NXX_CS_Disable(uint8_t chip);// chip = 1 OR 2

uint8_t W25NXX_Init(void);					//Initialize W25NXX
void W25NXX_ReadID(uint32_t * id1  , uint32_t * id2);    		//Read FLASH ID
void W25NXX_ReadSR(uint8_t ADD,uint8_t* SR1 , uint8_t* SR2);            //Read status register 
void W25NXX_Write_SR(uint8_t ADD,uint8_t SR);   //Write status register
void W25NXX_Write_Enable(uint8_t chip);  		//Write enable 
void W25NXX_Write_Disable(uint8_t chip);		//Write protection
void W25NXX_Read(uint8_t* pBuffer,uint32_t SectorReadAddr,uint32_t NumByteToRead);   //Read FLASH
uint16_t W25NXX_Read_Page(uint8_t* pBuffer,uint32_t SectorReadAddr,uint32_t NumByteToRead);
void W25NXX_Write(uint8_t* pBuffer,uint32_t SectorWriteAddr,uint32_t NumByteToWrite);//Write flash
uint16_t W25NXX_Write_Page(uint8_t* pBuffer,uint32_t SectorWriteAddr,uint32_t NumByteToWrite);
uint8_t W25NXX_Erase_Block128K(uint32_t block_add);	//Sector erase
void W25NXX_Wait_Busy(uint8_t blk_no);        	//Waiting for free
uint8_t W25NXX_ChipErase(void);		//erase the entire chip
uint8_t W25NXX_get_BUSSY(uint8_t chip);// chip = 1 OR 2
#endif
