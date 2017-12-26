#include "max7q.h"					   
#include "stdio.h"	 
#include "stdarg.h"	 
#include "string.h"	 
#include "math.h"  
#include "stm32l0xx_hal.h"
#include "usart.h"



uint8_t Gps_DISGLL[16]={0xB5,0x62,0x06,0x01,0x08,0x00,0xF0,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2A};
uint8_t Gps_DISGSA[16]={0xB5,0x62,0x06,0x01,0x08,0x00,0xF0,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x31};
uint8_t Gps_DISGSV[16]={0xB5,0x62,0x06,0x01,0x08,0x00,0xF0,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x38};
uint8_t Gps_DISVTG[16]={0xB5,0x62,0x06,0x01,0x08,0x00,0xF0,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x46};
//uint8_t Gps_STOPPED[16]={0xB5,0x62,0x06,0x57,0x08,0x00,0x01,0x00,0x00,0x00,0x50,0x4F,0x54,0x53,0xAC,0x85};
uint8_t Gps_POWSAVE[10]={0xB5,0x62,0x06,0x11,0x02,0x00,0x08,0x01,0x22,0x92};
uint8_t Gps_STOP[12]={0xB5,0x62,0x06,0x04,0x04,0x00,0x00,0x00,0x08,0x00,0x16,0x74};
uint8_t Gps_START[12]={0xB5,0x62,0x06,0x04,0x04,0x00,0x00,0x00,0x09,0x00,0x17,0x76};


//��buf����õ���cx���������ڵ�λ��
//����ֵ:0~0XFE,������������λ�õ�ƫ��.
//       0XFF,���������ڵ�cx������							  
u8 NMEA_Comma_Pos(u8 *buf,u8 cx)
{	 		    
	u8 *p=buf;
	while(cx)
	{		 
		if(*buf=='*'||*buf<' '||*buf>'z')return 0XFF;//����'*'���߷Ƿ��ַ�,�򲻴��ڵ�cx������
		if(*buf==',')cx--;
		buf++;
	}
	return buf-p;	 
}
//m^n����
//����ֵ:m^n�η�.
u32 NMEA_Pow(u8 m,u8 n)
{
	u32 result=1;	 
	while(n--)result*=m;    
	return result;
}
//strת��Ϊ����,��','����'*'����
//buf:���ִ洢��
//dx:С����λ��,���ظ����ú���
//����ֵ:ת�������ֵ
int NMEA_Str2num(u8 *buf,u8*dx)
{
	u8 *p=buf;
	u32 ires=0,fres=0;
	u8 ilen=0,flen=0,i;
	u8 mask=0;
	int res;
	while(1) //�õ�������С���ĳ���
	{
		if(*p=='-'){mask|=0X02;p++;}//�Ǹ���
		if(*p==','||(*p=='*'))break;//����������
		if(*p=='.'){mask|=0X01;p++;}//����С������
		else if(*p>'9'||(*p<'0'))	//�зǷ��ַ�
		{	
			ilen=0;
			flen=0;
			break;
		}	
		if(mask&0X01)flen++;
		else ilen++;
		p++;
	}
	if(mask&0X02)buf++;	//ȥ������
	for(i=0;i<ilen;i++)	//�õ�������������
	{  
		ires+=NMEA_Pow(10,ilen-1-i)*(buf[i]-'0');
	}
	if(flen>5)flen=5;	//���ȡ5λС��
	*dx=flen;	 		//С����λ��
	for(i=0;i<flen;i++)	//�õ�С����������
	{  
		fres+=NMEA_Pow(10,flen-1-i)*(buf[ilen+1+i]-'0');
	} 
	res=ires*NMEA_Pow(10,flen)+fres;
	if(mask&0X02)res=-res;		   
	return res;
}	  							 

