#define BUTTON_PORT 7
#define AMOUNT_PORT A0
#define RLED_PORT 4//RGB LED 출력 포트
#define GLED_PORT 3
#define BLED_PORT 2
#define P_amount 500
#define commonAnode false
//Common Cathode 사용중  음극이 같은 상태

#include <Stepper.h>
#include <Servo.h>
#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include <SoftwareSerial.h> //블루투스 시리얼 통신 라이브러리 추가
SoftwareSerial mySerial(10,11);//블루투스 설정 BTSerial(RX,TX)
float r,g,b;
float c,m,y,w;
int amount;
const int stepsPerRevolution = 2048; 
int ratio[5]={0,0,0,0,0};
int data[3]={-1,-1,-1};
byte gammatable[256];

Servo myservo;
Stepper MC(stepsPerRevolution,29,25,27,23);
Stepper MM(stepsPerRevolution,37,33,35,31);
Stepper MY(stepsPerRevolution,45,41,43,39);
Stepper MW(stepsPerRevolution,53,49,51,47);

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  MotorSetting();
  GammaSetting();

  pinMode(BUTTON_PORT,INPUT);//버튼 입력 받는 곳
  pinMode(AMOUNT_PORT,INPUT);//가변 저항 입력 받는 곳 (가변저항 다리 중 가운데 나머지 첫번째 다리와 마지막 다리는 각각 5v와 GND에 연결해야됨)
  if (tcs.begin()) {
    Serial.println("센서 연결됨");
  } else {
    Serial.println("TCS34725 센서를 찾을 수 없습니다.");
    while (1);
  }
 tcs.begin();
}
//아래 세 함수는 색상의 비율을 구해내는 함수들

void loop() {
  
  //아래 코드는 어플을 통해 색을 출력하는경우
  
  if (mySerial.available() && data[0]==-1 && data[1]==-1 && data[2]==-1) {       
      data[0]=mySerial.parseInt();
  }
  
  if (mySerial.available() &&  data[0]!=-1 && data[1]==-1 && data[2]==-1) {       
      data[1]=mySerial.parseInt();
  }
  
  if (mySerial.available() &&  data[0]!=-1 && data[1]!=-1 && data[2]==-1) {       
      data[2]=mySerial.parseInt();
      Serial.println(data[0]);
      Serial.println(data[1]);
      Serial.println(data[2]);
      
    float r_=data[0]/255.0;
    float g_=data[1]/255.0;
    float b_=data[2]/255.0;//각 rgb값을 계산의 용의를 위해 1보다 작은 값으로 변경
    //_는 RGB의 1보다 작거나 같은 비율 값 (실제 RGB값을 각각 255로 나눈 값)
    float w_ = getwhite(r_,g_,b_);//하얀색을 구하고
    float c_ = rgbtocmy(r_,w_);//하얀색을 r을 고려하여 c_ 계산
    float m_ = rgbtocmy(g_,w_);//하얀색과 g를 고려하여 m_ 계산
    float y_ = rgbtocmy(b_,w_);//하얀색과 b를 고려하여 y_ 계산
    amount=allpaint(analogRead(AMOUNT_PORT));//0~1023 단계로 물감량 조절가능
    setglobal(c_,m_,y_,w_);//c_,m_,y_,w_를 c,m,y,w의 전역 변수에 할당 및 변환
    MotorControl();//물감 짜기
    data[0]=-1;
    data[1]=-1;
    data[2]=-1;
  }

  
  //아래 코드는 펜을 통해 색을 출력하는경우
  
  if (digitalRead(BUTTON_PORT)==1){

    tcs.getRGB(&r, &g, &b);//rgb값을 얻음
    float r_=gammatable[(int)r]/255.0;
    float g_=gammatable[(int)g]/255.0;
    float b_=gammatable[(int)b]/255.0;//각 rgb값을 계산의 용의를 위해 1보다 작은 값으로 변경
    //_는 RGB의 1보다 작거나 같은 비율 값 (실제 RGB값을 각각 255로 나눈 값)
    float w_ = getwhite(r_,g_,b_);//하얀색을 구하고
    float c_ = rgbtocmy(r_,w_);//하얀색을 r을 고려하여 c_ 계산
    float m_ = rgbtocmy(g_,w_);//하얀색과 g를 고려하여 m_ 계산
    float y_ = rgbtocmy(b_,w_);//하얀색과 b를 고려하여 y_ 계산
    amount=allpaint(analogRead(AMOUNT_PORT));//0~1023 단계로 물감량 조절가능
    setglobal(c_,m_,y_,w_);//c_,m_,y_,w_를 c,m,y,w의 전역 변수에 할당 및 변환

    PrintSerial();//시리얼로 색깔 출력
    PrintLED();//RGB LED로 출력
    int start=millis();//타이머 시작 //계속 연속으로 버튼을 누를때 잠시 멈추게 함
    while (digitalRead(BUTTON_PORT)==1){
      int dt = millis()-start;//타이머 종료 밀리초는 10^(-3)
      
      if (dt>=2000){//2초 이상 눌렸을때 실행 (if와 else로 하지 않은 이유는 처음부터 2초 이상 눌렀을때도 상관없이 항상 rgb값과 센서값을 측정해야 되기 때문이다.
        MotorControl();//물감 짜기
        while (digitalRead(BUTTON_PORT)==1);//계속 실행시 잠시 멈춤
      }
    }
  }
}

