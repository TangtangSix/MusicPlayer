#include "i2s.h"  
#include "delay.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32F407������
//I2S ��������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2017/5/3
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2014-2024
//All rights reserved									  
//******************************************************************************** 
//V1.1 20141220  
//����I2S2_SampleRate_Set����ODDλ���õ�bug 	
////////////////////////////////////////////////////////////////////////////////// 	
 
I2S_HandleTypeDef I2S2_Handler;			//I2S2���
DMA_HandleTypeDef I2S2_TXDMA_Handler;   //I2S2����DMA���

//I2S2��ʼ��
//I2S_Standard:I2S��׼,�������ã�I2S_STANDARD_PHILIPS/I2S_STANDARD_MSB/
//				       I2S_STANDARD_LSB/I2S_STANDARD_PCM_SHORT/I2S_STANDARD_PCM_LONG
//I2S_Mode:I2S����ģʽ,�������ã�I2S_MODE_SLAVE_TX/I2S_MODE_SLAVE_RX/I2S_MODE_MASTER_TX/I2S_MODE_MASTER_RX
//I2S_Clock_Polarity:ʱ�ӵ�ƽ����������Ϊ:I2S_CPOL_LOW/I2S_CPOL_HIGH
//I2S_DataFormat:���ݳ���,�������ã�I2S_DATAFORMAT_16B/I2S_DATAFORMAT_16B_EXTENDED/I2S_DATAFORMAT_24B/I2S_DATAFORMAT_32B
void I2S2_Init(u32 I2S_Standard,u32 I2S_Mode,u32 I2S_Clock_Polarity,u32 I2S_DataFormat)
{
	I2S2_Handler.Instance=SPI2;
	I2S2_Handler.Init.Mode=I2S_Mode;					//IISģʽ
	I2S2_Handler.Init.Standard=I2S_Standard;			//IIS��׼
	I2S2_Handler.Init.DataFormat=I2S_DataFormat;		//IIS���ݳ���
	I2S2_Handler.Init.MCLKOutput=I2S_MCLKOUTPUT_ENABLE;	//��ʱ�����ʹ��
	I2S2_Handler.Init.AudioFreq=I2S_AUDIOFREQ_DEFAULT;	//IISƵ������
	I2S2_Handler.Init.CPOL=I2S_Clock_Polarity;			//����״̬ʱ�ӵ�ƽ
	I2S2_Handler.Init.ClockSource=I2S_CLOCK_PLL;		//IISʱ��ԴΪPLL
	HAL_I2S_Init(&I2S2_Handler); 
	
	SPI2->CR2|=1<<1;									//SPI2 TX DMA����ʹ��.
	__HAL_I2S_ENABLE(&I2S2_Handler);					//ʹ��I2S2	
}

//I2S�ײ�������ʱ��ʹ�ܣ��������ã�DMA����
//�˺����ᱻHAL_I2S_Init()����
//hi2s:I2S���
void HAL_I2S_MspInit(I2S_HandleTypeDef *hi2s)
{
	GPIO_InitTypeDef GPIO_Initure;
    
	__HAL_RCC_SPI2_CLK_ENABLE();        		//ʹ��SPI2/I2S2ʱ��
	__HAL_RCC_GPIOB_CLK_ENABLE();               //ʹ��GPIOBʱ��
	__HAL_RCC_GPIOC_CLK_ENABLE();               //ʹ��GPIOCʱ��
	
    //��ʼ��PB12/13
    GPIO_Initure.Pin=GPIO_PIN_12|GPIO_PIN_13;  
    GPIO_Initure.Mode=GPIO_MODE_AF_PP;          //���츴��
    GPIO_Initure.Pull=GPIO_PULLUP;              //����
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;         //����
    GPIO_Initure.Alternate=GPIO_AF5_SPI2;       //����ΪSPI/I2S  
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);         //��ʼ��
	
	//��ʼ��PC2/PC3/PC6
	GPIO_Initure.Pin=GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_6; 
	HAL_GPIO_Init(GPIOC,&GPIO_Initure);         //��ʼ��	
}

