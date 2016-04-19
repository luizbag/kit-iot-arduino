/*               Bibliotecas   */

#include <DHT.h>
#include <Ethernet.h>
#include <SPI.h>


/*               Defines   */
#define DHTPIN 2 // pino do sensor
#define DHTTYPE DHT11 // tipo do sensor nosso caso DHT11

/*               Configuração de rede do Arduíno   */
byte mac[] = { 0x78, 0x2B, 0xCB, 0xED, 0x1B, 0x8A }; //MAC ADDRESS ETHERNET NOTEBOOK ERIC
byte ip[] = { 172, 16, 0, 78 };
byte subnet[] = {255, 255, 0, 0};
byte gateway[] = { 172, 16, 0, 234 };
byte server[] = {172, 16, 254, 11};
EthernetClient client;


DHT dht(DHTPIN, DHTTYPE);

/*               Variaveis globais   */
long previousMillis = 0;
unsigned long currentMillis = 0;
long interval = 1000; // intervalo de leitura(milisegundos)
long unsigned int lowIn; //PIR
long unsigned int pause = 5000; //Para consertar a falha de leitura do PIR

int t = -1, h = -1;  // temperatura, umidade
int ldr = A3, p = -1 , ldr_out = A2; //Entrada analógica LDR
int calibrationTime = 30, pirPin = 3, ledPin = 10, ini = -1, fim = -1; //Tempo de calibração PIR, pino digital PIR, led do PIR, inicio da movimentacao, fim da movimentacao
int ventilador = 7, buzzer = 5, led = 6;//reles

String data, data_LDR, data_PIR;

/*               API Key do usuário que irá acessar as informações no servidor   */
String API_USER = "eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJfaWQiOiI1NzE0ZmFhNWE2MTI0YmU3NmQ3YjhmNWYiLCJlbWFpbCI6ImFkbUB0aWNraXRpb3QuY29tIn0.g5mQDCiAzz35pFui0ttArcqBtv7rh4hSfnt7o5sH9BQ";
boolean lockLow = true, takeLowTime;//Condicionais PIR

String cria_data(int n, String nome);
void acesso_MONGO(String data, String data_LDR, String data_PIR);
String cria_dataTU(int n, int n1, String nome, String nome2);
void dados_MONGO(String data, String apikey);

int aux_fim, aux_ini, cont = 1, contf = 1; //variaveis para ajudar a zerar o sensor de presença caso a presença termine


