
/********************************** (C) COPYRIGHT *******************************
* File Name          :CompatibilityHID.C
* Author             : WCH
* Version            : V1.7
* Date               : 2023/05/31
* Description        : CH554模拟HID兼容设备，支持中断上下传，支持控制端点上下传，支持全速传输
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
********************************************************************************/

#include "CH552.H"
#include "Debug.H"
#include "UART1.H"
#include <stdio.h>
#include <string.h>

#define THIS_ENDP0_SIZE         64
#define ENDP2_IN_SIZE           64
#define ENDP2_OUT_SIZE          64

UINT8X  Ep0Buffer[MIN(64,THIS_ENDP0_SIZE+2)] _at_ 0x0000;                          //端点0 OUT&IN缓冲区，必须是偶地址
UINT8X  Ep2Buffer[MIN(64,ENDP2_IN_SIZE+2)+MIN(64,ENDP2_OUT_SIZE+2)] _at_ MIN(64,THIS_ENDP0_SIZE+2);//端点2 IN&OUT缓冲区,必须是偶地址

UINT8   SetupReq,Ready,UsbConfig;
UINT16  SetupLen;
PUINT8  pDescr;                                                                    //USB配置标志
USB_SETUP_REQ   SetupReqBuf;                                                       //暂存Setup包
#define UsbSetupBuf     ((PUSB_SETUP_REQ)Ep0Buffer)  


#pragma  NOAREGS
/*设备描述符*/
UINT8C DevDesc[18] = {
	0x12,        // bLength
	0x01,        // bDescriptorType (Device)
	0x10, 0x01,  // bcdUSB 1.10
	0x00,        // bDeviceClass (Use class information in the Interface Descriptors)
	0x00,        // bDeviceSubClass 
	0x00,        // bDeviceProtocol 
	THIS_ENDP0_SIZE,        // bMaxPacketSize0 64
	0x96, 0xAA,  // idVendor  0xAA96
	0x33, 0xAA,  // idProduct 0xAA32
	0x01, 0x00,  // bcdDevice 0.00
	0x01,        // iManufacturer (String Index)
	0x02,        // iProduct (String Index)
	0x00,        // iSerialNumber (String Index)
	0x01,        // bNumConfigurations 1 配置数量
	// 18 bytes
	};
UINT8C CfgDesc[41] =
{	
	0x09,        //   bLength
	0x02,        //   bDescriptorType (Configuration)
	0x29, 0x00,  //   wTotalLength 41
	0x01,        //   bNumInterfaces 1，总的接口数量
	0x01,        //   bConfigurationValue,当前的标志
	0x00,        //   iConfiguration (String Index)
	0xA0,        //   bmAttributes Remote Wakeup
	0x23,        //   bMaxPower 70mA

	0x09,        //   bLength
	0x04,        //   bDescriptorType (Interface)
	0x00,        //   bInterfaceNumber 0
	0x00,        //   bAlternateSetting
	0x02,        //   bNumEndpoints 2
	0x03,        //   bInterfaceClass
	0x00,        //   bInterfaceSubClass，是否是boot
	0x00,        //   bInterfaceProtocol，鼠标还是键盘，boot=0无效
	0x00,        //   iInterface (String Index)

	0x09,        //   bLength
	0x21,        //   bDescriptorType (HID)
	0x00, 0x01,  //   bcdHID 1.00
	0x21,        //   bCountryCode
	0x01,        //   bNumDescriptors
	0x22,        //   bDescriptorType[0] (HID)
	0x22, 0x00,  //   wDescriptorLength[0] 34

	0x07,        //   bLength
	0x05,        //   bDescriptorType (Endpoint)
	0x82,        //   bEndpointAddress (IN/D2H)
	0x03,        //   bmAttributes (Interrupt)
	ENDP2_IN_SIZE, 0x00,  //   wMaxPacketSize 64
	0x01,        //   bInterval 1 (unit depends on device speed)

	0x07,        //   bLength
	0x05,        //   bDescriptorType (Endpoint)
	0x02,        //   bEndpointAddress (OUT/H2D)
	0x03,        //   bmAttributes (Interrupt)
	ENDP2_OUT_SIZE, 0x00,  //   wMaxPacketSize 100
	0x01,        //   bInterval 1 (unit depends on device speed)

	// 41 bytes
};
/*字符串描述符 略*/ 

