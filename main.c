/****************************************Copyright (c)****************************************************
**                                        
**                                 
**
**--------------File Info---------------------------------------------------------------------------------
** File name:			     main.c
** Last modified Date: 2016.3.27         
** Last Version:		   1.1
** Descriptions:		   使用的SDK版本-SDK_11.0.0
**				
**--------------------------------------------------------------------------------------------------------
** Created by:			FiYu
** Created date:		2016-7-1
** Version:			    1.0
** Descriptions:		红外接收实验
**--------------------------------------------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "app_uart.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "nrf_drv_timer.h"

/* 开发板中nRF51822管脚资源分配
P0.00：输入：光敏电阻检测        P0.16：输出：OLED_CLK
P0.01：I2C_SCL，MPU6050			     P0.17：输入：按键检测S1
P0.02：输出：OLED片选					   P0.18：输入：按键检测S2
P0.03：输入：MPU6050中断信号     P0.19：输入：按键检测S3
P0.04：外部ADC检测(电位器)       P0.20：输入：按键检测S4
P0.05：I2C_SDA，MPU6050          P0.21：输出：驱动指示灯D1
P0.06：OLED复位                  P0.22：输出：驱动指示灯D2
P0.07：输出：RGB三色指示灯驱动   P0.23：输出：驱动指示灯D3
P0.08：输出：OLED数据/命令选择   P0.24：输出：驱动指示灯D4
P0.09：UART_TXD                  P0.25：存储器片选
P0.10：传感器连接                P0.26：32.768KHz晶振
P0.11：UART_RXD                  P0.27：32.768KHz晶振
P0.12：输出：驱动蜂鸣器          P0.28：存储器数据传输
P0.13：输出：RGB三色指示灯驱动   P0.29：存储器时钟
P0.14：输出：RGB三色指示灯驱动   P0.30：存储器数据传输
P0.15：输出：OLED_MOSI
*/

#define	User_code		0xFD02		// 定义红外接收用户码  


#define    IR_POWER      25   //供电管脚，这么做是为了省电
#define    IR_RECE_PIN   14   //红外解码管脚
#define    Ir_Pin        nrf_gpio_pin_read(IR_RECE_PIN)// 定义红外接收输入端口

	        


/*************	用户系统配置	**************/
#define TIME		125		  	    // 选择定时器时间125us, 红外接收要求在60us~250us之间


/******************** 红外采样时间宏定义, 用户不要随意修改	*******************/
//数据格式: Synchro,AddressH,AddressL,data,/data, (total 32 bit). 
#if ((TIME <= 250) && (TIME >= 60))			// TIME决定测量误差，TIME太大防错能力降低，TIME太小会干扰其它中断函数执行。
	#define	IR_sample			TIME		// 定义采样时间，在60us~250us之间，
#endif


uint8_t	IR_SampleCnt;		          // 采样次数计数器，通用定时器对红外口检测次数累加记录
uint8_t	IR_BitCnt;			          // 记录位数
uint8_t	IR_UserH;			            // 用户码(地址)高字节
uint8_t	IR_UserL;			            // 用户码(地址)低字节
uint8_t	IR_data;			            // 键原码
volatile uint8_t	IR_DataShit;		// 键反码
uint8_t	IR_code;			// 红外键码	 	
bool		Ir_Pin_temp;		        // 记录红外引脚电平的临时变量
bool		IR_Sync;			        // 同步标志（1——已收到同步信号，0——没收到）
bool		IrUserErr;		            // 用户码错误标志
bool		IR_OK;			// 完成一帧红外数据接收的标志（0，未收到，1，收到一帧完整数据）

bool		F0;

#define IR_SYNC_MAX		(15000/IR_sample)	// 同步信号SYNC 最大时间 15ms(标准值9+4.5=13.5ms)
#define IR_SYNC_MIN		(9700 /IR_sample)	// 同步信号SYNC 最小时间 9.5ms，(连发信号标准值9+2.25=11.25ms)
#define IR_SYNC_DIVIDE	(12375/IR_sample)	// 区分13.5ms同步信号与11.25ms连发信号，11.25+（13.5-11.25）/2=12.375ms
#define IR_DATA_MAX		(3000 /IR_sample)	// 数据最大时间3ms    (标准值2.25 ms)
#define IR_DATA_MIN		(600  /IR_sample)	// 数据最小时,0.6ms   (标准值1.12 ms)
#define IR_DATA_DIVIDE	(1687 /IR_sample)	// 区分数据 0与1,1.12+ (2.25-1.12)/2 =1.685ms
#define IR_BIT_NUMBER		32				// 32位数据




#define UART_TX_BUF_SIZE 256                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 1                           /**< UART RX buffer size. */

const nrf_drv_timer_t TIMER_LED = NRF_DRV_TIMER_INSTANCE(0);

uint8_t Ir_Ver;

