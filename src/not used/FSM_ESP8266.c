///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FINITE STATE MACHINE - MÁQUINA DE ESTADOS FINITOS PARA EVENTOS DE EMPILHADEIRA
// EMPRESA: CONTROL + PLUS
// AUTOR: Vitor M. dos S. Alho
// DATA: 07/02/2020
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <uart.h>
#include "FSM_Ethernet.h"
#include "delay.h"
#include "FSM_DataHora.h"
#include "FSM_ESP8266.h"
#include "uart_driver.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <time.h>
#include "BSP/pin_manager.h"
#include "setup_usb.h"
#include "delay.h"
#include "RTC/rtc.h"
#include "RTC/DS1307.h"
#include "BSP/bsp.h"
#include "clp.h"

#define SIM 1
#define NAO 0

#define LIGADO 1
#define DESLIGADO 0

#define CONECTADO 1
#define DESCONECTADO 0

#define NUM_MAX_ERROS_TIMEOUT 1

#define NUM_MAX_ERROS_MAQUINA_TIMEOUT 2

#define TEMPO_ENTRE_ESTADOS_FSM_ESP8266 100

#define TEMPO_AGUARDANDO_ACK_ESP8266 500

#define TEMPO_AGUARDANDO_ACK_MSG_WIFI_ESP8266 10000 //20000 //MARCOS - DIMINUI TEMPO DE TENTATIVA DE CONEX�O.

enum estadosDaMaquina{
        AGUARDANDO_TAREFA=0,
        VERIFICA_SE_JA_INICIALIZOU,
        
        RESET_MODULO_WIFI,          
        AGUARDANDO_ACK_0,
        
        ENVIAR_COMANDO_AT,
        AGUARDANDO_ACK_1,
        
        ENVIAR_COMANDO_AT0,
        AGUARDANDO_ACK_2,
        
        ENVIAR_COMANDO_RFPOWER,
        AGUARDANDO_ACK_3,
        
        ENVIAR_COMANDO_CIPMUX,
        AGUARDANDO_ACK_4, 
        
        ENVIAR_COMANDO_CWMODE,
        AGUARDANDO_ACK_5,
        
        ENVIAR_COMANDO_CIPMODE,
        AGUARDANDO_ACK_6,
        
        ENVIAR_COMANDO_CWJAP,
        AGUARDANDO_ACK_7,
        
        ENVIAR_COMANDO_CIPSTART,
        AGUARDANDO_ACK_8,
        
        ENVIAR_COMANDO_CIPSEND,
        AGUARDANDO_ACK_9,
        
        FIM_CICLO
}estados_ESP8266;

