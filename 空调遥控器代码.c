#include<reg51.h>
#define uchar unsigned char
#define uint unsigned int
#define LCD1602_DB P0
unsigned char code led_mod[10]={0x28, 0x7E, 0xA2,0x62,0x74, 0x61, 0x21,0x7A, 0x20,0x60};
sbit RS=P2^0;
sbit RW=P2^1;
sbit E=P2^2;
sbit beep=P3^5;
sbit speed_led=P3^7; //⻛风速灯接⼝口
uchar k=1;//按键1控制变量量
uchar m=0;//按键4(模式)控制变量量
uchar temperature=27; //温度变量量
uchar speed_num=1; //⻛风速控制变量量
uint high,low;
uchar a=1; //步进电机⾥里里⾯面控制正转反转的变量量
uint motor_contrl_1=0; //计算步进电机转动次数的变量量(6次⼀一反转)
uchar motor_contrl_2=0; //实现步进电机⻓长按功能的辅助变量量
uchar hour=1,min=59; //倒计时⼩小时和分钟变量量
uchar bell_num=0,bell_hour=0,bell_min=0; //bell_num记录按了了⼏几次， bell_hour， bell_min⽤用来实现闪烁功能的

/**********
延时函数
**********/
void delay(uint z)
{
	uint i, j;
	for (i = 0; i < z; i++)
		for (j = 0; j < 125; j++);
}

/**********
计时器器初始化
**********/
void time_initialize()
{
	EA = 1;
	TR0 = 1;
	ET0 = 1;
	TMOD = 0x11;
}

/**********
变量量初始化函数，终于倒计时结束后的初始化
**********/
void all_initialize()
{
	k = 1;
	m = 0;
	temperature = 27;
	speed_num = 1;
	a = 1;
	motor_contrl_1 = 0;
	motor_contrl_2 = 0;
	hour = 2;
	min = 59;
	bell_num = 0;
	bell_hour = 0;
	bell_min = 0;
}

/**********
按键扫描函数
**********/
uchar KeyScan()
{
	uchar i;
	uchar h, l; //⾏值和列值存储变量量
	P1 = 0xf0; //给P1置初值11110000
	if ((P1 & 0xf0) != 0xf0) //检测是否有按键按下
	{
		delay(40); //延时消抖
		if ((P1 & 0xf0) != 0xf0) //再次检测
		{
			l = P1 & 0xf0; //存列列值
			P1 = l | 0x0f;
			h = P1 & 0x0f; //存⾏行行值
			while ((P1 & 0x0f != 0x0f)); //等待按键提起
			for (i = 0; i < 30; i++) //蜂鸣器器响
			{
				beep = ~beep;
				delay(1);
			}
			return(l + h); //返回键值
		}
	}
}

/**********
1602写数据函数
**********/
void LcdWriteData(uchar dat) 
{
	RS = 1;
	RW = 0;
	LCD1602_DB = dat;
	delay(1);
	E = 1;
	delay(2);
	E = 0;
}

/**********
1602写指令函数
**********/
void LcdWriteCmd(uchar cmd) 
{
	RS = 0;
	RW = 0;
	LCD1602_DB = cmd;
	delay(1);
	E = 1;
	delay(2);
	E = 0;
}

/**********
指针函数，控制写入位置
**********/
void set_cursor_pos(unsigned char x, unsigned char y) 
{
	if (y == 0)
	{
		LcdWriteCmd(0x80 + x);
	}
	else
	{
		LcdWriteCmd(0xc0 + x);
	}
}

/**********
按键1函数，利用变量k，实现开显示与关显示
**********/
void key_1() 
{
	if (k == 1)
		LcdWriteCmd(0x0c);
	else
		LcdWriteCmd(0x08);
	k++;
	if (k > 2) k = 1;
}

