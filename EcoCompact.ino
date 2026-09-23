/********************** INCLUDES *********************/
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <HTTPClient.h>


/******************* DEFINIÇÃO PINOS ******************/
// Motor
#define PinoMotorDesce 18  //ESP pino 25
#define PinoMotorSobe 19  //Esp pino 26

// Botões
#define PinoBtnEmergencia 32  //Esp pino 32
#define PinoBtnPausa 33  //Esp pino 33
#define PinoBtnAcionamento 23  //Esp pino 34

// Sensor
#define PinoTriggerSensor1 12
#define PinoTriggerSensor2 13
#define PinoTriggerSensor3 14

#define PinoEchoSensor1 25
#define PinoEchoSensor2 26
#define PinoEchoSensor3 27

/********************* CONECTIVIDADE ********************/
// Wi-Fi credentials
#define WIFI_SSID "WIFI_SSID"
#define WIFI_PASSWORD "WIFI_PASSWORD"

 // Firebase credentials
#define API_KEY "API_KEY"
#define DATABASE_URL "DATABASE_URL" 

// Firebase setup
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

/********************* VARIÁVEIS ********************/
char* listaEstadosPrensa[] = {"esperando", "emergencia", "descendo", "subindo", "pausando"};

enum EstadosPrensa{
  	esperando = 0,
    emergencia,
	descendo,
  	subindo,
    pausando //termina a ação e entra em estado de pausa
};

enum PwmFuncao { 
  movendo = 0, 
  trocando 
};

//PWM
int pwmPasso = 25; //define o salto da função softStart()
int pwmMaximo = 220; // define o PWM máximo usado no softStart() e softStop()
int pwmAtual = 0; 

//BOTÕES
bool estadoBtnAcionamento = 0;
bool estadoBtnEmergencia = 0;
bool estadoBtnPausa = 0;

bool firebaseEstadoBtnAcionamento = 0;
bool firebaseEstadoBtnEmergencia = 0;
bool firebaseEstadoBtnPausa = 0;

bool estadoAnteriorEmergencia = 0;
bool estadoAnteriorPausa = 0;
bool esperandoPausa = 0;

//DELAYS
unsigned long tempoUltimaEmergencia = 0;
unsigned long tempoUltimaPausa = 0;
unsigned long tempoUltimaLeitura = 0; //tempo entre sensores lerem
unsigned long tempoUltimaLeituraGeral = 0; //tempo pausa após todos os sensores lerem
unsigned long tempoUltimaAcao = 0; //SoftSart e SoftStop
unsigned long tempoInicioWifi = 0;
unsigned long tempoInicioDescida = 0;
unsigned long tempoMaximoDescida = 4000;

int tempoDebounce = 200;
int tempoDelay = 200;
int DelayEntreLeitura = 60;
int DelayEntreLeituraGeral = 2500;
int tempoDelayWifi = 15000;

//SENSORES
int sensorAtual = 1; // sensor que vai ser lido 

float valorSensor1 = 0;
float valorSensor2 = 0;
float valorSensor3 = 0;

float distanciaCapacidade = 7;
int valorCapacidade = 0;
int valorMaximoSensor = 3;
int valorMinimoSensor = 20;

//INFOS ADICIONAIS
String nomeCompactador = "Copacmata"; //nome de 0-15 caracteres


//LCD
byte SimboloEsperando[8] = { B11111, B10001, B01010, B00100, B01010, B10001, B11111, B00000 };
byte SimboloEmergencia[8] = { B00100, B00100, B00100, B00100, B00100, B00000, B00100, B00000 };
byte SimboloDescendo[8] = { B00100, B00100, B00100, B00100, B10101, B01110, B00100, B00000 };
byte SimboloSubindo[8] = { B00100, B01110, B10101, B00100, B00100, B00100, B00100, B00000 };
byte SimboloPausa[8] = { B00000, B01010, B01010, B01010, B01010, B01010, B00000, B00000 };
bool blinkPausa = 0;

/************************** INSTÂNCIAS **********************/
EstadosPrensa estadoPrensa = esperando;
EstadosPrensa estadoAnteriorPrensa = pausando;
PwmFuncao pwmFuncao = movendo;
LiquidCrystal_I2C lcd(0x27, 16, 2);
FirebaseData streamFbdo;


/************************** PROTÓTIPO **********************/
void estadoSerial(EstadosPrensa estadoAtual);
void atualizarLcd(EstadosPrensa estadoAtual);