//�����ʼ��㹫ʽ:Fs=I2SxCLK/[256*(2*I2SDIV+ODD)]
//I2SxCLK=(HSE/pllm)*PLLI2SN/PLLI2SR
//һ��HSE=8Mhz 
//pllm:��Sys_Clock_Set���õ�ʱ��ȷ����һ����8
//PLLI2SN:һ����192~432 
//PLLI2SR:2~7
//I2SDIV:2~255
//ODD:0/1
//I2S��Ƶϵ����@pllm=8,HSE=8Mhz,��vco����Ƶ��Ϊ1Mhz
//���ʽ:������/10,PLLI2SN,PLLI2SR,I2SDIV,ODD
const u16 I2S_PSC_TBL[][5]=
{
	{800 ,256,5,12,1},		//8Khz������
	{1102,429,4,19,0},		//11.025Khz������ 
	{1600,213,2,13,0},		//16Khz������
	{2205,429,4, 9,1},		//22.05Khz������
	{3200,213,2, 6,1},		//32Khz������
	{4410,271,2, 6,0},		//44.1Khz������
	{4800,258,3, 3,1},		//48Khz������
	{8820,316,2, 3,1},		//88.2Khz������
	{9600,344,2, 3,1},  	//96Khz������
	{17640,361,2,2,0},  	//176.4Khz������ 
	{19200,393,2,2,0},  	//192Khz������
};

//����I2S��DMA����,HAL��û���ṩ�˺���
//���������Ҫ�Լ������Ĵ�����дһ��
void I2S_DMA_Enable(void)
{
    u32 tempreg=0;
    tempreg=SPI2->CR2;    	//�ȶ�����ǰ������			
	tempreg|=1<<1;			//ʹ��DMA
	SPI2->CR2=tempreg;		//д��CR1�Ĵ�����
}

//����SAIA�Ĳ�����(@MCKEN)
//samplerate:������,��λ:Hz
//����ֵ:0,���óɹ�;1,�޷�����.
u8 I2S2_SampleRate_Set(u32 samplerate)
{   
    u8 i=0; 
	u32 tempreg=0;
    RCC_PeriphCLKInitTypeDef RCCI2S2_ClkInitSture;  
	
	for(i=0;i<(sizeof(I2S_PSC_TBL)/10);i++)//�����Ĳ������Ƿ����֧��
	{
		if((samplerate/10)==I2S_PSC_TBL[i][0])break;
	}
    if(i==(sizeof(I2S_PSC_TBL)/10))return 1;//�ѱ���Ҳ�Ҳ���
	
    RCCI2S2_ClkInitSture.PeriphClockSelection=RCC_PERIPHCLK_I2S;	//����ʱ��Դѡ�� 
    RCCI2S2_ClkInitSture.PLLI2S.PLLI2SN=(u32)I2S_PSC_TBL[i][1];    	//����PLLI2SN
    RCCI2S2_ClkInitSture.PLLI2S.PLLI2SR=(u32)I2S_PSC_TBL[i][2];    	//����PLLI2SR
    HAL_RCCEx_PeriphCLKConfig(&RCCI2S2_ClkInitSture);             	//����ʱ��
	
	RCC->CR|=1<<26;					//����I2Sʱ��
	while((RCC->CR&1<<27)==0);		//�ȴ�I2Sʱ�ӿ����ɹ�. 
	tempreg=I2S_PSC_TBL[i][3]<<0;	//����I2SDIV
	tempreg|=I2S_PSC_TBL[i][4]<<8;	//����ODDλ
	tempreg|=1<<9;					//ʹ��MCKOEλ,���MCK
	SPI2->I2SPR=tempreg;			//����I2SPR�Ĵ��� 
	return 0;
} 

