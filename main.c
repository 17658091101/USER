/*
 * @Author: Z X
 * @Date: 2021-08-09 00:56:20
 * @LastEditTime: 2021-08-27 14:42:17
 * @LastEditors: Z X
 * @Description: ������ϵͳ�����հ棡����
 * 				�����л�����/���ģʽ��
 * 				����ģʽ������˫����Ļ���գ��ֿɸ�Ӧ��������
 * 				���ģʽ�������Ļ�����л�ͼƬ��˫����Ļ�м�ɾ����ǰͼƬ��
 * 				������Ӧ��ֻ����Ϊ�����رգ�������ӱ�������Ӻ��Զ��رգ�
 * @FilePath: \USER\main.c
 * BUG���ӣ������̣�������
 */
#include "led.h"
#include "delay.h"
#include "key.h"
#include "sys.h"
#include "lcd.h"
#include "usart.h"
#include "string.h"
#include "ov7670.h"
#include "tpad.h"
#include "timer.h"
#include "exti.h"
#include "usmart.h"
#include "malloc.h"
#include "sdio_sdcard.h"
#include "w25qxx.h"    
#include "ff.h"  
#include "exti.h"
#include "exfuns.h"   
#include "text.h"
#include "piclib.h"	
#include "touch.h"

extern u8 ov_sta;	//��exit.c���涨��
extern u8 ov_frame;	//��timer.c���涨��	
extern u8 data_ready;	

float Intervals=0;
u16 color_r;
u16 color_g;
u16 color_b;

/**
 * @funcname: Init_All();
 * @description: ��ʼ������
 * @param {*}
 * @return Init_ok 1��ʼ������	0��ʼ������
 */
u8 Init_All()
{
	u8 Init_ok=1;
	delay_init();	    	 //��ʱ������ʼ��	  
	NVIC_Configuration(); 	 //����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	uart_init(115200);	 	//���ڳ�ʼ��Ϊ115200
 	LED_Init();			    //LED�˿ڳ�ʼ��
	BEEP_Init();			//��������ʼ��
	LCD_Init();				//LCD��Ļ��ʼ��
	KEY_Init();				//������ʼ��
	TP_Init();				//��������ʼ��
	EXTIX_Init();			//�ⲿ�жϳ�ʼ��
	ganying_Init();			//��Ӧģ���ʼ��
	W25QXX_Init();			//��ʼ��W25Q64
	my_mem_init(SRAMIN);	//��ʼ���ڲ��ڴ��
	exfuns_init();			//Ϊfatfs��ر��������ڴ�
 	f_mount(fs[0],"0:",1); 		//����SD��
 	f_mount(fs[1],"1:",1); 		//����FLASH.
	POINT_COLOR=RED;
	usmart_dev.init(72);	//��ʼ��USMART
 	POINT_COLOR=RED;//��������Ϊ��ɫ
	while(font_init()) 				//����ֿ�
	{
		Init_ok=0;
		LCD_ShowString(30,50,200,16,16,"Font Error!");
		delay_ms(200);
		LCD_Fill(30,50,240,66,WHITE);//�����ʾ
	}
	return Init_ok;
}

/**
 * @funcname: ov7670_clock_set();
 * @description: ����CPU��Ƶ��
 * @param {u8 PLL}	Ƶ��
 * @return {*}
 */
void ov7670_clock_set(u8 PLL) 
{ 
	u8 temp=0;
	RCC->CFGR&=0XFFFFFFFC;	
	RCC->CR&=~0x01000000;  	  
	RCC->CFGR&=~(0XF<<18);	
	PLL-=2;//����2����λ 
	RCC->CFGR|=PLL<<18;   	
	RCC->CFGR|=1<<16;	  	//PLLSRC ON  
	FLASH->ACR|=0x12;	   
	RCC->CR|=0x01000000;  	//PLLON 
	while(!(RCC->CR>>25));	
	RCC->CFGR|=0x02;	
	while(temp!=0x02)     
	{    
		temp=RCC->CFGR>>2; 
		temp&=0x03; 
	}
}

/**
 * @funcname: camera_refresh();
 * @description: ����LCD��ʾ
 * @param {*}
 * @return {*}
 */
