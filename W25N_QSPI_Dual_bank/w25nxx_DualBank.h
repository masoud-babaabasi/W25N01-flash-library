/*********************************************************************************************************
*
* File                : W25nxx_DualBank.h
* Hardware Environment: 
* Version             : V1.0
* By                  : Masoud Babaabasi
* Date				  : May 2022
*this library is especificly designed for W25N01XX (128MB) with Dual bank quad spi interface
*
*********************************************************************************************************/

#ifndef __W25NXX_DUAL_H
#define __W25NXX_DUAL_H
#include "main.h"   

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
#define W25NXX_LoadRandomProgramData		0x34//0x84
#define W25NXX_ProgramExec							0x10
#define W25NXX_PageRead									0x13
#define W25NXX_FastRead									0x6B//0x0B
#define W25NXX_BlockErase								0xD8 
#define W25NXX_JedecDeviceID						0x9F 

#define W25NXX_TIMOUT										500
#define W25NXX_QUAD_TIMEOUT 						700
	
#define W25N01_PageSize									(2 * 2048 ) 
#define W25N01_BlockSize								(64)
#define W25N01_BlockNum									(1024)
#define	W25N01_EraseSize								(64 * 2 * 2048) //256KB	

#define W25NXX_SectorSize									W25N01_PageSize
#define W25NXX_SectorNum									((W25N01_BlockNum * W25N01_BlockSize * W25N01_PageSize)/W25NXX_SectorSize)
#define W25NXX_BlkSecNum									(W25N01_PageSize / W25NXX_SectorSize)
#define W25NXX_EraseSecNum								(W25N01_EraseSize / W25NXX_SectorSize)

uint8_t W25NXX_Init(void);					//Initialize W25NXX
void W25NXX_ReadID(uint32_t * id1 , uint32_t * id2);    		//Read FLASH ID
void W25NXX_ReadSR(uint8_t ADD,uint8_t* SR1 , uint8_t* SR2);            //Read status register 
void W25NXX_Write_SR(uint8_t ADD,uint8_t SR);   //Write status register
void W25NXX_Write_Enable(void);  		//Write enable 
void W25NXX_Write_Disable(void);		//Write protection
void W25NXX_Read(uint8_t* pBuffer,uint32_t SectorReadAddr,uint32_t NumByteToRead);   //Read FLASH
uint16_t W25NXX_Read_Page(uint8_t* pBuffer,uint32_t SectorReadAddr,uint32_t NumByteToRead);
void W25NXX_Write(uint8_t* pBuffer,uint32_t SectorWriteAddr,uint32_t NumByteToWrite);//Write flash
uint16_t W25NXX_Write_Page(uint8_t* pBuffer,uint32_t SectorWriteAddr,uint32_t NumByteToWrite);
uint8_t W25NXX_Erase_Block256K(uint32_t block_add);	//Sector erase
void W25NXX_Wait_Busy(uint8_t blk_no);        	//Waiting for free
uint8_t W25NXX_ChipErase(void);		//erase the entire chip
uint8_t W25NXX_get_BUSSY(void);
#endif
