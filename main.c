//ICC-AVR application builder : 2014-09-01 10:12:12
// Target : M128
// Crystal: 7.3728Mhz

#include <iom128v.h>
#include <macros.h>

unsigned char xbeeAddr[17] = {0x7E, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFE, 0x00, 0x00};

void send(void);
void Uart0_SendArray(unsigned char *buffer,unsigned char len);
void Uart1_SendArray(unsigned char *buffer,unsigned char len);
void DataSendXbee(unsigned char *p_data,unsigned int _length);

void port_init(void)
{
    unsigned char i,j;
    PORTA = 0x00;
    DDRA  = 0x00;
    DDRB  = 0xF0;
    PORTB = 0x00;
    PORTC = 0x00; //m103 output only
    DDRC  = 0xFF;
}

//UART0 initialize
// desired baud rate: 115200
// actual: baud rate:115200 (0.0%)
// char size: 8 bit
// parity: Disabled
void uart0_init(void)
{
    UCSR0B = 0x00; //disable while setting baud rate
    UCSR0A = 0x02;
    UCSR0C = 0x06;
    UBRR0L = 0x07; //set baud rate lo
    UBRR0H = 0x00; //set baud rate hi
    UCSR0B = 0x98;
}

unsigned char netcard_data[200] = {0};
unsigned char data_index_net = 0;
unsigned char netdata_length = 0;
#pragma interrupt_handler uart0_rx_isr:iv_USART0_RXC
void uart0_rx_isr(void)
{
    unsigned char data_length;

    netcard_data[data_index_net] = UDR0;

    if(data_index_net == 2) {
        data_length = (unsigned int)(netcard_data[1] << 8) + (unsigned int)netcard_data[2] + 4;
        xbeeAddr[1] = (unsigned char)((data_length+14) >> 8);
        xbeeAddr[2] = (unsigned char)(data_length+14);
    }

    data_index_net++;

    if(data_index_net == data_length) {
        unsigned char i;
        for(i = 5;i < 13;i++){
            xbeeAddr[i] = netcard_data[i-1];
        }
        DataSendXbee(netcard_data,data_length);
    }
}

void DataSendXbee(unsigned char *p_data,unsigned int _length)
{
    unsigned char cs = 0;
    unsigned char i;

    Uart1_SendArray(xbeeAddr,15);
    Uart1_SendArray(p_data,_length);

    for(i = 3; i < 17; i++)	{
		cs += xbeeAddr[i];
	}
	for(i = 0; i < _length; i++) {
        cs += p_data[i];
	}
	cs = 0xFF - cs;

	while(!(UCSR1A & (1<<UDRE1)));
    UDR1 = cs;


}
//UART1 initialize
// desired baud rate:115200
// actual baud rate:115200 (0.0%)
// char size: 8 bit
// parity: Disabled
void uart1_init(void)
{
    UCSR1B = 0x00; //disable while setting baud rate
    UCSR1A = 0x02;
    UCSR1C = 0x06;
    UBRR1L = 0x07; //set baud rate lo  115200
    UBRR1H = 0x00; //set baud rate hi
    UCSR1B = 0x98;
}

unsigned char data_index = 0;
unsigned int length;
unsigned char xbee[200] = {0};
#pragma interrupt_handler uart1_rx_isr:iv_USART1_RXC
void uart1_rx_isr(void)
{

//    send();
//    while(!(UCSR0A&(1<< RXC1)));
    xbee[data_index] = UDR1;

    if(data_index == 2)//第一二位为长度
    {
        length = (((unsigned int)xbee[1])<<8) + (unsigned int)xbee[2];
    }

    data_index++;

    if(data_index == length + 4)
    {
        if((length == 22) && (xbee[16] == 0xA0)) {  //中继节点发来的心跳信号，直接丢掉
        //xbee[100] = {0};
            data_index =0;
            length = 0;
            return;
        }

        else if((length == 49) && (xbee[16] == 0x21)) {  //中继节点发来的数据更新包，需要上传标签号、温度和电压
            unsigned char i;
            while(!(UCSR0A & (1<<UDRE0)));
            UDR0 = xbee[26]; //标签号
            for(i = 37;i <= 40;i++) {
                while(!(UCSR0A & (1<<UDRE0)));
                UDR0 = xbee[i];
            }
            data_index = 0;
            return;
        }
        else {
            unsigned int i;
            for(i = 15 ; i < length + 3 ; i ++ )
            {
                while(!(UCSR0A & (1<<UDRE0)));
                UDR0 = xbee[i];
            }
            data_index = 0;
        }
    }
    else if(data_index >= 199)
    {
        data_index = 0;
    }
}

void send(void)
{
    while(!(UCSR0A & (1<<UDRE0)));
    UDR0 = 0xDD;
}

void Uart0_SendArray(unsigned char *buffer,unsigned char len)  //从xbee收到数据转发到网卡
{
    char i;

    for(i=0; i<len; i++)
    {
        while((UCSR0A & (1<<UDRE0)) == 0);
        UDR0 = buffer[i];
    }
}

void Uart1_SendArray(unsigned char *buffer,unsigned char len)   //上位机传来数据用UART1发送到xbee
{
    char i;

    for(i=0; i<len; i++)
    {
        while((UCSR1A & (1<<UDRE1)) == 0);
        UDR1 = buffer[i];
    }
}

//call this routine to initialize all peripherals
void init_devices(void)
{
// unsigned char i,j;
//stop errant interrupts until set up
    CLI(); //disable all interrupts
    XDIV  = 0x00; //xtal divider
    XMCRA = 0x00; //external memory
    port_init();
    uart0_init();
    uart1_init();

    MCUCR = 0x00;
    EICRA = 0x00; //extended ext ints
    EICRB = 0x00; //extended ext ints
    EIMSK = 0x00;
    TIMSK = 0x00; //timer interrupt sources
    ETIMSK = 0x00; //extended timer interrupt sources
    SEI(); //re-enable interrupts
//all peripherals are now initialized
}

void main(void)
{
    unsigned int x,y;

    init_devices();
//    for(x = 0; x < 100; x++)
//        for(y = 0; y < 500; y++);
    PORTB &= ~(1 << 4);
    PORTC |= (1 << 4);
//    send();
    for(x = 0; x < 300; x++)
        for(y = 0; y < 500; y++)
            NOP();
    PORTC &= ~(1 << 4);
    PORTB |= (1 << 4);
    SEI();
//    send();

    while(1)
    {


  /*      for(x = 0; x < 500; x++)
            for(y = 0; y < 500; y++)
                NOP();

            send();
            */
    }
}
