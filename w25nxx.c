/*********************************************************************************************************
*
* File                : ws_W25nxx.c
* Hardware Environment: 
* Version             : V1.0
* By                  : Masoud Babaabasi
* Data                : May 2022
*
* this library is especificly designed for W25N01XX (128MB) with QUAD SPI interface
*
*
*********************************************************************************************************/
#include "w25nxx.h"
#include "stm32h7xx_hal_qspi.h"
#include "string.h" 

extern QSPI_HandleTypeDef hqspi;

static void my_delay(uint32_t counter){
	while(counter--);
}
uint8_t W25NXX_Init(){
	uint32_t quad_bk1;
	W25NXX_ReadID(& quad_bk1 );
	if( quad_bk1 != 0xefaa21 ) return 0;
	W25NXX_Write_SR(W25NXX_StatusReg1,0x00);
	W25NXX_Write_SR(W25NXX_StatusReg2 , 0x18 );
	return 1;
}

//write enble instruction is neede befor the erase and write 
// set the WEL bit
void W25NXX_Write_Enable(){
	QSPI_CommandTypeDef cmd = {0};
	cmd.Instruction = W25NXX_WriteEnable;
	cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	HAL_QSPI_Command(&hqspi , &cmd , W25NXX_QUAD_TIMEOUT);
}
//W25NXX write prohibited
//Clear WEL bit
void W25NXX_Write_Disable(void)   
{  
	QSPI_CommandTypeDef cmd = {0};
	cmd.Instruction = W25NXX_WriteDisable;
	cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	HAL_QSPI_Command(&hqspi , &cmd , W25NXX_QUAD_TIMEOUT);
} 
//set rest instruction
//befor initialize
void W25NXX_reset(){
	QSPI_CommandTypeDef cmd = {0};
	cmd.Instruction = W25NXX_RESET;
	cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	HAL_QSPI_Command(&hqspi , &cmd , W25NXX_QUAD_TIMEOUT);
	HAL_Delay(50);
}

