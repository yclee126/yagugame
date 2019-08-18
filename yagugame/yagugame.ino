//yagugame v2.4
//
//Coded by yclee126 2017/09/17 ~ 2018/09/18
//(playground version)
//
//arduino pro mini used

#include<Wire.h>
#include<EEPROM.h>

byte interfacetype = 1; //0:가변저항 1:로터리인코더 2:mpu
const byte interfacenums = 3; //인터페이스 종류 갯수
const byte additionalsettings = 1; //추가 설정 항목 갯수
const byte additionalspace = 2; //추가 EEPROM 공간 바이트 수
const int boundary = 1013; //가변저항 사용시 최대 신호
const byte next = A7; //next 버튼의 핀 위치
const byte digitalnext = false; //next핀의 디지털 여부
const byte numinterface = A6; //로터리 엔코더의 핀 위치
const boolean sg1 = 1; //0 => +극 공통 / 1 => -극 공통
/* 세그먼트 위치
---------seg1---------
----------------------
---seg6--------seg2---
----------------------
---------seg7---------
----------------------
---seg5--------seg3---
----------------------
---------seg4---------
-----------------seg8-

/* digit 위치와 스위치 위치
-digit1--digit2--digit3--digit4--(로터리인코더)--digit5--digit6--digit7--digit8-
-스위치------------------------------------------------------------------스위치-

*/
const byte seg1 = 9;
const byte seg2 = 7;
const byte seg3 = 5;
const byte seg4 = 4;
const byte seg5 = 3;
const byte seg6 = 8;
const byte seg7 = 6;
const byte seg8 = 2;
byte digit1 = A3;
byte digit2 = A2;
byte digit3 = A1;
byte digit4 = A0;
byte digit5 = 13;
byte digit6 = 12;
byte digit7 = 11;
byte digit8 = 10;

byte p1[4]; //실제 p1의 숫자
byte p2[4]; //실제 p2의 숫자
byte p11[4]; //p1의 예측
byte p22[4]; //p2의 예측
byte error = 0; //같은숫자 체크용 변수
byte strike = 0; //스트라이크 갯수
byte ball = 0; //볼 갯수
char credit[] = "        CODED BY YCLEE126 2018-09-15 vEr 2.4        "; //만든이 문장
char byteviewalarmc = ""; //put in binary 8seg data
char hiddenmsg = ""; //put in ascii characters
unsigned long tmptime; //millis() 값을 임시로 저장할 변수
boolean playertwo; //1p/2p 모드 설정 변수
byte playcount[2];
const int msgtime = 1000; //메시지 전환 시간

const boolean sg0 = !sg1;
const boolean dg1 = !sg1;
const boolean dg0 = sg1;

const int MPU_addr=0x68;  // I2C address of the MPU-6050
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
long reksum;
unsigned long sum;

byte tdigit1;
byte tdigit2;
byte tdigit3;
byte tdigit4;
boolean digitswapstate = 0;

byte buf = 0; //인터페이스의 값을 임시로 저장할 변수
char wheelstate;
char delta;
char prevwheelstate;
char usernum;
char userbuf;
byte wheelbuffersize = 6; //휠 감도
char titlenum;

unsigned long dottime;
byte prevdotdigit;
boolean dotstate = 0;
byte dotdigit;

int dataaddr;
int indexaddr;

byte refreshrate = 2;
boolean counttick = 0;

int microtime = 13;
byte selecttype;

void setup() {
//  Serial.begin(115200); //디버깅용
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
  getrandomseed();
  pinMode(seg1,OUTPUT);
  pinMode(seg2,OUTPUT);
  pinMode(seg3,OUTPUT);
  pinMode(seg4,OUTPUT);
  pinMode(seg5,OUTPUT);
  pinMode(seg6,OUTPUT);
  pinMode(seg7,OUTPUT);
  pinMode(seg8,OUTPUT);
  pinMode(digit1,OUTPUT);
  pinMode(digit2,OUTPUT);
  pinMode(digit3,OUTPUT);
  pinMode(digit4,OUTPUT);
  pinMode(digit5,OUTPUT);
  pinMode(digit6,OUTPUT);
  pinMode(digit7,OUTPUT);
  pinMode(digit8,OUTPUT);
  if(digitalnext) pinMode(next,OUTPUT);
  if(checkeeprom('Y')){
    interfacetype = EEPROM.read(dataaddr);
    wheelbuffersize = EEPROM.read(dataaddr+1);
  }
  else{
    for (int i = 0 ; i < EEPROM.length() ; i++) {
      EEPROM.update(i, 0);
      sevseg('i','n','i','t',i/1000%10,i/100%10,i/10%10,i%10);
    }
    indexaddr = random(0,EEPROM.length()-additionalspace);
    dataaddr = indexaddr + 1;
    EEPROM.update(indexaddr,'Y');
    EEPROM.update(dataaddr,interfacetype);
    EEPROM.update (dataaddr+1,wheelbuffersize);
  }
  switch(wheelbuffersize){
    case 2:
    case 3:
      refreshrate = 2;
      break;
    case 4:
    case 5:
      refreshrate = 1;
      break;
    default:
      refreshrate = 0;
      break;
  }
}