/*HID类报表描述符*/
UINT8C HIDRepDesc[ ] =
{
	0x06, 0x00, 0xFF,  // Usage Page (Vendor Defined 0xFF00)
	0x09, 0x01,        // Usage (0x01)
	0xA1, 0x01,        // Collection (Application)
	0x09, 0x02,        //   Usage (0x02)
	0x15, 0x00,        //   Logical Minimum (0)
	0x26, 0x00, 0xFF,  //   Logical Maximum (-256)
	0x75, 0x08,        //   Report Size (8)
	0x95, THIS_ENDP0_SIZE,        //   Report Count (64)
	0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
	0x09, 0x02,        //   Usage (0x02)
	0x15, 0x00,        //   Logical Minimum (0)
	0x26, 0x00, 0xFF,  //   Logical Maximum (-256)
	0x75, 0x08,        //   Report Size (8)
	0x95, THIS_ENDP0_SIZE,        //   Report Count (64)
	0x91, 0x06,        //   Output (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
	0xC0              // End Collection
	
	// 34 bytes
};

unsigned char  code ManufacturerDes[]={
													  10, 0x03,
														'Y', 0, 'R', 0, 'K', 0, 'B', 0
                           };                                   //产品字符串描述符

unsigned char  code ProductDes[]={
													  20, 0x03,
														'S', 0, 'P', 0, 'Z', 0, '_', 0, 'C', 0, 'F', 0, 'G', 0, '_', 0, 'R', 0
                           };                                   //产品字符串描述符

UINT8X UserEp2Buf[64];                                           //用户数据定义
UINT8 Endp2Busy = 0;
UINT8 Endp2Recv = 0;
UINT8 recv_len = 0;

