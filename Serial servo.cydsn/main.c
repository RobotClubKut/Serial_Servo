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
    uint8 i;
    int pos_h,pos_l,pos, posf;
    int servo_Data=0;
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
            CyDelay(2);
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
        pos=pos_h|pos_l;
        posf=(10000-pos)/30;
        sprintf(Buffer,"%d %d\n",(int)pos, (int)posf);
        UART_Debug_PutString(Buffer);
    } 
}
void Free(int ID)
{
    uint8 i;
    int pos_h,pos_l,pos, posf;
    char Buffer[40];
    unsigned char tx[3]={0,0,0};
    unsigned char rx[6];
    //ポジション
    tx[0]=0x80|ID;
    tx[1]=0x00;
    tx[2]=0x00;            
    //送信
    for (i = 0;i < 3;i++){
       tx_write(tx[i]); // コマンドを１バイトずつ送信する
    }
    //受信
    for (i = 0;i < 6;i++){
        rx[i] = tx_read(); // モーターからの返値を受け取り、rxに代入する
    }
    pos_h=((int)rx[4]<<7)&0x3f80; 
    pos_l=(int)rx[5]&0x7f;
    pos=pos_h|pos_l; //値に変換
    posf=(10000-pos)/30; //角度に変換
    sprintf(Buffer,"%d %d\n",(int)pos, (int)posf);
    UART_Debug_PutString(Buffer);
}
    //サーボ速度//
void speed(uint8 ID, uint8 speed){
    uint8 i;
    unsigned char tx[3]={0,0,0};
    tx[0]=0xC0|ID;
    tx[1]=0x02;
    tx[2]=speed;
    //送信
    for (i = 0;i < 3;i++){
        UART_Servo_PutChar(tx[i]); // コマンドを１バイトずつ送信する
        CyDelay(1);
    }
}
    //ストレッチ0~254//
void stretch(uint8 ID, uint8 strc){
    uint8 i;
    unsigned char tx[3]={0,0,0};
    tx[0]=0xC0|ID;
    tx[1]=0x01;
    tx[2]=strc;
    for (i = 0;i < 3;i++){
        UART_Servo_PutChar(tx[i]);
        CyDelay(1);
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
    //UART_Servo_EnableTxInt();
    UART_Debug_Start();
    UART_Debug_EnableTxInt();
    isr_1_StartEx(motor_isr);
}
int main()
{
    LinData control;
    //char Buffer[40];
    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    init();
    LINSlaveInit();
    CyGlobalIntEnable;
    /* Uncomment this line to enable global interrupts. */
    //--- 一秒待つ　----//
    CyDelay(1000);
    //--- 速度調節 ---//
    speed(0,50);
    speed(1,50);
    speed(2,50);
    speed(3,50);
    speed(4,50);
    speed(5,50);
    //--- ストレッチ ---//
    stretch(2,30);
    stretch(3,100);
    stretch(4,100);
    stretch(5,100);
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
            servo(0,90,1);
        }
        else
        {
            servo(0,180,1);
        }
    //--- アーム位置　---//
        if(control.servo.command.position == 1)
        {
            servo(1,165,1);
        }
        else
        {
            // 初期位置　//
            servo(1,0,1);        
            // 格納コンテナ選択　//
            if(control.servo.command.selectContainer == 0)//黄
            {
                servo(2,0,1);
            }
            else if(control.servo.command.selectContainer == 1)//赤
            {
                servo(2,-45,1);
            }
            else if(control.servo.command.selectContainer == 2)//青
            {
                servo(2,45,1);
            }
            else if(control.servo.command.selectContainer == 3)//初期位置(あまり)
            {
                servo(2,0,1);
            }
        }
        // 排出機構 //
        if(control.servo.command.pad==1)
        {
        //---　ボール赤排出　---//
            if(control.servo.command.emissionRed == 1)
            {
                //open
                servo(3,86,1);
            }
            else
            {
                //close
                servo(3,35,1);
            }
        //---　ボール黄排出　---//
            if(control.servo.command.emissionYellow == 1)
            {
                servo(4,86,1);
            }
            else
            {
                servo(4,37,1);
            }       
        //---　ボール青排出　---//
            if(control.servo.command.emissionBlue == 1)
            {
                servo(5,71,1);
            }
            else
            {
                servo(5,23,1);
            }
        }
        else{
            servo(3,17,1);
            servo(4,18,1);
            servo(5,2,1);
        }    
        //--- デバッグ ---//
        /*
        sprintf(Buffer,"[%d]\n",(int)control.servo.command.emissionYellow);
        UART_Debug_PutString(Buffer);
        */
    }
}

/* [] END OF FILE */