void loop() {
//  while(1) sevseg('[',2,5,']','t','u','r','n'); //FND 테스트용
//부팅화면 표시 (컨셉 & 디버깅 목적)
  tmptime = millis();
  while(millis() - tmptime < 1000) sevseg(10,10,10,10,10,10,10,10);
  
selecttype:
  getrandomseed();
  ball = 0;
  strike = 0;
  playcount[0] = 0;
  playcount[1] = 0;
  usernum = 0;
  resetdigit(); 
  while(!nextswitch()){
  selecttype = selectnum();
    switch(selecttype){
      case 0:
        sevseg('1','P','L','Y','-','-','-','-');
        break;
      case 1:
        sevseg('-','-','-','-','1','P','L','Y');
        break;
      case 2:
        sevseg('1','P','L','Y','2','P','L','Y');
        break;
      case 3:
        sevseg('2','P','L','Y','1','P','L','Y');
        break;
    }
  }
  switch(selecttype){
    case 0:
    playertwo = false;
    break;
  case 1:
    playertwo = false;
    digitswap();
    break;
  case 2:
    playertwo = true;
    break;
  case 3:
    playertwo = true;
    digitswap();
    break;
  }
  tmptime = millis();
  while(nextswitch()){
    if(millis() - tmptime >= 1000){
      if(settingsmenu()){
        dotdigit = 0;
        goto selecttype;
      }
    }
  }
  if(playertwo){
//p1 숫자설정
    if(getnumdisplay(p1,'S','E','T','1',true)) goto selecttype;
    turntozero();
//p2 숫자설정
    digitswap();
    getnumdisplay(p2,'S','E','T','2',true);
    digitswap();
    turntozero();
  }
  else{            //1인용 모드이면 p2숫자 자동생성
    while(error != 4){
      error = 0;
      for(int i = 0;i<4;i++) p2[i] = random(10);
      for(byte i = 0;i<4;i++){
        for(byte k = 0;k<4;k++){
          if(p2[k] == p2[i]) error++;
        }
      }
    }
  for(int i=0;i<4;i++) p1[i] = '-';
  }
  while(1){ //게임이 끝날 때까지 계속
    tickplaycounter();
    getnumdisplay(p11,'-','P','1','-',true); //p1 시작
    if(judgenum(p1,p11,p2)) goto selecttype; //결과도출
    if(playertwo){ //p2시작
      digitswap();
      getnumdisplay(p22,'-','P','2','-',true); //p2 시작
      if(judgenum(p2,p22,p1)) goto selecttype; //결과도출
      digitswap();
    }
  }
}

void blinkdot(byte ddgt){ //숫자 입력시 점 깜박임 함수
  switch (ddgt){
    case 1:
      dotdigit = digit1;
      break;
    case 2:
      dotdigit = digit2;
      break;
    case 3:
      dotdigit = digit3;
      break;
    case 4:
      dotdigit = digit4;
      break;
    case 5:
      dotdigit = digit5;
      break;
    case 6:
      dotdigit = digit6;
      break;
    case 7:
      dotdigit = digit7;
      break;
    case 8:
      dotdigit = digit8;
      break;
  }
  if(prevdotdigit != dotdigit) dottime = millis();
  dotstate = !bitRead(millis() - dottime,8);
  prevdotdigit = dotdigit;
}

void sevseg(byte a,byte b,byte c,byte d,byte e,byte f,byte g,byte h){ //digit전환
  sevswitch(a);
  digitalWrite(digit1,dg1);
  getusernum();
  digitalWrite(digit1,dg0);
  sevswitch(b);
  digitalWrite(digit2,dg1);
  getusernum();
  digitalWrite(digit2,dg0);
  sevswitch(c);
  digitalWrite(digit3,dg1);
  getusernum();
  digitalWrite(digit3,dg0);
  sevswitch(d);
  digitalWrite(digit4,dg1);
  getusernum();
  digitalWrite(digit4,dg0);
  sevswitch(e);
  digitalWrite(digit5,dg1);
  getusernum();
  digitalWrite(digit5,dg0);
  sevswitch(f);
  digitalWrite(digit6,dg1);
  getusernum();
  digitalWrite(digit6,dg0);
  sevswitch(g);
  digitalWrite(digit7,dg1);
  getusernum();
  digitalWrite(digit7,dg0);
  sevswitch(h);
  digitalWrite(digit8,dg1);
  getusernum();
  digitalWrite(digit8,dg0);
  sevswitch(11);
  digitalWrite(dotdigit,dg1);
  getusernum();
  digitalWrite(dotdigit,dg0);
}

