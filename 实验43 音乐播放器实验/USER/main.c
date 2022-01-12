#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "lcd.h"
#include "usmart.h"
#include "sram.h"
#include "malloc.h" 
#include "sdio_sdcard.h"
#include "w25qxx.h"
#include "ff.h"
#include "exfuns.h"
#include "string.h"
#include "fontupd.h"
#include "text.h"
#include "wm8978.h"	
#include "remote.h"
#include "audioplay.h"


#define ADDR_FLASH_SECTOR_5 ((uint32_t)0x08020000)
//#define ADDR_FLASH_SECTOR_5 ((uint32_t)0x08022800)
//#define ADDR_FLASH_SECTOR_5 ((uint32_t)0x08025000)

void writeFlash(uint32_t addr,uint8_t *data,uint16_t size);
const u8 DATA[]= {
0xa9 ,0x00 ,0xa9 ,0x00 ,0x0b ,0x01 ,0x0b ,0x01 ,0x01 ,0x00 ,0x01 ,0x00 ,0x78 ,0xff ,0x79 ,0xff ,
0xc9 ,0xfe ,0xca ,0xfe ,0x79 ,0xfe ,0x79 ,0xfe ,0x64 ,0xfe ,0x64 ,0xfe ,0xb5 ,0xfe ,0xb5 ,0xfe ,
0x11 ,0xff ,0x11 ,0xff ,0xca ,0xff ,0xca ,0xff ,0x6d ,0x00 ,0x6d ,0x00 ,0x0f ,0x01 ,0x10 ,0x01 ,
0x4f ,0x01 ,0x4e ,0x01 ,0x28 ,0x01 ,0x29 ,0x01 ,0x94 ,0x00 ,0x94 ,0x00 ,0xe9 ,0xff ,0xe9 ,0xff ,
0x0a ,0xff ,0x0b ,0xff ,0x54 ,0xfe ,0x54 ,0xfe ,0x9f ,0xfd ,0x9f ,0xfd ,0x52 ,0xfd ,0x52 ,0xfd ,
0x30 ,0xfd ,0x30 ,0xfd ,0x68 ,0xfd ,0x69 ,0xfd ,0xf5 ,0xfd ,0xf5 ,0xfd ,0x1d ,0xff ,0x1c ,0xff ,
0xc6 ,0xff ,0xc6 ,0xff ,0xf6 ,0x00 ,0xf6 ,0x00 ,0x75 ,0x01 ,0x74 ,0x01 ,0x02 ,0x02 ,0x02 ,0x02 ,
0xf6 ,0x01 ,0xf7 ,0x01 ,0xd9 ,0x01 ,0xd9 ,0x01 ,0x66 ,0x01 ,0x66 ,0x01 ,0x35 ,0x01 ,0x34 ,0x01 ,
0x3c ,0x00 ,0x3b ,0x00 ,0xc0 ,0xff ,0xc2 ,0xff ,0x4f ,0xff ,0x4f ,0xff ,0xd0 ,0xfe ,0xd1 ,0xfe ,
0xa7 ,0xfe ,0xa6 ,0xfe ,0x1c ,0xff ,0x1c ,0xff ,0x5b ,0xff ,0x5d ,0xff ,0x63 ,0x00 ,0x63 ,0x00 ,
0x09 ,0x01 ,0x09 ,0x01 ,0x90 ,0x01 ,0x90 ,0x01 ,0x85 ,0x01 ,0x85 ,0x01 ,0x4b ,0x01 ,0x4b ,0x01 ,
0x54 ,0x00 ,0x54 ,0x00 ,0x3c ,0xff ,0x3c ,0xff ,0x3c ,0xfe ,0x3c ,0xfe ,0x38 ,0xfe ,0x37 ,0xfe ,
0x0d ,0xff ,0x0d ,0xff ,0x4a ,0x01 ,0x49 ,0x01 ,0x3e ,0x04 ,0x3e ,0x04 ,0x95 ,0x07 ,0x94 ,0x07 ,
0x2a ,0x0a ,0x2a ,0x0a ,0xc8 ,0x0a ,0xc9 ,0x0a ,0xf5 ,0x08 ,0xf6 ,0x08 ,0x34 ,0x05 ,0x34 ,0x05 ,
0x36 ,0xff ,0x37 ,0xff ,0x98 ,0xf8 ,0x98 ,0xf8 ,0x02 ,0xf3 ,0x01 ,0xf3 ,0xd5 ,0xee ,0xd4 ,0xee ,};

