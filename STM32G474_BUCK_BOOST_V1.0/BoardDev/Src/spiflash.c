/******************** (C) COPYRIGHT 2024 Robot ********************
* File:     spiflash.c
* Version:  V1.0
* Date:     2024/08/29
* Author:   Robot
*================================================================*/

/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "spiflash.h"

#define RETRYTIME				3
#define W25QXX_USE_MALLOC		0
/**
  * @brief  SPI_ReadWriteOneByte.
  * @param  TxData: spi to write
  * @retval uint8_t: read from spi
**/
static uint8_t SPI_ReadWriteOneByte(uint8_t TxData)
{	
	uint8_t RxData;
	
	HAL_SPI_TransmitReceive(&W25QXX_SPI, &TxData, &RxData, 1, 1000);
	
	return RxData;
}

/**
  * @brief  SPI_ReadInDMA.
  * @param  RxData: read in dma from spi, size: size of spi rx
  * @retval void
**/
static void SPI_ReadInDMA(uint8_t *RxData, uint16_t size)
{	
	HAL_StatusTypeDef errocode;
	uint8_t retryTime = RETRYTIME;
	
	do
	{
		errocode = HAL_SPI_Receive_DMA(&W25QXX_SPI, RxData, size);
		if(errocode == HAL_OK)
			break;
	}while(retryTime--);	
}

/**
  * @brief  SPI_WriteInDMA.
  * @param  TxData: write to spi, size: size of spi tx
  * @retval void
**/
static void SPI_WriteInDMA(uint8_t *TxData, uint16_t size)
{	
	HAL_StatusTypeDef errocode;
	uint8_t retryTime = RETRYTIME;
	
	do
	{
		errocode = HAL_SPI_Transmit_DMA(&W25QXX_SPI, TxData, size);
		if(errocode == HAL_OK)
			break;
	}while(retryTime--);	
}

/******************************************************************************
**  FUNCTION      : W25QXX_ReadID
**  DESCRIPTION   : W25Qxx Read DeviceID
**  PARAMETERS    : NA
**  RETURN        : uint16_t：DeviceID
******************************************************************************/
uint16_t W25QXX_ReadID( void )
{
	uint32_t temp=0;

	SPIFLASH_CS(0);  //拉低CS,开始传输
	SPI_ReadWriteOneByte( 0x90 );
	SPI_ReadWriteOneByte( 0x00 );
	SPI_ReadWriteOneByte( 0x00 );
	SPI_ReadWriteOneByte( 0x00 );

	temp |= ( SPI_ReadWriteOneByte( 0xFF )<<8 ); //高8位
	temp |= SPI_ReadWriteOneByte( 0xFF );     //低8位
	SPIFLASH_CS(1);  //拉高CS,结束传输

	return temp;
}

/******************************************************************************
**  FUNCTION      : W25QXX_WriteSR
**  DESCRIPTION   : W25Qxx SR Write
**  PARAMETERS    : sr: the data of to write W25QXX
**  RETURN        : NA
******************************************************************************/
void W25QXX_WriteSR( uint8_t sr )
{
	SPIFLASH_CS(0);                          //使能器件
	SPI_ReadWriteOneByte( W25X_WriteStatusReg ); //发送写取状态寄存器命令
	SPI_ReadWriteOneByte( sr );          //写入一个字节
	SPIFLASH_CS(1);                          //取消片选
}