void camera_refresh()
{
	u32 i,j;
	u16 color;
	if(ov_sta==2)
	{
		LCD_Scan_Dir(U2D_L2R);		//���ϵ���,������ 
		LCD_SetCursor(0x00,0x0000);	//���ù��λ�� 
		LCD_WriteRAM_Prepare();     //��ʼд��GRAM	
		OV7670_RRST=0;				//��ʼ��λ��ָ�� 
		OV7670_RCK_L;
		OV7670_RCK_H;
		OV7670_RCK_L;
		OV7670_RRST=1;				//��λ��ָ����� 
		OV7670_RCK_H;  
		if(ov7670_config.mode){
			for(i=0; i<ov7670_config.height; i++)
			{
			//	LCD_SetCursor(i+ov7670_config.xsta,ov7670_config.ysta);	//���ù��λ�� 
			//	LCD_WriteRAM_Prepare();     //��ʼд��GRAM	
				for(j=0; j<ov7670_config.width; j++)
				{
					OV7670_RCK_L;
					color = GPIOC->IDR&0XFF;	//������
					OV7670_RCK_H;
					
					color<<=8;
					OV7670_RCK_L;//��Ϊ���õ��� YUYV������ڶ����ֽ�û�ã�����Ҫ��
				//	color |= GPIOC->IDR&0XFF;	//
					OV7670_RCK_H;
					
					color_r = color&0xf800;
					color_b = color>>11;
					color >>=5;
					color_g =color&0x07e0; 
					
					LCD->LCD_RAM= color_r + color_g + color_b;   
				}
			}
		}else{
			for(i=0; i<ov7670_config.height; i++)
			{
			//	LCD_SetCursor(i+ov7670_config.xsta,ov7670_config.ysta);	//���ù��λ�� 
			//	LCD_WriteRAM_Prepare();     //��ʼд��GRAM	
				for(j=0; j<ov7670_config.width; j++)
				{
					OV7670_RCK_L;
					color = GPIOC->IDR&0XFF;	//������
					OV7670_RCK_H;
					
					color<<=8;
					OV7670_RCK_L;
					color |= GPIOC->IDR&0XFF;	//������
					OV7670_RCK_H;
					
					LCD->LCD_RAM=color;   
				}
	 
			}
		}		
		EXTI_ClearITPendingBit(EXTI_Line8);  //���LINE8�ϵ��жϱ�־λ
		ov_sta=0;					//��ʼ��һ�βɼ�
 		ov_frame++; 
	  LCD_Scan_Dir(DFT_SCAN_DIR);	//�ָ�Ĭ��ɨ�跽�� 
	}
}	

/**
 * @funcname: camera_new_pathname(u8 *pname);
 * @description: �ļ������������⸲�ǣ�
 * 				 ��ϳ�:����"0:PICTURE/PIC13141.bmp"���ļ���
 * @param {u8 *pname}
 * @return {*}
 */
void camera_new_pathname(u8 *pname)
{
	u8 res;
	u16 index=0;
	while(index<0XFFFF)
	{
		sprintf((char*)pname,"0:PICTURE/PIC%05d.bmp",index);
		res=f_open(ftemp,(const TCHAR*)pname,FA_READ);//���Դ�����ļ�
		if(res==FR_NO_FILE)break;		//���ļ���������=����������Ҫ��.
		index++;
	}
}

/**
 * @funcname: pic_get_tnum(u8 *path);
 * @description: �õ�path·����,Ŀ���ļ����ܸ���
 * @param {u8 *path}	·��
 * @return {u16 rval}	����Ч�ļ���
 */
