#include "main.h"
#include "usart.h"
#define ADDR_FLASH_SECTOR_4 ((uint32_t)0x08010000)  //扇区4起始地址, 64 Kbytes
uint8_t readBuf[254];
void readFlash(uint32_t address,uint8_t *readBuf,uint16_t NumToRead);	
void writeFlash(uint32_t addr,uint8_t *data,uint16_t size);
void readFlash(uint32_t address,uint8_t *readBuf,uint16_t size)   	
{
	uint16_t i;
	uint16_t tmpBuf;
	for(i=0;i<size;i+=2)
	{
		tmpBuf=*(__IO uint16_t *)(address+i);
		//低八位
		readBuf[i]=tmpBuf&0xff;
		//高八位
		readBuf[i+1]=tmpBuf>>8;
	}
}

 
void writeFlash(uint32_t address,uint8_t *data,uint16_t size)
{
	FLASH_EraseInitTypeDef FlashSet;
	/* 解锁FLASH*/
	HAL_FLASH_Unlock();
 
	/* 擦除FLASH*/
	/*初始化FLASH_EraseInitTypeDef*/
	/*擦除方式页擦除FLASH_TYPEERASE_PAGES，块擦除FLASH_TYPEERASE_MASSERASE*/
	/*擦除页数*/
	/*擦除地址*/

	FlashSet.TypeErase = FLASH_TYPEERASE_SECTORS;
	FlashSet.Sector  = ADDR_FLASH_SECTOR_4;
	FlashSet.NbSectors  = 1;
	
	/*设置PageError，调用擦除函数*/
	uint32_t PageError = 0;
	HAL_FLASHEx_Erase(&FlashSet, &PageError);
 
	uint16_t tmpbuf;
	/*对FLASH烧写*/
	for(uint16_t i = 0;i< size ;i+=2)
    {
		tmpbuf = (*(data + i + 1) << 8) + (*(data + i));
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD , address + i , tmpbuf);
    }
	//HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, WriteFlashData);
 
	/* 4/4锁住FLASH*/
	HAL_FLASH_Lock();
}