/************************************************
 ALIENTEK 探索者STM32F407开发板 实验43
 音乐播放器实验-HAL库函数版
 技术支持：www.openedv.com
 淘宝店铺：http://eboard.taobao.com 
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司  
 作者：正点原子 @ALIENTEK
************************************************/

int main(void)
{

    HAL_Init();                   	//初始化HAL库    
    Stm32_Clock_Init(336,8,2,7);  	//设置时钟,168Mhz
	delay_init(168);               	//初始化延时函数
	uart_init(115200);             	//初始化USART
	usmart_dev.init(84); 		    //初始化USMART
	LED_Init();						//初始化LED	
	Remote_Init();					//初始化红外
	KEY_Init();						//初始化KEY
	LCD_Init();                     //初始化LCD
	SRAM_Init();					//初始化外部SRAM  
	W25QXX_Init();				    //初始化W25Q128
	WM8978_Init();					//初始化WM8978
	WM8978_HPvol_Set(63,63);		//耳机音量设置
	WM8978_SPKvol_Set(50);			//喇叭音量设置
	
	my_mem_init(SRAMIN);			//初始化内部内存池
	my_mem_init(SRAMEX);			//初始化外部内存池
	my_mem_init(SRAMCCM);			//初始化CCM内存池 
	exfuns_init();					//为fatfs相关变量申请内存  
 	f_mount(fs[0],"0:",1); 			//挂载SD卡   
  

	/*每次写入大概10kb的flash,分多次写入,每次写入都需要重新修改flash地址
	写完之后无需再调用写入函数
	下一次地址起始=上一次结束地址=上一次地址起始+写入字节大小*/
	printf("写flash\r\n");
	writeFlash(ADDR_FLASH_SECTOR_5,(u8 *)DATA,sizeof(DATA));
	

	while(1)
	{ 	
		audio_play();
	} 
}


//void readFlash(uint32_t address,uint8_t *readBuf,uint16_t size)   	
//{
//	uint16_t i;
//	uint16_t tmpBuf;
//	printf("开始读flash\r\n");
//	for(i=0;i<size;i+=2)
//	{
//		tmpBuf=*(__IO uint16_t *)(address+i);
//		//低八位
//		readBuf[i]=tmpBuf&0xff;
//		//高八位
//		readBuf[i+1]=tmpBuf>>8;
//	}
//}

/**
写flash
address 起始地址
data 数据
size 数据大小

**/
void writeFlash(u32 address,u8 *data,u16 size)
{
	FLASH_EraseInitTypeDef FlashSet;
	HAL_StatusTypeDef FlashStatus = HAL_OK;
	u16 tmpbuf;
	u16 i;
	u32 PageError = 0;
	
	//解锁FLASH
	HAL_FLASH_Unlock();
	//擦除FLASH

	
	//第一次写入之前调用擦除函数进行擦除,后续无需再调用,否则会吧之前写入的数据擦除掉
	//除非需要覆盖之前的flash 

	//初始化FLASH_EraseInitTypeDef
	//擦除方式
	FlashSet.TypeErase = FLASH_TYPEERASE_SECTORS;
	//擦除起始页
	FlashSet.Sector  = 5;
	//擦除结束页
	FlashSet.NbSectors  = 6;
	FlashSet.VoltageRange = FLASH_VOLTAGE_RANGE_3; 
	
	printf("擦除\r\n");
	//调用擦除函数
	HAL_FLASHEx_Erase(&FlashSet, &PageError);
	FlashStatus = FLASH_WaitForLastOperation(1000); //等待上次操作完成

	//对FLASH烧写
	printf("开始写flash\r\n");
	for( i= 0;i< size ;i+=2)
	{
		//把8位转16位,得到半个字节
		//每两个8位,一个当低八位,一个当高八位
		tmpbuf = (*(data + i + 1) << 8) + (*(data + i));
		//写半个字节
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD , address + i , tmpbuf);
	}

 
	//锁住FLASH
	HAL_FLASH_Lock();
}