/*******************************************************************************
* Function Name  : USBDeviceInit()
* Description    : USB设备模式配置,设备模式启动，收发端点配置，中断开启
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void USBDeviceInit(void)
{
	IE_USB = 0;
	USB_CTRL = 0x00;                                                           // 先设定USB设备模式
	UDEV_CTRL = bUD_PD_DIS;                                                    // 禁止DP/DM下拉电阻	
    UDEV_CTRL &= ~bUD_LOW_SPEED;                                               //选择全速12M模式，默认方式
    USB_CTRL &= ~bUC_LOW_SPEED;
    UEP2_DMA = Ep2Buffer;                                                      //端点2数据传输地址
    UEP2_3_MOD |= bUEP2_TX_EN | bUEP2_RX_EN;                                   //端点2发送接收使能
    UEP2_3_MOD &= ~bUEP2_BUF_MOD;                                              //端点2收发各64字节缓冲区
    UEP0_DMA = Ep0Buffer;                                                      //端点0数据传输地址
    UEP4_1_MOD &= ~(bUEP4_RX_EN | bUEP4_TX_EN);                                //端点0单64字节收发缓冲区
	USB_DEV_AD = 0x00;
	USB_CTRL |= bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN;                     // 启动USB设备及DMA，在中断期间中断标志未清除前自动返回NAK
	UDEV_CTRL |= bUD_PORT_EN;                                                  // 允许USB端口
	USB_INT_FG = 0xFF;                                                         // 清中断标志
	USB_INT_EN = bUIE_SUSPEND | bUIE_TRANSFER | bUIE_BUS_RST;
	IE_USB = 1;
}

/*******************************************************************************
* Function Name  : Enp2BlukIn()
* Description    : USB设备模式端点2的批量上传
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void Enp2BlukIn()
{
		memcpy( Ep2Buffer+MAX_PACKET_SIZE, UserEp2Buf, sizeof(UserEp2Buf));        //加载上传数据
    if( Ready )
    {
        UEP2_T_LEN = ENDP2_IN_SIZE;                                              //上传最大包长度
        UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;                  //有数据时上传数据并应答ACK
        Endp2Busy = 1;                                                           
    }
}

/*******************************************************************************
* Function Name  : DeviceInterrupt()
* Description    : CH559USB中断处理函数
*******************************************************************************/
void    DeviceInterrupt( void ) interrupt INT_NO_USB using 1                    //USB中断服务程序,使用寄存器组1
{
    UINT8  i, errflag;
    UINT16 len;
    if(UIF_TRANSFER)                                                            //USB传输完成标志
    {
        switch (USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP))
        {
        case UIS_TOKEN_IN | 2:                                                  //endpoint 2# 端点批量上传
            UEP2_T_LEN = 0;                                                    //预使用发送长度一定要清空
            UEP2_CTRL ^= bUEP_T_TOG;                                           //手动翻转
            Endp2Busy = 0 ;
			UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_NAK;           //默认应答NAK
            break;
        case UIS_TOKEN_OUT | 2:                                                 //endpoint 2# 端点批量下传
            if ( U_TOG_OK )                                                     // 不同步的数据包将丢弃
            {
								len = USB_RX_LEN;                                               //接收数据长度，数据从Ep2Buffer首地址开始存放
                UEP2_CTRL ^= bUEP_R_TOG;                                        //手动翻转
							Endp2Recv = 1;
							recv_len = len;
//								for ( i = 0; i < len; i ++ )
//                {
//                    Ep2Buffer[MAX_PACKET_SIZE+i] = Ep2Buffer[i];         // OUT数据原样发回
//                }
//                UEP2_T_LEN = len;
//                UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;       // 允许上传
            }
            break;
        case UIS_TOKEN_SETUP | 0:                                               //SETUP事务
            UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK; 
			len = USB_RX_LEN;
            if(len == (sizeof(USB_SETUP_REQ)))
            {
                SetupLen = ( (UINT16)UsbSetupBuf->wLengthH << 8 ) | UsbSetupBuf->wLengthL;
                len = 0;                                                         // 默认为成功并且上传0长度
                errflag = 0;
                SetupReq = UsbSetupBuf->bRequest;							
                if ( ( UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK ) != USB_REQ_TYP_STANDARD )/*HID类命令*/
                {
					switch( SetupReq )                                             
					{
						case 0x01:                                                  //GetReport
							pDescr = UserEp2Buf;                                    //控制端点上传输据
							if(SetupLen >= THIS_ENDP0_SIZE)                         //大于端点0大小，需要特殊处理
							{
								len = THIS_ENDP0_SIZE;
							}													 
							else
							{													 
								len = SetupLen;              											
							}													 
							break;
						case 0x02:                                                   //GetIdle
							break;	
						case 0x03:                                                   //GetProtocol
							break;				
						case 0x09:                                                   //SetReport										
							break; 
						case 0x0A:                                                   //SetIdle
							break;	
						case 0x0B:                                                   //SetProtocol
							break;
						default:
							errflag = 0xFF;  				                              /*命令不支持*/					
							break;
					}	
					if( SetupLen > len )
					{
						SetupLen = len;    //限制总长度
					}
					len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen;   //本次传输长度
					memcpy(Ep0Buffer,pDescr,len);                                     //加载上传数据
					SetupLen -= len;
					pDescr += len;									
                }
                else                                                             //标准请求
                {
                    switch(SetupReq)                                             //请求码
                    {
                    case USB_GET_DESCRIPTOR:
                        switch(UsbSetupBuf->wValueH)
                        {
                        case 1:                                                  //设备描述符
                            pDescr = DevDesc;                                    //把设备描述符送到要发送的缓冲区
                            len = sizeof(DevDesc);
                            break;
                        case 2:                                                  //配置描述符
                            pDescr = CfgDesc;                                    //把设备描述符送到要发送的缓冲区
                            len = sizeof(CfgDesc);
                            break;
												case 3:
														if (UsbSetupBuf->wValueL == 1) {
																pDescr = ManufacturerDes;
														} else if (UsbSetupBuf->wValueL == 2) {
																pDescr = ProductDes;
														} else {
																errflag = 0xFF;   
															break;
														}
														len = pDescr[0];
														break;
                        case 0x22:                                               //报表描述符
                            pDescr = HIDRepDesc;                                 //数据准备上传
                            len = sizeof(HIDRepDesc);                            
                            break;
                        default:
                            errflag = 0xFF;                                          //不支持的命令或者出错
                            break;
                        }
                        if ( SetupLen > len )
                        {
                            SetupLen = len;    //限制总长度
                        }
                        len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen;//本次传输长度
                        memcpy(Ep0Buffer,pDescr,len);                            //加载上传数据
                        SetupLen -= len;
                        pDescr += len;
                        break;
                    case USB_SET_ADDRESS:
                        SetupLen = UsbSetupBuf->wValueL;                         //暂存USB设备地址
                        break;
                    case USB_GET_CONFIGURATION:
                        Ep0Buffer[0] = UsbConfig;
                        if ( SetupLen >= 1 )
                        {
                            len = 1;
                        }
                        break;
                    case USB_SET_CONFIGURATION:
                        UsbConfig = UsbSetupBuf->wValueL;
						if(UsbConfig)
						{
							Ready = 1;                                            //set config命令一般代表usb枚举完成的标志
						}
                        break;
                    case 0x0A:
                        break;
                    case USB_CLEAR_FEATURE:                                      //Clear Feature
                        if ( ( UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP )// 端点
                        {
                            switch( UsbSetupBuf->wIndexL )
                            {
                            case 0x82:
                                UEP2_CTRL = UEP2_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
                                break;
                            case 0x81:
                                UEP1_CTRL = UEP1_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
                                break;
                            case 0x02:
                                UEP2_CTRL = UEP2_CTRL & ~ ( bUEP_R_TOG | MASK_UEP_R_RES ) | UEP_R_RES_ACK;
                                break;
                            default:
                                errflag = 0xFF;                                       // 不支持的端点
                                break;
                            }
                        }
                        else
                        {
                            errflag = 0xFF;                                           // 不是端点不支持
                        }
                        break;
                    case USB_SET_FEATURE:                                         /* Set Feature */
                        if( ( UsbSetupBuf->bRequestType & 0x1F ) == 0x00 )        /* 设置设备 */
                        {
                            if( ( ( ( UINT16 )UsbSetupBuf->wValueH << 8 ) | UsbSetupBuf->wValueL ) == 0x01 )
                            {
                                if( CfgDesc[ 7 ] & 0x20 )
                                {
                                    /* 设置唤醒使能标志 */
                                }
                                else
                                {
                                    errflag = 0xFF;                                    /* 操作失败 */
                                }
                            }
                            else
                            {
                                errflag = 0xFF;                                        /* 操作失败 */
                            }
                        }
                        else if( ( UsbSetupBuf->bRequestType & 0x1F ) == 0x02 )    /* 设置端点 */
                        {
                            if( ( ( ( UINT16 )UsbSetupBuf->wValueH << 8 ) | UsbSetupBuf->wValueL ) == 0x00 )
                            {
                                switch( ( ( UINT16 )UsbSetupBuf->wIndexH << 8 ) | UsbSetupBuf->wIndexL )
                                {
                                case 0x82:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* 设置端点2 IN STALL */
                                    break;
                                case 0x02:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;/* 设置端点2 OUT Stall */
                                    break;
                                case 0x81:
                                    UEP1_CTRL = UEP1_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* 设置端点1 IN STALL */
                                    break;
                                default:
                                    errflag = 0xFF;                                     /* 操作失败 */
                                    break;
                                }
                            }
                            else
                            {
                                errflag = 0xFF;                                         /* 操作失败 */
                            }
                        }
                        else
                        {
                            errflag = 0xFF;                                             /* 操作失败 */
                        } 
                        break;
                    case USB_GET_STATUS:
                        Ep0Buffer[0] = 0x00;
                        Ep0Buffer[1] = 0x00;
                        if ( SetupLen >= 2 )
                        {
                            len = 2;
                        }
                        else
                        {
                            len = SetupLen;
                        }
                        break;
                    default:
                        errflag = 0xFF;                                                  //操作失败
                        break;
                    }
                }
            }
            else
            {
                errflag = 0xFF;                                                          //包长度错误
            }
            if(errflag == 0xFF)
            {
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;//STALL
            }
            else if(len <= THIS_ENDP0_SIZE)                                         //上传数据或者状态阶段返回0长度包
            {
                UEP0_T_LEN = len;
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;//默认数据包是DATA1，返回应答ACK
            }
            else
            {
                UEP0_T_LEN = 0;  //虽然尚未到状态阶段，但是提前预置上传0长度数据包以防主机提前进入状态阶段
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;//默认数据包是DATA1,返回应答ACK
            }
            break;
        case UIS_TOKEN_IN | 0:                                                      //endpoint0 IN
            switch(SetupReq)
            {
            case USB_GET_DESCRIPTOR:
            case HID_GET_REPORT:							
                len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen;     //本次传输长度
                memcpy( Ep0Buffer, pDescr, len );                                   //加载上传数据
                SetupLen -= len;
                pDescr += len;
                UEP0_T_LEN = len;
                UEP0_CTRL ^= bUEP_T_TOG;                                            //同步标志位翻转
                break;
            case USB_SET_ADDRESS:
                USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | SetupLen;
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            default:
                UEP0_T_LEN = 0;                                                      //状态阶段完成中断或者是强制上传0长度数据包结束控制传输
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            }
            break;
        case UIS_TOKEN_OUT | 0:  // endpoint0 OUT
            len = USB_RX_LEN;
            if(SetupReq == 0x09)
            {
                if(Ep0Buffer[0])
                {
                    printf("Light on Num Lock LED!\n");
                }
                else if(Ep0Buffer[0] == 0)
                {
                    printf("Light off Num Lock LED!\n");
                }
            }
            UEP0_CTRL ^= bUEP_R_TOG;                                     //同步标志位翻转		
            break;
        default:
            break;
        }
        UIF_TRANSFER = 0;                                                           //写0清空中断
    }
    else if(UIF_BUS_RST)                                                                 //设备模式USB总线复位中断
    {
        UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        UEP2_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        USB_DEV_AD = 0x00;
        UIF_SUSPEND = 0;
        UIF_TRANSFER = 0;
        Ready = 0;
		Endp2Busy = 0;
        UIF_BUS_RST = 0;                                                             //清中断标志
    }
    else if (UIF_SUSPEND)                                                                 //USB总线挂起/唤醒完成
    {
        UIF_SUSPEND = 0;
        if ( USB_MIS_ST & bUMS_SUSPEND )                                             //挂起
        {
#ifdef DE_PRINTF
            printf( "zz" );                                                          //睡眠状态
//             while ( XBUS_AUX & bUART0_TX )
//             {
//                 ;    //�ȴ��������
//             }
#endif
//             SAFE_MOD = 0x55;
//             SAFE_MOD = 0xAA;
//             WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO;                                   //USB或者RXD0有信号时可被唤醒
//             PCON |= PD;                                                               //睡眠
//             SAFE_MOD = 0x55;
//             SAFE_MOD = 0xAA;
//             WAKE_CTRL = 0x00;
        }
    }
    else {                                                                             //意外的中断,不可能发生的情况
        USB_INT_FG = 0xFF;                                                             //清中断标志
//      printf("UnknownInt  N");
    }
}

