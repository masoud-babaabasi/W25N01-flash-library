/*********************************************************************************************************
*
* File                : ws_W25nxx_SPI_Dual.c
* Hardware Environment: 
* Build Environment   : RealView MDK-ARM  Version: 5.32
* Version             : V1.0
* By                  : M.Babaabsi 2022June
*
*
*********************************************************************************************************/
#include "w25nxx_SPI_Dual.h"
#include "string.h" 

extern SPI_HandleTypeDef W25_SPI;
static void my_delay(uint32_t counter){
	while(counter--);
}
inline void W25NXX_CS_Enable(uint8_t chip){
	switch(chip){
		case 1:			
			HAL_GPIO_WritePin(W25_CS1_PORT, W25_CS1_PIN, GPIO_PIN_RESET);
		break;
		
		case 2:			
			HAL_GPIO_WritePin(W25_CS2_PORT, W25_CS2_PIN, GPIO_PIN_RESET);
		break;
		default:
		break;
	}
}

inline void W25NXX_CS_Disable(uint8_t chip){
	switch(chip){
		case 1:			
			HAL_GPIO_WritePin(W25_CS1_PORT, W25_CS1_PIN, GPIO_PIN_SET);
		break;
		
		case 2:			
			HAL_GPIO_WritePin(W25_CS2_PORT, W25_CS2_PIN, GPIO_PIN_SET);
		break;
		default:
		break;
	}
}


uint8_t W25NXX_Init(){
	uint32_t quad_bk1 , quad_bk2;
	W25NXX_Write_SR(W25NXX_StatusReg1,0x00);
	W25NXX_Write_SR(W25NXX_StatusReg2 , 0x18 );
	W25NXX_ReadID(&quad_bk1 , &quad_bk2);
	if( quad_bk1 != 0xefaa21 || quad_bk2 != 0xefaa21 ) return 0;
	return 1;
}