enum{
    PRIMARIO = 0,
    SECUNDARIO
}servidorConectado;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VARIAVEL: maquinaDeEstadosLiberada_ESP8266
// UTILIZADA EM: inicializaMaquinaDeEstados_ESP8266
// FUNÇÃO: saber quando a maquina de estado está liberada para executar
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    unsigned char maquinaDeEstadosLiberada_ESP8266  = 0;   
    
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VARIAVEL: delay
// UTILIZADA EM: executaMaquinaDeEstados_ESP8266
// FUNÇÃO: causar um delay no envio de mensagens no estadoAtual = ENVIAR_MENSAGEM_1
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    unsigned int delayExecucao_ESP8266 = 0;
    
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VARIAVEL: leitorAcabouDeLigar_ESP8266
// UTILIZADA EM: inicializaMaquinaDeEstados_ESP8266
// FUNÇÃO: armazena se o leitor acabou de ser ligado. 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////    
    unsigned char leitorAcabouDeLigar_ESP8266 = SIM;
    
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VARIAVEL: estadoAtual
// UTILIZADA EM: executaMaquinaDeEstados_ESP8266
// FUNÇÃO: armazena o estado atual da maquina de estado
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    unsigned char estadoAtual_ESP8266 = AGUARDANDO_TAREFA;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VARIAVEL: estadoAnterior
// UTILIZADA EM: executaMaquinaDeEstados_ESP8266
// FUNÇÃO: armazena o estado anterior da maquina de estado
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    unsigned char estadoAnterior_ESP8266 = -1;
    
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VARIAVEL: stringMensagemESP8266
// UTILIZADA EM: executaMaquinaDeEstados_ESP8266
// FUNÇÃO: armazena a string do evento que é enviado pela serial
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    char stringMensagemESP8266[300];
    
    char ESP8266_inicializado = 0;
    
    char ipServidorPrimario[15] = {"10.159.158.10"};
    char portaServidorPrimario[5] = {"6000"};
    
    char ipServidorSecundario[15] = {"10.159.158.10"};
    char portaServidorSecundario[5] = {"6100"};
    
    char *ipServidorEmUso;
    int portaServidorEmUso;
    
    int errosDeTimeout = 0;
    char servidorEmUso = PRIMARIO;
    
    char statusConexaoWifi_ESP8266 = DESCONECTADO;
    
    int contadorDelayLeds=0;

    //Teste conexao WiFi entre APs - 11/03/2020 - ArcelorMittal Juiz de Fora
    
    int numVezesReconectou = 0;
    
    Calendar dataHora_ESP8266;
    
    char stringLogWifi[300];
    
    char wifiBusy = NAO;
    
    
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////      FUNÇÕES      /////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////    

    char ESP8266_jaFoiInicializado(void){
        return ESP8266_inicializado;
    }
    
    void habilitaMaquinaDeEstados_ESP8266(void){
            //enviarMensagem = ENVIAR_MENSAGEM_1;
            maquinaDeEstadosLiberada_ESP8266 = SIM;
            
        }

    void bloqueiaMaquinaDeEstados_ESP8266(void){
            maquinaDeEstadosLiberada_ESP8266 = 0;
        }

    void limpaBufferNaMaquinaDeEstados_ESP8266(void)  {
            int i=0;
            for(i=0;i<sizeof(bufferInterrupcaoUART4);i++){
                bufferInterrupcaoUART4[i]=0;
            }
        }

    void inicializaMaquinaDeEstados_ESP8266(void){    
        if(leitorAcabouDeLigar_ESP8266){
            leitorAcabouDeLigar_ESP8266 = 0;    
            //ipServidorEmUso = ipServidorPrimario;
            //portaServidorEmUso = portaServidorPrimario;
            ipServidorEmUso = ipRemotoPrincipal;
            portaServidorEmUso = portaRemotaPrincipal;
        }
        habilitaMaquinaDeEstados_ESP8266();
    }
    
     void incrementaContadorLeds_FSM_ESP8266(void){
        contadorDelayLeds++;
    }
     
     void zeraContadorLeds_FSM_ESP8266(void){
        contadorDelayLeds=0;
    }
    
    void incrementaContadorExecucao_FSM_ESP8266(void){
        delayExecucao_ESP8266++;
    }
    
    void zeraContadorExecucao_FSM_ESP8266(void){
        delayExecucao_ESP8266 = 0;
    }   
  
    void resetarErrosDeTimeoutNoWifi(void){
        errosDeTimeout = 0;
    }
    
    void ocorreuErroDeTimeoutNoWifi(void){
        if(ESP8266_jaFoiInicializado()){
            errosDeTimeout++;
            if(errosDeTimeout >= NUM_MAX_ERROS_TIMEOUT){
                errosDeTimeout = 0;
                ESP8266_inicializado = 0;    
                
                switch(servidorEmUso){
                    case PRIMARIO:
                        trocarIpDeServidor(SECUNDARIO);
                        break;
                    case SECUNDARIO:
                        trocarIpDeServidor(PRIMARIO);
                        break;
                }   
                inicializaMaquinaDeEstados_ESP8266();
            }
        }
    }
    
    void ocorreuErroDeTimeoutNoWifiDentroDaMaquinaDeEstados(void){
       
        errosDeTimeout++;
        if(errosDeTimeout >= NUM_MAX_ERROS_MAQUINA_TIMEOUT){
            errosDeTimeout = 0;
            ESP8266_inicializado = 0;               
            
            switch(servidorEmUso){
                case PRIMARIO:
                    trocarIpDeServidor(SECUNDARIO);
                    break;
                case SECUNDARIO:
                    trocarIpDeServidor(PRIMARIO);
                    break;
            }   
            inicializaMaquinaDeEstados_ESP8266();
        }        
    }
    
    unsigned int solicitarTrocaDeServidor = 0;
    
    void trocarIpDeServidor(char servidor){  
        
        solicitarTrocaDeServidor = SIM;
        switch(servidor){
            case PRIMARIO:
                servidorEmUso = PRIMARIO;
                //ipServidorEmUso = ipServidorPrimario;
                //portaServidorEmUso = portaServidorPrimario;
                ipServidorEmUso = ipRemotoPrincipal;
                portaServidorEmUso = portaRemotaPrincipal;
            break;
            case SECUNDARIO: 
                servidorEmUso = SECUNDARIO;
                //ipServidorEmUso = ipServidorSecundario;
                //portaServidorEmUso = portaServidorSecundario;
                ipServidorEmUso = ipRemotoSecundario;
                portaServidorEmUso = portaRemotaSecundaria;
            break;
        }     
        
    }
    
    extern unsigned int debugInterfaceWifi_Silent;    
    unsigned int wifiLiberadoParaUso = 0;
    
    void executaMaquinaDeEstados_ESP8266(void){     
                        
        incrementaContadorLeds_FSM_ESP8266();
        incrementaContadorExecucao_FSM_ESP8266();
        
        if(contadorDelayLeds>200 && statusConexaoWifi_ESP8266 == DESCONECTADO){
            contadorDelayLeds = 0;
            LED_ZIG_Toggle();
        }
        if(statusConexaoWifi_ESP8266 == CONECTADO){
            LED_ZIG_SetHigh();
        }
        
        switch(estadoAtual_ESP8266)
        {
            case AGUARDANDO_TAREFA:   
                if(delayExecucao_ESP8266 > TEMPO_ENTRE_ESTADOS_FSM_ESP8266){  
                    zeraContadorExecucao_FSM_ESP8266();
                   
                    if(maquinaDeEstadosLiberada_ESP8266){ 
                        if(solicitarTrocaDeServidor){
                            estadoAtual_ESP8266 = ENVIAR_COMANDO_CIPSTART;  
                        }
                        else{
                            estadoAtual_ESP8266 = VERIFICA_SE_JA_INICIALIZOU;                              
                        }
                        estadoAnterior_ESP8266 = AGUARDANDO_TAREFA; 
                    }                           
                }
            break;
            case VERIFICA_SE_JA_INICIALIZOU: 

                if(delayExecucao_ESP8266 > TEMPO_ENTRE_ESTADOS_FSM_ESP8266){
                    zeraContadorExecucao_FSM_ESP8266();
                    
                    if(maquinaDeEstadosLiberada_ESP8266){  
                        if(!ESP8266_inicializado){                   
                            bloqueiaMaquinaDeEstados_KeepAlive(); // MARCOS - Bloquei KeepAlive
                            statusConexaoWifi_ESP8266 = DESCONECTADO;                             
                            estadoAtual_ESP8266 = RESET_MODULO_WIFI;
                            estadoAnterior_ESP8266 = VERIFICA_SE_JA_INICIALIZOU; 
                        }         
                        else{
                            estadoAtual_ESP8266 = AGUARDANDO_TAREFA;
                            estadoAnterior_ESP8266 = VERIFICA_SE_JA_INICIALIZOU; 
                        }
                    }                     
                }
            break;
            
            case RESET_MODULO_WIFI:
                
                if(delayExecucao_ESP8266 > TEMPO_ENTRE_ESTADOS_FSM_ESP8266){
                    zeraContadorExecucao_FSM_ESP8266();

                    if(maquinaDeEstadosLiberada_ESP8266){ 
                        sprintf(stringMensagemESP8266,"+++"); // sai para modo AT
                        escreveMensagemESP8266(stringMensagemESP8266);
                        delay_ms(50);
                        limpaBufferNaMaquinaDeEstados_ESP8266();
                        sprintf(stringMensagemESP8266,"AT+RST\r\n");
                        escreveMensagemESP8266(stringMensagemESP8266);
                      
                        estadoAtual_ESP8266=AGUARDANDO_ACK_0;
                        estadoAnterior_ESP8266=RESET_MODULO_WIFI;      
                    }                     
                }
            break;            
            case AGUARDANDO_ACK_0:
                if(delayExecucao_ESP8266 < TEMPO_AGUARDANDO_ACK_ESP8266 + 5000){
                
                    if(maquinaDeEstadosLiberada_ESP8266){
                       if(strstr(bufferInterrupcaoUART4,"ready")!=0){          
                             limpaBufferNaMaquinaDeEstados_ESP8266();
                             zeraContadorExecucao_FSM_ESP8266(); //MARCOS - ZEREI PARA REINICIAR PARA A PROXIMA FUN��O
                             estadoAtual_ESP8266 = ENVIAR_COMANDO_AT;
                             estadoAnterior_ESP8266 = AGUARDANDO_ACK_0; 
                         }
                    }  
                }
                else{
                    zeraContadorExecucao_FSM_ESP8266();
                    estadoAtual_ESP8266=AGUARDANDO_TAREFA;
                    estadoAnterior_ESP8266=AGUARDANDO_ACK_0;

                    sprintf(stringMensagemESP8266,"+++"); // sai para modo AT
                    escreveMensagemESP8266(stringMensagemESP8266);
                }            
            break;  
                       
            case ENVIAR_COMANDO_AT:  // AT COMMAND
                
                if(delayExecucao_ESP8266 > TEMPO_ENTRE_ESTADOS_FSM_ESP8266){
                    zeraContadorExecucao_FSM_ESP8266();         
                    
                    if(maquinaDeEstadosLiberada_ESP8266){                 
                        sprintf(stringMensagemESP8266,"AT\r\n");
                        escreveMensagemESP8266(stringMensagemESP8266);
                     
                        estadoAtual_ESP8266=AGUARDANDO_ACK_1;
                        estadoAnterior_ESP8266=ENVIAR_COMANDO_AT;                        
                    } 
                }
                
            break;           
            case AGUARDANDO_ACK_1:                    
                if(delayExecucao_ESP8266 < TEMPO_AGUARDANDO_ACK_ESP8266){
                    
                    if(maquinaDeEstadosLiberada_ESP8266){
                       if(strstr(bufferInterrupcaoUART4,"OK")!=0){          
                             limpaBufferNaMaquinaDeEstados_ESP8266();
                             zeraContadorExecucao_FSM_ESP8266(); //MARCOS - ZEREI PARA REINICIAR PARA A PROXIMA FUN��O
                             estadoAtual_ESP8266 = ENVIAR_COMANDO_AT0;
                             estadoAnterior_ESP8266 = AGUARDANDO_ACK_1; 
                         }
                    }  
                }
                else{
                    zeraContadorExecucao_FSM_ESP8266();
                    estadoAtual_ESP8266=AGUARDANDO_TAREFA;
                    estadoAnterior_ESP8266=AGUARDANDO_ACK_1;
                    
                    sprintf(stringMensagemESP8266,"+++");
                    escreveMensagemESP8266(stringMensagemESP8266);
                }
            break;
            
            case ENVIAR_COMANDO_AT0:  
                
                if(delayExecucao_ESP8266 > TEMPO_ENTRE_ESTADOS_FSM_ESP8266){
                    zeraContadorExecucao_FSM_ESP8266();     
                    
                    if(maquinaDeEstadosLiberada_ESP8266){                 
                        
                        //sprintf(stringMensagemESP8266,"AT+CWMODE_CUR=1\r\n"); 
                        sprintf(stringMensagemESP8266,"ATE0\r\n");                     
                        escreveMensagemESP8266(stringMensagemESP8266);
                     
                        //estadoAtual_ESP8266=AGUARDANDO_ACK_2;
                        estadoAtual_ESP8266=AGUARDANDO_ACK_2;
                        estadoAnterior_ESP8266=ENVIAR_COMANDO_AT0;                        
                    } 
                }
                
            break;          
            case AGUARDANDO_ACK_2:                    
                if(delayExecucao_ESP8266 < TEMPO_AGUARDANDO_ACK_ESP8266){
                    if(maquinaDeEstadosLiberada_ESP8266){
                       if(strstr(bufferInterrupcaoUART4,"OK")!=0){           
                             limpaBufferNaMaquinaDeEstados_ESP8266();
                             zeraContadorExecucao_FSM_ESP8266(); //MARCOS - ZEREI PARA REINICIAR PARA A PROXIMA FUN��O
                             estadoAtual_ESP8266 = ENVIAR_COMANDO_RFPOWER;
                             estadoAnterior_ESP8266 = AGUARDANDO_ACK_2; 
                         }
                    }  
                }
                else{
                    zeraContadorExecucao_FSM_ESP8266();
                    estadoAtual_ESP8266=AGUARDANDO_TAREFA;
                    estadoAnterior_ESP8266=AGUARDANDO_ACK_2;                                          
                }
            break;
            
            case ENVIAR_COMANDO_RFPOWER:  
                
                if(delayExecucao_ESP8266 > TEMPO_ENTRE_ESTADOS_FSM_ESP8266){
                    zeraContadorExecucao_FSM_ESP8266();     
                    
                    if(maquinaDeEstadosLiberada_ESP8266){                 
                                                
                        //sprintf(stringMensagemESP8266,"AT+CWAUTOCONN=0\r\n"); 
                        sprintf(stringMensagemESP8266,"AT+RFPOWER=82\r\n");
                        escreveMensagemESP8266(stringMensagemESP8266);
                     
                        estadoAtual_ESP8266=AGUARDANDO_ACK_3;
                        estadoAnterior_ESP8266=ENVIAR_COMANDO_RFPOWER;                        
                    } 
                }
                
            break;
            case AGUARDANDO_ACK_3:                    
                if(delayExecucao_ESP8266 < TEMPO_AGUARDANDO_ACK_ESP8266){
                    if(maquinaDeEstadosLiberada_ESP8266){
                       if(strstr(bufferInterrupcaoUART4,"OK")!=0){           
                             limpaBufferNaMaquinaDeEstados_ESP8266(); 
                             zeraContadorExecucao_FSM_ESP8266(); //MARCOS - ZEREI PARA REINICIAR PARA A PROXIMA FUN��O
                             estadoAtual_ESP8266 = ENVIAR_COMANDO_CIPMUX;
                             estadoAnterior_ESP8266 = AGUARDANDO_ACK_3; 
                        }
                    }  
                }
                else{
                    zeraContadorExecucao_FSM_ESP8266();
                    estadoAtual_ESP8266=AGUARDANDO_TAREFA;
                    estadoAnterior_ESP8266=AGUARDANDO_ACK_3;                                          
                }
            break;
            
            case ENVIAR_COMANDO_CIPMUX:  
                
                if(delayExecucao_ESP8266 > TEMPO_ENTRE_ESTADOS_FSM_ESP8266){
                    zeraContadorExecucao_FSM_ESP8266();     
                    
                    if(maquinaDeEstadosLiberada_ESP8266){                 
                        limpaBufferNaMaquinaDeEstados_ESP8266(); 
                        sprintf(stringMensagemESP8266,"AT+CIPMUX=0\r\n");        
                        escreveMensagemESP8266(stringMensagemESP8266);
                     
                        estadoAtual_ESP8266=AGUARDANDO_ACK_4;
                        estadoAnterior_ESP8266=ENVIAR_COMANDO_CIPMUX;                        
                    } 
                }
                
            break;
            case AGUARDANDO_ACK_4:                    
                if(delayExecucao_ESP8266 < TEMPO_AGUARDANDO_ACK_ESP8266){
                    if(maquinaDeEstadosLiberada_ESP8266){
                        if((strstr(bufferInterrupcaoUART4,"OK")!=0) || (strstr(bufferInterrupcaoUART4,"link is builded")!=0)){           
                            limpaBufferNaMaquinaDeEstados_ESP8266();
                            zeraContadorExecucao_FSM_ESP8266(); //MARCOS - ZEREI PARA REINICIAR PARA A PROXIMA FUN��O
                            estadoAtual_ESP8266 = ENVIAR_COMANDO_CWMODE;
                            estadoAnterior_ESP8266 = AGUARDANDO_ACK_4; 
                        }
                    }
                }
                else{
                    zeraContadorExecucao_FSM_ESP8266();
                    estadoAtual_ESP8266=AGUARDANDO_TAREFA;
                    estadoAnterior_ESP8266=AGUARDANDO_ACK_4;     
                }
            break;
            
            case ENVIAR_COMANDO_CWMODE:  
                
                if(delayExecucao_ESP8266 > TEMPO_ENTRE_ESTADOS_FSM_ESP8266){
                    zeraContadorExecucao_FSM_ESP8266();     
                    
                    if(maquinaDeEstadosLiberada_ESP8266){                 
                                                
                        sprintf(stringMensagemESP8266,"AT+CWMODE=3\r\n");
                        escreveMensagemESP8266(stringMensagemESP8266);
                     
                        estadoAtual_ESP8266=AGUARDANDO_ACK_5;
                        estadoAnterior_ESP8266=ENVIAR_COMANDO_CWMODE;                        
                    } 
                }
                
            break;
            case AGUARDANDO_ACK_5:                    
                if(delayExecucao_ESP8266 < TEMPO_AGUARDANDO_ACK_ESP8266){
                    if(maquinaDeEstadosLiberada_ESP8266){
                       if(strstr(bufferInterrupcaoUART4,"OK")!=0){           
                             limpaBufferNaMaquinaDeEstados_ESP8266();
                             zeraContadorExecucao_FSM_ESP8266(); //MARCOS - ZEREI PARA REINICIAR PARA A PROXIMA FUN��O
                             estadoAtual_ESP8266 = ENVIAR_COMANDO_CIPMODE;
                             estadoAnterior_ESP8266 = AGUARDANDO_ACK_5; 
                         }
                    }  
                }
                else{
                    zeraContadorExecucao_FSM_ESP8266();
                    estadoAtual_ESP8266=AGUARDANDO_TAREFA;
                    estadoAnterior_ESP8266=AGUARDANDO_ACK_5;                                          
                }
            break;
            
            case ENVIAR_COMANDO_CIPMODE:  
                
                if(delayExecucao_ESP8266 > TEMPO_ENTRE_ESTADOS_FSM_ESP8266){
                    zeraContadorExecucao_FSM_ESP8266();     
                    
                    if(maquinaDeEstadosLiberada_ESP8266){                 
                        limpaBufferNaMaquinaDeEstados_ESP8266();                         
                        sprintf(stringMensagemESP8266,"AT+CIPMODE=1\r\n");
                        escreveMensagemESP8266(stringMensagemESP8266);
                     
                        estadoAtual_ESP8266=AGUARDANDO_ACK_6;
                        estadoAnterior_ESP8266=ENVIAR_COMANDO_CIPMODE;                        
                    } 
                }
                
            break;
            case AGUARDANDO_ACK_6:                    
                if(delayExecucao_ESP8266 < TEMPO_AGUARDANDO_ACK_ESP8266){
                    if(maquinaDeEstadosLiberada_ESP8266){
                       if(strstr(bufferInterrupcaoUART4,"OK")!=0){           
                            limpaBufferNaMaquinaDeEstados_ESP8266();
                            zeraContadorExecucao_FSM_ESP8266(); //MARCOS - ZEREI PARA REINICIAR PARA A PROXIMA FUN��O
                            estadoAtual_ESP8266 = ENVIAR_COMANDO_CWJAP;
                            estadoAnterior_ESP8266 = AGUARDANDO_ACK_6; 
                        }
                    }  
                }
                else{
                    zeraContadorExecucao_FSM_ESP8266();
                    estadoAtual_ESP8266=AGUARDANDO_TAREFA;
                    estadoAnterior_ESP8266=AGUARDANDO_ACK_6;                                          
                }
            break;
            
            case ENVIAR_COMANDO_CWJAP:  
                
                if(delayExecucao_ESP8266 > TEMPO_ENTRE_ESTADOS_FSM_ESP8266){
                    zeraContadorExecucao_FSM_ESP8266();     
                    
                    if(maquinaDeEstadosLiberada_ESP8266){                 
                        limpaBufferNaMaquinaDeEstados_ESP8266(); 
                               
                        #ifndef DEBUG
                        sprintf(stringMensagemESP8266,"AT+CWJAP_CUR=\"%s\",\"%s\"\r\n",ssidWifi,senhaWifi); // Credenciais de acesso da rede industrial 
                        #else
                        sprintf(stringMensagemESP8266,"AT+CWJAP_CUR=\"RedeTeste\",\"1234567890\"\r\n"); // Credenciais de acesso da rede industrial 
                        #endif
                        escreveMensagemESP8266(stringMensagemESP8266);
                        
                        estadoAtual_ESP8266=AGUARDANDO_ACK_7;
                        estadoAnterior_ESP8266=ENVIAR_COMANDO_CWJAP;                        
                    } 
                }
                
            break;
            case AGUARDANDO_ACK_7:                    
                if(delayExecucao_ESP8266 < TEMPO_AGUARDANDO_ACK_MSG_WIFI_ESP8266){
                    if(maquinaDeEstadosLiberada_ESP8266){
                        if(strstr(bufferInterrupcaoUART4,"WIFI CONNECTED")!=0 || strstr(bufferInterrupcaoUART4,"OK")!=0){           
                            limpaBufferNaMaquinaDeEstados_ESP8266(); 
                            statusConexaoWifi_ESP8266 = CONECTADO;
                            zeraContadorExecucao_FSM_ESP8266(); //MARCOS - ZEREI PARA REINICIAR PARA A PROXIMA FUN��O
                            estadoAtual_ESP8266 = ENVIAR_COMANDO_CIPSTART;
                            estadoAnterior_ESP8266 = AGUARDANDO_ACK_7;
                             
                            if(logConectividadeWifi){
                               numVezesReconectou++;
                               //dataHora_ESP8266 = localtime(&Tempo);
                               RTC_calendarRequest(&dataHora_ESP8266);
                               sprintf(stringLogWifi,"\rNumero de conexoes realizadas: %02d\n\rConectou as: %02d:%02d:%02d\n",numVezesReconectou,dataHora_ESP8266.tm_hour,dataHora_ESP8266.tm_min,dataHora_ESP8266.tm_sec);                             
                               escreverMensagemUSB(stringLogWifi);  
                            }
                        }
                    }  
                }
                else{
                    limpaBufferNaMaquinaDeEstados_ESP8266();
                    statusConexaoWifi_ESP8266 = DESCONECTADO;
                    zeraContadorExecucao_FSM_ESP8266();
                    estadoAtual_ESP8266=AGUARDANDO_TAREFA; 
                    estadoAnterior_ESP8266=AGUARDANDO_ACK_7;                                          
                }
            break;
            
            case ENVIAR_COMANDO_CIPSTART:  
                
                if(delayExecucao_ESP8266 > TEMPO_ENTRE_ESTADOS_FSM_ESP8266){
                    zeraContadorExecucao_FSM_ESP8266();     
                    
                    if(maquinaDeEstadosLiberada_ESP8266){                 
                        limpaBufferNaMaquinaDeEstados_ESP8266(); 
                        
                        if(solicitarTrocaDeServidor){
                            // volta para o modo de comandos AT
                            //solicitarTrocaDeServidor = 0;                            
                            sprintf(stringMensagemESP8266,"+++"); 
                            escreveMensagemESP8266(stringMensagemESP8266);
                            delay_ms(50);                            
                            limpaBufferNaMaquinaDeEstados_ESP8266();  
                        }                        
                        
                        sprintf(stringMensagemESP8266,"AT+CIPSTART=\"TCP\",\"%s\",%d\r\n",ipServidorEmUso,portaServidorEmUso);
                        escreveMensagemESP8266(stringMensagemESP8266);
                            
                        estadoAtual_ESP8266=AGUARDANDO_ACK_8;
                        estadoAnterior_ESP8266=ENVIAR_COMANDO_CIPSTART;                        
                    } 
                }
                
            break;
            case AGUARDANDO_ACK_8:                    
                if(delayExecucao_ESP8266 < TEMPO_AGUARDANDO_ACK_ESP8266){
                    if(maquinaDeEstadosLiberada_ESP8266){
                       if((strstr(bufferInterrupcaoUART4,"OK")!=0) || (strstr(bufferInterrupcaoUART4,"CONNECT")!=0)){           
                             //limpaBufferNaMaquinaDeEstados_ESP8266(); 
                             //sprintf(stringMensagemESP8266,"AT+CIPSTART=\"TCP\",\"%s\",%d\r\n",ipServidorEmUso,portaServidorEmUso);
                             //escreveMensagemESP8266(stringMensagemESP8266);
                             
                             
                             estadoAtual_ESP8266 = ENVIAR_COMANDO_CIPSEND;
                             estadoAnterior_ESP8266 = AGUARDANDO_ACK_8;
                             solicitarTrocaDeServidor = 0;
                             statusConexaoWifi_ESP8266 = CONECTADO;
                             wifiLiberadoParaUso = SIM;
                        }
                       else{                      
                           if(strstr(bufferInterrupcaoUART4,"ALREADY CONNECTED")!=0){
                               limpaBufferNaMaquinaDeEstados_ESP8266(); 
                               sprintf(stringMensagemESP8266,"AT+CIPCLOSE\r\n");
                               escreveMensagemESP8266(stringMensagemESP8266);
                               estadoAtual_ESP8266=ENVIAR_COMANDO_CIPSTART;
                               estadoAnterior_ESP8266=AGUARDANDO_ACK_8;      
                           }
                           else{
                               if(strstr(bufferInterrupcaoUART4,"no ip")!=0)
                                {                 
                                     limpaBufferNaMaquinaDeEstados_ESP8266(); 
                                     solicitarTrocaDeServidor = 0;
                                     if(logConectividadeWifi){
                                         //dataHora_ESP8266 = localtime(&Tempo);
                                         RTC_calendarRequest(&dataHora_ESP8266);
                                         sprintf(stringLogWifi,"\rPerda de conexao wi-fi as: %02d:%02d:%02d\n",dataHora_ESP8266.tm_hour,dataHora_ESP8266.tm_min,dataHora_ESP8266.tm_sec);
                                         escreverMensagemUSB(stringLogWifi);
                                     }
                                     statusConexaoWifi_ESP8266 = DESCONECTADO;
                                     estadoAtual_ESP8266 = ENVIAR_COMANDO_CWJAP;                              
                                }
                                else{
                                    if(strstr(bufferInterrupcaoUART4,"ERROR")!=0){
                                    limpaBufferNaMaquinaDeEstados_ESP8266(); 
                                    switch(servidorEmUso){
                                         case PRIMARIO:
                                             trocarIpDeServidor(SECUNDARIO);
                                             estadoAtual_ESP8266 = ENVIAR_COMANDO_CIPSTART;                                        
                                             break;
                                         case SECUNDARIO:
                                             trocarIpDeServidor(PRIMARIO);
                                             estadoAtual_ESP8266 = ENVIAR_COMANDO_CIPSTART;                               
                                             break;
                                     }   
                                     estadoAnterior_ESP8266 = AGUARDANDO_ACK_8; 
                                    }
                                }
                           }                           
                           wifiLiberadoParaUso = 0;
                       }
                    }  
                }
                else{
                    ocorreuErroDeTimeoutNoWifiDentroDaMaquinaDeEstados();
                    zeraContadorExecucao_FSM_ESP8266();
                    estadoAtual_ESP8266=AGUARDANDO_TAREFA;
                    estadoAnterior_ESP8266=AGUARDANDO_ACK_8;                                          
                }
            break;
            case ENVIAR_COMANDO_CIPSEND:  
                
                if(delayExecucao_ESP8266 > TEMPO_ENTRE_ESTADOS_FSM_ESP8266){
                    zeraContadorExecucao_FSM_ESP8266();     
                    
                    if(maquinaDeEstadosLiberada_ESP8266){                 
                        limpaBufferNaMaquinaDeEstados_ESP8266();                   
                        sprintf(stringMensagemESP8266,"AT+CIPSEND\r\n");
                        escreveMensagemESP8266(stringMensagemESP8266);
                     
                        estadoAtual_ESP8266=AGUARDANDO_ACK_9;
                        estadoAnterior_ESP8266=ENVIAR_COMANDO_CIPSEND;                        
                    } 
                }
                
            break;
            case AGUARDANDO_ACK_9:                    
                if(delayExecucao_ESP8266 < TEMPO_AGUARDANDO_ACK_ESP8266){
                    if(maquinaDeEstadosLiberada_ESP8266){
                       if((strstr(bufferInterrupcaoUART4,"OK")!=0) && (strstr(bufferInterrupcaoUART4,'>')!=0)){           
                             limpaBufferNaMaquinaDeEstados_ESP8266(); 
                             zeraContadorExecucao_FSM_ESP8266(); //MARCOS - ZEREI PARA REINICIAR PARA A PROXIMA FUN��O
                             estadoAtual_ESP8266 = FIM_CICLO;
                             estadoAnterior_ESP8266 = AGUARDANDO_ACK_9; 
                         }
                    }  
                }
                else{
                    zeraContadorExecucao_FSM_ESP8266();
                    estadoAtual_ESP8266=AGUARDANDO_TAREFA;
                    estadoAnterior_ESP8266=AGUARDANDO_ACK_9;                                          
                }
            break;            
            case FIM_CICLO: 
                if(delayExecucao_ESP8266 > TEMPO_ENTRE_ESTADOS_FSM_ESP8266){
                    if(maquinaDeEstadosLiberada_ESP8266){
                        zeraContadorExecucao_FSM_ESP8266();
                       
                        ESP8266_inicializado = SIM;
                        //statusConexaoWifi_ESP8266 = CONECTADO;
                        bloqueiaMaquinaDeEstados_ESP8266();
                        //habilitaMaquinaDeEstados_KeepAlive(); // MARCOS - Bloquei KeepAlive

                        estadoAtual_ESP8266=AGUARDANDO_TAREFA;
                        estadoAnterior_ESP8266=FIM_CICLO;
                    }      
                }                
            break;        
            default:
            break;
        }
    }