void sevswitch(byte ch){ //segment 데이터
  switch (ch){
    case 'O':
    case '0':
    case 0:
      sevwrite(sg1,sg1,sg1,sg1,sg1,sg1,sg0,sg0);
      break;
    case 'i':
    case 'I':
    case '1':  
    case 1:
      sevwrite(sg0,sg1,sg1,sg0,sg0,sg0,sg0,sg0);
      break;
    case '2':
    case 2:
      sevwrite(sg1,sg1,sg0,sg1,sg1,sg0,sg1,sg0);
      break;
    case '3': 
    case 3:
      sevwrite(sg1,sg1,sg1,sg1,sg0,sg0,sg1,sg0);
      break;
    case '4':
    case 4:
      sevwrite(sg0,sg1,sg1,sg0,sg0,sg1,sg1,sg0);
      break;
    case 's':
    case 'S':
    case '5':
    case 5:
      sevwrite(sg1,sg0,sg1,sg1,sg0,sg1,sg1,sg0);
      break;
    case '6':
    case 6:
      sevwrite(sg1,sg0,sg1,sg1,sg1,sg1,sg1,sg0);
      break;
    case '7':
    case 7:
      sevwrite(sg1,sg1,sg1,sg0,sg0,sg1,sg0,sg0);
      break;
    case '8': 
    case 8:
      sevwrite(sg1,sg1,sg1,sg1,sg1,sg1,sg1,sg0);
      break;
    case '9':
    case 9:
      sevwrite(sg1,sg1,sg1,sg1,sg0,sg1,sg1,sg0);
      break;
    case 'B':
    case 'b':
      sevwrite(sg0,sg0,sg1,sg1,sg1,sg1,sg1,sg0);
      break;
    case 'h':
    case 'H':
      sevwrite(sg0,sg1,sg1,sg0,sg1,sg1,sg1,sg0);
      break;
    case 'R':
    case 'r':
      sevwrite(sg0,sg0,sg0,sg0,sg1,sg0,sg1,sg0);
      break;
    case 'w':
    case 'v':
    case 'u':
      sevwrite(sg0,sg0,sg1,sg1,sg1,sg0,sg0,sg0);
      break;
    case 'M':
    case 'm':
    case 'n':
      sevwrite(sg0,sg0,sg1,sg0,sg1,sg0,sg1,sg0);
      break;
    case ' ':
      sevwrite(sg0,sg0,sg0,sg0,sg0,sg0,sg0,sg0);
      break;
    case 'e':
    case 'E':
      sevwrite(sg1,sg0,sg0,sg1,sg1,sg1,sg1,sg0);
      break;
    case 'o':
      sevwrite(sg0,sg0,sg1,sg1,sg1,sg0,sg1,sg0);
      break;
    case '-':
      sevwrite(sg0,sg0,sg0,sg0,sg0,sg0,sg1,sg0);
      break;
    case 'T':
    case 't':
      sevwrite(sg0,sg0,sg0,sg1,sg1,sg1,sg1,sg0);
      break;
    case 'P':
    case 'p':
      sevwrite(sg1,sg1,sg0,sg0,sg1,sg1,sg1,sg0);
      break;
    case 'C':
      sevwrite(sg1,sg0,sg0,sg1,sg1,sg1,sg0,sg0);
      break;
    case 'c':
      sevwrite(sg0,sg0,sg0,sg1,sg1,sg0,sg1,sg0);
      break;
    case 'D':
    case 'd':
      sevwrite(sg0,sg1,sg1,sg1,sg1,sg0,sg1,sg0);
      break;
    case 'Y':
    case 'y':
      sevwrite(sg0,sg1,sg1,sg1,sg0,sg1,sg1,sg0);
      break;
    case 'g':
    case 'G':
      sevwrite(sg1,sg0,sg1,sg1,sg1,sg1,sg0,sg0);
      break;
    case 'l':
    case 'L':
      sevwrite(sg0,sg0,sg0,sg1,sg1,sg1,sg0,sg0);
      break;
    case 'a':
    case 'A':
      sevwrite(sg1,sg1,sg1,sg0,sg1,sg1,sg1,sg0);
      break;
    case 'N':
      sevwrite(sg1,sg1,sg1,sg0,sg1,sg1,sg0,sg0);
      break;
    case '.':
      sevwrite(sg0,sg0,sg0,sg0,sg0,sg0,sg0,sg1);
      break;
    case 10://boot 화면
      sevwrite(sg1,sg1,sg1,sg1,sg1,sg1,sg1,sg1);
      break;
	case 11://blink dot
      sevwrite(sg0,sg0,sg0,sg0,sg0,sg0,sg0,dotstate?sg1:sg0);
      dotstate = 0;
      break;
    case 'f':
    case 'F':
      sevwrite(sg1,sg0,sg0,sg0,sg1,sg1,sg1,sg0);
      break;
    case 'j':
    case 'J':
      sevwrite(sg1,sg1,sg1,sg1,sg1,sg0,sg0,sg0);
      break;
    case 'k':
    case 'K':
      sevwrite(sg0,sg1,sg0,sg1,sg1,sg1,sg1,sg0);
      break;
    case 'q':
    case 'Q':
      sevwrite(sg1,sg1,sg1,sg0,sg0,sg1,sg1,sg0);
      break;
    case 'W':
    case 'U':
      sevwrite(sg0,sg1,sg1,sg1,sg1,sg1,sg0,sg0);
      break;
    case 'V':
      sevwrite(sg0,sg1,sg0,sg1,sg0,sg1,sg0,sg0);
      break;
    case 'x':
    case 'X':
      sevwrite(sg0,sg1,sg1,sg0,sg1,sg1,sg0,sg0);
      break;
    case 'z':
    case 'Z':
      sevwrite(sg1,sg1,sg0,sg1,sg1,sg0,sg0,sg0);
      break;
    case '?':
      sevwrite(sg1,sg1,sg0,sg1,sg0,sg0,sg0,sg0);
      break;
    case '!':
      sevwrite(sg1,sg1,sg0,sg1,sg0,sg1,sg1,sg0);
      break;
    case '[': //「
      sevwrite(sg1,sg0,sg0,sg0,sg0,sg1,sg0,sg0);
      break;
    case ']': // 」
      sevwrite(sg0,sg0,sg1,sg1,sg0,sg0,sg0,sg0);
      break;
	default:
      sevwrite(bitRead(ch,0)?sg1:sg0,bitRead(ch,1)?sg1:sg0,bitRead(ch,2)?sg1:sg0,bitRead(ch,3)?sg1:sg0,bitRead(ch,4)?sg1:sg0,bitRead(ch,5)?sg1:sg0,bitRead(ch,6)?sg1:sg0,bitRead(ch,7)?sg1:sg0);
      break;
  }
}

