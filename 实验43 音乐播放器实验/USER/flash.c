#include "main.h"
#include "usart.h"
#define ADDR_FLASH_SECTOR_4 ((uint32_t)0x08010000)  //����4��ʼ��ַ, 64 Kbytes
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
		//�Ͱ�λ
		readBuf[i]=tmpBuf&0xff;
		//�߰�λ
		readBuf[i+1]=tmpBuf>>8;
	}
}

 
void writeFlash(uint32_t address,uint8_t *data,uint16_t size)
{
	FLASH_EraseInitTypeDef FlashSet;
	/* ����FLASH*/
	HAL_FLASH_Unlock();
 
	/* ����FLASH*/
	/*��ʼ��FLASH_EraseInitTypeDef*/
	/*������ʽҳ����FLASH_TYPEERASE_PAGES�������FLASH_TYPEERASE_MASSERASE*/
	/*����ҳ��*/
	/*������ַ*/

	FlashSet.TypeErase = FLASH_TYPEERASE_SECTORS;
	FlashSet.Sector  = ADDR_FLASH_SECTOR_4;
	FlashSet.NbSectors  = 1;
	
	/*����PageError�����ò�������*/
	uint32_t PageError = 0;
	HAL_FLASHEx_Erase(&FlashSet, &PageError);
 
	uint16_t tmpbuf;
	/*��FLASH��д*/
	for(uint16_t i = 0;i< size ;i+=2)
    {
		tmpbuf = (*(data + i + 1) << 8) + (*(data + i));
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD , address + i , tmpbuf);
    }
	//HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, WriteFlashData);
 
	/* 4/4��סFLASH*/
	HAL_FLASH_Lock();
}