u16 pic_get_tnum(u8 *path)
{	  
	u8 res;
	u16 rval=0;
 	DIR tdir;	 		//��ʱĿ¼
	FILINFO tfileinfo;	//��ʱ�ļ���Ϣ	
	u8 *fn;
    res=f_opendir(&tdir,(const TCHAR*)path); 	//��Ŀ¼
  	tfileinfo.lfsize=_MAX_LFN*2+1;				//���ļ�����󳤶�
	tfileinfo.lfname=mymalloc(SRAMIN,tfileinfo.lfsize);//Ϊ���ļ������������ڴ�
	if(res==FR_OK&&tfileinfo.lfname!=NULL)
	{
		while(1)//��ѯ�ܵ���Ч�ļ���
		{
	        res=f_readdir(&tdir,&tfileinfo);       		//��ȡĿ¼�µ�һ���ļ�
	        if(res!=FR_OK||tfileinfo.fname[0]==0)break;	//������/��ĩβ��,�˳�		  
     		fn=(u8*)(*tfileinfo.lfname?tfileinfo.lfname:tfileinfo.fname);			 
			res=f_typetell(fn);	
			if((res&0XF0)==0X50)//ȡ����λ,�����ǲ���ͼƬ�ļ�	
			{
				rval++;//��Ч�ļ�������1
			}
		}
	} 
	return rval;
}

/**
 * @funcname: camera_shoot(u8 sd_ok,u8 *pname);
 * @description: ���պ���
 * @param {u8 sd_ok}
 * 		  {u8 *pname}
 * @return {*}
 */
void camera_shoot(u8 sd_ok,u8 *pname)	
{
	if(sd_ok)
	{
		camera_new_pathname(pname);//�õ��ļ���
		if(bmp_encode(pname,(lcddev.width-240)/2,(lcddev.height-320)/2,240,320,0))//��������
		{
			Show_Str(40,130,240,12,"д���ļ�����!",12,0);					
		}else 
		{
			Show_Str(40,130,240,12,"���ճɹ�!",12,0);
			Show_Str(40,150,240,12,"����Ϊ:",12,0);
			Show_Str(40+42,150,240,12,pname,12,0);		    
			delay_ms(100);
		}
	}else //��ʾSD������
	{
		Show_Str(40,130,240,12,"SD������!",12,0);
		Show_Str(40,150,240,12,"���չ��ܲ�����!",12,0);			
	}
	delay_ms(1000);//�ȴ�1����
	LCD_Clear(BLACK);
}

/**
 * @funcname: picviewer_play();
 * @description: �������
 * @param {*}
 * @return {*}
 */