void sevwrite(byte one,byte two,byte three,byte four,byte five,byte six,byte seven,byte eight){ //segment 전환
  digitalWrite(seg1,one);
  digitalWrite(seg2,two);
  digitalWrite(seg3,three);
  digitalWrite(seg4,four);
  digitalWrite(seg5,five);
  digitalWrite(seg6,six);
  digitalWrite(seg7,seven);
  digitalWrite(seg8,eight);
}

byte getusernum(){ //숫자 입력
  switch (interfacetype){
    case 0:
       //가변저항 사용시
    int sensorvalue; 
    sensorvalue = analogRead(numinterface);
    if(sensorvalue <= 1) return 0;
    else{
      if(map(sensorvalue,0,boundary,1,10) == 10) return 0;
      else return map(sensorvalue,0,boundary,1,10);
    }
    break;
  case 1:
//로터리 인코더 사용시
    if(bitRead(millis(),refreshrate) == 1 && counttick){
      wheelstate = map(analogRead(numinterface),-93,123,0,1);
      delta = prevwheelstate - wheelstate + 3;
      switch(delta){
        case 2:
        userbuf--;
        break;
      case 6:
        userbuf--;
        break;
      case 4:
        userbuf++;
        break;
      case 0:
        userbuf++;
        break;
      default:
  //안움직이면 아무것도안함
        break;
      }
    if(userbuf == wheelbuffersize + 1){
      usernum++;
      if(usernum == 10) usernum = 0;
      userbuf = 1;
    }
    if(userbuf == 0){
      usernum--;
      if(usernum == -1) usernum = 9;
      userbuf = wheelbuffersize;
    }
  /* 로터리인코더 디버깅
  //  if(wheelstate != prevwheelstate){
      Serial.print("usernum : ");
      Serial.print(usernum,DEC);
      Serial.print(" , wheelstate : ");
      Serial.print(wheelstate,DEC);
      Serial.print(" , userbuf : ");
      Serial.println(userbuf,DEC);
  //  }
  */
      prevwheelstate = wheelstate;
      counttick = 0;
      //microdelaytwo();
      return usernum;
      break;
    }
    else if(!bitRead(millis(),refreshrate) && !counttick){
        counttick = 1;
        microdelay();
        return usernum;
    }
    else{
      microdelay();
      return usernum;
    }
  }
}