//����GPGGA��Ϣ
//gpsx:nmea��Ϣ�ṹ��
//buf:���յ���GPS���ݻ������׵�ַ
void NMEA_GPGGA_Analysis(GPS_DATA *gpsx,u8 *buf)
{
	u8 *p1,dx;			 
	u8 posx;    
	p1=(u8*)strstr((const char *)buf,"$GPGGA");
	posx=NMEA_Comma_Pos(p1,6);								//�õ�GPS״̬
	if(posx!=0XFF)gpsx->Gps_Sta=NMEA_Str2num(p1+posx,&dx);	
	posx=NMEA_Comma_Pos(p1,7);								//�õ����ڶ�λ��������
	if(posx!=0XFF)gpsx->Posslnum=NMEA_Str2num(p1+posx,&dx); 
	posx=NMEA_Comma_Pos(p1,9);								//�õ����θ߶�
	if(posx!=0XFF)gpsx->Altitude=NMEA_Str2num(p1+posx,&dx);  
}

//����GPRMC��Ϣ
//gpsx:nmea��Ϣ�ṹ��
//buf:���յ���GPS���ݻ������׵�ַ
void NMEA_GPRMC_Analysis(GPS_DATA *gpsx,u8 *buf)
{
	u8 *p1,dx;			 
	u8 posx;     
	u32 temp;	    
	p1=(u8*)strstr((const char *)buf,"$GPRMC");
	posx=NMEA_Comma_Pos(p1,1);								//�õ�UTCʱ��
	if(posx!=0XFF)
	{
		temp=NMEA_Str2num(p1+posx,&dx)/NMEA_Pow(10,dx);	 	//�õ�UTCʱ��,ȥ��ms
		gpsx->UTC.hour=temp/10000;
		gpsx->UTC.min=(temp/100)%100;
		gpsx->UTC.sec=temp%100;	 	 
	}			  			  
	posx=NMEA_Comma_Pos(p1,3);								//�õ�γ��
	if(posx!=0XFF)
	{
		temp=NMEA_Str2num(p1+posx,&dx);		 	
		gpsx->Latitude=temp/NMEA_Pow(10,dx-2);
	}
	posx=NMEA_Comma_Pos(p1,4);								//��γ���Ǳ�γ 
	if(posx!=0XFF)gpsx->NS=*(p1+posx);					 
 	posx=NMEA_Comma_Pos(p1,5);								//�õ�����
	if(posx!=0XFF)
	{												  
		temp=NMEA_Str2num(p1+posx,&dx);		 	 
		gpsx->Longitude=temp/NMEA_Pow(10,dx-2);	
	}
	posx=NMEA_Comma_Pos(p1,6);								//������������
	if(posx!=0XFF)gpsx->EW=*(p1+posx);		 
	posx=NMEA_Comma_Pos(p1,9);								//�õ�UTC����
	if(posx!=0XFF)
	{
		temp=NMEA_Str2num(p1+posx,&dx);		 				//�õ�UTC����
		gpsx->UTC.date=temp/10000;
		gpsx->UTC.month=(temp/100)%100;
		gpsx->UTC.year=2000+temp%100;	 	 
	} 
}


void GPS_Analysis(GPS_DATA *gpsx,u8 *buf)
{
	NMEA_GPGGA_Analysis(gpsx,buf);	//GPGGA���� 	
	NMEA_GPRMC_Analysis(gpsx,buf);	//GPRMC����
}

//GPSУ��ͼ���
//buf:���ݻ������׵�ַ
//len:���ݳ���
//cka,ckb:����У����.
void Ublox_CheckSum(u8 *buf,u16 len,u8* cka,u8*ckb)
{
	u16 i;
	*cka=0;*ckb=0;
	for(i=0;i<len;i++)
	{
		*cka=*cka+buf[i];
		*ckb=*ckb+*cka;
	}
}

void GPS_Init(void)
{
	GPS_ON;
	HAL_Delay(100);
	Usart1SendData(Gps_DISGLL,16);
	HAL_Delay(10);
	Usart1SendData(Gps_DISGSA,16);
	HAL_Delay(10);
	Usart1SendData(Gps_DISGSV,16);
	HAL_Delay(10);
	Usart1SendData(Gps_DISVTG,16);
	HAL_Delay(50);
	Usart1SendData(Gps_STOP,12);
}


/* config gps modue output, only gprmc*/