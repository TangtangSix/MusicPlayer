#include "wavplay.h" 
#include "audioplay.h"
#include "usart.h" 
#include "delay.h" 
#include "malloc.h"
#include "ff.h"
#include "i2s.h"
#include "wm8978.h"
#include "key.h"
#include "led.h"
#include "remote.h"
#include "main.h"

//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32F407������
//WAV �������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2014/6/29
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2014-2024
//All rights reserved				
//********************************************************************************
//V1.0 ˵��
//1,֧��16λ/24λWAV�ļ�����
//2,��߿���֧�ֵ�192K/24bit��WAV��ʽ. 
////////////////////////////////////////////////////////////////////////////////// 	
 
__wavctrl wavctrl;		//WAV���ƽṹ��
vu8 wavtransferend=0;	//i2s������ɱ�־
vu8 wavwitchbuf=0;		//i2sbufxָʾ��־
u32 data_size=28040; 
//WAV������ʼ��
//fname:�ļ�·��+�ļ���
//wavx:wav ��Ϣ��Žṹ��ָ��
//����ֵ:0,�ɹ�;1,���ļ�ʧ��;2,��WAV�ļ�;3,DATA����δ�ҵ�.
u8 wav_decode_init(u8* fname,__wavctrl* wavx)
{
	FIL*ftemp;
	u8 *buf; 
	u32 br=0;
	u8 res=0;
	
	ChunkRIFF *riff;
	ChunkFMT *fmt;
	ChunkFACT *fact;
	ChunkDATA *data;
	ftemp=(FIL*)mymalloc(SRAMIN,sizeof(FIL));
	buf=mymalloc(SRAMIN,512);
	if(ftemp&&buf)	//�ڴ�����ɹ�
	{
		res=f_open(ftemp,(TCHAR*)fname,FA_READ);//���ļ�
		if(res==FR_OK)
		{
			f_read(ftemp,buf,512,&br);	//��ȡ512�ֽ�������
			riff=(ChunkRIFF *)buf;		//��ȡRIFF��
			if(riff->Format==0X45564157)//��WAV�ļ�
			{
				fmt=(ChunkFMT *)(buf+12);	//��ȡFMT�� 
				fact=(ChunkFACT *)(buf+12+8+fmt->ChunkSize);//��ȡFACT��
				if(fact->ChunkID==0X74636166||fact->ChunkID==0X5453494C)wavx->datastart=12+8+fmt->ChunkSize+8+fact->ChunkSize;//����fact/LIST���ʱ��(δ����)
				else wavx->datastart=12+8+fmt->ChunkSize;  
				data=(ChunkDATA *)(buf+wavx->datastart);	//��ȡDATA��
				if(data->ChunkID==0X61746164)//�����ɹ�!
				{
					wavx->audioformat=fmt->AudioFormat;		//��Ƶ��ʽ
					wavx->nchannels=fmt->NumOfChannels;		//ͨ����
					wavx->samplerate=fmt->SampleRate;		//������
					wavx->bitrate=fmt->ByteRate*8;			//�õ�λ��
					wavx->blockalign=fmt->BlockAlign;		//�����
					wavx->bps=fmt->BitsPerSample;			//λ��,16/24/32λ
					
					wavx->datasize=data->ChunkSize;			//���ݿ��С
					wavx->datastart=wavx->datastart+8;		//��������ʼ�ĵط�. 
					 
					printf("wavx->audioformat:%d\r\n",wavx->audioformat);
					printf("wavx->nchannels:%d\r\n",wavx->nchannels);
					printf("wavx->samplerate:%d\r\n",wavx->samplerate);
					printf("wavx->bitrate:%d\r\n",wavx->bitrate);
					printf("wavx->blockalign:%d\r\n",wavx->blockalign);
					printf("wavx->bps:%d\r\n",wavx->bps);
					printf("wavx->datasize:%d\r\n",wavx->datasize);
					printf("wavx->datastart:%d\r\n",wavx->datastart);  
				}else res=3;//data����δ�ҵ�.
			}else res=2;//��wav�ļ�
			
		}else res=1;//���ļ�����
	}
	f_close(ftemp);
	myfree(SRAMIN,ftemp);//�ͷ��ڴ�
	myfree(SRAMIN,buf); 
	return 0;
}

