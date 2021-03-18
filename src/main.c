#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "FSM_DataHora.h"
#include "FSM_Ethernet.h"
#include "FSM_ESP8266.h"
#include "FSM_TabelaDeEstacionamento.h"
#include "FSM_KeepAlive.h"
#include "setup_usb.h"

#include "system.h"
#include "BSP/pin_manager.h"
#include "BSP/bsp.h"
#include "BSP/rfid_bsp.h"

#include "tabelaEstacionamento.h"
#include "tmr2.h"
#include "log.h"
#include "delay.h"

#include "RTC/DS1307.h"  
#include "EEPROM/24LC256.h"
#include "FLASH/flash.h"
#include "RFID/as3993.h"
#include "RFID/gen2.h"

#ifdef __PIC24FJ256DA210__
_CONFIG1(WDTPS_PS1 & FWPSA_PR32 & ALTVREF_ALTVREDIS & WINDIS_OFF & FWDTEN_OFF & ICS_PGx1 & GWRP_OFF & GCP_ON & JTAGEN_OFF)
_CONFIG2(POSCMOD_HS & IOL1WAY_OFF & OSCIOFNC_OFF & FCKSM_CSDCMD & FNOSC_PRIPLL & PLL96MHZ_ON & PLLDIV_DIV3 & IESO_OFF)
_CONFIG3(WPFP_WPFP0 & SOSCSEL_LPSOSC & WUTSEL_LEG & ALTPMP_ALPMPDIS & WPDIS_WPDIS & WPCFG_WPCFGDIS & WPEND_WPENDMEM)
#endif 

/////////////////////////////////////////////
// Green list criada a partir de tabelaEstacionamento.h
/////////////////////////////////////////////
    TabelaDeEpcDeEstacionamento __attribute__((far)) listaDeVeiculosLiberados;     // lista principal ordenada com as tags lidas
    TabelaDeEpcDeEstacionamento __attribute__((far)) listaDeVeiculosLiberadosTest; // utilizada para check na gravacao de dados na flash
    TabelaDeEpcDeEstacionamento __attribute__((far)) listaDeVeiculosLidosDuranteMovimento_Entrada; // lista desordenada com as tags em movimento
    TabelaDeEpcDeEstacionamento __attribute__((far)) listaDeVeiculosLidosDuranteMovimento_Saida; // lista desordenada com as tags em movimento
    
/////////////////////////////////////////////
// RTC DS1307 library variables
/////////////////////////////////////////////
    Calendar calendar;
    RTC_DS1307_stats RTC_DS1307_status;
    // Quando precisar ler o calendario do RTC, usar a linha abaixo:
    // status = RTC_DS1307_I2C_read_calendar( &calendar ); 

/////////////////////////////////////////////
// RTC DS1307 library variables
/////////////////////////////////////////////
    EEPROM_24LC256_stats EEPROM_24LC256_status;
    // Para leitura e escrita na eeprom, usar linhas abaixo  
    //EEPROM_24LC256_I2C_write_uchar(0,200,0x55);
    //EEPROM_24LC256_I2C_read_uchar(0,200,&var);

void marsOne_init(void);
uint8_t realizaLeituraDeAntena(uint8_t antena);
int8_t obtemDadosDaMemoriaFLASH(void);

void marsOne_init(void){
        
    ////////////////////////////////////////////////////////
    // Rotina de inicializacao visual do leitor
    ////////////////////////////////////////////////////////
    
    BSP_setBuzzer(OFF);
    BSP_setLed(NUM_MAX_LEDS,OFF);
    BSP_setRelay(NUM_MAX_RELAYS,OFF);
    
    BSP_setBuzzer(ON);
    BSP_setLed(NUM_MAX_LEDS,ON);

    delay_ms (1000);

    BSP_setBuzzer(OFF);
    BSP_setLed(NUM_MAX_LEDS,OFF);
    
    BSP_RS485_setDirection(RS485_INPUT);        
}

int8_t obtemDadosDaMemoriaFLASH(void){
    obtemListaDeVeiculosLiberados();
    return 0;
}

uint8_t realizaLeituraDeAntena(uint8_t antena){
    BSP_RFID_selectAntenna(antena);        
    return BSP_RFID_searchForTags();
}

int8_t verificaTagValida(uint8_t *tag){
    uint8_t i;
    uint8_t *p = tag;
    
    if( *tag == 0x30 ){
        tag += 3;
        for( i = 3; i < EPCLENGTH-1; i++){            
            if( *tag != 0x00 )
            {
                return -1;
            }
            else{
                tag++;
            }
        }
    }
    else{
        return -1;
    }
    
    tag = p;
    
    return 1;
}

EPC_Estacionamento epcLido;

#define MOVIMENTO_SAIDA 0
#define MOVIMENTO_ENTRADA 1
#define MOVIMENTO_ENTRADA_SAIDA 2
#define MOVIMENTO_SAIDA_ENTRADA 3

#define SAIDA_PORTAO_ENTRADA RELAY_1
#define SAIDA_ALARME_PORTAO_ENTRADA RELAY_3
#define ENTRADA_SENSOR_FECHAMENTO_PORTAO_ENTRADA INPUT_1
#define ENTRADA_SENSOR_BARREIRA_PORTAO_ENTRADA INPUT_3

#define SAIDA_PORTAO_SAIDA RELAY_2
#define SAIDA_ALARME_PORTAO_SAIDA RELAY_4
#define ENTRADA_SENSOR_FECHAMENTO_PORTAO_SAIDA INPUT_2
#define ENTRADA_SENSOR_BARREIRA_PORTAO_SAIDA INPUT_4

int8_t adicionaNovaTagNaLista(TabelaDeEpcDeEstacionamento *lista, EPC_Estacionamento tag){
    if(!buscarRegistroNaTabelaDeEpcDeEstacionamentoDesordenada(lista, tag)){
        adicionarRegistroNaTabelaDeEpcDeEstacionamento(lista, tag);
    }
    return 0;
}

int main(void){
            
    SYSTEM_Initialize();
    
    marsOne_init();         
     
    obtemDadosDaMemoriaFLASH();
    
    obtemParametrosDaMemoriaEEPROM();
    
    inicializaMaquinaDeEstados_ESP8266();   
    
    inicializaMaquinaDeEstados_TabelaDeEstacionamento();
    
    inicializaMaquinaDeEstados_DataHora();
    
    inicializaMaquinaDeEstados_KeepAlive(); 
                    
    while(1){
       
    }
    
    return 0;
}
