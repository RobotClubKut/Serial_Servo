/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include <project.h>
#include <stdio.h>
#define tx_write  UART_Servo_PutChar
#define tx_read   UART_Servo_GetChar

int g_timerFlag;

void servo(uint8 id,int kakudo,uint8 power)
{
    int servo_Data=0;
    uint8 pos_h=0,pos_l,pos=0, i;
    char Buffer[40];
    unsigned char tx[3]={0,0,0};
    unsigned char rx[6];
    //　出力　//
    if(power==1)
    {
        servo_Data = 10000 -( 30 * kakudo );
        tx[0] = 0x80 | id;
        tx[1] = (unsigned char)(servo_Data>>7) & 0x7F;
        tx[2] = (unsigned char)servo_Data & 0x7F;
        for(i = 0;i < 3;i++)
        {
            tx_write(tx[i]);
            CyDelay(1);
        }
    }
    //　角度読み取り　//
    else
    {
        servo_Data=kakudo * 0;
        tx[0] = 0x80 | id;
        tx[1] = (unsigned char)(servo_Data>>7) & 0x7F;
        tx[2] = (unsigned char)servo_Data & 0x7F;
        for(i = 0;i < 3;i++)
        {
            tx_write(tx[i]);
        }
        for(i = 0;i < 6;i++)
        {
            rx[i] = tx_read();
        }
        pos_h = ((int)rx[4]<<7) & 0x3f80;
        pos_l = (int)rx[5] & 0x7f;
        pos = pos_h | pos_l;
        pos = (10000-pos)/30;
        sprintf(Buffer,"ID=[%d]  [%d]\n",id,pos);
        UART_Debug_PutString(Buffer);
    } 
}
    //サーボ速度//
void speed(int ID, int speed){
    uint8 i;
    unsigned char tx[3]={0,0,0};
    tx[0]=0xC0|ID;
    tx[1]=0x02;
    tx[2]=speed;
    //送信
    for (i = 0;i < 3;i++){
        UART_Servo_PutChar(tx[i]); // コマンドを１バイトずつ送信する
    }
}
//--- LIN初期化 ---//
void LINSlaveInit()
{
	if(0u != l_sys_init())
    {
		CyHalt(0x00);
	}
	
	if(0u != l_ifc_init(LIN_1_IFC_HANDLE))
    {
		CyHalt(0x00);
	}
}
union Motion
{
	uint8 Trans;
	struct
	{
        uint8  grabBall        : 1;//ID:0 つかむorはなす
        uint8  position        : 1;//ID:1 アームの上下
        uint8  selectContainer : 2;//ID:2 コンテナの選択
        uint8  emissionRed     : 1;//ID:3 排出
        uint8  emissionYellow  : 1;//ID:4
        uint8  emissionBlue    : 1;//ID:5 
        uint8  pad             : 1;//あまり
	}command;
};
typedef struct
{
	union Motion servo;
} LinData;
//--- 一定周期で割り込み ---//
CY_ISR(motor_isr)
{
	g_timerFlag = 1;
}
void init()
{
    UART_Servo_Start();
//  UART_Servo_EnableTxInt();
    UART_Debug_Start();
    UART_Debug_EnableTxInt();
    isr_1_StartEx(motor_isr);
}
int main()
{
    LinData control;
    char Buffer[40];
    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    
    init();
    LINSlaveInit();
    CyGlobalIntEnable;
    /* Uncomment this line to enable global interrupts. */
    
    //--- 一秒待つ　----//
    CyDelay(1000);
    //--- 速度調節 ---///
    speed(0,50);
    speed(1,50);
    speed(2,50);
    speed(3,50);
    speed(4,50);
    speed(5,50);
    for(;;)
    {
    //--- マスターから受信 ---//
		if(l_flg_tst_target() == 1)
		{
			control.servo.Trans = (uint8)l_u8_rd_target();
            l_flg_clr_target();
		}
            //--- 掴むor離す　---//
        if(control.servo.command.grabBall == 1)
        {
            servo(0,100,1);
        }
        else
        {
            servo(0,140,1);
        }
    //--- アーム位置　---//
        if(control.servo.command.position == 1)
        {
            servo(1,100,1);
        }
        else
        {
            // 初期位置　//
            servo(1,-24,1);
            
            // 格納コンテナ選択　//
            if(control.servo.command.selectContainer == 0)
            {
                servo(2,0,1);
            }
            else if(control.servo.command.selectContainer == 1)
            {
                servo(2,150,1);
            }
            else if(control.servo.command.selectContainer == 2)
            {
                servo(2,180,1);
            }
            else if(control.servo.command.selectContainer == 3)
            {
                servo(2,207,1);
            }
        }
    //---　ボール赤排出　---//
        if(control.servo.command.emissionRed == 1)
        {
            //open
            servo(3,75,1);
        }
        else
        {
            //close
            servo(3,-14,1);
        }
    //---　ボール黄排出　---//
        if(control.servo.command.emissionYellow == 1)
        {
            servo(4,89,1);
        }
        else
        {
            servo(4,1,1);
        }       
        
    //---　ボール青排出　---//
        if(control.servo.command.emissionBlue == 1)
        {
            servo(5,90,1);
        }
        else
        {
            servo(5,0,1);
        }        
        //--- デバッグ ---//
        sprintf(Buffer,"[%d]\n",(int)control.servo.command.emissionYellow);
        UART_Debug_PutString(Buffer);
    }
}

/* [] END OF FILE */