/******************************************************************************
**  FUNCTION      : W25QXX_ReadSR
**  DESCRIPTION   : W25Qxx SR Read
**  PARAMETERS    : NA
**  RETURN        : uint8_t: the data of reading W25QXX
                    BIT7  6   5   4   3   2   1   0
                    SPR   RV  TB BP2 BP1 BP0 WEL BUSY
                    SPR:默认0,状态寄存器保护位,配合WP使用
                    TB,BP2,BP1,BP0:FLASH区域写保护设置
                    WEL:写使能锁定
                    BUSY:忙标记位(1,忙;0,空闲)
                    默认:0x00
******************************************************************************/
uint8_t W25QXX_ReadSR( void )
{
	uint8_t sr=0;

	SPIFLASH_CS(0);                          //使能器件
	SPI_ReadWriteOneByte( W25X_ReadStatusReg ); //发送读取状态寄存器命令
	sr = SPI_ReadWriteOneByte( 0x00 );         //读取一个字节
	SPIFLASH_CS(1);                          //取消片选
	return sr;      //返回sr
}

/******************************************************************************
**  FUNCTION      : W25QXX_WaitBusy
**  DESCRIPTION   : W25Qxx Wait Busy
**  PARAMETERS    : NA
**  RETURN        : NA
******************************************************************************/
void W25QXX_WaitBusy( void )
{
	while( ( W25QXX_ReadSR()&0x01 )==0x01 );    // 等待BUSY位清空
}

/******************************************************************************
**  FUNCTION      : W25QXX_PowerDown
**  DESCRIPTION   : W25Qxx Down Power
**  PARAMETERS    : NA
**  RETURN        : NA
******************************************************************************/
void W25QXX_Power_Down( void )
{
	SPIFLASH_CS(0);                           //使能器件
	SPI_ReadWriteOneByte( W25X_PowerDown ); //发送掉电命令
	SPIFLASH_CS(1);                           //取消片选
	SPIFLASH_Delay1us( 3 );                   //等待TPD
}

/******************************************************************************
**  FUNCTION      : W25QXX_PowerUp
**  DESCRIPTION   : W25Qxx Up Power
**  PARAMETERS    : NA
**  RETURN        : NA
******************************************************************************/
void W25QXX_Power_Up( void )
{
	SPIFLASH_CS(0);                              //使能器件
	SPI_ReadWriteOneByte( W25X_ReleasePowerDown ); //  send W25X_PowerDown command 0xAB
	SPIFLASH_CS(1);                              //取消片选
	SPIFLASH_Delay1us( 3 );                            //等待TRES1
}

/******************************************************************************
**  FUNCTION      : W25QXX_Write_Enable
**  DESCRIPTION   : W25Qxx Enable Write
**  PARAMETERS    : NA
**  RETURN        : NA
******************************************************************************/
void W25QXX_Write_Enable( void )
{
	SPIFLASH_CS(0);                  //使能器件
	SPI_ReadWriteOneByte( W25X_WriteEnable ); //发送写使能
	SPIFLASH_CS(1);                  //取消片选
}

/******************************************************************************
**  FUNCTION      : W25QXX_Write_Enable
**  DESCRIPTION   : W25Qxx Enable Write
**  PARAMETERS    : NA
**  RETURN        : NA
******************************************************************************/
void W25QXX_Write_Disable( void )
{
	SPIFLASH_CS(0);                          //使能器件
	SPI_ReadWriteOneByte( W25X_WriteDisable ); //发送写禁止指令
	SPIFLASH_CS(1);                          //取消片选
}

/******************************************************************************
**  FUNCTION      : W25QXX_ReadBuf
**  DESCRIPTION   : W25Qxx read buffer
**  PARAMETERS    : pBuffer:数据存储区
                    ReadAddr:开始读取的地址(24bit)
                    NumByteToRead:要读取的字节数(最大65535)
**  RETURN        : NA
******************************************************************************/
uint8_t W25QXX_ReadBuf( uint8_t *pBuffer,uint32_t ReadAddr,uint16_t NumByteToRead )
{


	SPIFLASH_CS(0);                              //使能器件
	SPI_ReadWriteOneByte( W25X_ReadData );            //发送读取命令
	SPI_ReadWriteOneByte( ( uint8_t )( ( ReadAddr )>>16 ) ); //发送24bit地址
	SPI_ReadWriteOneByte( ( uint8_t )( ( ReadAddr )>>8 ) );
	SPI_ReadWriteOneByte( ( uint8_t )ReadAddr );

	SPI_ReadInDMA( pBuffer, NumByteToRead);      

	SPIFLASH_CS(1);
	
	return 0;
}

