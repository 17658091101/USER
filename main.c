/*
 * @Author: Z X
 * @Date: 2021-08-09 00:56:20
 * @LastEditTime: 2021-08-27 14:42:17
 * @LastEditors: Z X
 * @Description: 防盗油系统程序终版！！！
 * 				按键切换拍照/相册模式；
 * 				拍照模式，即可双击屏幕拍照，又可感应报警拍照
 * 				相册模式，点击屏幕两侧切换图片，双击屏幕中间删除当前图片；
 * 				报警响应后，只能人为按键关闭（后又添加报警半分钟后自动关闭）
 * @FilePath: \USER\main.c
 * BUG保佑（反向毒奶）！！！
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

extern u8 ov_sta;	//在exit.c里面定义
extern u8 ov_frame;	//在timer.c里面定义	
extern u8 data_ready;	

float Intervals=0;
u16 color_r;
u16 color_g;
u16 color_b;

/**
 * @funcname: Init_All();
 * @description: 初始化函数
 * @param {*}
 * @return Init_ok 1初始化正常	0初始化错误
 */
u8 Init_All()
{
	u8 Init_ok=1;
	delay_init();	    	 //延时函数初始化	  
	NVIC_Configuration(); 	 //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	uart_init(115200);	 	//串口初始化为115200
 	LED_Init();			    //LED端口初始化
	BEEP_Init();			//报警器初始化
	LCD_Init();				//LCD屏幕初始化
	KEY_Init();				//按键初始化
	TP_Init();				//触摸屏初始化
	EXTIX_Init();			//外部中断初始化
	ganying_Init();			//感应模块初始化
	W25QXX_Init();			//初始化W25Q64
	my_mem_init(SRAMIN);	//初始化内部内存池
	exfuns_init();			//为fatfs相关变量申请内存
 	f_mount(fs[0],"0:",1); 		//挂载SD卡
 	f_mount(fs[1],"1:",1); 		//挂载FLASH.
	POINT_COLOR=RED;
	usmart_dev.init(72);	//初始化USMART
 	POINT_COLOR=RED;//设置字体为红色
	while(font_init()) 				//检查字库
	{
		Init_ok=0;
		LCD_ShowString(30,50,200,16,16,"Font Error!");
		delay_ms(200);
		LCD_Fill(30,50,240,66,WHITE);//清除显示
	}
	return Init_ok;
}

/**
 * @funcname: ov7670_clock_set();
 * @description: 设置CPU的频率
 * @param {u8 PLL}	频率
 * @return {*}
 */