/************************** SETUP **************************/
void setup(){
    Serial.begin(9600);

    pinMode(PinoMotorDesce, OUTPUT);
    pinMode(PinoMotorSobe, OUTPUT);
    pinMode(PinoBtnEmergencia, INPUT_PULLUP);
    pinMode(PinoBtnPausa, INPUT_PULLUP);
    pinMode(PinoBtnAcionamento, INPUT_PULLUP);
    
    //LCD
    lcd.init();
    lcd.backlight();
    lcd.createChar(0, SimboloEsperando);
    lcd.createChar(1, SimboloEmergencia);
    lcd.createChar(2, SimboloDescendo);
    lcd.createChar(3, SimboloSubindo);
    lcd.createChar(4, SimboloPausa);
    lcd.setCursor(0,0);
    lcd.print("Conectando...");


    //SENSORES
    pinMode(PinoTriggerSensor1, OUTPUT);
    pinMode(PinoTriggerSensor2, OUTPUT);
    pinMode(PinoTriggerSensor3, OUTPUT);
    
    pinMode(PinoEchoSensor1, INPUT);
    pinMode(PinoEchoSensor2, INPUT);
    pinMode(PinoEchoSensor3, INPUT);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    

    while (WiFi.status() != WL_CONNECTED && millis() - tempoInicioWifi < tempoDelayWifi) {
        Serial.print(".");
        delay(500);
    }
    
    Serial.println(" Connected!");

    if(temInternet()){
        config.api_key = API_KEY;
        config.database_url = DATABASE_URL;

        config.token_status_callback = tokenStatusCallback;

        if (Firebase.signUp(&config, &auth, "", "")) {
            Serial.println("Signup OK");
        } else {
            Serial.printf("Signup error: %s\n", config.signer.signupError.message.c_str());
        }

        Firebase.begin(&config, &auth);
        Firebase.reconnectWiFi(true);

        if(Firebase.RTDB.beginStream(&streamFbdo, "/home")){
            Serial.println("Stream iniciado");
        } else {
            Serial.println(streamFbdo.errorReason());
        }

        if (Firebase.ready()) {
            nomeCompactador = getStringFirebase("/info/nome");
            valorMaximoSensor = getIntFirebase("/configs/sensorAltura/maximo");
            valorMinimoSensor = getIntFirebase("/configs/sensorAltura/minimo");
            pwmMaximo = getIntFirebase("/configs/pwm/maximo");
            pwmPasso = getIntFirebase("/configs/pwm/passo");
        }
    }
  
    //desligar led_builtin
    pinMode(2, OUTPUT);
    digitalWrite(2, LOW);
}

void loop() {
      //if(WiFi.status() == WL_CONNECTED && temInternet()){
          if(Firebase.ready() && Firebase.RTDB.readStream(&streamFbdo)){
            if(streamFbdo.streamAvailable()){
                String path = streamFbdo.dataPath();
    
                if(path == "/estadoBtnEmergencia"){
                    firebaseEstadoBtnEmergencia = streamFbdo.boolData();
    
                } else if(path == "/estadoBtnPausa"){
                    firebaseEstadoBtnPausa = streamFbdo.boolData();
    
                } else if(path == "/estadoBtnAcionamento"){
                    firebaseEstadoBtnAcionamento = streamFbdo.boolData();
                }
            }
        //}

    }
  
    estadoSerial(estadoPrensa);
    lerBotao();
    verificarEstado();

    switch(estadoPrensa){
        case esperando:
          esperar();
          break;
        
        case emergencia:
          alertar();
          break;
        
        case descendo:
      	  mover();
          break;
        
        case subindo:
          mover();
          break;

        case pausando:
          pausar();
          break;
    }
}


/**************** Funções **********************/
void esperar(){
    if(esperandoPausa){
        esperandoPausa = 0;
        estadoAnteriorPrensa = estadoPrensa;
        estadoPrensa = pausando;
        return;
    }

    if(estadoPrensa == esperando){
        //Caso Acionamento da Prensa
        estadoBtnAcionamento = !digitalRead(PinoBtnAcionamento);
        
        if((estadoBtnAcionamento || firebaseEstadoBtnAcionamento) && !esperandoPausa){
            setBoolFirebase("/home/estadoBtnAcionamento", false);
            estadoAnteriorPrensa = estadoPrensa;
            estadoPrensa = descendo;
            tempoInicioDescida = millis();
            return;
        } 

        if(sensorAtual == 3){
            sensorAtual = 1;
        }

        if(millis() - tempoUltimaLeituraGeral >= DelayEntreLeituraGeral){
            if(millis() - tempoUltimaLeitura >= DelayEntreLeitura && sensorAtual == 1){
                lerSensor(PinoTriggerSensor1, PinoEchoSensor1, valorSensor1);
            }
            
            if(millis() - tempoUltimaLeitura >= DelayEntreLeitura && sensorAtual == 2){
                lerSensor(PinoTriggerSensor2, PinoEchoSensor2, valorSensor2);
                tempoUltimaLeituraGeral = millis();
                
                if(valorSensor1 <= distanciaCapacidade){
                    if(valorSensor2 <= distanciaCapacidade && valorCapacidade != 100){
                        setIntFirebase("/home/percCapacidade", 100);
                        valorCapacidade = 100;
                    } else if(valorSensor2 >= distanciaCapacidade && valorCapacidade != 50) {
                        setIntFirebase("/home/percCapacidade", 50);
                        valorCapacidade = 50;
                    }
                } else if(valorCapacidade != 0){
                    setIntFirebase("/home/percCapacidade", 0);
                    valorCapacidade = 0;
                }
            }   
        }

        
        
    }
}