void microdelay(){
  double uselesstimer;
  for(uselesstimer = 0;uselesstimer<microtime;uselesstimer++);
}

void microdelaytwo(){
  double uselesstimer;
}

boolean nextswitch(){
  if(digitalnext) return digitalRead(next);
  else return constrain(analogRead(next)-750,0,1);
}

void digitswap(){
  tdigit1 = digit1;
  tdigit2 = digit2;
  tdigit3 = digit3;
  tdigit4 = digit4;
  digit1 = digit5;
  digit2 = digit6;
  digit3 = digit7;
  digit4 = digit8;
  digit5 = tdigit1;
  digit6 = tdigit2;
  digit7 = tdigit3;
  digit8 = tdigit4;
  digitswapstate = !digitswapstate;
}

void resetdigit(){
  if(digitswapstate) digitswap();
}

void turntozero(){
  switch (interfacetype){
    case 0:
//가변저항 사용시
      while(getusernum() != 0){
        tmptime = millis();
        while(millis() - tmptime <= msgtime && getusernum() != 0) sevseg('t','u','r','n','t','o',' ','0');
        tmptime = millis();
        while(millis() - tmptime <= msgtime && getusernum() != 0) sevseg(' ',' ',' ',' ',' ',' ',' ',' ');
      }
      break;
    case 1:
//로터리 인코더 사용시
      usernum = 0;
      break;
  }
}

byte selectnum(){
  switch (interfacetype){
    case 0:
//가변저항 이용시
      if(map(analogRead(numinterface),0,boundary,0,4) == 4) return 3;
      else return map(analogRead(numinterface),0,boundary,0,4);
      break;
    case 1:
//로터리 인코더 사용시
      switch(usernum){
        case 4:
          usernum = 0;
          break;
        case 9:
          usernum = 3;
          break;
      }
      return usernum;
  }
}

void getrandomseed(){
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,14,true);  // request a total of 14 registers
  AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)     
  AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
  GyX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
  GyY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
  GyZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
  reksum = AcX + AcY + AcZ + Tmp + GyX + GyY + GyZ;
  sum = abs(reksum);
  randomSeed(sum);
}

boolean getnumdisplay(byte playernum[],char display1,char display2,char display3,char display4,boolean errorcheck){
  usernum = 0;
  do{
    while(!nextswitch()){ //1번째자리
      buf = getusernum();
      blinkdot(1);
      sevseg(buf,'-','-','-',display1,display2,display3,display4);
    }
    playernum[0] = buf;
    while(nextswitch()) sevseg(buf,'-','-','-',display1,display2,display3,display4);
    
    while(!nextswitch()){ //2번째자리
      buf = getusernum();
      blinkdot(2);
      sevseg(playernum[0],buf,'-','-',display1,display2,display3,display4);
    }
    playernum[1] = buf;
    while(nextswitch()) sevseg(playernum[0],buf,'-','-',display1,display2,display3,display4);
    
    while(!nextswitch()){ //3번째자리
      buf = getusernum();
      blinkdot(3);
      sevseg(playernum[0],playernum[1],buf,'-',display1,display2,display3,display4);
    }
    playernum[2] = buf;
    while(nextswitch()) sevseg(playernum[0],playernum[1],buf,'-',display1,display2,display3,display4);
    
    while(!nextswitch()){ //4번째자리
      buf = getusernum();
      blinkdot(4);
      sevseg(playernum[0],playernum[1],playernum[2],buf,display1,display2,display3,display4);
    }
    playernum[3] = buf;
    while(nextswitch()) sevseg(playernum[0],playernum[1],playernum[2],buf,display1,display2,display3,display4);

    if(playcount[0] == 0 && playcount[1] == 0){
      if(playernum[0] == 3 && playernum[1] == 6 && playernum[2] == 2 && playernum[3] == 2){
        showcredit(); //이스터 에그
        return 1;
      }
      if(playernum[0] == 6 && playernum[1] == 0 && playernum[2] == 5 && playernum[3] == 0){
        mputest(); //mpu6050 디버깅용
        return 1;
      }
      if(playernum[0] == 5 && playernum[1] == 3 && playernum[2] == 2 && playernum[3] == 3){
        vieweepromadr(); //EEPROM 주소 확인
        return 1;
      }
      if(playernum[0] == 0 && playernum[1] == 0 && playernum[2] == 0 && playernum[3] == 4){
        adjustdisplay(); //디스플레이 조정
        return 1;
      }
    }

    if(errorcheck){
      error = 0;
      for(byte i = 0;i<4;i++){ //오류체크
        for(byte k = 0;k<4;k++){
          if(playernum[k] == playernum[i]) error++;
        }
      }
      
      if(error != 4){ //오류시 메시지 출력
        tmptime = millis();
        while(millis() - tmptime < 1000) sevseg('E','r','o','r',display1,display2,display3,display4);
        error = 0;
      }
    }
    else error = 4;
  } while(error == 0);//오류검출시 계속실행
  return 0;
}