/*
 * ANOTA��O DOS PROCEDIMENTOS PARA INICIALIZA��O, OPERA��O E PARAMETRIZA��O DO ESP8266
 * 
 * ====================================================================================
 * ======================== COMANDOS DA M�QUINA DE ESTADO =============================
 * ====================================================================================
 * INTERFACE COMMANDS
 *    EXIT TRANSPARENT MODE -> #CMD37;+++<NUL> *    
 *    ECHO OFF              -> #CMD37;ATE0<CR><LF><NUL>
 *    RFPOWER               -> #CMD37;AT+RFPOWER=82<CR><LF><NUL>
 * WIFI-COMMANDS 
 *    WI-FI CONNECT         -> #CMD37;AT+CWJAP_DEF="Alho_Home","P!nn\@pl#"<CR><LF><NUL>
 * TCP IP COMMANDS    
 *    CONNECTION STATUS     -> #CMD37;AT+CIPSTATUS<CR><LF><NUL>
 *    SINGLE CONNECTION     -> #CMD37;AT+CIPMUX=0<CR><LF><NUL>
 *    TRANSMISSION MODE     -> #CMD37;AT+CIPMODE=1<CR><LF><NUL>
 *    START CONNECTION      -> #CMD37;AT+CIPSTART="TCP","192.168.1.105",9000<CR><LF><NUL>
 *    TRANSPARENT MODE      -> #CMD37;AT+CIPSEND<CR><LF><NUL>
 * 
 * ====================================================================================
 * ============================ COMANDOS DO SOFTWARE ==================================
 * ====================================================================================
 * INTERFACE COMMANDS
 *    EXIT TRANSPARENT MODE -> #CMD37;+++<NUL>
 *    RESET                 -> #CMD37;AT+RST<CR><LF><NUL>
 *    RESTORE DEFAULT       -> #CMD37;AT+RESTORE<CR><LF><NUL>
 *    ECHO OFF              -> #CMD37;ATE0<CR><LF><NUL>
 * WIFI-COMMANDS
 *    FUNCTION MODE         -> #CMD37;AT+CWMODE_DEF=1<CR><LF><NUL>
 *    DHCP CONFIGURATION    -> #CMD37;AT+CWDHCP_DEF=1,1<CR><LF><NUL>
 *    AUTO CONNECTION       -> #CMD37;AT+CWAUTOCONN=1<CR><LF><NUL>
 *    DEFINE MAC ADDRESS    -> #CMD37;AT+CIPSTAMAC_DEF="32:33:34:35:36:37"<CR><LF><NUL>
 *    DEFINE IP ADDRESS     -> #CMD37;AT+CIPSTA_DEF="192.168.0.100","192.168.1.1","255.255.255.0"<CR><LF><NUL>
 *    WI-FI CONNECT         -> #CMD37;AT+CWJAP_DEF="Alho_Home","P!nn\@pl#"<CR><LF><NUL>
 * TCP IP COMMANDS    
 *    CONNECTION STATUS     -> #CMD37;AT+CIPSTATUS<CR><LF><NUL>
 *    SINGLE CONNECTION     -> #CMD37;AT+CIPMUX=0<CR><LF><NUL>
 *    TRANSMISSION MODE     -> #CMD37;AT+CIPMODE=1<CR><LF><NUL>
 *    START CONNECTION      -> #CMD37;AT+CIPSTART="TCP","192.168.1.105",9000<CR><LF><NUL>
 *    TRANSPARENT MODE      -> #CMD37;AT+CIPSEND<CR><LF><NUL>
 * 
 * ====================================================================================
 * ================================ INICIALIZA��O =====================================
 * ====================================================================================
 * INTERFACE COMMANDS
 *    EXIT TRANSPARENT MODE -> #CMD37;+++<NUL>
 *    RESET                 -> #CMD37;AT+RST<CR><LF><NUL>
 *    RESTORE DEFAULT       -> #CMD37;AT+RESTORE<CR><LF><NUL>
 *    ECHO OFF              -> #CMD37;ATE0<CR><LF><NUL>
 *    RFPOWER               -> #CMD37;AT+RFPOWER=82<CR><LF><NUL>
 * WIFI-COMMANDS
 *    FUNCTION MODE         -> #CMD37;AT+CWMODE_DEF=1<CR><LF><NUL>
 *    DHCP CONFIGURATION    -> #CMD37;AT+CWDHCP_DEF=1,1<CR><LF><NUL>
 *    AUTO CONNECTION       -> #CMD37;AT+CWAUTOCONN=1<CR><LF><NUL>
 *    DEFINE MAC ADDRESS    -> #CMD37;AT+CIPSTAMAC_DEF="32:33:34:35:36:37"<CR><LF><NUL>
 *    DEFINE IP ADDRESS     -> #CMD37;AT+CIPSTA_DEF="192.168.0.100","192.168.1.1","255.255.255.0"<CR><LF><NUL>
 *    WI-FI CONNECT         -> #CMD37;AT+CWJAP_DEF="Alho_Home","P!nn\@pl#"<CR><LF><NUL>
 * TCP IP COMMANDS    
 *    CONNECTION STATUS     -> #CMD37;AT+CIPSTATUS<CR><LF><NUL>
 *    SINGLE CONNECTION     -> #CMD37;AT+CIPMUX=0<CR><LF><NUL>
 *    TRANSMISSION MODE     -> #CMD37;AT+CIPMODE=1<CR><LF><NUL>
 *    START CONNECTION      -> #CMD37;AT+CIPSTART="TCP","192.168.1.105",9000<CR><LF><NUL>
 *    TRANSPARENT MODE      -> #CMD37;AT+CIPSEND<CR><LF><NUL>
 */