void ov7670_clock_set(u8 PLL) 
{ 
	u8 temp=0;
	RCC->CFGR&=0XFFFFFFFC;	
	RCC->CR&=~0x01000000;  	  
	RCC->CFGR&=~(0XF<<18);	
	PLL-=2;//抵消2个单位 
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
 * @description: 更新LCD显示
 * @param {*}
 * @return {*}
 */
void camera_refresh()
{
	u32 i,j;
	u16 color;
	if(ov_sta==2)
	{
		LCD_Scan_Dir(U2D_L2R);		//从上到下,从左到右 
		LCD_SetCursor(0x00,0x0000);	//设置光标位置 
		LCD_WriteRAM_Prepare();     //开始写入GRAM	
		OV7670_RRST=0;				//开始复位读指针 
		OV7670_RCK_L;
		OV7670_RCK_H;
		OV7670_RCK_L;
		OV7670_RRST=1;				//复位读指针结束 
		OV7670_RCK_H;  
		if(ov7670_config.mode){
			for(i=0; i<ov7670_config.height; i++)
			{
			//	LCD_SetCursor(i+ov7670_config.xsta,ov7670_config.ysta);	//设置光标位置 
			//	LCD_WriteRAM_Prepare();     //开始写入GRAM	
				for(j=0; j<ov7670_config.width; j++)
				{
					OV7670_RCK_L;
					color = GPIOC->IDR&0XFF;	//读数据
					OV7670_RCK_H;
					
					color<<=8;
					OV7670_RCK_L;//因为设置的是 YUYV输出，第二个字节没用，不需要读
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
			//	LCD_SetCursor(i+ov7670_config.xsta,ov7670_config.ysta);	//设置光标位置 
			//	LCD_WriteRAM_Prepare();     //开始写入GRAM	
				for(j=0; j<ov7670_config.width; j++)
				{
					OV7670_RCK_L;
					color = GPIOC->IDR&0XFF;	//读数据
					OV7670_RCK_H;
					
					color<<=8;
					OV7670_RCK_L;
					color |= GPIOC->IDR&0XFF;	//读数据
					OV7670_RCK_H;
					
					LCD->LCD_RAM=color;   
				}
	 
			}
		}		
		EXTI_ClearITPendingBit(EXTI_Line8);  //清除LINE8上的中断标志位
		ov_sta=0;					//开始下一次采集
 		ov_frame++; 
	  LCD_Scan_Dir(DFT_SCAN_DIR);	//恢复默认扫描方向 
	}
}	

/**
 * @funcname: camera_new_pathname(u8 *pname);
 * @description: 文件名自增（避免覆盖）
 * 				 组合成:形如"0:PICTURE/PIC13141.bmp"的文件名
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
		res=f_open(ftemp,(const TCHAR*)pname,FA_READ);//尝试打开这个文件
		if(res==FR_NO_FILE)break;		//该文件名不存在=正是我们需要的.
		index++;
	}
}

/**
 * @funcname: pic_get_tnum(u8 *path);
 * @description: 得到path路径下,目标文件的总个数
 * @param {u8 *path}	路径
 * @return {u16 rval}	总有效文件数
 */
u16 pic_get_tnum(u8 *path)
{	  
	u8 res;
	u16 rval=0;
 	DIR tdir;	 		//临时目录
	FILINFO tfileinfo;	//临时文件信息	
	u8 *fn;
    res=f_opendir(&tdir,(const TCHAR*)path); 	//打开目录
  	tfileinfo.lfsize=_MAX_LFN*2+1;				//长文件名最大长度
	tfileinfo.lfname=mymalloc(SRAMIN,tfileinfo.lfsize);//为长文件缓存区分配内存
	if(res==FR_OK&&tfileinfo.lfname!=NULL)
	{
		while(1)//查询总的有效文件数
		{
	        res=f_readdir(&tdir,&tfileinfo);       		//读取目录下的一个文件
	        if(res!=FR_OK||tfileinfo.fname[0]==0)break;	//错误了/到末尾了,退出		  
     		fn=(u8*)(*tfileinfo.lfname?tfileinfo.lfname:tfileinfo.fname);			 
			res=f_typetell(fn);	
			if((res&0XF0)==0X50)//取高四位,看看是不是图片文件	
			{
				rval++;//有效文件数增加1
			}
		}
	} 
	return rval;
}

/**
 * @funcname: camera_shoot(u8 sd_ok,u8 *pname);
 * @description: 拍照函数
 * @param {u8 sd_ok}
 * 		  {u8 *pname}
 * @return {*}
 */
void camera_shoot(u8 sd_ok,u8 *pname)	
{
	if(sd_ok)
	{
		camera_new_pathname(pname);//得到文件名
		if(bmp_encode(pname,(lcddev.width-240)/2,(lcddev.height-320)/2,240,320,0))//拍照有误
		{
			Show_Str(40,130,240,12,"写入文件错误!",12,0);					
		}else 
		{
			Show_Str(40,130,240,12,"拍照成功!",12,0);
			Show_Str(40,150,240,12,"保存为:",12,0);
			Show_Str(40+42,150,240,12,pname,12,0);		    
			delay_ms(100);
		}
	}else //提示SD卡错误
	{
		Show_Str(40,130,240,12,"SD卡错误!",12,0);
		Show_Str(40,150,240,12,"拍照功能不可用!",12,0);			
	}
	delay_ms(1000);//等待1秒钟
	LCD_Clear(BLACK);
}

/**
 * @funcname: picviewer_play();
 * @description: 数码相框
 * @param {*}
 * @return {*}
 */
u8 picviewer_play(void)
{
	u8 res_p;
	DIR picdir;	 		//图片目录
	FILINFO picfileinfo;//文件信息
	u8 *fn;   			//长文件名
	u8 *pname_p;		//带路径的文件名(显示用)
	u16 totpicnum; 		//图片文件总数
	u16 curindex;		//图片当前索引
	u16 temp;
	u16 *picindextbl;	//图片索引表
	u8 K_Touch;			//
	
	while(f_opendir(&picdir,"0:/PICTURE"))//打开图片文件夹
	{
		Show_Str(30,170,240,16,"PICTURE文件夹错误!",16,0);
		delay_ms(200);
		LCD_Fill(30,170,240,186,WHITE);//清除显示
		delay_ms(200);
	}
	totpicnum=pic_get_tnum("0:/PICTURE"); //得到总有效文件数
	while(totpicnum==NULL)//图片文件为0		
	{
		Show_Str(30,170,240,16,"没有图片文件!",16,0);
		delay_ms(200);				  
		LCD_Fill(30,170,240,186,WHITE);//清除显示	     
		delay_ms(200);
	}
	picfileinfo.lfsize=_MAX_LFN*2+1;						//长文件名最大长度
	picfileinfo.lfname=mymalloc(SRAMIN,picfileinfo.lfsize);	//为长文件缓存区分配内存
	pname_p=mymalloc(SRAMIN,picfileinfo.lfsize);			//为带路径的文件名分配内存
	picindextbl=mymalloc(SRAMIN,2*totpicnum);				//申请2*totpicnum个字节的内存,用于存放图片索引
	while(picfileinfo.lfname==NULL||pname_p==NULL||picindextbl==NULL)		//内存分配出错
	{
		Show_Str(30,190,240,16,"内存分配失败!",16,0);
		delay_ms(200);
		LCD_Fill(30,190,240,146,WHITE);//清除显示
		delay_ms(200);
	}
	//记录索引
	res_p=f_opendir(&picdir,"0:/PICTURE"); //打开目录
	if(res_p==FR_OK)
	{
		curindex=0;//当前索引为0
		while(1)//全部查询一遍
		{
			temp=picdir.index;								//记录当前index
			res_p=f_readdir(&picdir,&picfileinfo);       	//读取目录下的一个文件
			if(res_p!=FR_OK||picfileinfo.fname[0]==0)break;	//错误了/到末尾了,退出		  
			fn=(u8*)(*picfileinfo.lfname?picfileinfo.lfname:picfileinfo.fname);			 
			res_p=f_typetell(fn);	
			if((res_p&0XF0)==0X50)//取高四位,看看是不是图片文件	
			{
				picindextbl[curindex]=temp;//记录索引
				curindex++;
			}
		}
	}
	piclib_init();					//初始化画图
	curindex=0;						//从0开始显示
	res_p=f_opendir(&picdir,(const TCHAR*)"0:/PICTURE");//打开目录
	while(res_p==FR_OK)		//打开成功
	{
		dir_sdi(&picdir,picindextbl[curindex]);			//改变当前目录索引	   
		res_p=f_readdir(&picdir,&picfileinfo);       	//读取目录下的一个文件
		if(res_p!=FR_OK||picfileinfo.fname[0]==0)break;	//错误了/到末尾了,退出
		fn=(u8*)(*picfileinfo.lfname?picfileinfo.lfname:picfileinfo.fname);			 
		strcpy((char*)pname_p,"0:/PICTURE/");		//复制路径(目录)
		strcat((char*)pname_p,(const char*)fn);  	//将文件名接在后面
		LCD_Clear(BLACK);
		ai_load_picfile(pname_p,0,0,lcddev.width,lcddev.height,1);//显示图片
		Show_Str(2,2,240,16,pname_p,16,1); 		//显示图片名字
		while(1)
		{
			K_Touch=pic_tp_scan();		//扫描按键
			if(K_Touch==1)		//上一张
			{
				if(curindex)curindex--;
				else curindex=totpicnum-1;
				break;
			}
			if(K_Touch==3)//下一张
			{
				curindex++;
				if(curindex>=totpicnum)curindex=0;//到末尾的时候,自动从头开始
				break;
			}
			else if(K_Touch==2)
			{
				//删除当前图片（不知道对不对）
				f_unlink((const TCHAR*)pname_p);
				delay_ms(200);
			}
			if(camera_flag)
				return 1;
			delay_ms(10);
		}
		res_p=0;
	}
	myfree(SRAMIN,picfileinfo.lfname);	//释放内存			    
	myfree(SRAMIN,pname_p);				//释放内存			    
	myfree(SRAMIN,picindextbl);			//释放内存		
	return 0; 
}

/**
 * @funcname: camera_play();
 * @description: 摄像头显示
 * @param {*}
 * @return {*}
 */
u8 camera_play(void)
{
	u8 res;
	u8 *pname;				//带路径的文件名 
	u8 var=0;				//0，正常；1，感应到人体靠近；2，判断是否有人逗留，拍照取证；3，停止拍照，报警器开启
	u8 switch_button;  		//KEY1 拍照；KEY2 摄像头显示；WK_UP 图像显示
	u8 sd_ok=1;				//0,sd卡不正常;1,SD卡正常.

	res=f_mkdir("0:/PICTURE");		//创建PICTURE文件夹
	if(res!=FR_EXIST&&res!=FR_OK) 	//发生了错误
	{
		Show_Str(30,150,240,16,"SD error!",16,0);
		delay_ms(200);
		Show_Str(30,170,240,16,"camera can't use!",16,0);
		sd_ok=0;
	}else
		sd_ok=1;
	pname=mymalloc(SRAMIN,30);	//为带路径的文件名分配30个字节的内存
	while(pname==NULL)		//内存分配出错
	{
		Show_Str(30,190,240,16,"内存分配失败!",16,0);
		delay_ms(200);
		LCD_Fill(30,190,240,146,WHITE);//清除显示
		delay_ms(200);
	}
	while(OV7670_Init())//初始化OV7670
	{
		LCD_ShowString(60,130,200,16,16,"OV7670 Error!!");
		delay_ms(200);
		LCD_Fill(60,130,239,246,WHITE);
		delay_ms(200);
	}
	LCD_ShowString(60,130,200,16,16,"OV7670 Init OK");
	OV7670_Window_Set(176,10,320,240);	//设置窗口

	TIM6_Int_Init(10000,7199);			//10Khz计数频率,1秒钟中断									  
	EXTI8_Init();						//使能定时器捕获
	OV7670_CS=0;
	LCD_Clear(BLACK);
	config_ov7670_OutPut(20,60,320,240,0);
	while(1)
	{
		switch_button=KEY_Scan(0);
		if(picture_flag)
			break;
		if(Intervals>=0.1)	//判断是否有人逗留（时间大于3秒）
		{
			Intervals=0;
			var=2;
		}
		else
			Intervals=0;
		if(SR501&&RCWL_0516)	//感应到人体靠近
		{
			delay_ms(10);
			if(SR501&&RCWL_0516)
			{
				Show_Str(0,0,240,16,"People Runing!",16,0);
				var=1;
			}
		}
		if(var==1)		//开始计时
		{
			TIM3_Int_Init(999,7199);//10Khz的计数频率，计数到1000为100ms
			if(D80NK_L==0||D80NK_R==0)	//经过光电管，结束计时
			{
				TIM_Cmd(TIM3, DISABLE);  //失能（函数外使能）
				REG=0;
				Intervals=Intervals_ms+Intervals_s;	//最终时间
				Intervals_s=0;
				var=0;
			}
		}
		else if(var==2)	//有人停留，拍照取证
		{
			var=3;
			camera_shoot(sd_ok,pname);	//拍照保存至SD卡
		}
		else if(var==3)
		{
			while(1)
			{
				camera_refresh();//更新显示
				TIM3_Int_Init(999,7199);//10Khz的计数频率，计数到1000为100ms
				switch_button=KEY_Scan(0);
				if((int)(Intervals_ms*1000)%200==0)
				{
					BEEP=1;	//蜂鸣器短叫
					REG=!REG;
				}
				if (Intervals_s==30)
				{
					TIM_Cmd(TIM3, DISABLE);  //失能（函数外使能）
					Intervals_s=0;
					BEEP=0;//关闭蜂鸣器
					REG=0;//关闭灯光
					var=0;
					break;
				}
				else if(switch_button==KEY_RIGHT)
				{
					BEEP=0;//关闭蜂鸣器
					REG=0;//关闭灯光
					var=0;
					break;
				}
			}
		}
		else delay_ms(5);
		camera_refresh();//更新显示
	}
	return 0;
}

/**
 * @funcname:  main(void);
 * @description: 主函数
 * @param {*}
 * @return {*}
 */
int main(void)
{
	while (!Init_All())			//初始化错误
		LCD_ShowString(60,50,200,24,24,"Init_All error!");			
	
	LCD_ShowString(60,50,200,16,16,"WarShip STM32");
	LCD_ShowString(30,70,200,16,16,"Anti-theft oil Project");
	LCD_ShowString(65,90,200,16,16,"author@ZX");
	LCD_ShowString(65,110,200,16,16,"2021/8/10");
	//感应摄像
A:	while (camera_flag) 
		camera_play();
	//照片显示
	while (picture_flag)
		if(picviewer_play())
			goto A;
}