u8 picviewer_play(void)
{
	u8 res_p;
	DIR picdir;	 		//ͼƬĿ¼
	FILINFO picfileinfo;//�ļ���Ϣ
	u8 *fn;   			//���ļ���
	u8 *pname_p;		//��·�����ļ���(��ʾ��)
	u16 totpicnum; 		//ͼƬ�ļ�����
	u16 curindex;		//ͼƬ��ǰ����
	u16 temp;
	u16 *picindextbl;	//ͼƬ������
	u8 K_Touch;			//
	
	while(f_opendir(&picdir,"0:/PICTURE"))//��ͼƬ�ļ���
	{
		Show_Str(30,170,240,16,"PICTURE�ļ��д���!",16,0);
		delay_ms(200);
		LCD_Fill(30,170,240,186,WHITE);//�����ʾ
		delay_ms(200);
	}
	totpicnum=pic_get_tnum("0:/PICTURE"); //�õ�����Ч�ļ���
	while(totpicnum==NULL)//ͼƬ�ļ�Ϊ0		
	{
		Show_Str(30,170,240,16,"û��ͼƬ�ļ�!",16,0);
		delay_ms(200);				  
		LCD_Fill(30,170,240,186,WHITE);//�����ʾ	     
		delay_ms(200);
	}
	picfileinfo.lfsize=_MAX_LFN*2+1;						//���ļ�����󳤶�
	picfileinfo.lfname=mymalloc(SRAMIN,picfileinfo.lfsize);	//Ϊ���ļ������������ڴ�
	pname_p=mymalloc(SRAMIN,picfileinfo.lfsize);			//Ϊ��·�����ļ��������ڴ�
	picindextbl=mymalloc(SRAMIN,2*totpicnum);				//����2*totpicnum���ֽڵ��ڴ�,���ڴ��ͼƬ����
	while(picfileinfo.lfname==NULL||pname_p==NULL||picindextbl==NULL)		//�ڴ�������
	{
		Show_Str(30,190,240,16,"�ڴ����ʧ��!",16,0);
		delay_ms(200);
		LCD_Fill(30,190,240,146,WHITE);//�����ʾ
		delay_ms(200);
	}
	//��¼����
	res_p=f_opendir(&picdir,"0:/PICTURE"); //��Ŀ¼
	if(res_p==FR_OK)
	{
		curindex=0;//��ǰ����Ϊ0
		while(1)//ȫ����ѯһ��
		{
			temp=picdir.index;								//��¼��ǰindex
			res_p=f_readdir(&picdir,&picfileinfo);       	//��ȡĿ¼�µ�һ���ļ�
			if(res_p!=FR_OK||picfileinfo.fname[0]==0)break;	//������/��ĩβ��,�˳�		  
			fn=(u8*)(*picfileinfo.lfname?picfileinfo.lfname:picfileinfo.fname);			 
			res_p=f_typetell(fn);	
			if((res_p&0XF0)==0X50)//ȡ����λ,�����ǲ���ͼƬ�ļ�	
			{
				picindextbl[curindex]=temp;//��¼����
				curindex++;
			}
		}
	}
	piclib_init();					//��ʼ����ͼ
	curindex=0;						//��0��ʼ��ʾ
	res_p=f_opendir(&picdir,(const TCHAR*)"0:/PICTURE");//��Ŀ¼
	while(res_p==FR_OK)		//�򿪳ɹ�
	{
		dir_sdi(&picdir,picindextbl[curindex]);			//�ı䵱ǰĿ¼����	   
		res_p=f_readdir(&picdir,&picfileinfo);       	//��ȡĿ¼�µ�һ���ļ�
		if(res_p!=FR_OK||picfileinfo.fname[0]==0)break;	//������/��ĩβ��,�˳�
		fn=(u8*)(*picfileinfo.lfname?picfileinfo.lfname:picfileinfo.fname);			 
		strcpy((char*)pname_p,"0:/PICTURE/");		//����·��(Ŀ¼)
		strcat((char*)pname_p,(const char*)fn);  	//���ļ������ں���
		LCD_Clear(BLACK);
		ai_load_picfile(pname_p,0,0,lcddev.width,lcddev.height,1);//��ʾͼƬ
		Show_Str(2,2,240,16,pname_p,16,1); 		//��ʾͼƬ����
		while(1)
		{
			K_Touch=pic_tp_scan();		//ɨ�谴��
			if(K_Touch==1)		//��һ��
			{
				if(curindex)curindex--;
				else curindex=totpicnum-1;
				break;
			}
			if(K_Touch==3)//��һ��
			{
				curindex++;
				if(curindex>=totpicnum)curindex=0;//��ĩβ��ʱ��,�Զ���ͷ��ʼ
				break;
			}
			else if(K_Touch==2)
			{
				//ɾ����ǰͼƬ����֪���Բ��ԣ�
				f_unlink((const TCHAR*)pname_p);
				delay_ms(200);
			}
			if(camera_flag)
				return 1;
			delay_ms(10);
		}
		res_p=0;
	}
	myfree(SRAMIN,picfileinfo.lfname);	//�ͷ��ڴ�			    
	myfree(SRAMIN,pname_p);				//�ͷ��ڴ�			    
	myfree(SRAMIN,picindextbl);			//�ͷ��ڴ�		
	return 0; 
}

/**
 * @funcname: camera_play();
 * @description: ����ͷ��ʾ
 * @param {*}
 * @return {*}
 */