//���buf
//buf:������
//size:���������
//bits:λ��(16/24)
//����ֵ:���������ݸ���
u32 wav_buffill(u8 *buf,u16 size,u8 bits)
{
	u16 readlen=0;
	u32 bread;
	u16 i;
	u8 *p;
	if(bits==24)//24bit��Ƶ,��Ҫ����һ��
	{
		readlen=(size/4)*3;							//�˴�Ҫ��ȡ���ֽ���
		f_read(audiodev.file,audiodev.tbuf,readlen,(UINT*)&bread);	//��ȡ����
		p=audiodev.tbuf;
		for(i=0;i<size;)
		{
			buf[i++]=p[1];
			buf[i]=p[2]; 
			i+=2;
			buf[i++]=p[0];
			printf("%d",buf[i]);
			p+=3;
		} 
		bread=(bread*4)/3;		//����Ĵ�С.
	}else 
	{
		f_read(audiodev.file,buf,size,(UINT*)&bread);//16bit��Ƶ,ֱ�Ӷ�ȡ����  
		
		if(bread<size)//����������,����0
		{
			for(i=bread;i<size-bread;i++)buf[i]=0; 
		}
	}
	return bread;
}  
//WAV����ʱ,I2S DMA����ص�����
void wav_i2s_dma_tx_callback(void) 
{   
	u16 i;
	if(DMA1_Stream4->CR&(1<<19))
	{
		wavwitchbuf=0;
		if((audiodev.status&0X01)==0)
		{
			for(i=0;i<WAV_I2S_TX_DMA_BUFSIZE;i++)//��ͣ
			{
				audiodev.i2sbuf1[i]=0;//���0
			}
		}
	}else 
	{
		wavwitchbuf=1;
		if((audiodev.status&0X01)==0)
		{
			for(i=0;i<WAV_I2S_TX_DMA_BUFSIZE;i++)//��ͣ
			{
				audiodev.i2sbuf2[i]=0;//���0
			}
		}
	}
	wavtransferend=1;
} 
//�õ���ǰ����ʱ��
//fx:�ļ�ָ��
//wavx:wav���ſ�����
void wav_get_curtime(FIL*fx,__wavctrl *wavx)
{
	long long fpos;  	
 	wavx->totsec=wavx->datasize/(wavx->bitrate/8);	//�����ܳ���(��λ:��) 
	fpos=fx->fptr-wavx->datastart; 					//�õ���ǰ�ļ����ŵ��ĵط� 
	wavx->cursec=fpos*wavx->totsec/wavx->datasize;	//��ǰ���ŵ��ڶ�������?	
}
//����ĳ��WAV�ļ�
//fname:wav�ļ�·��.
//����ֵ:
//KEY0_PRES:��һ��
//KEY2_PRES:��һ��
//����:����
u8 wav_play_song(u8* fname)
{
	u8 key;
	u8 key2;
	u8 t=0; 
	u8 res;  
	u32 fillnum; 
	audiodev.file=(FIL*)mymalloc(SRAMIN,sizeof(FIL));
	audiodev.i2sbuf1=mymalloc(SRAMIN,WAV_I2S_TX_DMA_BUFSIZE);
	audiodev.i2sbuf2=mymalloc(SRAMIN,WAV_I2S_TX_DMA_BUFSIZE);
	audiodev.tbuf=mymalloc(SRAMIN,WAV_I2S_TX_DMA_BUFSIZE);
	if(audiodev.file&&audiodev.i2sbuf1&&audiodev.i2sbuf2&&audiodev.tbuf)
	{ 
		res=wav_decode_init(fname,&wavctrl);//�õ��ļ�����Ϣ
		if(res==0)//�����ļ��ɹ�
		{
//			printf("%s",fname);
			if(wavctrl.bps==16)
			{
				WM8978_I2S_Cfg(2,0);	//�����ֱ�׼,16λ���ݳ���
				I2S2_Init(I2S_STANDARD_PHILIPS,I2S_MODE_MASTER_TX,I2S_CPOL_LOW,I2S_DATAFORMAT_16B_EXTENDED);	//�����ֱ�׼,��������,ʱ�ӵ͵�ƽ��Ч,16λ��չ֡����
			}else if(wavctrl.bps==24)
			{
				WM8978_I2S_Cfg(2,2);	//�����ֱ�׼,24λ���ݳ���
				I2S2_Init(I2S_STANDARD_PHILIPS,I2S_MODE_MASTER_TX,I2S_CPOL_LOW,I2S_DATAFORMAT_24B);	//�����ֱ�׼,��������,ʱ�ӵ͵�ƽ��Ч,24λ����
			}
			I2S2_SampleRate_Set(wavctrl.samplerate);//���ò�����
			I2S2_TX_DMA_Init(audiodev.i2sbuf1,audiodev.i2sbuf2,WAV_I2S_TX_DMA_BUFSIZE/2); //����TX DMA
			i2s_tx_callback=wav_i2s_dma_tx_callback;			//�ص�����ָwav_i2s_dma_callback
			
			audio_stop();
			
			res=f_open(audiodev.file,(TCHAR*)fname,FA_READ);	//���ļ�
			if(res==0)
			{
				
				f_lseek(audiodev.file, wavctrl.datastart);		//�����ļ�ͷ
				fillnum=wav_buffill(audiodev.i2sbuf1,WAV_I2S_TX_DMA_BUFSIZE,wavctrl.bps);
				fillnum=wav_buffill(audiodev.i2sbuf2,WAV_I2S_TX_DMA_BUFSIZE,wavctrl.bps);
				
				
				audio_start();  
				while(res==0)
				{ 
					while(wavtransferend==0);//�ȴ�wav�������; 
					wavtransferend=0;
					if(fillnum!=WAV_I2S_TX_DMA_BUFSIZE)//���Ž���?
					{
						res=KEY0_PRES;
						break;
					} 
 					if(wavwitchbuf)
						fillnum=wav_buffill(audiodev.i2sbuf2,WAV_I2S_TX_DMA_BUFSIZE,wavctrl.bps);//���buf2
					else 
						fillnum=wav_buffill(audiodev.i2sbuf1,WAV_I2S_TX_DMA_BUFSIZE,wavctrl.bps);//���buf1
					while(1)
					{	
						//ɨ�����ң��
						key2=Remote_Scan();
						//ɨ�谴��
						key=KEY_Scan(0); 
						if(key==WKUP_PRES || key2==56)//��ͣ
						{
							printf("��ͣ\r\n");
							
							if(audiodev.status&0X01)audiodev.status&=~(1<<0);
							else audiodev.status|=0X01;  
						}
						if(key==KEY2_PRES||key2==24 )//��һ��
						{
//							printf("�л�����\r\n");
//							key=key2;
							
								printf("�л���һ�׸���\r\n");
								res=KEY2_PRES; 
							
							
							break; 
						}
						if(key==KEY0_PRES||key2==74 )//��һ��
						{
//							printf("�л�����\r\n");
//							key=key2;
							
								printf("�л���һ�׸���\r\n");
								res=KEY0_PRES; 
							
							
							break; 
						}
						wav_get_curtime(audiodev.file,&wavctrl);//�õ���ʱ��͵�ǰ���ŵ�ʱ�� 
						//audio_msg_show(wavctrl.totsec,wavctrl.cursec,wavctrl.bitrate);
						t++;
						if(t==20)
						{
							t=0;
 							LED0=!LED0;
						}
						if((audiodev.status&0X01)==0)delay_ms(10);
						else break;
					}
				}
				audio_stop(); 
			}else res=0XFF; 
		}
		
		else {res=0XFF;}
	}else res=0XFF; 
	myfree(SRAMIN,audiodev.tbuf);	//�ͷ��ڴ�
	myfree(SRAMIN,audiodev.i2sbuf1);//�ͷ��ڴ�
	myfree(SRAMIN,audiodev.i2sbuf2);//�ͷ��ڴ� 
	myfree(SRAMIN,audiodev.file);	//�ͷ��ڴ� 
	return res;
} 
	