void MotorSetting(){
  MC.setSpeed(8);
  MM.setSpeed(8);
  MY.setSpeed(8);
  MW.setSpeed(8);
  myservo.attach(5);
  for (int i =-40; i<=70 ;i+=10){
    Serial.println(i);
    myservo.write(i);
    delay (50);
  }
  delay (1000);
  myservo.write(55);
  delay (1000);
  myservo.detach();
}
void GammaSetting(){
  for (int i=0; i<256; i++) {
    float x = i;
    x /= 255;
    x = pow(x, 2.5);
    x *= 255;

    if (commonAnode) {
      gammatable[i] = 255 - x;
    } else {
      gammatable[i] = x;
    }
    //Serial.println(gammatable[i]);
  }
}
float getwhite(float r_, float g_, float b_){//하얀색 값은 그렇게 중요하지 않음
  //하얀색과 R,G,B의 최대값이 비례한다는 것만 밝히면됨
  //하얀색이 무엇이든 간에 C,M,Y는 이에따라 자동 조절됨
  float list[3]={r_,g_,b_};//최댓값 구하기
  float maxi=0;
  for (int i = 0;i<3;i++){
    if (list[i]>=maxi){
      maxi = list[i];
    }
  }
  return maxi;
}
float rgbtocmy(float p_,float w_){//rgb값을 화이트 값을 고려하여 실수형으로 변경
  return (w_-p_)/(w_*1.0);
}
void setglobal(float c_,float m_, float y_, float w_){//글로벌 변수로 변경
  c = P_amount * (c_+(1-w_));
  m = P_amount * (m_+(1-w_));
  y = P_amount * (y_+(1-w_));
  w = P_amount * w_;
}
int allpaint(float a){
  return a/200;
}
//시리얼 출력 함수
void PrintSerial(){
    Serial.print("R: "); Serial.print((int)r); Serial.print(" ");
    Serial.print("G: "); Serial.print((int)g); Serial.print(" ");
    Serial.print("B: "); Serial.print((int)b); Serial.print(" ");
    Serial.println(" ");
    Serial.print("C: "); Serial.print((int)c); Serial.print(" ");
    Serial.print("M: "); Serial.print((int)m); Serial.print(" ");
    Serial.print("Y: "); Serial.print((int)y); Serial.print(" ");
    Serial.print("W: "); Serial.print((int)w); Serial.print(" ");
    Serial.println(" ");
    Serial.print("A: "); Serial.println(amount);
    Serial.println(" ");
Serial.println(" ");
}
//물감 출력함수
void MotorControl(){
    myservo.attach(5);
    myservo.write(10);
    delay(1000);       
    for(int i=0;i<=amount;i++){
      MC.step(-c);
      delay(100);
      MM.step(-m);
      delay(100);
      MY.step(-y);
      delay(100);
      MW.step(-w/2);
      delay(100);
    }
    delay(1000);
    for (int i =10;i<=70;i+=1){
      myservo.write(i);
      delay (50);
    }
    myservo.detach();
}
//LED 표시 함수
void PrintLED(){
  analogWrite(RLED_PORT, gammatable[(int)r]);
  analogWrite(GLED_PORT, gammatable[(int)g]);
  analogWrite(BLED_PORT, gammatable[(int)b]);
}