boolean judgenum(byte playernum[],byte guess[],byte opponent[]){
  for(int i=0;i<4;i++){
    if(opponent[i] == guess[i]) strike++;
    for(int k=0; k<4; k++){
      if(opponent[k] == guess[i]) ball++;
    }
  }
  ball = ball - strike;
  if(strike == 0 && ball == 0){
    while(!nextswitch()){
      tmptime = millis();
      while(millis() - tmptime <= msgtime && !nextswitch()) sevseg('o','u','t',' ',guess[0],guess[1],guess[2],guess[3]);
      tmptime = millis();
      while(millis() - tmptime <= msgtime && !nextswitch()) sevseg(' ',' ',' ',' ',guess[0],guess[1],guess[2],guess[3]);
    }
  }
  else{
    if(strike == 4){
      while(!nextswitch()){
        tmptime = millis();
        while(millis() - tmptime <= msgtime && !nextswitch()) sevseg('H','r','u','n',guess[0],guess[1],guess[2],guess[3]);
        if(playertwo){
          tmptime = millis();
          while(millis() - tmptime <= msgtime && !nextswitch()) sevseg(playernum[0],playernum[1],playernum[2],playernum[3],guess[0],guess[1],guess[2],guess[3]);
        }
        tmptime = millis();
        while(millis() - tmptime <= msgtime && !nextswitch()) sevseg('[',playcount[1],playcount[0],']','t','u','r','n');
      }
      while(nextswitch());
      return 1;
    }
    else{
      while(!nextswitch()){
        tmptime = millis();
        while(millis() - tmptime <= 500 && !nextswitch()) sevseg(strike,'s',ball,'b',guess[0],guess[1],guess[2],guess[3]);
        tmptime = millis();
        while(millis() - tmptime <= 500 && !nextswitch()) sevseg(strike,' ',ball,' ',guess[0],guess[1],guess[2],guess[3]);
      }
    }
  }
  while(nextswitch());
  strike = 0;
  ball = 0;
  return 0;
}

void tickplaycounter(){
  if(playcount[1] != '-'){
    playcount[0]++;
    if(playcount[0] == 10){
      playcount[0] = 0;
      playcount[1]++;
    }
    if(playcount[1] == 10){
      playcount[0] = '-';
      playcount[1] = '-';
    }
  }
}

void showcredit(){  //이스터 에그
  resetdigit();
  usernum = 6;
  int startpoint;
  int bytedoor = 0;
  boolean pressed = 1;
  for(int i=0;1;i++){
    tmptime = millis();
    if(credit[i+7] == 0){
      startpoint = i;
      break;
    }
    while(millis() - tmptime <= (10 - getusernum())*100){
      sevseg(credit[i],credit[i+1],credit[i+2],credit[i+3],credit[i+4],credit[i+5],credit[i+6],credit[i+7]);
      if(nextswitch() && pressed){
        bytedoor++;
        pressed = 0;
      }
      if(!nextswitch() && !pressed) pressed = 1;
    }
  }
  if(bytedoor > 11){
    if(nextswitch()){
      for(int i=startpoint;1;i++){
        tmptime = millis();
        while(millis() - tmptime <= (10 - getusernum())*100) byteview(credit[i],credit[i+1],credit[i+2],credit[i+3],credit[i+4],credit[i+5],credit[i+6],credit[i+7]);
      }
    }
  }
  else{
    if(nextswitch()){
      for(int i=startpoint;1;i++){
        tmptime = millis();
        while(millis() - tmptime <= (10 - getusernum())*100) sevseg(credit[i],credit[i+1],credit[i+2],credit[i+3],credit[i+4],credit[i+5],credit[i+6],credit[i+7]);
      }
    }
  }
}