uint32_t W25QXX_Read32bit(uint32_t ReadAddr)
{
	uint8_t data[4];
	
	W25QXX_ReadBuf(data,ReadAddr,4);
	
	return ((data[0]<<24) + (data[1]<<16) + (data[2]<<8) + data[3]);	
}

/******************************************************************************
**  FUNCTION      : W25QXX_WritePage
**  DESCRIPTION   : W25QXX Write Page
**  PARAMETERS    : pBuffer:数据存储区
                    WriteAddr:开始写入的地址(24bit)
                    NumByteToRead:要写入的字节数(最大256),该数不应该超过该页的剩余字节数!!!
**  RETURN        : NA
******************************************************************************/
uint8_t W25QXX_Write_Page( uint8_t *pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite )
{

	W25QXX_Write_Enable();                     		 //SET WEL
	SPIFLASH_CS(0);                              //使能器件
	SPI_ReadWriteOneByte( W25X_PageProgram );         //发送写页命令
	SPI_ReadWriteOneByte( ( uint8_t )( ( WriteAddr )>>16 ) ); //发送24bit地址
	SPI_ReadWriteOneByte( ( uint8_t )( ( WriteAddr )>>8 ) );
	SPI_ReadWriteOneByte( ( uint8_t )WriteAddr );

	SPI_WriteInDMA(pBuffer, NumByteToWrite);	//DMA 写

	SPIFLASH_CS(1);                              //取消片选
	W25QXX_WaitBusy();                          //等待写入结束
	
	return 0;
}

/******************************************************************************
**  FUNCTION      : W25QXX_EraseBlock
**  DESCRIPTION   : W25QXX Erase One Block
**  PARAMETERS    : Dst_Addr: Block addr
**  RETURN        : NA
******************************************************************************/
void W25QXX_Erase_Block( uint32_t Dst_Addr )
{
	//一个块为65536个字节即64K
#if DEBUG_EN
	printf( "\r\n正在擦写Flash的第 %d 个块......\r\n",Dst_Addr );   //监视falsh擦除情况,测试用
#endif
	Dst_Addr*=65536;
	W25QXX_Write_Enable();                      	//SET WEL
	W25QXX_WaitBusy();
	SPIFLASH_CS(0);                              //使能器件
	SPI_ReadWriteOneByte( W25X_BlockErase );         //发送块擦除指令
	SPI_ReadWriteOneByte( ( uint8_t )( ( Dst_Addr )>>16 ) ); //发送24bit地址
	SPI_ReadWriteOneByte( ( uint8_t )( ( Dst_Addr )>>8 ) );
	SPI_ReadWriteOneByte( ( uint8_t )Dst_Addr );
	SPIFLASH_CS(1);                              //取消片选
	W25QXX_WaitBusy();                         		 //等待擦除完成
}

/******************************************************************************
**  FUNCTION      : W25QXX_EraseSector
**  DESCRIPTION   : W25QXX Erase One Sector
**  PARAMETERS    : Dst_Addr: sector addr
**  RETURN        : NA
******************************************************************************/
void W25QXX_Erase_Sector( uint32_t Dst_Addr )
{
	//一个扇区为4096个字节即4K
#if DEBUG_EN
	printf( "\r\n正在擦写Flash的第 %d 个扇区......\r\n",Dst_Addr );   //监视falsh擦除情况,测试用
#endif
	Dst_Addr*=4096;
	W25QXX_Write_Enable();                      //SET WEL
	W25QXX_WaitBusy();
	SPIFLASH_CS(0);                              //使能器件
	SPI_ReadWriteOneByte( W25X_SectorErase );         //发送扇区擦除指令
	SPI_ReadWriteOneByte( ( uint8_t )( ( Dst_Addr )>>16 ) ); //发送24bit地址
	SPI_ReadWriteOneByte( ( uint8_t )( ( Dst_Addr )>>8 ) );
	SPI_ReadWriteOneByte( ( uint8_t )Dst_Addr );
	SPIFLASH_CS(1);                              //取消片选
	W25QXX_WaitBusy();                          //等待擦除完成
}