//Read the status register of W25NXX, W25NXX has 3 status registers
//Status register 1: add = 0xAx
//BIT7 6 5 4 3 2 1 0
//SRP0 BP3 BP2 BP1 BP0 TB WP_E SRP1
//Status register 2: add = 0xBx
//BIT7 6 5 4 3 2 1 0
//OTP-L OTP-E SR1-L BUF (R) (R) (R)
//Status register 3: add = 0xCx
//BIT7 6 5 4 3 2 1 0
//(R) LUT-F ECC1 ECC0 / P-FAIL E-Fail WEL BUSSY
//Return value: status register value
void W25NXX_ReadSR(uint8_t ADD,uint8_t* SR1)   
{  
	uint8_t byte[2]; 
	QSPI_CommandTypeDef cmd = {0};
	cmd.Instruction = W25NXX_ReadStatusReg;
	cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_1_LINE;
	cmd.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
	cmd.AlternateBytes = ADD ;
	cmd.DataMode = QSPI_DATA_1_LINE;
	cmd.NbData = 1;
	HAL_QSPI_Command(&hqspi , &cmd , W25NXX_QUAD_TIMEOUT);
	HAL_QSPI_Receive(&hqspi , byte , W25NXX_QUAD_TIMEOUT);
	*SR1 = byte[0];
}   
//Write W25NXX status register
void W25NXX_Write_SR(uint8_t ADD,uint8_t sr)   
{ 
	QSPI_CommandTypeDef cmd = {0};
	uint32_t tick_start;
	cmd.Instruction = W25NXX_WriteStatusReg;
	cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_1_LINE;
	cmd.AlternateBytesSize = QSPI_ALTERNATE_BYTES_16_BITS;
	cmd.AlternateBytes = (uint32_t)((ADD << 8) | sr);
	HAL_QSPI_Command(&hqspi , &cmd , W25NXX_QUAD_TIMEOUT);
	//Wait for the end of writing
	tick_start = HAL_GetTick();
	while( W25NXX_get_BUSSY() ){
		if( HAL_GetTick() - tick_start > W25NXX_TIMOUT ){
			break;
		}
	} 
} 
//return 1 if the BUSSY bit is set
uint8_t W25NXX_get_BUSSY(){
	uint8_t quad_bk1_sr;
	W25NXX_ReadSR(W25NXX_StatusReg3,& quad_bk1_sr );
	return (quad_bk1_sr & 0x01);
}
// read manufacture ID of the chip the ID for  W25N01 is 0xEFAA21 
void W25NXX_ReadID(uint32_t * id1 )
{
	uint8_t temp[6];
	QSPI_CommandTypeDef cmd = {0};
	cmd.Instruction = W25NXX_JedecDeviceID;
	cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	cmd.DummyCycles = 8;
	cmd.DataMode = QSPI_DATA_1_LINE;
	cmd.NbData = 3;
	HAL_QSPI_Command(&hqspi , &cmd , W25NXX_QUAD_TIMEOUT);
	HAL_QSPI_Receive(&hqspi , temp , W25NXX_QUAD_TIMEOUT);
	*id1 =(temp[0]<<16) | (temp[1]<<8) | temp[2];
}    
// read one page up to 2048 bytes
//	NumByteToRead <= 2048
uint16_t W25NXX_Read_Page(uint8_t* pBuffer,uint32_t SectroReadAddr,uint32_t NumByteToRead)   
{ 
	uint32_t address = SectroReadAddr * W25NXX_SectorSize; // byte address
	uint32_t PageAdd = address >> 11;
	uint32_t tick_start;
	if( NumByteToRead == 0 ) return 0;
	if( NumByteToRead > W25N01_PageSize) NumByteToRead = W25N01_PageSize;
	if( NumByteToRead > W25N01_PageSize - (address & 0x7ff ) ) NumByteToRead = W25N01_PageSize - (address & 0x7ff);
	
	QSPI_CommandTypeDef cmd = {0};
	cmd.Instruction = W25NXX_PageRead;
	cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
//	cmd.Address =  address >> 11 ; // page address = (address / W25N01_PageSize)
//	cmd.AddressMode = QSPI_ADDRESS_1_LINE;
//	cmd.AddressSize = QSPI_ADDRESS_16_BITS;
	cmd.DummyCycles = 8;
	cmd.DataMode = QSPI_DATA_1_LINE;
	cmd.NbData = 2 ;
	uint8_t buff[2];
	buff[0] = (PageAdd >> 8)& 0xff;
	buff[1] = (PageAdd & 0xff);
	HAL_QSPI_Command(&hqspi , &cmd , W25NXX_QUAD_TIMEOUT);	
	HAL_QSPI_Transmit(&hqspi ,buff , W25NXX_QUAD_TIMEOUT);
	//Wait for the bussy
	tick_start = HAL_GetTick();
	while( W25NXX_get_BUSSY() ){
		if( HAL_GetTick() - tick_start > W25NXX_TIMOUT ){
			return 0;
		}
	}  				 
	cmd.Instruction = W25NXX_FastRead;
	cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	cmd.Address = address &  0x7ff ; // column address = address & ( ( 1 << 11) - 1)
	cmd.AddressMode = QSPI_ADDRESS_1_LINE;
	cmd.AddressSize = QSPI_ADDRESS_16_BITS;
	cmd.DummyCycles = 8;
	cmd.DataMode = QSPI_DATA_4_LINES;
	cmd.NbData = NumByteToRead ;
	HAL_QSPI_Command(&hqspi , &cmd , W25NXX_QUAD_TIMEOUT);
	HAL_QSPI_Receive(&hqspi , pBuffer , W25NXX_QUAD_TIMEOUT);
	//Wait for the end of writing
	tick_start = HAL_GetTick();
	while( W25NXX_get_BUSSY() ){
		if( HAL_GetTick() - tick_start > W25NXX_TIMOUT ){
			return 0;
		}
	} 
	my_delay(2400); // roughly 5us
	return	NumByteToRead; // To check if the number of bytes read is less than expected. if the address is reaches the end of the page
}  
//SPI writes less than 2048 bytes of data in one page (0~65535)
//Write a maximum of (2048) bytes of data at the specified address
//pBuffer: data storage area
//WriteAddr: The address to start writing (maximum 27bit)
uint16_t W25NXX_Write_Page(uint8_t* pBuffer,uint32_t SectorWriteAddr,uint32_t NumByteToWrite)
{
	uint32_t address = SectorWriteAddr * W25NXX_SectorSize; // byte address
	uint32_t PageAdd = address >> 11;
	uint32_t tick_start;
	
	if( NumByteToWrite == 0 ) return 0;
	if( NumByteToWrite > W25N01_PageSize) NumByteToWrite = W25N01_PageSize;
	if( NumByteToWrite > W25N01_PageSize - (address & ( ( 1 << 11) - 1)) ) NumByteToWrite = W25N01_PageSize - (address & ( ( 1 << 11) - 1));
	
	QSPI_CommandTypeDef cmd = {0};
	cmd.Instruction = W25NXX_PageRead;
	cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
//	cmd.Address =  address >> 11 ; // page address = (address / W25N01_PageSize)
//	cmd.AddressMode = QSPI_ADDRESS_1_LINE;
//	cmd.AddressSize = QSPI_ADDRESS_16_BITS;
	cmd.DummyCycles = 8;
	cmd.DataMode = QSPI_DATA_1_LINE;
	cmd.NbData = 2 ;
	uint8_t buff[2];
	buff[0] = (PageAdd >> 8)& 0xff;
	buff[1] = (PageAdd & 0xff);
	HAL_QSPI_Command(&hqspi , &cmd , W25NXX_QUAD_TIMEOUT);	
	HAL_QSPI_Transmit(&hqspi ,buff , W25NXX_QUAD_TIMEOUT);
	//Wait for the bussy
	tick_start = HAL_GetTick();
	while( W25NXX_get_BUSSY() ){
		if( HAL_GetTick() - tick_start > W25NXX_TIMOUT ){
			return 0;
		}
	}   	
	W25NXX_Write_Enable();	
	cmd.Instruction = W25NXX_LoadRandomProgramData;
	cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	cmd.Address = (address & 0x7ff ); // column address
	cmd.AddressMode = QSPI_ADDRESS_1_LINE;
	cmd.AddressSize = QSPI_ADDRESS_16_BITS;
	cmd.DataMode = QSPI_DATA_4_LINES;
	cmd.NbData = NumByteToWrite ;
	cmd.DummyCycles = 0;
	HAL_QSPI_Command(&hqspi , &cmd , W25NXX_QUAD_TIMEOUT);	
	HAL_QSPI_Transmit(&hqspi , pBuffer , W25NXX_QUAD_TIMEOUT);
	 //Wait for the end of writing
	tick_start = HAL_GetTick();
	while( W25NXX_get_BUSSY() ){
		if( HAL_GetTick() - tick_start > W25NXX_TIMOUT ){
			return 0;
		}
	} 				  
	cmd.Instruction = W25NXX_ProgramExec;
	cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
//	cmd.Address = address >> 11;
//	cmd.AddressMode = QSPI_ADDRESS_1_LINE;
//	cmd.AddressSize = QSPI_ADDRESS_16_BITS;
	cmd.Address = 0;
	cmd.AddressMode = QSPI_ADDRESS_NONE;
	cmd.DataMode = QSPI_DATA_1_LINE;
	cmd.NbData = 2;
	cmd.DummyCycles = 8;
	buff[0] = (PageAdd >> 8)& 0xff;
	buff[1] = (PageAdd & 0xff);
	HAL_QSPI_Command(&hqspi , &cmd , W25NXX_QUAD_TIMEOUT);
	HAL_QSPI_Transmit(&hqspi ,buff , W25NXX_QUAD_TIMEOUT);
	//Wait for the end of writing
	tick_start = HAL_GetTick();
	while( W25NXX_get_BUSSY() ){
		if( HAL_GetTick() - tick_start > W25NXX_TIMOUT ){
			return 0;
		}
	} 
	my_delay(4800); // roughly 10us
	return NumByteToWrite;// To check if the number of bytes written is less than expected. If the address is reaches the end of the page
} 
//Write SPI FLASH
//Write data of the specified length at the specified address
//This function has erase operation!
//pBuffer: data storage area
//SectorWriteAddr: The address of the satrt page to write
//NumByteToWrite: The number of bytes to be written 
void W25NXX_Write(uint8_t* pBuffer,uint32_t SectorWriteAddr,uint32_t NumByteToWrite)   
{    
	uint32_t WriteAdd = SectorWriteAddr,ReadAdd;
	uint32_t EraseAdd , NextBlock;
	uint8_t buff_read[W25N01_PageSize];
	uint16_t PageAddMod;
	uint8_t *pDataRead = pBuffer ;
	uint16_t NumPageToWrite = NumByteToWrite / W25N01_PageSize;
	uint8_t NeedErase = 0;
	uint32_t write_backup_address;
	uint32_t start;
	
	while(NumByteToWrite){
		
		EraseAdd = (uint32_t)( WriteAdd / W25N01_BlockSize ) * W25N01_BlockSize;
		NextBlock = EraseAdd + W25N01_BlockSize;
		write_backup_address = W25N01_LAST_BLOCK;
		
		for(uint16_t i = 0 ; i < NumPageToWrite ; i++){
			if( ( WriteAdd  + i ) == NextBlock ) break; // reached end of the block
			W25NXX_Read_Page(buff_read, WriteAdd  + i , W25N01_PageSize);
			for( uint16_t j = 0 ; j < W25N01_PageSize ; j++){
				if( buff_read[ j ] != 0xff ){
					NeedErase = 1;
					break;
				}
			}
			if( NeedErase == 1) break;
		}
		
		if(NeedErase){
			//uint8_t buff_repalce[ W25N01_EraseSize ] ;
			ReadAdd = EraseAdd;
//			for( uint8_t i = 0 ; i < W25N01_BlockSize ; i++){
//				W25NXX_Read_Page(&buff_repalce[ W25N01_PageSize * i ] , ReadAdd , W25N01_PageSize);
//				ReadAdd++;
//			}
			//W25NXX_Erase_Block256K(write_backup_address);
			start = HAL_GetTick();
			while( W25NXX_Erase_Block128K(write_backup_address) != 1 && HAL_GetTick() - start < 1000);
			for( uint8_t i = 0 ; i < W25N01_BlockSize ; i++){
				W25NXX_Read_Page(buff_read , ReadAdd , W25N01_PageSize);
				ReadAdd++;
				W25NXX_Write_Page( buff_read , write_backup_address , W25N01_PageSize);
				write_backup_address++;
			}
			//W25NXX_Erase_Block256K(EraseAdd);
			start = HAL_GetTick();
			while( W25NXX_Erase_Block128K(EraseAdd) != 1 && HAL_GetTick() - start < 1000);
			write_backup_address = W25N01_LAST_BLOCK;

			for(PageAddMod = 0; PageAddMod < W25N01_BlockSize ; PageAddMod++){
				if( PageAddMod < WriteAdd % W25N01_BlockSize || NumPageToWrite == 0){
					W25NXX_Read_Page(buff_read , write_backup_address , W25N01_PageSize);
					W25NXX_Write_Page( buff_read , EraseAdd , W25N01_PageSize);
				}else{
					W25NXX_Write_Page( pDataRead , WriteAdd , W25N01_PageSize);
					pDataRead += W25N01_PageSize;
					NumByteToWrite -= W25N01_PageSize;
					NumPageToWrite-- ;
					WriteAdd++;
					//if( NumPageToWrite == 0 )  break;
				}
				write_backup_address++;
				EraseAdd++;
			}
			
//			for(PageAddMod = WriteAdd % W25N01_BlockSize; PageAddMod < W25N01_BlockSize ; PageAddMod++){
////				memcpy(&buff_repalce[ W25N01_PageSize * PageAddMod ] , pDataRead , W25N01_PageSize);
//				W25NXX_Write_Page( pDataRead , WriteAdd , W25N01_PageSize);
//
//				pDataRead += W25N01_PageSize;
//				NumByteToWrite -= W25N01_PageSize;
//				NumPageToWrite-- ;
//				WriteAdd++;
//				if( NumPageToWrite == 0 )  break;
//			}

//			for( uint8_t i = 0 ; i < W25N01_BlockSize ; i++){
//				W25NXX_Write_Page( &buff_repalce[ W25N01_PageSize * i ] , EraseAdd , W25N01_PageSize);
//				EraseAdd++;
//			}
		}
		else{ // does not need erase
			for(uint16_t i = 0 ; i < W25N01_BlockSize ; i++){
				if( ( WriteAdd  ) == NextBlock ) break; // reached end of the block
				W25NXX_Write_Page( pDataRead , WriteAdd , W25N01_PageSize);
				pDataRead += W25N01_PageSize;
				NumByteToWrite -= W25N01_PageSize;
				NumPageToWrite-- ;
				WriteAdd++;
				if( NumPageToWrite == 0 )  break;
			}
		}
		
		NeedErase = 0;
	}
}
//read an desired amount of bytes
void W25NXX_Read(uint8_t* pBuffer,uint32_t SectorReadAddr,uint32_t NumByteToRead)
{
	uint32_t ReadAdd = SectorReadAddr;
	uint16_t NumPageToRead = NumByteToRead / W25N01_PageSize;
	for(uint16_t i =0 ; i < NumPageToRead ; i++){
		W25NXX_Read_Page( &pBuffer[ W25N01_PageSize * i ] , ReadAdd , W25N01_PageSize);
		ReadAdd ++;
	}	
}