void byteview(byte a,byte b,byte c,byte d,byte e,byte f,byte g,byte h){
  sevwrite(bitRead(a,0)?sg1:sg0,bitRead(a,1)?sg1:sg0,bitRead(a,2)?sg1:sg0,bitRead(a,3)?sg1:sg0,bitRead(a,4)?sg1:sg0,bitRead(a,5)?sg1:sg0,bitRead(a,6)?sg1:sg0,bitRead(a,7)?sg1:sg0);
  digitalWrite(digit1,dg1);
  getusernum();
  digitalWrite(digit1,dg0);
  sevwrite(bitRead(b,0)?sg1:sg0,bitRead(b,1)?sg1:sg0,bitRead(b,2)?sg1:sg0,bitRead(b,3)?sg1:sg0,bitRead(b,4)?sg1:sg0,bitRead(b,5)?sg1:sg0,bitRead(b,6)?sg1:sg0,bitRead(b,7)?sg1:sg0);
  digitalWrite(digit2,dg1);
  getusernum();
  digitalWrite(digit2,dg0);
  sevwrite(bitRead(c,0)?sg1:sg0,bitRead(c,1)?sg1:sg0,bitRead(c,2)?sg1:sg0,bitRead(c,3)?sg1:sg0,bitRead(c,4)?sg1:sg0,bitRead(c,5)?sg1:sg0,bitRead(c,6)?sg1:sg0,bitRead(c,7)?sg1:sg0);
  digitalWrite(digit3,dg1);
  getusernum();
  digitalWrite(digit3,dg0);
  sevwrite(bitRead(d,0)?sg1:sg0,bitRead(d,1)?sg1:sg0,bitRead(d,2)?sg1:sg0,bitRead(d,3)?sg1:sg0,bitRead(d,4)?sg1:sg0,bitRead(d,5)?sg1:sg0,bitRead(d,6)?sg1:sg0,bitRead(d,7)?sg1:sg0);
  digitalWrite(digit4,dg1);
  getusernum();
  digitalWrite(digit4,dg0);
  sevwrite(bitRead(e,0)?sg1:sg0,bitRead(e,1)?sg1:sg0,bitRead(e,2)?sg1:sg0,bitRead(e,3)?sg1:sg0,bitRead(e,4)?sg1:sg0,bitRead(e,5)?sg1:sg0,bitRead(e,6)?sg1:sg0,bitRead(e,7)?sg1:sg0);
  digitalWrite(digit5,dg1);
  getusernum();
  digitalWrite(digit5,dg0);
  sevwrite(bitRead(f,0)?sg1:sg0,bitRead(f,1)?sg1:sg0,bitRead(f,2)?sg1:sg0,bitRead(f,3)?sg1:sg0,bitRead(f,4)?sg1:sg0,bitRead(f,5)?sg1:sg0,bitRead(f,6)?sg1:sg0,bitRead(f,7)?sg1:sg0);
  digitalWrite(digit6,dg1);
  getusernum();
  digitalWrite(digit6,dg0);
  sevwrite(bitRead(g,0)?sg1:sg0,bitRead(g,1)?sg1:sg0,bitRead(g,2)?sg1:sg0,bitRead(g,3)?sg1:sg0,bitRead(g,4)?sg1:sg0,bitRead(g,5)?sg1:sg0,bitRead(g,6)?sg1:sg0,bitRead(g,7)?sg1:sg0);
  digitalWrite(digit7,dg1);
  getusernum();
  digitalWrite(digit7,dg0);
  sevwrite(bitRead(h,0)?sg1:sg0,bitRead(h,1)?sg1:sg0,bitRead(h,2)?sg1:sg0,bitRead(h,3)?sg1:sg0,bitRead(h,4)?sg1:sg0,bitRead(h,5)?sg1:sg0,bitRead(h,6)?sg1:sg0,bitRead(h,7)?sg1:sg0);
  digitalWrite(digit8,dg1);
  getusernum();
  digitalWrite(digit8,dg0);
}

void mputest(){ //mpu6050 디버깅용
  resetdigit();
  while(!nextswitch()){
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_addr,14,true);  // request a total of 14 registers
    AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)     
    AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
    AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
    Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
    GyX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
    GyY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
    GyZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
    reksum = AcX + AcY + AcZ + Tmp + GyX + GyY + GyZ;
    sum = abs(reksum);
    sevseg(sum/10000000%10,sum/1000000%10,sum/100000%10,sum/10000%10,sum/1000%10,sum/100%10,sum/10%10,sum%10);
  }
  while(nextswitch());
}

void vieweepromadr(){
  resetdigit();
  char errordigit = '.';
  if(indexaddr/10000%10 != 0) errordigit = 'E';
  while(!nextswitch()) sevseg('A','d','r',errordigit,indexaddr/1000%10,indexaddr/100%10,indexaddr/10%10,indexaddr%10);
  while(nextswitch());
}