void mover(){
    if(millis() - tempoUltimaLeitura >= DelayEntreLeitura ){
        blinkPausa = !blinkPausa;
        if(esperandoPausa){
            lcd.setCursor(15,1);

            if(blinkPausa) {
                lcd.write(4);
            } else {
                lcd.print(" ");
            }
        }

        lerSensor(PinoTriggerSensor3, PinoEchoSensor3, valorSensor3);
    }

    if(estadoPrensa == descendo){
        if(valorSensor3 >= valorMaximoSensor || millis() - tempoInicioDescida >= tempoMaximoDescida){
            pwmFuncao = trocando;
        }
    } else if(estadoPrensa == subindo){
        if(valorSensor3 <= valorMinimoSensor){
            pwmFuncao = trocando;
        }
    }

    soft();     

    if(pwmFuncao == trocando && pwmAtual <= 0){
        pwmFuncao = movendo;
        estadoAnteriorPrensa = estadoPrensa;

        if(estadoPrensa == descendo) { 
            estadoPrensa = subindo;
            tempoInicioDescida = 0;
        } else if(estadoPrensa == subindo){
            estadoPrensa = esperando;
        }
        
    }
}

//emergencia
void alertar(){
    pwmAtual = 0;
    pwmFuncao = movendo;

    analogWrite(PinoMotorDesce, 0);
    analogWrite(PinoMotorSobe, 0);
}

void pausar(){  
}

void verificarEstado() {       
    if((estadoBtnEmergencia || firebaseEstadoBtnEmergencia) && estadoAnteriorEmergencia == 0 ) {
        if(millis() - tempoUltimaEmergencia < tempoDebounce) return;
        
        setBoolFirebase("/home/estadoBtnEmergencia", false);

        if(estadoPrensa == emergencia){
            //Caso esperando sair Emergência/Alerta
            tempoUltimaEmergencia = millis();
            estadoAnteriorPrensa = estadoPrensa;
            estadoPrensa = subindo;
            estadoAnteriorEmergencia = estadoBtnEmergencia;
            return;
        } else {
            //Caso Emergência/Alerta
            tempoUltimaEmergencia = millis();
            estadoAnteriorPrensa = estadoPrensa;
            estadoPrensa = emergencia;
            esperandoPausa = 0;
            estadoAnteriorEmergencia = estadoBtnEmergencia;
            return;    
        }
    } 

    //Caso Pausa
    if((estadoBtnPausa || firebaseEstadoBtnPausa) && estadoPrensa != emergencia && estadoAnteriorPausa == 0){
        if(millis() - tempoUltimaPausa < tempoDebounce) return;

        setBoolFirebase("/home/estadoBtnPausa", false);

        //Caso esperando Pausa
        if(esperandoPausa) return;
        
        
        if(estadoPrensa == pausando){
            //Caso esperando sair Pausa
            tempoUltimaPausa = millis();
            estadoAnteriorPrensa = estadoPrensa;
            estadoPrensa = esperando;
            estadoAnteriorPausa = estadoBtnPausa;
            return;
        } else {
            //Caso acione Pausa
            tempoUltimaPausa = millis();
            esperandoPausa = 1;
            estadoAnteriorPausa = estadoBtnPausa;
            Serial.println("Estado Prensa: Esperando Pausa");
            return;
        }
        
    }
}

void soft(){    
    if(estadoPrensa == emergencia){
        analogWrite(PinoMotorDesce, 0);
        analogWrite(PinoMotorSobe, 0);
        return;
    }

    if(millis() - tempoUltimaAcao >= tempoDelay){
        tempoUltimaAcao = millis();

		if (pwmFuncao == movendo && pwmAtual < pwmMaximo) {
            pwmAtual += pwmPasso;
            Serial.print("Acelerando - ");
        } else if (pwmFuncao == trocando){
            pwmAtual -= pwmPasso;
            Serial.print("Parando - ");
        } else {
        	return;
        }

        if(pwmAtual > pwmMaximo) pwmAtual = pwmMaximo; //segurança adicional caso maior
        if(pwmAtual < 0) pwmAtual = 0; //segurança adicional caso menor

        if (estadoPrensa == subindo){
            analogWrite(PinoMotorDesce, 0);
            analogWrite(PinoMotorSobe, pwmAtual);
            Serial.print("Subindo: ");

        } else if(estadoPrensa == descendo) {
            analogWrite(PinoMotorDesce, pwmAtual);
            analogWrite(PinoMotorSobe, 0);            
            Serial.print("Descendo: ");
        } 

        Serial.println(String(map(pwmAtual, 0, pwmMaximo, 0, 100)) + "%");
    }
}

