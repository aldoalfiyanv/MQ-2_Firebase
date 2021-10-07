
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>

//INITIALIZATIONS
#define FIREBASE_HOST "deteksi-nilai-polusi-dan-gas-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "pba33IpfJQU8cE0HDMQAjocfPwFE20pObr5yHiwl"
#define WIFI_SSID "Tani Rahayu"
#define WIFI_PASSWORD "12tanirahayu34" 


int sensorPin = A0;      //MQ-02 Sensor connected on

#define RL (5)                         //mendefinisikan Resistansi Beban (dalam kilo Ohm)
#define Ro_Clean_air (9.83)            //Faktor Udara Bersih (Berasal dari Lembar Data MQ-02)
#define Cal_sample_times  (50)         //mengambil 50 sampel di bagian kalibrasi
#define Cal_sample_interval  (500)     //penundaan antara setiap sampel saat kalibrasi (milidetik)

#define Read_sample_interval  (50)     //jumlah sampel dalam operasi normal
#define Read_sample_times  (5)         //penundaan antara setiap sampel saat kalibrasi

#define LPG_Gas (0)
#define SMOKE_Gas (1)

float LPGCurve[3] = {2.3,0.21,-0.47};   //dua titik diambil dari kurva (Berasal dari MQ-02 Datasheet)

float SMOKECurve[3] = {2.3,0.53,-0.44}; //dua titik diambil dari kurva (Berasal dari MQ-02 Datasheet)

float Ro = 10;                          //menginisialisasi Resistensi Ro dalam kilo ohm


void setup() {
  Serial.begin(115200);
  pinMode(sensorPin, INPUT);

  //Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH); //Connect to Firebase
  

  //Calibrate Sensor
  Serial.println("Calibrating Sensor...");
  Ro = MQCalibration(sensorPin);

  Serial.println("Calibration Completed.");
  Serial.print("Ro = ");
  Serial.print(Ro);
  Serial.println(" kohm");
  
}

void loop() {

  Serial.print("LPG : ");
  int lpg_value = MQGetGasPercentage(MQRead(sensorPin)/Ro,LPG_Gas);
  Firebase.set("lpg_value", lpg_value);                              //Send Value to Firebase Database
  Serial.print(lpg_value);
  Serial.println(" ppm");
  Serial.println("");

  Serial.print("SMOKE : ");
  int smoke_value = MQGetGasPercentage(MQRead(sensorPin)/Ro,SMOKE_Gas);
  Firebase.set("smoke_value", smoke_value);                          //Send Value to Firebase Database
  Serial.print(smoke_value);
  Serial.println(" ppm");
  Serial.println("");

  delay(200);

}


int  MQGetPercentage(float rs_ro_ratio, float *pcurve)
{
  return (pow(10,( ((log(rs_ro_ratio)-pcurve[1])/pcurve[2]) + pcurve[0])));
}


float MQCalibration(int sens_pin)
{
  int i;
  float val=0;

  for (i=0; i < Cal_sample_times; i++) {            //take multiple samples
    val += MQResistanceCalculation(analogRead(sens_pin));
    delay(Cal_sample_interval);
  }
  val = val/Cal_sample_times;                       //calculate the average value

  val = val/Ro_Clean_air;                                      //divided by RO yields the Ro (according to the chart in the datasheet)
                                                      

  return val; 
}


float MQResistanceCalculation(int raw_adc)
{
  return ( ((float)RL*(1023-raw_adc)/raw_adc));
}


float MQRead(int sens_pin)
{
  int i;
  float rs=0;

  for (i=0;i<Read_sample_times;i++) {
    rs += MQResistanceCalculation(analogRead(sens_pin));
    delay(Read_sample_interval);
  }

  rs = rs/Read_sample_times;

  return rs;  
}



int MQGetGasPercentage(float rs_ro_ratio, int gas_id)
{
  if ( gas_id == LPG_Gas ) {
     return MQGetPercentage(rs_ro_ratio,LPGCurve);
  } else if ( gas_id == SMOKE_Gas ) {
     return MQGetPercentage(rs_ro_ratio,SMOKECurve);
  }    

  return 0;
}