boolean settingsmenu(){
  resetdigit();
  byte disptype;
  byte currentnum;
  byte dispwheelbuffersize = wheelbuffersize - 1;
  disptype = interfacetype;
  while(nextswitch()) sevseg('S','E','T','T','I','N','G','S');
  delay(500);
  while(1){
    while(!nextswitch()){
      blinkdot(5);
      switch(disptype){
        case 0:
          sevseg('T','Y','P','E','0',' ','v','r');
          break;
        case 1:
          sevseg('T','Y','P','E','1',' ','r','t');
          break;
        case 2:
          sevseg('T','Y','P','E','2',' ','G','Y');
          break;
        case interfacenums:
          sevseg('S','E','T','T','1',' ','r','t');
          break;
        case interfacenums+1:
          sevseg('F','U','N','C','r',' ','s','t');
          break;
        case interfacenums+2:
          sevseg('F','U','N','C','E',' ','S','C');
          break;
      }
    }
    tmptime = millis();
    while(nextswitch()){
      blinkdot(5);
      switch(disptype){
        case 0:
          sevseg('T','Y','P','E','0',' ','v','r');
          break;
        case 1:
          sevseg('T','Y','P','E','1',' ','r','t');
          break;
        case 2:
          sevseg('T','Y','P','E','2',' ','G','Y');
          break;
        case interfacenums:
          sevseg('S','E','T','T','1',' ','r','t');
          break;
        case interfacenums+1:
          sevseg('F','U','N','C','r',' ','s','t');
          break;
        case interfacenums+2:
          sevseg('F','U','N','C','E',' ','S','C');
          break;
      }
      if(millis() - tmptime >= 1000){
        switch(disptype){
          case 0:
            EEPROM.update(dataaddr,disptype);
            interfacetype = disptype;
            while(nextswitch()) sevseg('0',' ','v','r','d','o','n','E');
            delay(500);
            return 1;
            break;
          case 1:
            EEPROM.update(dataaddr,disptype);
            interfacetype = disptype;
            while(nextswitch()) sevseg('1',' ','r','t','d','o','n','E');
            delay(500);
            return 1;
            break;
          case 2:
            EEPROM.update(dataaddr,disptype);
            interfacetype = disptype;
            while(nextswitch()) sevseg('2',' ','G','Y','d','o','n','E');
            delay(500);
            return 1;
            break;
          case interfacenums-1+/*additional settings count*/1:
            currentnum = getusernum();
            while(nextswitch()){
              blinkdot(8);
              sevseg('-','-','-',currentnum,'L','v',dispwheelbuffersize/10,dispwheelbuffersize%10);
            }
            while(1){
              while(!nextswitch()){
                currentnum = getusernum();
                blinkdot(8);
                sevseg('-','-','-',currentnum,'L','v',dispwheelbuffersize/10,dispwheelbuffersize%10);
              }
              tmptime = millis();
              while(nextswitch()){
                blinkdot(8);
                sevseg('-','-','-',currentnum,'L','v',dispwheelbuffersize/10,dispwheelbuffersize%10);
                if(millis() - tmptime >= 1000){
                  EEPROM.update(dataaddr+1,wheelbuffersize);
                  while(nextswitch()) sevseg('L','v',dispwheelbuffersize/10,dispwheelbuffersize%10,'d','o','n','E');
                  delay(500);
                  return 1;
                }
              }
              dispwheelbuffersize++;
              userbuf = 1;
              if(dispwheelbuffersize == 17) dispwheelbuffersize = 1;
              wheelbuffersize = dispwheelbuffersize+1;
                switch(wheelbuffersize){
                  case 2:
                  case 3:
                    refreshrate = 2;
                    break;
                  case 4:
                  case 5:
                    refreshrate = 1;
                    break;
                  default:
                    refreshrate = 0;
                    break;
                }
            }
            break;
          case interfacenums+additionalsettings:
            delay(500);
            EEPROM.update(indexaddr,0);
            while(1) sevseg('P','L','S',' ','r','S','t',' ');
            break;
          case interfacenums+additionalsettings+1:
            while(nextswitch()) sevseg(' ',' ',' ',' ',' ',' ',' ',' ');
            delay(500);
            return 1;
            break;
        }
      }
    }
    disptype++;
    if(disptype == interfacenums+additionalsettings+2) disptype = 0;
  }
}

boolean checkeeprom(char findthis){
  for(indexaddr = 0;indexaddr<EEPROM.length();indexaddr++){
    if(EEPROM.read(indexaddr) == findthis){
      dataaddr = indexaddr+1;
      return 1;
    }
  }
  return 0;
}

void adjustdisplay(){
  resetdigit();
  while(!nextswitch()){
    usernum = 5;
    sevseg('A','d','J',microtime/10000%10,microtime/1000%10,microtime/100%10,microtime/10%10,microtime%10);
    if(getusernum() - 5 > 0 && microtime < 32766) microtime++;
    if(getusernum() - 5 < 0 && microtime > 0) microtime--;
  }
  while(nextswitch());
}