/**********
温度写入函数
**********/
void Temp() 
{
	set_cursor_pos(0, 0);
	LcdWriteData(temperature / 10 + 0x30);
	LcdWriteData(temperature % 10 + 0x30);
	LcdWriteData('C');
}

/**********
模式写⼊函数
**********/
void Mode() 
{
	uchar n;
	uchar mode[3][4] = { {'A','U','T','O'},{'C','O','O','L'},{'H','O','T',' ' } };
	if (m > 2) m = 0;
	set_cursor_pos(5, 0);
	for (n = 0; n < 4; n++)
	{
		LcdWriteData(mode[m][n]);
	}
}

/**********
风速显示写⼊函数
**********/
void Speed() 
{
	uchar i;
	uchar speed = '<';
	if (speed_num > 3) //按3次后清屏回到⼀一档
	{
		speed_num = 1;
		LcdWriteCmd(0x01);
	}
	set_cursor_pos(11, 0);
	for (i = 0; i < speed_num; i++)
	{
		LcdWriteData(speed);
	}
}

/**********
步进电机控制函数， angle为⻆角度， dir控制正转或反转
**********/
void Motor(uchar angle, uchar dir) 
{
	uchar i;
	uchar tmp; //临时变量量，存储P3的中间变量量uchar index=0; //节拍输出索引
	uint beats = 0; //电机转动节拍数
	uchar code beatcode[8] = { 0x0e,0x0c,0x0d,0x09,0x0b,0x03,0x07,0x06 }; //节拍控制代码
	beats = angle * 4096 / 360;
	if (dir == 1)
	{
		for (i = 0; i < beats; i++)
		{
			tmp = P3;
			tmp = tmp & 0xf0; //为了了不不影响P3另外四个接⼝口
			tmp = tmp | beatcode[index];
			P3 = tmp;
			delay(10); //延迟，给步进电机⾜足够时间
			index++;
			if (index == 8) index = 0; //到8归0
		}
	}
	else
	{
		for (i = 0; i < beats; i++)
		{
			tmp = P3;
			tmp = tmp & 0xf0;
			tmp = tmp | beatcode[index];
			P3 = tmp;
			delay(10);
			index--;
			if (index == -1) index = 7;
		}
	}
}

/**********
风速led亮度调节函数， f为频率， p为占空⽐
**********/
void PWM(uint f, uchar p) 
{
	unsigned long tmp;
	tmp = 1000000 / f; //⼀一个周期所需要的计数值
	low = (tmp*(p)) / 100; //低电平需要的计数值
	high = tmp - low;
	TH0 = (65536 - high) / 256;
	TL0 = (65536 - high) % 256;
	speed_led = 1;
}

/**********
按键6函数，实现步进电机3个功能
**********/
void key_6() 
{
	delay(200); //⻓长按延时
	if (KeyScan() == 0xdd)
	{
		while (KeyScan() != 0xdd || motor_contrl_2++ < 5)
		{
			1motor_contrl_2++; //⻓长按辅助变量量
			if (motor_contrl_2 % 6 == 0) //实现⾃自动转6次后反转
			{
				a = ~a;
			}
			Motor(1, a);
		}
		motor_contrl_2 = 0;
	}
	if (motor_contrl_1 % 6 == 0) //独⽴立按6次后反转⽅方向
	{
		a = ~a;
	}
	motor_contrl_1++;
	Motor(1, a);
}