/******************************************************************************
**  FUNCTION      : W25QXX_Write_NoCheck
**  DESCRIPTION   : 无检验写SPI FLASH(必须确保所写的地址范围内的数据全部为0XFF,否则在非0XFF处写入的数据将失败!)
                    具有自动换页功能,在指定地址开始写入指定长度的数据,但是要确保地址不越界!
**  PARAMETERS    : pBuffer:数据存储区; WriteAddr:开始写入的地址(24bit); NumByteToWrite:要写入的字节数(最大65535)
**  RETURN        : void
******************************************************************************/
uint8_t W25QXX_Write_NoCheck( uint8_t *pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite )
{
	uint16_t pageremain;
	pageremain=256-WriteAddr%256; //单页剩余的字节数

	if( NumByteToWrite<=pageremain )pageremain=NumByteToWrite; //不大于256个字节

	while( 1 )
	{
		W25QXX_Write_Page( pBuffer,WriteAddr,pageremain );

		if( NumByteToWrite==pageremain )break; //写入结束了

		else //NumByteToWrite>pageremain
		{
			pBuffer+=pageremain;
			WriteAddr+=pageremain;

			NumByteToWrite-=pageremain;           //减去已经写入了的字节数

			if( NumByteToWrite>256 )pageremain=256; //一次可以写入256个字节

			else pageremain=NumByteToWrite;       //不够256个字节了
		}
	};
	
	return 0;
}

/******************************************************************************
**  FUNCTION      : W25QXX_Write
**  DESCRIPTION   : 写SPI FLASH,在指定地址开始写入指定长度的数据,该函数带擦除操作!
**  PARAMETERS    : pBuffer:数据存储区; WriteAddr:开始写入的地址(24bit); NumByteToWrite:要写入的字节数(最大65535)
**  RETURN        : void
******************************************************************************/
#if W25QXX_USE_MALLOC==0
uint8_t W25QXX_BUFFER[4096];
#endif

uint8_t W25QXX_WriteBuf( uint8_t *pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite )
{
	uint32_t secpos;
	uint16_t secoff;
	uint16_t secremain;
	uint16_t i;
	uint8_t *W25QXX_BUF;
	
#if (W25QXX_USE_MALLOC == 1)    //动态内存管理
	W25QXX_BUF = malloc( SRAMIN,4096 ); //申请内存
#else
	W25QXX_BUF=W25QXX_BUFFER;
#endif
	secpos=WriteAddr/4096;//扇区地址
	secoff=WriteAddr%4096;//在扇区内的偏移
	secremain=4096-secoff;//扇区剩余空间大小

	//printf("ad:%X,nb:%X\r\n",WriteAddr,NumByteToWrite);//测试用
	if( NumByteToWrite<=secremain )secremain=NumByteToWrite; //不大于4096个字节

	while( 1 )
	{
		W25QXX_ReadBuf( W25QXX_BUF,secpos*4096,4096 ); //读出整个扇区的内容

		for( i=0; i<secremain; i++ ) //校验数据
		{
			if( W25QXX_BUF[secoff+i]!=0XFF )break; //需要擦除
		}

		if( i<secremain ) //需要擦除
		{
			W25QXX_Erase_Sector( secpos );  //擦除这个扇区

			for( i=0; i<secremain; i++ )    //复制
			{
				W25QXX_BUF[i+secoff]=pBuffer[i];
			}

			W25QXX_Write_NoCheck( W25QXX_BUF,secpos*4096,4096 ); //写入整个扇区

		}

		else W25QXX_Write_NoCheck( pBuffer,WriteAddr,secremain ); //写已经擦除了的,直接写入扇区剩余区间.

		if( NumByteToWrite==secremain )break; //写入结束了

		else//写入未结束
		{
			secpos++;//扇区地址增1
			secoff=0;//偏移位置为0

			pBuffer+=secremain;  //指针偏移
			WriteAddr+=secremain;//写地址偏移
			NumByteToWrite-=secremain;              //字节数递减

			if( NumByteToWrite>4096 )secremain=4096; //下一个扇区还是写不完

			else secremain=NumByteToWrite;          //下一个扇区可以写完了
		}
	};

#if W25QXX_USE_MALLOC==1
	free( SRAMIN,W25QXX_BUF ); //释放内存
#endif
	
	return 0;
}