/*               SETUP  */
void setup()
{

  Serial.begin(115200);

  /*               Conexão com a ethernet estático  */
  /*int r = Ethernet.begin(mac);
    Serial.println(r);
    if (r == 0)
    Serial.println("Failed to configure Ethernet using DHCP");*/

  /*               Conexão com a ethernet através do MAC e o IP =, ou seja, dinamico  */
  if (Ethernet.begin(mac)) {
    Serial.println("Conexao com a Ethernet. MODO DINAMICO\n");
  }
  else {
    Serial.println("Conexao com a Ethernet MODO DINAMICO deu erro\n");
    Ethernet.begin(mac, ip);
  }

  /*               Iniciando o DTH  */

  dht.begin();
  Serial.print("\n");

  h = (int) dht.readHumidity();
  t = (int) dht.readTemperature();

  /*               Iniciando o PIR */
  pinMode(pirPin, INPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(pirPin, LOW);
  Serial.println("Calibrando o sensor "); //Calibração do Sensor em 30s
  for (int i = 0; i < calibrationTime; i++)
  {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" PRONTO!\n\n");
  Serial.println("SENSOR ATIVO");

  //inicialização Strings datas
  data = " ";
  data_LDR = " ";
  data_PIR = " ";

  pinMode(buzzer, OUTPUT);
  pinMode(led, OUTPUT);

  /*               Verificando conexão com a Ethernet  */
  client.stop();
  if (int n = client.connect("172.16.254.11", 80))
    Serial.println("DEU CERTO!!");
  else {
    Serial.println("Erro no client.connect");
    Serial.println(n);
  }
}// Fim do Setup

void loop()
{
  /*               Leitura do LRD  */
  int leitura = analogRead(ldr);

  /*               Convertendo o valor lido do LDR para 0 a 100, sendo 100 o máximo de escuridão */
  p = map(leitura, 0, 1023, 0, 100);
  if ( p > 95)
    digitalWrite(led, HIGH);
  else
    digitalWrite(led, LOW);

  /*               Leitura do DTH  */
  h =  dht.readHumidity();
  t =  dht.readTemperature();

  /*               Liga o ventilador se a temperatura for acima de 24°C  */
  if (t > 24)
    digitalWrite(ventilador, HIGH);
  else
    digitalWrite(ventilador, LOW);

  /*               Iniciando o PIR, observação NÃO MEXER */
  if (digitalRead(pirPin) == 1)
  {
    digitalWrite(ledPin, 1);
    if (lockLow)
    {
      digitalWrite(ventilador, HIGH);
      delay(2000);
      digitalWrite(ventilador, LOW);

      //makes sure we wait for a transition to LOW before any further output is made:
      lockLow = false;
      Serial.println("---");
      Serial.print("Presenca detectada em ");
      ini = millis() / 1000;


      if (cont == 1)
        aux_ini = ini;

      else if (cont == 2)
        if (ini == aux_ini)
        {
          ini = 0;
          cont = 0;
        }

      Serial.print(ini);
      Serial.println(" seg");
      delay(50);

      contf++;
    }//end if lockLow

    takeLowTime = true;
  }//end if digitalRead


  //NÃO MEXE NESSE PEDÇO DE CODIGO DO PIR PFVR
  if (digitalRead(pirPin) == 0)
  {
    digitalWrite(ledPin, 0);  //the led visualizes the sensors output pin state

    if (takeLowTime) {
      lowIn = millis();          //save the time of the transition from high to LOW
      takeLowTime = false;       //make sure this is only done at the start of a LOW phase
    }//end if takeLowTime

    //if the sensor is low for more than the given pause,
    //we assume that no more motion is going to happen
    if (!lockLow && millis() - lowIn > pause)
    {
      //makes sure this block of code is only executed again after a new motion sequence has been detected
      lockLow = true;
      Serial.print("Presenca terminou em ");      //output
      fim = (millis() - pause) / 1000;
      Serial.print(fim);
      Serial.println(" seg");
      Serial.print("\n\nDuracao: ");
      Serial.print(fim - ini);
      Serial.print(" seg");
      delay(50);
    }//end if !lockLow
    /*               LIBERADO PARA MEXER  */

    if (contf == 1)
      aux_fim = fim;

    else if (contf == 2)
      if (fim == aux_fim) {
        fim = 0;
        cont = 0;
      }

    contf++;
  }//end if DigitalRead

  /*               Finalizando o processo do PIR*/

  /*               Iniciando as variaveis de DATA para ser exibida*/
  data_PIR = cria_data(fim - ini, "tempo");
  data = cria_dataTU(t, h, "temperatura", "umidade");
  data_LDR = cria_data(p, "luminosidade");

  /*               Imprimindo as variaveis de DATA para ser exibida*/
  Serial.println(data);
  Serial.println(data_LDR);
  Serial.println(data_PIR);

  /*               Acesso ao mongo DB passando as STRINGS*/
  acesso_MONGO(data, data_LDR, data_PIR);
  delay(10000);

}//Finalizando o LOOP

/*               Setando as variaveis de data para serem exibidas*/
String cria_data(int n, String nome)
{
  String data;

  data = '{';
  data.concat('"');
  data.concat(nome);
  data.concat('"');
  data.concat(":");
  data.concat('"');
  data.concat(n);
  data.concat('"');
  data.concat('}');

  return data;
}

/*               Setando as variaveis de data para serem exibidas*/
String cria_dataTU(int n, int n1, String nome, String nome2)
{
  data = '{';
  data.concat('"');
  data.concat(nome);
  data.concat('"');
  data.concat(":");
  data.concat('"');
  data.concat(n); // concatenando os valores em uma unica string
  data.concat('"');
  data.concat(", ");
  data.concat('"');
  data.concat(nome2);
  data.concat('"');
  data.concat(":");
  data.concat('"');
  data.concat(n1);
  data.concat('"');
  data.concat("}");
  return data;
}

/*               Acessando o mongoDB e enviando as informações*/
void acesso_MONGO(String data, String data_LDR, String data_PIR)
{

  int n = 1;

  Serial.println(n);
  client.stop();
  /*               Dados para o DTH11*/
  if (n = client.connect("172.16.254.11", 80)) {
    //O QUE PRECISA MUDAR É ESSA URL DEPOIS DO /SENSORES/, CADA SENSOR QUE VOCÊ CRIA GERA UMA TAG DIFERENTE
    client.println("POST /kit-iot/sensores/5714fae9a6124be76d7b8f60  HTTP/1.1");
    dados_MONGO(data, API_USER);
    Serial.println("Passou\n");

    while (client.available())
      Serial.write(client.read());
  }
  else {
    Serial.println("Erro de conexao[1] ... ");
    Serial.println(n);

  }
  client.stop();  // desconectar


  /*               Dados para o LDR*/
  if (n = client.connect("172.16.254.11", 80)) {
    client.println("POST /kit-iot/sensores/5714fb4fa6124be76d7b8f61  HTTP/1.1");
    dados_MONGO(data_LDR, API_USER);
    while (client.available())
      Serial.write(client.read());

  }
  else
    Serial.println("Erro de conexao[2]...\n");

  client.stop();  // desconectar


  /*               Dados para o PIR*/
  if (n = client.connect("172.16.254.11", 80)) {
    client.println("POST /kit-iot/sensores/5714fb64a6124be76d7b8f62  HTTP/1.1");
    dados_MONGO(data_PIR, API_USER);
    Serial.println("Passou\n");
    while (client.available())
      Serial.write(client.read());

  }
  else
    Serial.println("Erro de conexao[3]...\n");

  client.stop();  // desconectar
}//fim dados

/*               Verificando dados do mongoDB*/
void dados_MONGO(String data, String apikey) {

  client.println("Host: 172.16.254.11");
  /*               Chave de API KEY no mongoDB*/
  client.println("Authorization: eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJfaWQiOiI1NzE0ZmFhNWE2MTI0YmU3NmQ3YjhmNWYiLCJlbWFpbCI6ImFkbUB0aWNraXRpb3QuY29tIn0.g5mQDCiAzz35pFui0ttArcqBtv7rh4hSfnt7o5sH9BQ");

  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(data.length());
  client.println();
  client.println(data);
}