/**********
倒计时函数
**********/
void bell() 
{
	switch (bell_num)
	{
	case 1: //按键7第⼀一次
	{
		ET1 = 1;
		TR1 = 1;
		set_cursor_pos(0, 1);
		LcdWriteData(hour / 10 + 0x30);
		LcdWriteData(hour % 10 + 0x30);
		LcdWriteData(':');
		LcdWriteData(min / 10 + 0x30);
		LcdWriteData(min % 10 + 0x30);
	} break;
	case 2: //按键7第⼆二次
	{
		ET1 = 0;
		TR1 = 0;
		set_cursor_pos(0, 1);
		LcdWriteData(hour / 10 + 0x30);
		LcdWriteData(hour % 10 + 0x30); //写⼊入⼩小时部分
		LcdWriteData(':');
		if (bell_min == 0)
		{
			set_cursor_pos(3, 1);
			LcdWriteData(min / 10 + 0x30); //写⼊入分钟部分
			LcdWriteData(min % 10 + 0x30);
		}
		else
		{
			set_cursor_pos(3, 1);
			LcdWriteData(' '); //对⼩小时部分⽤用空格进⾏行行清屏
			摆⻛风部分LcdWriteData(' ');
		}
	}break;
	case 3: //按键7第三次
	{
		set_cursor_pos(2, 1);
		LcdWriteData(':');
		LcdWriteData(min / 10 + 0x30);
		LcdWriteData(min % 10 + 0x30);
		if (bell_hour == 0)
		{
			set_cursor_pos(0, 1);
			LcdWriteData(hour / 10 + 0x30);
			LcdWriteData(hour % 10 + 0x30);
		}
		else
		{
			set_cursor_pos(0, 1);
			LcdWriteData(' ');
			LcdWriteData(' ');
		}
	} break;
	}
	if (bell_num == 4) bell_num = 1; //第四次直接回到第⼀一次，利利⽤用第⼀一次功能实现开始倒计时
}

/**********
按键8实现函数
**********/
void key_8() 
{
	if (bell_num == 2) //bell_num为2说明在调整分钟
	{
		min--;
		if (min == 0) min = 59;
	}
	if (bell_num == 3)
	{
		hour++;
		if (hour == 12) hour = 0;
	}
}

/**********
主函数
**********/
void main()
{
	time_initialize(); //计时器器初始化
	LcdWriteCmd(0x01);
	LcdWriteCmd(0x08);
	LcdWriteCmd(0x38); //1602初始化
	LcdWriteCmd(0x06); while (1)
	{
		PWM(100, 30 * speed_num); //点亮⻛风速⼩小灯，并按照⻛风速调节亮度
		switch (KeyScan())
		{
		case 0xee:key_1(); break;
		case 0xde:temperature++; break;
		case 0xbe:temperature--; break;
		case 0x7e: m++; break;
		case 0xed:speed_num++; break;
		case 0xdd: key_6(); break;
		case 0xbd: bell_num++; break;
		case 0x7d:key_8(); break;
		}
		Temp(); //显示温度
		Mode(); //显示模式
		Speed(); //显示⻛风速
		bell(); //显示倒计时
	}
}

/**********
中断函数1，⽤用来实现pwm和调节倒计时时间的闪烁功能
**********/
void timer_0() interrupt 1 
{
	static uchar count0 = 0;
	if (speed_led == 1)
	{
		TH0 = (65536 - low) / 256;
		TL0 = (65536 - low) % 256;
		speed_led = 0;
	}
	else
	{
		TH0 = (65536 - high) / 256;
		TL0 = (65536 - high) % 256;
		speed_led = 1;
	}
	count0++;
	if (count0 == 70) //70⽤用来控制闪烁频率
	{
		count0 = 0;
		bell_hour = ~bell_hour;
		bell_min = ~bell_min;
	}
}

/**********
中断函数2
**********/
void timer_1() interrupt 3 
{
	static uchar count1 = 0;
	TH1 = (65536 - 50000) / 256;
	TL1 = (65536 - 50000) % 256; //设置初值
	count1++; if (count1 == 2) //每2次50ms， min变化⼀一次
	{
		count1 = 0;
		min--;
		if (min == 0)
		{ //倒计时结束
			if (hour == 0)
			{
				ET1 = 0;
				TR1 = 0; //关闭计时
				all_initialize(); //把所有变量量初始化
				LcdWriteCmd(0x01); //清屏
				LcdWriteCmd(0x08); //关显示
			}
			hour--;
			min = 59;
		}
	}
}