main()
{
    UINT8 i;

    CfgFsys( );                                                           //CH559时钟选择配置
    mDelaymS(5);                                                          //修改主频等待内部晶振稳定,必加	
    mInitSTDIO( );                                                        //串口0初始化
#ifdef DE_PRINTF
    printf("start ...\n");
#endif	
	UART1Init( );                                                           //串口1初始化
		memset(UserEp2Buf, 0, sizeof(UserEp2Buf));
    USBDeviceInit();                                                      //USB设备模式初始化
    EA = 1;                                                               //允许单片机中断		
    UEP1_T_LEN = 0;                                                       //预使用发送长度一定要清空
    UEP2_T_LEN = 0;                                                       //预使用发送长度一定要清空
    Ready = 0;
    while(1)
    {
        if(Ready)
        {
						// while(Endp2Busy);                                           //如果忙（上一包数据没有传上去），则等待。
            // Enp2BlukIn();
						if (Endp2Recv) { // USB收到数据，通过UART发出去
							for (i=0; i < recv_len; i++) {
								CH554UART1SendByte(Ep2Buffer[i]);                                     //并通过串口1发回去
							}
							Endp2Recv = 0;
						}
//						if (U1RI!=0) { // UART 收到数据(组织成包后)，通过USB发出去
//							dat = CH554UART1RcvByte();                                //程序死等，直到收到一个字节
//							printf("%d", dat);
//							if (usb_pkg_s == 0) {
//								if (dat == 0xff) { // 第一个数据是帧头
//									usb_pkg_s = 1; // 开始接受一个包
//									usb_pkg_i = 0;
//									memset(UserEp2Buf, 0, sizeof(UserEp2Buf));
//								} else {
//									usb_pkg_i = 0;
//								}
//							} else {
//								if (usb_pkg_i == 0) {
//									usb_pkg_len = dat; // 第二个数据是数据长度
//									if (usb_pkg_len > ENDP2_IN_SIZE) { // 无效
//										usb_pkg_s = 0;
//										usb_pkg_i = 0;
//									}
//								} else if (usb_pkg_i-1 >= usb_pkg_len) { // 最后一个数据是帧尾
//									if (dat == 0xfe) { // 帧尾正确才发送
//										while(Endp2Busy);
//										Enp2BlukIn();
//									}
//									usb_pkg_s = 0;
//									usb_pkg_i = 0;
//								} else {
//									UserEp2Buf[usb_pkg_i-1] = dat; // 填充数据
//								}
//								usb_pkg_i++;
//							}
//						}
        } else {
				mDelaymS(100);                                                 //模拟单片机做其它事
				}
        
    }
}