/**
��ȡflash
address ��ȡ��ʼ��ַ
readBuf ��ȡ���ݴ��λ��
size ��ȡ�Ĵ�С
**/
void readFlash(uint32_t address,uint8_t *readBuf,uint16_t size)   	
{
	uint16_t i;
	uint16_t tmpBuf;
	printf("��ʼ��flash\r\n");
	for(i=0;i<size;i+=2)
	{
		tmpBuf=*(__IO uint16_t *)(address+i);
		//�Ͱ�λ
		readBuf[i]=tmpBuf&0xff;
		//�߰�λ
		readBuf[i+1]=tmpBuf>>8;
	}
	printf("��flash���\r\n");
}


/**
���Ż�ӭ������ʾ
��flash��ȡ��Ƶ�ļ�

**/
void hello(){
	u8 res;
	//ÿ�ζ�ȡflash��С
	u32 eachSize=8192;
	//��Ƶ�ļ���С
	u32 fillnum=data_size;
	//flash��ʼ��ַ
	u32 address=(u32)0x08020000;
	//�����ڴ�
	audiodev.i2sbuf1=mymalloc(SRAMIN,eachSize);
	audiodev.i2sbuf2=mymalloc(SRAMIN,eachSize);
	audiodev.tbuf=mymalloc(SRAMIN,eachSize);
	
	WM8978_I2S_Cfg(2,0);	//�����ֱ�׼,16λ���ݳ���
	I2S2_Init(I2S_STANDARD_PHILIPS,I2S_MODE_MASTER_TX,I2S_CPOL_LOW,I2S_DATAFORMAT_16B_EXTENDED);	//�����ֱ�׼,��������,ʱ�ӵ͵�ƽ��Ч,16λ��չ֡����
	I2S2_SampleRate_Set(8000);//���ò�����
	I2S2_TX_DMA_Init(audiodev.i2sbuf1,audiodev.i2sbuf2,eachSize/2); //����TX DMA
	i2s_tx_callback=wav_i2s_dma_tx_callback;			//�ص�����ָwav_i2s_dma_callback
	
	audio_start();  
	printf("��ʼ����\r\n");
	
	while(1)     
	{ 
		//���ݵ�ǰʣ�����ݴ�С��ÿ�β������ݴ�С��ȷ��ÿ�ζ�ȡbuf�Ĵ�С
		if(fillnum>=eachSize){
			fillnum-=eachSize;
		}
		else{
			eachSize=fillnum;
		}
		
		//��ȡflash
		readFlash(address,audiodev.i2sbuf1,eachSize);
		readFlash(address,audiodev.i2sbuf2,eachSize);
		
		//���ݵ�ǰʣ�����ݴ�С��ÿ�β������ݴ�С�������ȡ��ַ
		if(eachSize!=fillnum){
			address+=eachSize;
		}
		else{
			break;
		}
		
		while(wavtransferend==0);//�ȴ�wav�������; 
		wavtransferend=0;
		
		//����ת����ϵ�buf
		audiodev.status=1;
		while(1)
		{
			//�ж��Ƿ񲥷��굱ǰbuf
			if((audiodev.status&0X01)==0)delay_ms(10);
			else break;
		}
	

	}
	//ֹͣ����
	audio_stop();
	printf("ֹͣ����\r\n");	
	myfree(SRAMIN,audiodev.tbuf);	//�ͷ��ڴ�
	myfree(SRAMIN,audiodev.i2sbuf1);//�ͷ��ڴ�
	myfree(SRAMIN,audiodev.i2sbuf2);//�ͷ��ڴ� 
}