/******************************************************************************
**  FUNCTION      : W25QXX_ReadBuf
**  DESCRIPTION   : W25Qxx read buffer
**  PARAMETERS    : ReadAddr:读取的地址(24bit)
**  RETURN        : data:读出来的数
******************************************************************************/
uint8_t W25QXX_ReadByte( uint32_t ReadAddr )
{
	uint8_t data;

	SPIFLASH_CS(0);                              //使能器件
	SPI_ReadWriteOneByte( W25X_ReadData );            //发送读取命令
	SPI_ReadWriteOneByte( ( uint8_t )( ( ReadAddr )>>16 ) ); //发送24bit地址
	SPI_ReadWriteOneByte( ( uint8_t )( ( ReadAddr )>>8 ) );
	SPI_ReadWriteOneByte( ( uint8_t )ReadAddr );
	data=SPI_ReadWriteOneByte( 0x00 );    //读数
	SPIFLASH_CS(1);

	return data;
}

/******************************************************************************
**  FUNCTION      : W25QXX_WriteByte
**  DESCRIPTION   : W25QXX Write Byte
**  PARAMETERS    : data:要写入的字节
                    WriteAddr:要写入字节的地址(24bit)
**  RETURN        : 1:写入成功
******************************************************************************/
uint8_t W25QXX_WriteByte( uint8_t data,uint32_t WriteAddr )
{
//	uint32_t secpos;
//	uint8_t temp;

//	secpos=WriteAddr/4096;//扇区地址

//	temp = W25QXX_ReadByte( WriteAddr );

//	if( temp !=0XFF )
//	{
//		W25QXX_Erase_Sector( secpos );  //擦除这个扇区
//	}

	W25QXX_Write_Enable();                      //SET WEL
	SPIFLASH_CS(0);                          //使能器件
	SPI_ReadWriteOneByte( W25X_PageProgram );         //发送写页命令
	SPI_ReadWriteOneByte( ( uint8_t )( ( WriteAddr )>>16 ) ); //发送24bit地址
	SPI_ReadWriteOneByte( ( uint8_t )( ( WriteAddr )>>8 ) );
	SPI_ReadWriteOneByte( ( uint8_t )WriteAddr );
	SPI_ReadWriteOneByte( data ); //写数
	SPIFLASH_CS(1);                          //取消片选
	W25QXX_WaitBusy();                          //等待写入结束

	return 0;
}


/******************************************************************************
**  FUNCTION      : W25QXX_Erase_BinRegion
**  DESCRIPTION   : W25QXX Erase Bin’s region
**  PARAMETERS    : size:要写入bin的大小
**  RETURN        : 0:写入成功
******************************************************************************/
uint8_t W25QXX_Erase_BinRegion( uint32_t size )
{
	uint16_t i;
	uint8_t block;
	
	block = size/65536;		//计算bin所需要的块
	 
	for(i=1; i<block+2; i++)
	{
		W25QXX_Erase_Block(i);
	}
	
	return 0;
}