#if UART1_INTERRUPT
/*******************************************************************************
* Function Name  : UART1Interrupt(void)
* Description    : UART1 中断服务程序
*******************************************************************************/
		UINT8 dat;
		UINT8 usb_pkg_i = 0;
		UINT8 usb_pkg_s = 0;
		UINT8 usb_pkg_len = 0;
void UART1Interrupt( void ) interrupt INT_NO_UART1 using 1                       //串口1中断服务程序,使用寄存器组1
{
	if(U1RI)
	{
		dat = SBUF1;
		U1RI = 0;
		// CH554UART1SendByte(dat);
							if (usb_pkg_s == 0) {
								if (dat == 0xff) { // 第一个数据是帧头
									usb_pkg_s = 1; // 开始接受一个包
									usb_pkg_i = 0;
									memset(UserEp2Buf, 0, sizeof(UserEp2Buf));
								} else {
									usb_pkg_i = 0;
								}
							} else {
								if (usb_pkg_i == 0) {
									usb_pkg_len = dat; // 第二个数据是数据长度
									if (usb_pkg_len > ENDP2_IN_SIZE) { // 无效
										usb_pkg_s = 0;
										usb_pkg_i = 0;
									}
								} else if (usb_pkg_i-1 >= usb_pkg_len) { // 最后一个数据是帧尾
									if (dat == 0xfe) { // 帧尾正确才发送
										while(Endp2Busy);
										Enp2BlukIn();
									}
									usb_pkg_s = 0;
									usb_pkg_i = 0;
								} else {
									UserEp2Buf[usb_pkg_i-1] = dat; // 填充数据
								}
								usb_pkg_i++;
							}
	}
}
#endif