//****************************  红外接收模块  ********************************************
// 信号第1个下降沿时刻清零计数器并让计数器从0开始计数，第2个下降沿时刻计算计数器运行时间
// 因此检测的是每个信号从低电平开始到高电平结束这段时间，也就是脉冲周期。
void IR_RX(void)
{
	uint8_t	SampleTime;				// 信号周期
	IR_SampleCnt++;							// 定时器对红外口检测次数

	F0 = Ir_Pin_temp;						// 保存前一次此程序扫描到的红外端口电平	  
	Ir_Pin_temp = Ir_Pin;					// 读取当前红外接收输入端口电平
	if(F0 && !Ir_Pin_temp)					// 前一次采样高电平并且当前采样低电平，说明出现了下降沿
	{
		SampleTime = IR_SampleCnt;			// 脉冲周期
		IR_SampleCnt = 0;					// 出现了下降沿则清零计数器
		//******************* 接收同步信号 **********************************
		if(SampleTime > IR_SYNC_MAX)		IR_Sync = 0;	// 超出最大同步时间, 错误信息。
		else if(SampleTime >= IR_SYNC_MIN)		// SYNC
		{
			if(SampleTime >= IR_SYNC_DIVIDE)    // 区分13.5ms同步信号与11.25ms连发信号 
			{
				IR_Sync = 1;				    // 收到同步信号 SYNC
				IR_BitCnt = IR_BIT_NUMBER;  	// 赋值32（32位有用信号）
			}
		}
		//********************************************************************
		else if(IR_Sync)						// 已收到同步信号 SYNC
		{
			if((SampleTime < IR_DATA_MIN)|(SampleTime > IR_DATA_MAX)) IR_Sync=0;	// 数据周期过短或过长，错误
			else
			{
				IR_DataShit >>= 1;					// 键反码右移1位（发送端是低位在前，高位在后的格式）
				if(SampleTime >= IR_DATA_DIVIDE)	IR_DataShit |= 0x80;	// 区别是数据 0还是1

				//***********************  32位数据接收完毕 ****************************************
				if(--IR_BitCnt == 0)				 
				{
					IR_Sync = 0;					// 清除同步信号标志
					Ir_Ver = ~IR_DataShit;
					
					if((Ir_Ver) == IR_data)		// 判断数据正反码
					{
						if((IR_UserH == (User_code / 256)) && IR_UserL == (User_code % 256))
						{
								IrUserErr = 0;	    // 用户码正确
						}
						else	IrUserErr = 1;	    // 用户码错误
							
						IR_code      = IR_data;		// 键码值
						IR_OK   = 1;			    // 数据有效
					}
				}
				// 格式：  用户码L —— 用户码H —— 键码 —— 键反码
				// 功能：  将 “用户码L —— 用户码H —— 键码”通过3次接收交换后存入对应字节，
				//         这样写代码可以节省内存RAM占用，但是不如用数组保存好理解。        
				//         键反码前面部分代码已保存好了	:IR_DataShit |= 0x80;	          
				//**************************** 将接收的************************************************
				else if((IR_BitCnt & 7)== 0)		// 1个字节接收完成
				{
					IR_UserL = IR_UserH;			// 保存用户码高字节
					IR_UserH = IR_data;				// 保存用户码低字节
					IR_data  = IR_DataShit;			// 保存当前红外字节
				}
			}
		}  
	}
}


void uart_error_handle(app_uart_evt_t * p_event)
{
    if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_communication);
    }
    else if (p_event->evt_type == APP_UART_FIFO_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_code);
    }
}
/**
 * @brief Handler for timer events.
 */
void timer_led_event_handler(nrf_timer_event_t event_type, void* p_context)
{
    //static uint32_t i;
    //uint32_t led_to_invert = (1 << leds_list[(i++) % LEDS_NUMBER]);
    
    switch(event_type)
    {
        case NRF_TIMER_EVENT_COMPARE0:
            //nrf_gpio_pin_toggle(LED_1);
				    IR_RX();
            break;
        
        default:
            break;
    }    
}

/********************* 十六进制转ASCII函数 *************************/
unsigned char	HEX2ASCII(unsigned char dat)
{
	dat &= 0x0f;
	if(dat <= 9)	return (dat + '0');	//数字0~9
	return (dat - 10 + 'A');			    //字母A~F
}
/**********************************************************************************************
 * 描  述 : main函数
 * 入  参 : 无
 * 返回值 : 无
 ***********************************************************************************************/ 
int main(void)
{
    uint32_t time_ms = 125; //Time(in miliseconds) between consecutive compare events.
    uint32_t time_ticks;
	
	  nrf_gpio_cfg_input(IR_RECE_PIN,NRF_GPIO_PIN_PULLUP);
	
	  LEDS_CONFIGURE(LEDS_MASK);
    LEDS_OFF(LEDS_MASK);
	
	  nrf_gpio_cfg_output(IR_POWER);
	
    uint32_t err_code;
    const app_uart_comm_params_t comm_params =
    {
          RX_PIN_NUMBER,
          TX_PIN_NUMBER,
          RTS_PIN_NUMBER,
          CTS_PIN_NUMBER,
          APP_UART_FLOW_CONTROL_DISABLED,
          false,
          UART_BAUDRATE_BAUDRATE_Baud115200
    };

    APP_UART_FIFO_INIT(&comm_params,
                         UART_RX_BUF_SIZE,
                         UART_TX_BUF_SIZE,
                         uart_error_handle,
                         APP_IRQ_PRIORITY_LOW,
                         err_code);

    APP_ERROR_CHECK(err_code);
		
		//Configure TIMER_LED for generating simple light effect - leds on board will invert his state one after the other.
    err_code = nrf_drv_timer_init(&TIMER_LED, NULL, timer_led_event_handler);
    APP_ERROR_CHECK(err_code);
    
    time_ticks = nrf_drv_timer_us_to_ticks(&TIMER_LED, time_ms);
    
    nrf_drv_timer_extended_compare(
         &TIMER_LED, NRF_TIMER_CC_CHANNEL0, time_ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
    
    nrf_drv_timer_enable(&TIMER_LED);
    nrf_gpio_pin_set(IR_POWER);
    while (true)
    {
        if(IR_OK)	 	                    // 接收到一帧完整的红外数据
				{
					  while(app_uart_put(HEX2ASCII(IR_code >> 4)) != NRF_SUCCESS); 
					  while(app_uart_put(HEX2ASCII(IR_code)) != NRF_SUCCESS); 
					  IR_OK = 0;		              // 清除IR键按下标志
				}
    }
}
/********************************************END FILE*******************************************/
