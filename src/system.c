#include <stdint.h>

#include "system.h"
//#include "clp.h"

#include "uart_driver.h" //REMOVER FUTURAMENTE
#include "spi1.h"
#include "tmr2.h"
#include "EXT_INT/ext_int.h"
#include "EXT_INT/interrupt_manager.h"

#include "delay.h"

#include "I2C1/i2c1_driver.h"
#include "I2C3/i2c3_driver.h"

#include "RTC/DS1307.h"
#include "EEPROM/24LC256.h"
#include "RFID/as3993.h"

//#include "FSM_ESP8266.h"
//#include "FSM_TabelaDeEstacionamento.h"
#include "setup_usb.h"
//#include "FSM_DataHora.h"
//#include "FSM_KeepAlive.h"
//#include "FSM_EventosDePassagem.h"

#include "BSP/bsp.h"
#include "BSP/rfid_bsp.h"
#include "BSP/pin_manager.h"
#include "USR_TCP232.h"
#include "controleAcessoGaragem.h"


uint16_t contador100MiliSegundos = 0;
uint16_t contadorSegundos = 0;
uint16_t contadorMinutos = 0;


uint16_t counter_ms = 0;
uint16_t tickAlarme = 0;

void EX_INT1_CallBack(void){
     as3993Isr();
 }

volatile void tick(void){   

    contador100MiliSegundos = contador100MiliSegundos + 1;
    tickAlarme = tickAlarme + 1;
    
    if(tickAlarme > 300){
        tickAlarme = 0;
        trataAlarmeDoPortaoDeGaragem();
    }
    
//    if(contadorExecucaoMaquinaEstadosUSRTCP232 >= 1000){
//        contadorExecucaoMaquinaEstadosUSRTCP232 = 0;
//        executaMaquinaDeEstadosUSRTCP232();
//    }else{
//        contadorExecucaoMaquinaEstadosUSRTCP232++;
//    }
    
 
    
    if(contador100MiliSegundos > 100){    
        contador100MiliSegundos = 0;
        contadorSegundos = contadorSegundos + 1;
    
    }

    if(contadorSegundos >= 10){
        contadorSegundos = 0;
        contadorMinutos = contadorMinutos + 1;
        
    }
    
    if(contadorMinutos >= 60){
        contadorMinutos = 0;
               
    }
 
    //executaMaquinaDeEstados_TabelaDeEstacionamento();
    //executaMaquinaDeEstadosUSRTCP232();
    commandHandlerPortaUSB();
    //executaMaquinaDeEstados_EventosDePassagem();
}
    


uint16_t tick_getTimerCounter_ms(void){
    return contador100MiliSegundos;
}

uint16_t tick_getTimerCounter_seg(void){
    return contadorSegundos;
}

uint16_t tick_getTimerCounter_min(void){
    return contadorMinutos;
}

void AS3993_Init(void){
    
    uint16_t readerInitStatus;
    
    RFID_AS3993_load_callbacks(SPI1_Exchange8bitBuffer,
                               delay_ms,
                               delay_us,
                               EXT_INT_Enable,
                               EXT_INT_Disable,
                               EXT_INT_Clear,
                               BSP_RFID_setAS3993_enablePin,
                               BSP_RFID_AS3993_isEnabled,
                               BSP_RFID_setAS3993_SPI_enablePin);
       
    readerInitStatus = as3993Initialize(915000ULL);
    
    delay_ms(10);

    if(readerInitStatus){
        readerInitStatus = as3993Initialize(915000ULL);
    }
    
    as3993SetSensitivity(125);
}

void SYSTEM_Initialize(void){
    
    uint32_t baudrate, realrate;
    
    CNPU1bits.CN15PUE = 1; 
    CNPU2bits.CN16PUE = 1;
    CNPU2bits.CN19PUE = 1;
    CNPU4bits.CN49PUE = 1;
    CNPU4bits.CN56PUE = 1;
    CNPU4bits.CN57PUE = 1;
    CNPU2bits.CN28PUE = 1;
    
    INTCON1bits.NSTDIS = 1; //habilita o aninhamento de interrupcoes
    
    PIN_MANAGER_Initialize();
    
    //CLOCK_Initialize();
     
    INTERRUPT_Initialize();
    
    SPI1_Initialize();   
        
    TMR2_LoadInterruptCallback(tick);
    
    TMR2_Initialize();
   
    i2c1_driver_init(100000); // Inicializa o RTC com 100KHz
    
    i2c3_driver_init(100000); // Inicializa a EEPROM com 100 KHz
    
    baudrate = 115200ULL;   
    
    uartTxInitialize(SYSCLK, baudrate, &realrate); 
    
    uart2TxInitialize(SYSCLK, baudrate, &realrate); // ethernet
    
    uart3TxInitialize(SYSCLK, baudrate, &realrate); // usb
    
    uart4TxInitialize(SYSCLK, baudrate, &realrate); // wifi        
    
    EXT_INT_Initialize();
    
    delay_ms(10); // Tempo para os periféricos estabilizarem.
    
    // Carrega as funcoes da microchip para a biblioteca de abstracao
    RTC_DS1307_load_callbacks(  i2c1_driver_start,
                                i2c1_driver_restart,
                                i2c1_driver_stop,
                                i2c1_driver_TXData,
                                i2c1_driver_getRXData,
                                i2c1_driver_sendAck,                                
                                delay_ms);
    
    RTC_DS1307_I2C_config();   
    
    // Carrega as funcoes da microchip para a biblioteca de abstracao
    EEPROM_24LC256_load_callbacks(i2c3_driver_start,
                                  i2c3_driver_restart,
                                  i2c3_driver_stop,
                                  i2c3_driver_TXData,
                                  i2c3_driver_getRXData,
                                  i2c3_driver_sendAck,
                                  delay_ms);
    
    AS3993_Init();    
    
    loadLogCallbackFunction(uart3Tx);
}