//write enble instruction is neede befor the erase and write 
// set the WEL bit
void W25NXX_Write_Enable(uint8_t chip){
	uint8_t cmd[1] = {W25NXX_WriteEnable};
	W25NXX_CS_Enable(chip);
	HAL_SPI_Transmit(&W25_SPI, cmd, 1, W25NXX_TIMOUT);
	W25NXX_CS_Disable(chip);
}
//W25NXX write prohibited
//Clear WEL bit
void W25NXX_Write_Disable(uint8_t chip)   
{  	
	uint8_t cmd[1] = {W25NXX_WriteDisable};
	W25NXX_CS_Enable(chip);
	HAL_SPI_Transmit(&W25_SPI, cmd, 1, W25NXX_TIMOUT);
	W25NXX_CS_Disable(chip);
} 
//set rest instruction
//befor initialize
void W25NXX_reset(){
	uint8_t cmd[1] = {W25NXX_RESET};
	W25NXX_CS_Enable(1);
	HAL_SPI_Transmit(&W25_SPI, cmd, 1, W25NXX_TIMOUT);
	W25NXX_CS_Disable(1);
	W25NXX_CS_Enable(2);
	HAL_SPI_Transmit(&W25_SPI, cmd, 1, W25NXX_TIMOUT);
	W25NXX_CS_Disable(2);
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
void W25NXX_ReadSR(uint8_t ADD,uint8_t* SR1 , uint8_t* SR2)   
{  
	uint8_t cmd[2] = {W25NXX_ReadStatusReg , ADD};
	uint8_t temp[2];
	W25NXX_CS_Enable(1);
	HAL_SPI_Transmit(&W25_SPI, cmd, 2, W25NXX_TIMOUT);
	HAL_SPI_Receive(&W25_SPI,temp, 1, W25NXX_TIMOUT);
	W25NXX_CS_Disable(1);
	*SR1 = temp[0];
	
	W25NXX_CS_Enable(2);
	HAL_SPI_Transmit(&W25_SPI, cmd, 2, W25NXX_TIMOUT);
	HAL_SPI_Receive(&W25_SPI,temp, 1, W25NXX_TIMOUT);
	W25NXX_CS_Disable(2);
	*SR2 = temp[0];
}   
//Write W25NXX status register
void W25NXX_Write_SR(uint8_t ADD,uint8_t SR)
{ 
	uint32_t tick_start;
	uint8_t cmd[3] = {W25NXX_WriteStatusReg , ADD , SR};
	W25NXX_CS_Enable(1);
	HAL_SPI_Transmit(&W25_SPI, cmd, 3, W25NXX_TIMOUT);
	W25NXX_CS_Disable(1);
	//Wait for the end of writing
	tick_start = HAL_GetTick();
	while( W25NXX_get_BUSSY(1) ){
		if( HAL_GetTick() - tick_start > W25NXX_TIMOUT ){
			break;
		}
	} 
	
	W25NXX_CS_Enable(2);
	HAL_SPI_Transmit(&W25_SPI, cmd, 3, W25NXX_TIMOUT);
	W25NXX_CS_Disable(2);
	//Wait for the end of writing
	tick_start = HAL_GetTick();
	while( W25NXX_get_BUSSY(2) ){
		if( HAL_GetTick() - tick_start > W25NXX_TIMOUT ){
			break;
		}
	} 
} 
//return 1 if the BUSSY bit is set
uint8_t W25NXX_get_BUSSY(uint8_t chip){
	uint8_t quad_bk1_sr , quad_bk2_sr;
	W25NXX_ReadSR(W25NXX_StatusReg3,& quad_bk1_sr , & quad_bk2_sr);
	if( chip == 1) return (quad_bk1_sr & 0x01) ;
	if( chip == 2) return (quad_bk2_sr & 0x01) ;
	return 0;
}
// read manufacture ID of the chip the ID for  W25N01 is 0xEFAA21 
void W25NXX_ReadID(uint32_t * id1 , uint32_t * id2)
{
	uint8_t temp[3];	
	uint8_t cmd[2] = {W25NXX_JedecDeviceID , 0x00};
	
	W25NXX_CS_Enable(1);
	HAL_SPI_Transmit(&W25_SPI, cmd, 2, W25NXX_TIMOUT);
	HAL_SPI_Receive(&W25_SPI,temp, 3, W25NXX_TIMOUT);
	W25NXX_CS_Disable(1);
	*id1 =(temp[0]<<16) | (temp[1]<<8) | temp[2];
	
	W25NXX_CS_Enable(2);
	HAL_SPI_Transmit(&W25_SPI, cmd, 2, W25NXX_TIMOUT);
	HAL_SPI_Receive(&W25_SPI,temp, 3, W25NXX_TIMOUT);
	W25NXX_CS_Disable(2);
	*id2 =(temp[0]<<16) | (temp[1]<<8) | temp[2];
}    
// read one page up to 2048 bytes
//	NumByteToRead <= 2048
uint16_t W25NXX_Read_Page(uint8_t* pBuffer,uint32_t SectroReadAddr,uint32_t NumByteToRead)   
{ 
	uint32_t address = SectroReadAddr * W25NXX_SectorSize; // byte address
	uint32_t PageAdd = address >> 11;
	uint8_t chip_select = 1;
	uint32_t MAXpageOfEach = W25N01_BlockNum * W25N01_BlockSize / 2;
	if( PageAdd >= MAXpageOfEach ) chip_select = 2;
	PageAdd = PageAdd % MAXpageOfEach;
	uint32_t tick_start;
	if( NumByteToRead == 0 ) return 0;
	if( NumByteToRead > W25N01_PageSize) NumByteToRead = W25N01_PageSize;
	if( NumByteToRead > W25N01_PageSize - (address & 0x7ff ) ) NumByteToRead = W25N01_PageSize - (address & 0x7ff);
	
	uint8_t cmd[4] = {W25NXX_PageRead , 0x00 , (PageAdd >> 8)& 0xff , (PageAdd & 0xff)};
	W25NXX_CS_Enable(chip_select);
	HAL_SPI_Transmit(&W25_SPI, cmd, 4, W25NXX_TIMOUT);
	W25NXX_CS_Disable(chip_select);
	//Wait for the bussy
	tick_start = HAL_GetTick();
	while( W25NXX_get_BUSSY(chip_select) ){
		if( HAL_GetTick() - tick_start > W25NXX_TIMOUT ){
			return 0;
		}
	} 

	//read buffer
	cmd[0] = W25NXX_FastRead;
	cmd[1] = (( address &  0x7ff) >> 8) & 0xff ;
	cmd[2] = ( address &  0x7ff) & 0xff;
	cmd[3] = 0x00; //dummy cycle
	W25NXX_CS_Enable(chip_select);
	HAL_SPI_Transmit(&W25_SPI, cmd, 4, W25NXX_TIMOUT);
	HAL_SPI_Receive(&W25_SPI,pBuffer, NumByteToRead, W25NXX_TIMOUT);
	//HAL_SPI_Receive_DMA(&W25_SPI,pBuffer, NumByteToRead);
	W25NXX_CS_Disable(chip_select);
	//Wait for the end of writing
	tick_start = HAL_GetTick();
	while( W25NXX_get_BUSSY(chip_select) ){
		if( HAL_GetTick() - tick_start > W25NXX_TIMOUT ){
			return 0;
		}
	} 
	my_delay(840);
	return	NumByteToRead; // To check if the number of bytes read is less than expected. if the address is reaches the end of the page
}  
//SPI writes less than 4096 bytes of data in one page (0~65535)
//Write a maximum of (4096) bytes of data at the specified address
//pBuffer: data storage area
//WriteAddr: The address to start writing (maximum 27bit)
uint16_t W25NXX_Write_Page(uint8_t* pBuffer,uint32_t SectorWriteAddr,uint32_t NumByteToWrite)
{
	uint32_t address = SectorWriteAddr * W25NXX_SectorSize; // byte address
	uint32_t PageAdd = address >> 11;
	uint8_t chip_select = 1;
	uint32_t MAXpageOfEach = W25N01_BlockNum * W25N01_BlockSize / 2;
	if( PageAdd >= MAXpageOfEach ) chip_select = 2;
	PageAdd = PageAdd % MAXpageOfEach;
	uint32_t tick_start;
	
	if( NumByteToWrite == 0 ) return 0;
	if( NumByteToWrite > W25N01_PageSize) NumByteToWrite = W25N01_PageSize;
	if( NumByteToWrite > W25N01_PageSize - (address & 0x7ff ) ) NumByteToWrite = W25N01_PageSize - (address & 0x7ff );
	
	W25NXX_Write_Enable(chip_select);	
	uint8_t cmd[4] = {W25NXX_LoadRandomProgramData , (( address & 0x7ff ) >> 8)& 0xff , (( address & 0x7ff ) & 0xff)};
	W25NXX_CS_Enable(chip_select);
	HAL_SPI_Transmit(&W25_SPI, cmd, 3, W25NXX_TIMOUT);
	HAL_SPI_Transmit(&W25_SPI, pBuffer, NumByteToWrite, W25NXX_TIMOUT);
	//HAL_SPI_Transmit_DMA(&W25_SPI, pBuffer, NumByteToWrite);
	W25NXX_CS_Disable(chip_select);
	 //Wait for the end of writing
	tick_start = HAL_GetTick();
	while( W25NXX_get_BUSSY(chip_select) ){
		if( HAL_GetTick() - tick_start > W25NXX_TIMOUT ){
			return 0;
		}
	} 				  
	cmd[0] = W25NXX_ProgramExec;
	cmd[1] = 0x00 ;//dummy cycle
	cmd[2] = (PageAdd >> 8)& 0xff;
	cmd[3] = (PageAdd & 0xff); 
	W25NXX_CS_Enable(chip_select);
	HAL_SPI_Transmit(&W25_SPI, cmd, 4, W25NXX_TIMOUT);
	W25NXX_CS_Disable(chip_select);
	//Wait for the end of writing
	tick_start = HAL_GetTick();
	while( W25NXX_get_BUSSY(chip_select) ){
		if( HAL_GetTick() - tick_start > W25NXX_TIMOUT ){
			return 0;
		}
	} 
	my_delay(1680);
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
	uint8_t PageAddMod;
	uint8_t *pDataRead = pBuffer ;
	uint8_t NumPageToWrite = NumByteToWrite / W25N01_PageSize;
	uint8_t NeedErase = 0;
	
	while(NumByteToWrite){
		
		EraseAdd = (uint32_t)( WriteAdd / W25N01_BlockSize ) * W25N01_BlockSize;
		NextBlock = EraseAdd + W25N01_BlockSize;
		
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
			uint8_t buff_repalce[ W25N01_PageSize ] ;
			ReadAdd = EraseAdd;
			W25NXX_Erase_Block128K(W25N01_BackUpAdd);
			for( uint8_t i = 0 ; i < W25N01_BlockSize ; i++){
				W25NXX_Read_Page(  buff_repalce , ReadAdd , W25N01_PageSize);
				W25NXX_Write_Page( buff_repalce , W25N01_BackUpAdd + i , W25N01_PageSize);
				ReadAdd++;
			}
			W25NXX_Erase_Block128K(EraseAdd);
			/*for(PageAddMod = WriteAdd % W25N01_BlockSize; PageAddMod < W25N01_BlockSize ; PageAddMod++){
				memcpy(&buff_repalce[ W25N01_PageSize * PageAddMod ] , pDataRead , W25N01_PageSize);
				pDataRead += W25N01_PageSize;
				NumByteToWrite -= W25N01_PageSize;
				NumPageToWrite-- ;
				WriteAdd++;
				if( NumPageToWrite == 0 )  break;
			}*/
			for( uint8_t i = 0 ; i < W25N01_BlockSize ; i++){
				PageAddMod = WriteAdd % W25N01_BlockSize;
				if( i != PageAddMod || NumPageToWrite == 0 ){
					W25NXX_Read_Page(  buff_repalce , W25N01_BackUpAdd + i , W25N01_PageSize);
					W25NXX_Write_Page( buff_repalce, EraseAdd , W25N01_PageSize);
					EraseAdd++;
				}
				else{
					W25NXX_Write_Page( pDataRead, WriteAdd , W25N01_PageSize);
					pDataRead += W25N01_PageSize;
					NumByteToWrite -= W25N01_PageSize;
					NumPageToWrite-- ;
					WriteAdd++;
					EraseAdd++;
					//if( NumPageToWrite == 0 )  break;
				}
			}
			

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

//Erase a one blocks of. 64 * 2 * 2048 byte
//block_add: The sector address is set according to the actual capacity
//Minimum time to block a sector: 150ms
uint8_t W25NXX_Erase_Block128K(uint32_t block_add)   
{ 
	uint8_t chip_select = 1;
	uint32_t MAXpageOfEach = W25N01_BlockNum * W25N01_BlockSize / 2;
	if( block_add >= MAXpageOfEach) chip_select = 2;
	block_add = block_add % MAXpageOfEach;  
	uint32_t tick_start;
  W25NXX_Write_Enable(chip_select);	 
	//Wait for end of earase
	tick_start = HAL_GetTick();
	while( W25NXX_get_BUSSY(chip_select) ){
		if( HAL_GetTick() - tick_start > W25NXX_TIMOUT ){
			return 0;
		}
	}
	uint8_t cmd[4] = {W25NXX_BlockErase , 0x00 , (block_add >> 8)& 0xff , (block_add & 0xff) };
	W25NXX_CS_Enable(chip_select);
	HAL_SPI_Transmit(&W25_SPI, cmd, 4, W25NXX_TIMOUT);
	W25NXX_CS_Disable(chip_select);
	//Wait for end of earase
	tick_start = HAL_GetTick();
	uint8_t quad_bk1_sr , quad_bk2_sr;
	W25NXX_ReadSR(W25NXX_StatusReg3,& quad_bk1_sr , &quad_bk2_sr);
	while( quad_bk1_sr & 0x01  || quad_bk2_sr & 0x01){
		W25NXX_ReadSR(W25NXX_StatusReg3,& quad_bk1_sr ,& quad_bk2_sr );
		if( HAL_GetTick() - tick_start > W25NXX_TIMOUT || (quad_bk1_sr & 0x04) || (quad_bk2_sr & 0x04)){
			return 0;
		}
	}
	HAL_Delay(1);
	return 1;	
}


uint8_t W25NXX_ChipErase(void){
	for(uint32_t i=0 ; i < W25N01_BlockNum * W25N01_BlockSize ; i += W25N01_BlockSize){
		if( W25NXX_Erase_Block128K( i ) == 0) return 0;
	}
	return 1;
} 