void lerBotao(){
    estadoBtnEmergencia = !digitalRead(PinoBtnEmergencia);
    estadoBtnPausa = !digitalRead(PinoBtnPausa);

    if(estadoBtnEmergencia == 0 && estadoAnteriorEmergencia){
        tempoUltimaEmergencia = millis();
        estadoAnteriorEmergencia = estadoBtnEmergencia;
    }

    if(estadoBtnPausa == 0 && estadoAnteriorPausa){
        tempoUltimaPausa = millis();
        estadoAnteriorPausa = estadoBtnPausa;
    }

}

//FUNÇÕES - LCD
void estadoSerial(EstadosPrensa estadoAtual){
    if(estadoAnteriorPrensa != estadoAtual) {
        Serial.println("Estado Prensa: " + String(listaEstadosPrensa[estadoAtual]));
        setIntFirebase("/home/estadoPrensa", estadoPrensa);
        estadoAnteriorPrensa = estadoPrensa;
        atualizarLcd(estadoPrensa);
    }
}

void atualizarLcd(EstadosPrensa estadoAtual){
  lcd.setCursor(0,0);
  lcd.print(nomeCompactador);
  lcd.setCursor(0,1);
  lcd.write(estadoAtual);
  lcd.print(listaEstadosPrensa[estadoAtual]);
  lcd.print("           ");
}

//FUNÇÕES - SENSOR
float ativarSensor(int PinoTrigger, int PinoEcho){
	digitalWrite(PinoTrigger, HIGH);
  	delayMicroseconds(10);
	digitalWrite(PinoTrigger, LOW);
  
	float valor = pulseIn(PinoEcho, HIGH, 30000);
  	
  	return valor/58; // divisão para transformar valor em centímetros
}

void lerSensor(int PinoTrigger, int PinoEcho, float &valorSensor){
    valorSensor = ativarSensor(PinoTrigger, PinoEcho);
    Serial.println("Sensor " + String(sensorAtual) + ": " + String(valorSensor)); 
    
    if(estadoPrensa != esperando){
        sensorAtual = 3;
    } else {
        sensorAtual = (sensorAtual == 2) ? 1 : sensorAtual + 1;
    }
    
    tempoUltimaLeitura = millis();
}


//FUNÇÕES - FIREBASE
bool getBoolFirebase(String caminho) {
  if (Firebase.ready()) {
    if (Firebase.RTDB.getBool(&fbdo, caminho.c_str())) {
      return fbdo.boolData();
    }
  }

  Serial.println("Erro ao ler bool:");
  Serial.println(fbdo.errorReason());
  return false;
}

bool setBoolFirebase(String caminho, bool valor) {
  if (Firebase.ready()) {
    if (Firebase.RTDB.setBool(&fbdo, caminho.c_str(), valor)) {
      return true;
    }
  }

  Serial.println("Erro ao gravar bool:");
  Serial.println(fbdo.errorReason());
  return false;
}

int getIntFirebase(String caminho) {
  if (Firebase.ready()) {
    if (Firebase.RTDB.getInt(&fbdo, caminho.c_str())) {
      return fbdo.intData();
    }
  }

  Serial.println("Erro ao ler int:");
  Serial.println(fbdo.errorReason());
  return -1;
}

bool setIntFirebase(String caminho, int valor) {
  if (Firebase.ready()) {
    if (Firebase.RTDB.setInt(&fbdo, caminho.c_str(), valor)) {
      return true;
    }
  }

  Serial.println("Erro ao gravar int:");
  Serial.println(fbdo.errorReason());
  return false;
}

String getStringFirebase(String caminho) {
  if (Firebase.ready()) {
    if (Firebase.RTDB.getString(&fbdo, caminho.c_str())) {
      return fbdo.stringData();
    }
  }

  Serial.println("Erro ao ler string:");
  Serial.println(fbdo.errorReason());
  return "";
}

bool setStringFirebase(String caminho, String valor) {
  if (Firebase.ready()) {
    if (Firebase.RTDB.setString(&fbdo, caminho.c_str(), valor)) {
      return true;
    }
  }

  Serial.println("Erro ao gravar string:");
  Serial.println(fbdo.errorReason());
  return false;
}


bool temInternet(){
    if (WiFi.status() != WL_CONNECTED) return false;
  
    HTTPClient http;

    http.begin("http://clients3.google.com/generate_204");

    int codigo = http.GET();

    http.end();

    return codigo == 204;
}