//I2S2 TX DMA����
//����Ϊ˫����ģʽ,������DMA��������ж�
//buf0:M0AR��ַ.
//buf1:M1AR��ַ.
//num:ÿ�δ���������
void I2S2_TX_DMA_Init(u8* buf0,u8 *buf1,u16 num)
{  
    __HAL_RCC_DMA1_CLK_ENABLE();                                    		//ʹ��DMA1ʱ��
    __HAL_LINKDMA(&I2S2_Handler,hdmatx,I2S2_TXDMA_Handler);         		//��DMA��I2S��ϵ����
	
    I2S2_TXDMA_Handler.Instance=DMA1_Stream4;                       		//DMA1������4                     
    I2S2_TXDMA_Handler.Init.Channel=DMA_CHANNEL_0;                  		//ͨ��0
    I2S2_TXDMA_Handler.Init.Direction=DMA_MEMORY_TO_PERIPH;         		//�洢��������ģʽ
    I2S2_TXDMA_Handler.Init.PeriphInc=DMA_PINC_DISABLE;             		//���������ģʽ
    I2S2_TXDMA_Handler.Init.MemInc=DMA_MINC_ENABLE;                 		//�洢������ģʽ
    I2S2_TXDMA_Handler.Init.PeriphDataAlignment=DMA_PDATAALIGN_HALFWORD;   	//�������ݳ���:16λ
    I2S2_TXDMA_Handler.Init.MemDataAlignment=DMA_MDATAALIGN_HALFWORD;    	//�洢�����ݳ���:16λ
    I2S2_TXDMA_Handler.Init.Mode=DMA_CIRCULAR;                      		//ʹ��ѭ��ģʽ 
    I2S2_TXDMA_Handler.Init.Priority=DMA_PRIORITY_HIGH;             		//�����ȼ�
    I2S2_TXDMA_Handler.Init.FIFOMode=DMA_FIFOMODE_DISABLE;          		//��ʹ��FIFO
    I2S2_TXDMA_Handler.Init.MemBurst=DMA_MBURST_SINGLE;             		//�洢������ͻ������
    I2S2_TXDMA_Handler.Init.PeriphBurst=DMA_PBURST_SINGLE;          		//����ͻ�����δ��� 
    HAL_DMA_DeInit(&I2S2_TXDMA_Handler);                            		//�������ǰ������
    HAL_DMA_Init(&I2S2_TXDMA_Handler);	                            		//��ʼ��DMA

    HAL_DMAEx_MultiBufferStart(&I2S2_TXDMA_Handler,(u32)buf0,(u32)&SPI2->DR,(u32)buf1,num);//����˫����
    __HAL_DMA_DISABLE(&I2S2_TXDMA_Handler);                         		//�ȹر�DMA 
    delay_us(10);                                                   		//10us��ʱ����ֹ-O2�Ż������� 	
    __HAL_DMA_ENABLE_IT(&I2S2_TXDMA_Handler,DMA_IT_TC);             		//������������ж�
    __HAL_DMA_CLEAR_FLAG(&I2S2_TXDMA_Handler,DMA_FLAG_TCIF0_4);     		//���DMA��������жϱ�־λ
    HAL_NVIC_SetPriority(DMA1_Stream4_IRQn,0,0);                    		//DMA�ж����ȼ�
    HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
} 

//I2S DMA�ص�����ָ��
void (*i2s_tx_callback)(void);	//TX�ص����� 
//DMA1_Stream4�жϷ�����
void DMA1_Stream4_IRQHandler(void)
{  
    if(__HAL_DMA_GET_FLAG(&I2S2_TXDMA_Handler,DMA_FLAG_TCIF0_4)!=RESET) //DMA�������
    {
        __HAL_DMA_CLEAR_FLAG(&I2S2_TXDMA_Handler,DMA_FLAG_TCIF0_4);     //���DMA��������жϱ�־λ
        i2s_tx_callback();	//ִ�лص�����,��ȡ���ݵȲ����������洦��  
    } 
} 

//I2S��ʼ����
void I2S_Play_Start(void)
{   	
	
	__HAL_DMA_ENABLE(&I2S2_TXDMA_Handler);//����DMA TX����  			
}

//�ر�I2S����
void I2S_Play_Stop(void)
{ 
	__HAL_DMA_DISABLE(&I2S2_TXDMA_Handler);//��������;		 	 
} 