u8 camera_play(void)
{
	u8 res;
	u8 *pname;				//��·�����ļ��� 
	u8 var=0;				//0��������1����Ӧ�����忿����2���ж��Ƿ����˶���������ȡ֤��3��ֹͣ���գ�����������
	u8 switch_button;  		//KEY1 ���գ�KEY2 ����ͷ��ʾ��WK_UP ͼ����ʾ
	u8 sd_ok=1;				//0,sd��������;1,SD������.

	res=f_mkdir("0:/PICTURE");		//����PICTURE�ļ���
	if(res!=FR_EXIST&&res!=FR_OK) 	//�����˴���
	{
		Show_Str(30,150,240,16,"SD error!",16,0);
		delay_ms(200);
		Show_Str(30,170,240,16,"camera can't use!",16,0);
		sd_ok=0;
	}else
		sd_ok=1;
	pname=mymalloc(SRAMIN,30);	//Ϊ��·�����ļ�������30���ֽڵ��ڴ�
	while(pname==NULL)		//�ڴ�������
	{
		Show_Str(30,190,240,16,"�ڴ����ʧ��!",16,0);
		delay_ms(200);
		LCD_Fill(30,190,240,146,WHITE);//�����ʾ
		delay_ms(200);
	}
	while(OV7670_Init())//��ʼ��OV7670
	{
		LCD_ShowString(60,130,200,16,16,"OV7670 Error!!");
		delay_ms(200);
		LCD_Fill(60,130,239,246,WHITE);
		delay_ms(200);
	}
	LCD_ShowString(60,130,200,16,16,"OV7670 Init OK");
	OV7670_Window_Set(176,10,320,240);	//���ô���

	TIM6_Int_Init(10000,7199);			//10Khz����Ƶ��,1�����ж�									  
	EXTI8_Init();						//ʹ�ܶ�ʱ������
	OV7670_CS=0;
	LCD_Clear(BLACK);
	config_ov7670_OutPut(20,60,320,240,0);
	while(1)
	{
		switch_button=KEY_Scan(0);
		if(picture_flag)
			break;
		if(Intervals>=0.1)	//�ж��Ƿ����˶�����ʱ�����3�룩
		{
			Intervals=0;
			var=2;
		}
		else
			Intervals=0;
		if(SR501&&RCWL_0516)	//��Ӧ�����忿��
		{
			delay_ms(10);
			if(SR501&&RCWL_0516)
			{
				Show_Str(0,0,240,16,"People Runing!",16,0);
				var=1;
			}
		}
		if(var==1)		//��ʼ��ʱ
		{
			TIM3_Int_Init(999,7199);//10Khz�ļ���Ƶ�ʣ�������1000Ϊ100ms
			if(D80NK_L==0||D80NK_R==0)	//�������ܣ�������ʱ
			{
				TIM_Cmd(TIM3, DISABLE);  //ʧ�ܣ�������ʹ�ܣ�
				REG=0;
				Intervals=Intervals_ms+Intervals_s;	//����ʱ��
				Intervals_s=0;
				var=0;
			}
		}
		else if(var==2)	//����ͣ��������ȡ֤
		{
			var=3;
			camera_shoot(sd_ok,pname);	//���ձ�����SD��
		}
		else if(var==3)
		{
			while(1)
			{
				camera_refresh();//������ʾ
				TIM3_Int_Init(999,7199);//10Khz�ļ���Ƶ�ʣ�������1000Ϊ100ms
				switch_button=KEY_Scan(0);
				if((int)(Intervals_ms*1000)%200==0)
				{
					BEEP=1;	//�������̽�
					REG=!REG;
				}
				if (Intervals_s==30)
				{
					TIM_Cmd(TIM3, DISABLE);  //ʧ�ܣ�������ʹ�ܣ�
					Intervals_s=0;
					BEEP=0;//�رշ�����
					REG=0;//�رյƹ�
					var=0;
					break;
				}
				else if(switch_button==KEY_RIGHT)
				{
					BEEP=0;//�رշ�����
					REG=0;//�رյƹ�
					var=0;
					break;
				}
			}
		}
		else delay_ms(5);
		camera_refresh();//������ʾ
	}
	return 0;
}

/**
 * @funcname:  main(void);
 * @description: ������
 * @param {*}
 * @return {*}
 */
int main(void)
{
	while (!Init_All())			//��ʼ������
		LCD_ShowString(60,50,200,24,24,"Init_All error!");			
	
	LCD_ShowString(60,50,200,16,16,"WarShip STM32");
	LCD_ShowString(30,70,200,16,16,"Anti-theft oil Project");
	LCD_ShowString(65,90,200,16,16,"author@ZX");
	LCD_ShowString(65,110,200,16,16,"2021/8/10");
	//��Ӧ����
A:	while (camera_flag) 
		camera_play();
	//��Ƭ��ʾ
	while (picture_flag)
		if(picviewer_play())
			goto A;
}

