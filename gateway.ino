/*
* Autor: Eziom Alves
* Data: 21 de Outubro de 2022.
*/

#include "heltec.h"
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>


/* utilizado para subscribe */
#define TOPICO_PUBLISH   "MQAr/PMS5003"    /*tópico MQTT de envio de informações para Broker
                                                 IMPORTANTE: recomenda-se fortemente alterar os nomes
                                                             desses tópicos. Caso contrário, há grandes
                                                             chances de você controlar e monitorar o módulo
                                                             de outra pessoa (pois o broker utilizado contém 
                                                             dispositivos do mundo todo). Altere-o para algo 
                                                             o mais único possível para você. */

//utilizado para publish (MAC do gateway)
#define ID_MQTT  "F008D1DD2C58"     /* id mqtt (para identificação de sessão)
                                       IMPORTANTE: este deve ser único no broker (ou seja, 
                                                   se um client MQTT tentar entrar com o mesmo 
                                                   id de outro já conectado ao broker, o broker 
                                                   irá fechar a conexão de um deles). Pelo fato
                                                   do broker utilizado conter  dispositivos do mundo 
                                                   todo, recomenda-se fortemente que seja alterado 
                                                   para algo o mais único possível para você.*/

/* Definicoes para comunicação com radio LoRa */

#define BAND               915E6  /* 915MHz de frequencia */

/* Constantes */

const char* SSID = ""; // coloque aqui o SSID/nome da rede WI-FI que deseja se conectar
const char* PASSWORD = ""; // coloque aqui a senha da rede WI-FI que deseja se conectar

//MQTT
const char* BROKER_MQTT = "test.mosquitto.org"; //URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1883; // Porta do Broker MQTT
  
/* Variáveis e objetos globais*/
WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient


/* Definicoes gerais */
#define DEBUG_SERIAL_BAUDRATE    9600

/* Local prototypes */
bool init_comunicacao_lora(void);
void envia_informacoes_por_mqtt(float MP25, float MP10);
void init_wifi(void);
void init_MQTT(void);
void reconnect_wifi(void); 
void reconnect_MQTT(void);
void verifica_conexoes_wifi_e_MQTT(void);
bool init_comunicacao_lora(void);

/* typedefs */
typedef struct __attribute__((__packed__))  
{
  float MP25;
  float MP10;
}TDadosLora;

/* Função: inicializa e conecta-se na rede WI-FI desejada
 * Parâmetros: nenhum
 *Retorno: nenhum
*/
void init_wifi(void) 
{
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");    
    reconnect_wifi();
}

/* Função: inicializa parâmetros de conexão MQTT(endereço do broker e porta)
 * Parâmetros: nenhum
 * Retorno: nenhum
*/
void init_MQTT(void) 
{
    //informa qual broker e porta deve ser conectado
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);   
}


/* Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
 *         em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
 * Parâmetros: nenhum
 * Retorno: nenhum
*/
void reconnect_MQTT(void) 
{
    while (!MQTT.connected()) 
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) 
            Serial.println("Conectado com sucesso ao broker MQTT!");
        else 
        {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);
        }
    }
}



 
/* Função: reconecta-se ao WiFi
 * Parâmetros: nenhum
 * Retorno: nenhum
*/
void reconnect_wifi(void) 
{
    Heltec.display->clear();
    Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
    Heltec.display->setFont(ArialMT_Plain_10);
    /* se já está conectado a rede WI-FI, nada é feito. 
       Caso contrário, são efetuadas tentativas de conexão */
    if (WiFi.status() == WL_CONNECTED)
        return;
        
    WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI
    
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(100);
        Serial.print(".");
    }

    Heltec.display->drawString(0, 0, "Gateway LoRa");
    Heltec.display->drawString(0, 10, "SSID:");
    Heltec.display->drawString(30, 10, (String)SSID);
    Heltec.display->drawString(0, 20, "IP:");
    Heltec.display->drawString(15, 20, WiFi.localIP().toString().c_str());
    Heltec.display->drawString(0, 30, "MAC:");
    Heltec.display->drawString(25, 30, WiFi.macAddress());
    Heltec.display->display();

}
 
/* Função: verifica o estado das conexões WiFI e ao broker MQTT. 
 *         Em caso de desconexão (qualquer uma das duas), a conexão
 *        é refeita.
 * Parâmetros: nenhum
 * Retorno: nenhum
*/
void verifica_conexoes_wifi_e_MQTT(void)
{
    /* se não há conexão com o WiFI, a conexão é refeita */ 
    reconnect_wifi(); 

    /* se não há conexão com o Broker, a conexão é refeita  */ 
    if (!MQTT.connected()) 
        reconnect_MQTT(); 
}


/* 
 * Função: envia por MQTT as informações de temperatura e umidade lidas, assim como as temperaturas máxima e mínima
 * Parâmetros: - Temperatura lida
 *             - Umidade relativa do ar lida
 * Retorno: nenhum
 */
void envia_informacoes_por_mqtt(float MP25, float MP10)
{
  DynamicJsonDocument doc(1024);
  char mensagem_MQTT[200];
  
  doc["MP10"] = MP10;
  doc["MP25"] = MP25;
    
  serializeJson(doc, mensagem_MQTT);
                                                                                                                              
  MQTT.publish(TOPICO_PUBLISH, mensagem_MQTT);  
}


/* Funcao de setup */
void setup() 
{
    
    Serial.begin(DEBUG_SERIAL_BAUDRATE);
    while (!Serial);

    Heltec.begin( true /*Habilita o Display*/, 
                  true /*Heltec.Heltec.Heltec.LoRa Disable*/, 
                  true /*Habilita debug Serial*/, 
                  true /*Habilita o PABOOST*/, 
                  BAND /*Frequência BAND*/); 

    Heltec.display->init();
    Heltec.display->flipScreenVertically();  
    Heltec.display->setFont(ArialMT_Plain_16);
    Heltec.display->clear();
    Heltec.display->drawString(33, 5, "Iniciado");
    Heltec.display->drawString(10, 30, "com Sucesso!");
    Heltec.display->display();
    /* inicializações do WI-FI e MQTT */
    init_wifi();
    init_MQTT();
}

/* Programa principal */
void loop() 
{
    char byte_recebido;
    int packet_size = 0;
    TDadosLora dados_lora;
    char * ptInformaraoRecebida = NULL;
    
    /* Verifica se as conexões MQTT e wi-fi estão ativas 
     Se alguma delas não estiver ativa, a reconexão é feita */
    verifica_conexoes_wifi_e_MQTT();

    
    /* Verifica se chegou alguma informação do tamanho esperado */
    packet_size = LoRa.parsePacket();

        
    if (packet_size == sizeof(TDadosLora)) 
    {
        Serial.println("[LoRa Receiver] Há dados a serem lidos");
        
        /* Recebe os dados conforme protocolo */               
        ptInformaraoRecebida = (char *)&dados_lora;  
        while (LoRa.available()) 
        {
            byte_recebido = (char)LoRa.read();
            *ptInformaraoRecebida = byte_recebido;
            ptInformaraoRecebida++;
        }

        Serial.print("MP25: ");
        Serial.println(dados_lora.MP25);
         Serial.print("MP10: ");
        Serial.println(dados_lora.MP10);
        

        envia_informacoes_por_mqtt(dados_lora.MP25, dados_lora.MP10);
    }

  /* Faz o keep-alive do MQTT */
  MQTT.loop();
} 