//Erase a one blocks of. 64 * 2048 byte
//block_add: The sector address is set according to the actual capacity
//Minimum time to block a sector: 150ms
uint8_t W25NXX_Erase_Block128K(uint32_t block_add)   
{   
	uint32_t tick_start;
  W25NXX_Write_Enable();	 
	//Wait for end of earase
	tick_start = HAL_GetTick();
	while( W25NXX_get_BUSSY() ){
		if( HAL_GetTick() - tick_start > W25NXX_TIMOUT ){
			return 0;
		}
	}
	QSPI_CommandTypeDef cmd = {0};
	cmd.Instruction = W25NXX_BlockErase;
	cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	cmd.DummyCycles = 8;
	cmd.DataMode = QSPI_DATA_1_LINE;
	cmd.NbData = 2;
//	cmd.Address = block_add;
//	cmd.AddressMode = QSPI_ADDRESS_1_LINE;
//	cmd.AddressSize = QSPI_ADDRESS_16_BITS;
	uint8_t buff[2];
	buff[0] = (block_add >> 8)& 0xff;
	buff[1] = (block_add & 0xff);
	
	HAL_QSPI_Command(&hqspi , &cmd , W25NXX_QUAD_TIMEOUT);
	HAL_QSPI_Transmit(&hqspi ,buff , W25NXX_QUAD_TIMEOUT);
	//Wait for end of earase
	tick_start = HAL_GetTick();
	uint8_t quad_bk1_sr;
	W25NXX_ReadSR(W25NXX_StatusReg3,& quad_bk1_sr );
	while( quad_bk1_sr & 0x01 ){
		W25NXX_ReadSR(W25NXX_StatusReg3,& quad_bk1_sr );
		if( HAL_GetTick() - tick_start > W25NXX_TIMOUT || (quad_bk1_sr & 0x04) ){
			return 0;
		}
	}
	HAL_Delay(1);
	return 1;	
}


uint8_t W25NXX_ChipErase(void){
	for(uint32_t i=0 ; i < W25N01_BlockNum * W25N01_BlockSize ; i += W25N01_BlockSize){
		//if( W25NXX_Erase_Block128K( i ) == 0) return 0;
		W25NXX_Erase_Block128K(i) ;
	}
	return 1;
}
//Waiting for free
void W25NXX_Wait_Busy(uint8_t blk_no)   
{   
	while( W25NXX_get_BUSSY() &( 1 << blk_no) );   // Wait for the BUSY bit to clear
}   







