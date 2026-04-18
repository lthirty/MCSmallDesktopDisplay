/* *****************************************************************
 * 
 * SmallDesktopDisplay
 *    小型桌面显示器
 * 
 * 原  作  者：Misaka
 * 修      改：微车游
 * 讨  论  群：811058758、887171863
 * 创 建 日 期：2021.07.19
 * 最后更改日期：2026.04.18
 * 更 改 说 明：V1.1添加串口调试，波特率115200\8\n\1；增加版本号显示。
 *            V1.2亮度和城市代码保存到EEPROM，断电可保存
 *            V1.3.1 更改smartconfig改为WEB配网模式，同时在配网的同时增加亮度、屏幕方向设置。
 *            V1.3.2 增加wifi休眠模式，仅在需要连接的情况下开启wifi，其他时间关闭wifi。增加wifi保存至eeprom（目前仅保存一组ssid和密码）
 *            V1.3.3  修改WiFi保存后无法删除的问题。目前更改为使用串口控制，输入 0x05 重置WiFi数据并重启。
 *                    增加web配网以及串口设置天气更新时间的功能。
 *            V1.3.4  修改web配网页面设置，将wifi设置页面以及其余设置选项放入同一页面中。
 *                    增加web页面设置是否使用DHT传感器。（使能DHT后才可使用）
 *            V1.4    增加web服务器，使用web网页进行设置。由于使用了web服务器，无法开启WiFi休眠。
 *                    注意，此版本中的DHT11传感器和太空人图片选择可以通过web网页设置来进行选择，无需通过使能标志来重新编译。
 *            V1.4.1  2026.04.09
 *                    1) 开机等待WiFi连接画面Logo改为布布头像。
 *                    2) 正常运行画面右下角区域改为布布头像。
 *                    3) 去掉运行画面右下角上方“内温”文字。
 *                    4) 修复运行画面头像每1-2秒闪动的问题（仅在整屏刷新时重绘头像）。
 *            V1.4.2  2026.04.10
 *                    1) 开机Logo改为客户横标图（保持比例并居中显示，避免遮挡进度条）。
 *                    2) 配置提示文案改为中文图片渲染，解决字库缺字显示方框问题。
 *                    3) 运行画面右下角图片改为“竖标-1-1.jpg”。
 *            V1.4.3  2026.04.10
 *                    1) 配置提示区域新增本机IP访问地址（http://192.168.x.x）。
 *                    2) 明确提示“sd3.local”与“本机IP地址”均可登录配置页。
 *            V1.4.5  2026.04.17
 *                    1) 去掉开机页“LIAN LI-”版本号显示。
 *                    2) 配置提示区域去掉“sd3.local”，仅保留IP登录方式。
 *                    3) 版本记录改为中文并紧跟在 V1.4.3 后。
 *            V1.4.8  2026.04.17
 *                    1) Web常驻管理页显示固件版本号。
 *                    2) 增加城市名称输入栏，输入后同步城市代码。
 *                    3) 点击保存后立即刷新天气，屏幕显示新城市名和对应天气。
 *                    4) 版本记录使用中文并更新最后更改日期。
 *            V1.4.9  2026.04.17
 *                    1) 修复Web管理页所有文字不显示的问题（Version变量未正确嵌入JS导致脚本崩溃）。
 *                    2) 修复Web管理页按钮文字不显示的问题（input元素需设置value而非textContent）。
 *            V1.5.0  2026.04.18
 *                    1) WiFi配网界面字体从font2(16px)放大到font4(26px)，提升可读性。
 *                    2) 编译后自动备份bin文件和项目压缩包到01.Backup目录。
 *            V1.5.1  2026.04.18
 *                    1) WiFi配网界面改为三行显示：Pls Connect WiFi / SSID: / AutoConnectAP。
 *                    2) 进度条和Connecting WiFi文字下移，避免与SSID文字重叠。
 *                    3) 开机Logo下移至上半区居中位置。
 *                    4) Service IP页面改用英文font4显示，去掉http://前缀，修复中文字库OOM崩溃。
 *                    5) 默认Logo改为横标白.png，右下角图标改为竖标白-纯图片.png。
 *                    6) 右下角图标位置上移（RUN_IMG_Y=175）。
 *                    7) 默认城市改为北京（101010100）。
 *                    8) Web端城市切换改为异步模式，避免在HTTP请求中发起天气查询导致内存不足重启。
 * 
 * 引 脚 分 配： SCK  GPIO14
 *             MOSI  GPIO13
 *             RES   GPIO2
 *             DC    GPIO0
 *             LCDBL GPIO5
 *             
 *             增加DHT11温湿度传感器，传感器接口为 GPIO 12
 * 
 *    感谢群友 @你别失望  提醒发现WiFi保存后无法重置的问题，目前已解决。详情查看更改说明！
 * *****************************************************************/
#define Version  "V1.5.1"
/* *****************************************************************
 *  库文件、头文件
 * *****************************************************************/
#include "ArduinoJson.h"
#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <TFT_eSPI.h> 
#include <SPI.h>
#include <TJpg_Decoder.h>
#include <EEPROM.h>
#include <LittleFS.h>
#include "qr.h"
#include "number.h"
#include "weathernum.h"


/* *****************************************************************
 *  配置使能位
 * *****************************************************************/
//WEB配网使能标志位----WEB配网打开后会默认关闭smartconfig功能
#define WM_EN   1
//Web服务器使能标志位----打开后将无法使用wifi休眠功能。
#define WebSever_EN  1



//注意，此版本中的DHT11传感器和太空人图片选择可以通过web网页设置来进行选择，无需通过使能标志来重新编译。
//设定DHT11温湿度传感器使能标志
#define DHT_EN  1
//设置太空人图片是否使用
#define imgAst_EN 1



#if WM_EN
#include <WiFiManager.h>
//WiFiManager 参数
WiFiManager wm; // global wm instance
// WiFiManagerParameter custom_field; // global param ( for non blocking w params )
#endif

#if DHT_EN
#include "DHT.h"
#define DHTPIN  12
#define DHTTYPE DHT11
DHT dht(DHTPIN,DHTTYPE);
#endif





/* *****************************************************************
 *  字库、图片库
 * *****************************************************************/
#include "font/ZdyLwFont_20.h"
#include "img/boot_logo_lianli.h"
#include "img/config_tip_cn.h"
#include "img/indoor_lianli.h"
#include "img/temperature.h"
#include "img/humidity.h"

#if imgAst_EN
#include "img/pangzi/i0.h"
#include "img/pangzi/i1.h"
#include "img/pangzi/i2.h"
#include "img/pangzi/i3.h"
#include "img/pangzi/i4.h"
#include "img/pangzi/i5.h"
#include "img/pangzi/i6.h"
#include "img/pangzi/i7.h"
#include "img/pangzi/i8.h"
#include "img/pangzi/i9.h"

int Anim = 0;           //太空人图标显示指针记录
int AprevTime = 0;      //太空人更新时间记录
#endif



/* *****************************************************************
 *  参数设置
 * *****************************************************************/

struct config_type
{
  char stassid[32];//定义配网得到的WIFI名长度(最大32字节)
  char stapsw[64];//定义配网得到的WIFI密码长度(最大64字节)
};

//---------------修改此处""内的信息--------------------
//如开启WEB配网则可不用设置这里的参数，前一个为wifi ssid，后一个为密码
config_type wificonf ={{""},{""}};


int updateweater_time = 10; //天气更新时间  X 分钟
int LCD_Rotation = 0;   //LCD屏幕方向
int LCD_BL_PWM = 8;//屏幕亮度0-100，默认50
String cityCode = "101010100";  //天气城市代码 北京:101010100
//----------------------------------------------------

//LCD屏幕相关设置
TFT_eSPI tft = TFT_eSPI();  // 引脚请自行配置tft_espi库中的 User_Setup.h文件
TFT_eSprite clk = TFT_eSprite(&tft);
#define LCD_BL_PIN 5    //LCD背光引脚
uint16_t bgColor = 0x0000;
const int RUN_IMG_X = 170;
const int RUN_IMG_Y = 175;
const int RUN_IMG_W = 60;
const int RUN_IMG_H = 54;

File webUploadFile;
String webUploadError = "";
uint32_t webUploadBytes = 0;
uint32_t webWrittenBytes = 0;
uint8_t bootLogoDrawn = 0;

//其余状态标志位
uint8_t Wifi_en = 1; //wifi状态标志位  1：打开    0：关闭
uint8_t UpdateWeater_en = 0; //更新时间标志位
int prevTime = 0;       //滚动显示更新标志位
int DHT_img_flag = 0;   //DHT传感器使用标志位
uint8_t indoorAvatarDirty = 1; //右下角头像重绘标志位，避免周期性闪烁


//EEPROM参数存储地址位
int BL_addr = 1;//被写入数据的EEPROM地址编号  1亮度
int Ro_addr = 2; //被写入数据的EEPROM地址编号  2 旋转方向
int DHT_addr = 3;//3 DHT使能标志位
int UpWeT_addr = 4; //4 更新时间记录
int CC_addr = 10;//被写入数据的EEPROM地址编号  10城市
int wifi_addr = 30; //被写入数据的EEPROM地址编号  20wifi-ssid-psw
int Warn_Number1 = 0,Warn_Value1 = 0,Warn_Number2 = 0,Warn_Value2 = 0,Warn_Flag = 1;
time_t prevDisplay = 0;       //显示时间显示记录
unsigned long weaterTime = 0; //天气更新时间记录
String SMOD = "";//串口数据存储


/*** Component objects ***/
Number      dig;
WeatherNum  wrat;


uint32_t targetTime = 0;   

int tempnum = 0;   //温度百分比
int huminum = 0;   //湿度百分比
int tempcol =0xffff;   //温度显示颜色
int humicol =0xffff;   //湿度显示颜色

//Web网站服务器
ESP8266WebServer server(80);// 建立esp8266网站服务器对象


//NTP服务器参数
static const char ntpServerName[] = "ntp6.aliyun.com";
const int timeZone = 8;     //东八区

//wifi连接UDP设置参数
WiFiUDP Udp;
WiFiClient wificlient;
unsigned int localPort = 8000;
float duty=0;


//函数声明
time_t getNtpTime();
void digitalClockDisplay(int reflash_en);
void printDigits(int digits);
String num2str(int digits);
void sendNTPpacket(IPAddress &address);
void LCD_reflash(int en);
void savewificonfig();
void readwificonfig();
void deletewificonfig();
bool isJpgFileName(const String& filename);
bool drawCustomJpegInBox(const char* path, int x, int y, int boxW, int boxH);
bool validateUploadedJpeg(const char* path, String& err);
void drawBootLogo();
void drawRuntimeCornerImage();
void handleBootUpload();
void handleRunUpload();
#if WebSever_EN
void Web_Sever_Init();
void Web_Sever();
#endif
void saveCityCodetoEEP(int * citycode);
void readCityCodefromEEP(int * citycode);
int getCityCodeByName(String cityName);

/* *****************************************************************
 *  函数
 * *****************************************************************/
//读取保存城市代码
void saveCityCodetoEEP(int * citycode)
{
  for(int cnum=0;cnum<5;cnum++)
  {
    EEPROM.write(CC_addr+cnum,*citycode%100);//城市地址写入城市代码
    EEPROM.commit();//保存更改的数据
    *citycode = *citycode/100;
    delay(5);
  }
}
void readCityCodefromEEP(int * citycode)
{
  for(int cnum=5;cnum>0;cnum--)
  {          
    *citycode = *citycode*100;
    *citycode += EEPROM.read(CC_addr+cnum-1); 
    delay(5);
  }
}

int getCityCodeByName(String cityName)
{
  cityName.trim();
  cityName.replace(" ", "");
  if(cityName.endsWith("市")) cityName.remove(cityName.length() - 1);
  if(cityName == "北京") return 101010100;
  if(cityName == "上海") return 101020100;
  if(cityName == "广州") return 101280101;
  if(cityName == "深圳") return 101280601;
  if(cityName == "杭州") return 101210101;
  if(cityName == "南京") return 101190101;
  if(cityName == "武汉") return 101200101;
  if(cityName == "西安") return 101110101;
  if(cityName == "重庆") return 101040100;
  if(cityName == "成都") return 101270101;
  if(cityName == "长沙") return 101250101;
  if(cityName == "珠海") return 101280701;
  return 0;
}

//wifi ssid，psw保存到eeprom
void savewificonfig()
{
  //开始写入
  uint8_t *p = (uint8_t*)(&wificonf);
  for (int i = 0; i < sizeof(wificonf); i++)
  {
    EEPROM.write(i + wifi_addr, *(p + i)); //在闪存内模拟写入
  }
  delay(10);
  EEPROM.commit();//执行写入ROM
  delay(10);
}
//删除原有eeprom中的信息
void deletewificonfig()
{
  config_type deletewifi ={{""},{""}};
  uint8_t *p = (uint8_t*)(&deletewifi);
  for (int i = 0; i < sizeof(deletewifi); i++)
  {
    EEPROM.write(i + wifi_addr, *(p + i)); //在闪存内模拟写入
  }
  delay(10);
  EEPROM.commit();//执行写入ROM
  delay(10);
}

//从eeprom读取WiFi信息ssid，psw
void readwificonfig()
{
  uint8_t *p = (uint8_t*)(&wificonf);
  for (int i = 0; i < sizeof(wificonf); i++)
  {
    *(p + i) = EEPROM.read(i + wifi_addr);
  }
  // EEPROM.commit();
  // ssid = wificonf.stassid;
  // pass = wificonf.stapsw;
  Serial.printf("Read WiFi Config.....\r\n");
  Serial.printf("SSID:%s\r\n",wificonf.stassid);
  Serial.printf("PSW:%s\r\n",wificonf.stapsw);
  Serial.printf("Connecting.....\r\n");
}

//TFT屏幕输出函数
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
  if ( y >= tft.height() ) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  // Return 1 to decode next block
  return 1;
}

bool isJpgFileName(const String& filename)
{
  String lower = filename;
  lower.toLowerCase();
  return lower.endsWith(".jpg") || lower.endsWith(".jpeg");
}

bool drawCustomJpegInBox(const char* path, int x, int y, int boxW, int boxH)
{
  if(!LittleFS.exists(path)) return false;

  tft.fillRect(x, y, boxW, boxH, bgColor);
  uint16_t w = 0, h = 0;
  JRESULT sizeJr = TJpgDec.getFsJpgSize(&w, &h, path, LittleFS);
  if(sizeJr == JDR_OK && w > 0 && h > 0)
  {
    int scale = 1;
    while(scale < 8 && ((int)(w / (scale * 2)) > boxW || (int)(h / (scale * 2)) > boxH)) scale *= 2;
    int drawW = (int)(w / scale);
    int drawH = (int)(h / scale);
    int drawX = x + (boxW - drawW) / 2;
    int drawY = y + (boxH - drawH) / 2;
    TJpgDec.setJpgScale(scale);
    JRESULT jr = TJpgDec.drawFsJpg(drawX, drawY, path, LittleFS);
    TJpgDec.setJpgScale(1);
    return jr == JDR_OK;
  }

  TJpgDec.setJpgScale(1);
  JRESULT jr = TJpgDec.drawFsJpg(x, y, path, LittleFS);
  TJpgDec.setJpgScale(1);
  return jr == JDR_OK;
}

bool validateUploadedJpeg(const char* path, String& err)
{
  if(!LittleFS.exists(path))
  {
    err = "Upload failed: file not found";
    return false;
  }

  File f = LittleFS.open(path, "r");
  if(!f)
  {
    err = "Upload failed: file open error";
    return false;
  }

  uint8_t b1 = f.read();
  uint8_t b2 = f.read();
  size_t sz = f.size();
  f.close();
  if(sz < 16)
  {
    err = "Upload failed: file content invalid";
    return false;
  }
  if(!(b1 == 0xFF && b2 == 0xD8))
  {
    err = "Upload failed: not JPEG";
    return false;
  }

  uint16_t w = 0, h = 0;
  JRESULT jr = TJpgDec.getFsJpgSize(&w, &h, path, LittleFS);
  if(jr != JDR_OK)
  {
    err = "Upload failed: JPEG parse error";
    return false;
  }
  return true;
}

void drawBootLogo()
{
  if(!drawCustomJpegInBox("/boot.jpg", 20, 64, 200, 70))
    TJpgDec.drawJpg(20, 64, boot_logo_lianli, sizeof(boot_logo_lianli));
}

void drawRuntimeCornerImage()
{
  if(!drawCustomJpegInBox("/run.jpg", RUN_IMG_X, RUN_IMG_Y, RUN_IMG_W, RUN_IMG_H))
  {
    tft.fillRect(RUN_IMG_X, RUN_IMG_Y, RUN_IMG_W, RUN_IMG_H, bgColor);
    TJpgDec.drawJpg(RUN_IMG_X, RUN_IMG_Y, indoor_lianli, sizeof(indoor_lianli));
  }
}

//进度条函数
byte loadNum = 6;
void loading(byte delayTime)//绘制进度条
{
  // 开机等待联网时优先显示上传Logo
  if(!bootLogoDrawn){ drawBootLogo(); bootLogoDrawn = 1; }

  clk.setColorDepth(8);
  
  clk.createSprite(220, 70);//创建窗口
  clk.fillSprite(0x0000);   //填充率

  clk.drawRoundRect(0,0,220,16,8,0xFFFF);       //空心圆角矩形
  clk.fillRoundRect(3,3,loadNum,10,5,0xFFFF);   //实心圆角矩形
  clk.setTextDatum(CC_DATUM);   //设置文本数据
  clk.setTextColor(TFT_GREEN, 0x0000); 
  clk.drawString("Connecting WiFi...",110,44,4);
  clk.setTextColor(TFT_WHITE, 0x0000); 
  clk.pushSprite(10,150);  //窗口位置
  
  //clk.setTextDatum(CC_DATUM);
  //clk.setTextColor(TFT_WHITE, 0x0000); 
  //clk.pushSprite(130,180);
  
  clk.deleteSprite();
  loadNum += 1;
  delay(delayTime);
}

//湿度图标显示函数
void humidityWin()
{
  clk.setColorDepth(8);
  
  huminum = huminum/2;
  clk.createSprite(52, 6);  //创建窗口
  clk.fillSprite(0x0000);    //填充率
  clk.drawRoundRect(0,0,52,6,3,0xFFFF);  //空心圆角矩形  起始位x,y,长度，宽度，圆弧半径，颜色
  clk.fillRoundRect(1,1,huminum,4,2,humicol);   //实心圆角矩形
  clk.pushSprite(45,222);  //窗口位置
  clk.deleteSprite();
}

//温度图标显示函数
void tempWin()
{
  clk.setColorDepth(8);
  
  clk.createSprite(52, 6);  //创建窗口
  clk.fillSprite(0x0000);    //填充率
  clk.drawRoundRect(0,0,52,6,3,0xFFFF);  //空心圆角矩形  起始位x,y,长度，宽度，圆弧半径，颜色
  clk.fillRoundRect(1,1,tempnum,4,2,tempcol);   //实心圆角矩形
  clk.pushSprite(45,192);  //窗口位置
  clk.deleteSprite();
}

#if DHT_EN
//外接DHT11传感器，显示数据
void IndoorTem()
{
  if(!indoorAvatarDirty) return;

  // 仅在需要时重绘，防止每2秒调用时出现闪屏
  drawRuntimeCornerImage();
  indoorAvatarDirty = 0;
}
#endif

#if !WM_EN
//微信配网函数
void SmartConfig(void)
{
  WiFi.mode(WIFI_STA);    //设置STA模式
  //tft.pushImage(0, 0, 240, 240, qr);
  tft.pushImage(0, 0, 240, 240, qr);
  Serial.println("\r\nWait for Smartconfig...");    //打印log信息
  WiFi.beginSmartConfig();      //开始SmartConfig，等待手机端发出用户名和密码
  while (1)
  {
    Serial.print(".");
    delay(100);                   // wait for a second
    if (WiFi.smartConfigDone())//配网成功，接收到SSID和密码
    {
    Serial.println("SmartConfig Success");
    Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
    Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
    break;
    }
  }
  loadNum = 194;
}
#endif


//串口调试设置函数
void Serial_set()
{
  String incomingByte = "";
  if(Serial.available()>0)
  {
    
    while(Serial.available()>0)//监测串口缓存，当有数据输入时，循环赋值给incomingByte
    {
      incomingByte += char(Serial.read());//读取单个字符值，转换为字符，并按顺序一个个赋值给incomingByte
      delay(2);//不能省略，因为读取缓冲区数据需要时间
    }    
    if(SMOD=="0x01")//设置1亮度设置
    {
      int LCDBL = atoi(incomingByte.c_str());//int n = atoi(xxx.c_str());//String转int
      if(LCDBL>=0 && LCDBL<=100)
      {
        EEPROM.write(BL_addr, LCDBL);//亮度地址写入亮度值
        EEPROM.commit();//保存更改的数据
        delay(5);
        LCD_BL_PWM = EEPROM.read(BL_addr); 
        delay(5);
        SMOD = "";
        Serial.printf("亮度调整为：");
        analogWrite(LCD_BL_PIN, 1023 - (LCD_BL_PWM*10));
        Serial.println(LCD_BL_PWM);
        Serial.println("");
      }
      else
        Serial.println("亮度调整错误，请输入0-100");
    } 
    if(SMOD=="0x02")//设置2地址设置
    {
      int CityCODE = 0;
      int CityC = atoi(incomingByte.c_str());//int n = atoi(xxx.c_str());//String转int
      if(CityC>=101000000 && CityC<=102000000 || CityC == 0)
      {
        saveCityCodetoEEP(&CityC);
        // for(int cnum=0;cnum<5;cnum++)
        // {
        //   EEPROM.write(CC_addr+cnum,CityC%100);//城市地址写入城市代码
        //   EEPROM.commit();//保存更改的数据
        //   CityC = CityC/100;
        //   delay(5);
        // }
        readCityCodefromEEP(&CityC);
        // for(int cnum=5;cnum>0;cnum--)
        // {          
        //   CityCODE = CityCODE*100;
        //   CityCODE += EEPROM.read(CC_addr+cnum-1); 
        //   delay(5);
        // }
        
        cityCode = CityCODE;
        
        if(cityCode == "0")
        {
          Serial.println("城市代码调整为：自动");
          getCityCode();  //获取城市代码
        }
        else
        {
          Serial.printf("城市代码调整为：");
          Serial.println(cityCode);
        }
        Serial.println("");
        getCityWeater();//更新城市天气  
        SMOD = "";
      }
      else
        Serial.println("城市调整错误，请输入9位城市代码，自动获取请输入0");
    }   
    if(SMOD=="0x03")//设置3屏幕显示方向
    {
      int RoSet = atoi(incomingByte.c_str());
      if(RoSet >= 0 && RoSet <= 3)
      {
        EEPROM.write(Ro_addr, RoSet);//屏幕方向地址写入方向值
        EEPROM.commit();//保存更改的数据
        SMOD = "";
      //设置屏幕方向后重新刷屏并显示
        tft.setRotation(RoSet);
        tft.fillScreen(0x0000);
        indoorAvatarDirty = 1;
        LCD_reflash(1);//屏幕刷新程序
        UpdateWeater_en = 1;
        TJpgDec.drawJpg(15,183,temperature, sizeof(temperature));  //温度图标
        TJpgDec.drawJpg(15,213,humidity, sizeof(humidity));  //湿度图标

        Serial.print("屏幕方向设置为：");
        Serial.println(RoSet);
      }
      else 
      {
        Serial.println("屏幕方向值错误，请输入0-3内的值");
      }
    }
    if(SMOD=="0x04")//设置天气更新时间
    {
      int wtup = atoi(incomingByte.c_str());//int n = atoi(xxx.c_str());//String转int
      if(wtup>=1 && wtup<=60)
      {
        EEPROM.write(UpWeT_addr, wtup);//亮度地址写入亮度值
        EEPROM.commit();//保存更改的数据
        delay(5);
        updateweater_time = wtup;
        SMOD = "";
        Serial.printf("天气更新时间更改为：");
        Serial.print(updateweater_time);
        Serial.println("分钟");
      }
      else
        Serial.println("更新时间太长，请重新设置（1-60）");
    } 
    else
    {
      SMOD = incomingByte;
      delay(2);
      if(SMOD=="0x01")
        Serial.println("请输入亮度值，范围0-100");
      else if(SMOD=="0x02")
        Serial.println("请输入9位城市代码，自动获取请输入0"); 
      else if(SMOD=="0x03")
      {
        Serial.println("请输入屏幕方向值，"); 
        Serial.println("0-USB接口朝下");
        Serial.println("1-USB接口朝右");
        Serial.println("2-USB接口朝上");
        Serial.println("3-USB接口朝左");
      }
      else if(SMOD=="0x04")
      {
        Serial.print("当前天气更新时间："); 
        Serial.print(updateweater_time);
        Serial.println("分钟");
        Serial.println("请输入天气更新时间（1-60）分钟"); 
      }
      else if(SMOD=="0x05")
      {
        Serial.println("重置WiFi设置中......");
        delay(10);
        wm.resetSettings();
        deletewificonfig();
        delay(10);
        Serial.println("重置WiFi成功");
        SMOD = "";
        ESP.restart();
      }
      else
      {
        Serial.println("");
        Serial.println("请输入需要修改的代码：");
        Serial.println("亮度设置输入        0x01");
        Serial.println("地址设置输入        0x02");
        Serial.println("屏幕方向设置输入    0x03");
        Serial.println("更改天气更新时间    0x04");
        Serial.println("重置WiFi(会重启)    0x05");
        Serial.println("");
      }
    }
  }
}

#if WebSever_EN
//web网站相关函数
//web设置页面
void handleconfig()
{
  bool bootOk = server.hasArg("boot_upload_ok");
  bool runOk = server.hasArg("run_upload_ok");
  int web_cc,web_setro,web_lcdbl,web_upt,web_dhten,web_cc_name;
  String web_city_name;

  if (server.hasArg("web_city_name") || server.hasArg("web_ccode") || server.hasArg("web_bl") ||
      server.hasArg("web_upwe_t") || server.hasArg("web_DHT11_en") || server.hasArg("web_set_rotation"))
  {
    web_city_name = server.arg("web_city_name");
    web_cc_name = getCityCodeByName(web_city_name);
    web_cc    = server.arg("web_ccode").toInt();
    if(web_cc_name>=101000000 && web_cc_name<=102000000) web_cc = web_cc_name;
    web_setro = server.arg("web_set_rotation").toInt();
    web_lcdbl = server.arg("web_bl").toInt();
    web_upt   = server.arg("web_upwe_t").toInt();
    web_dhten = server.arg("web_DHT11_en").toInt();

    if(web_cc>=101000000 && web_cc<=102000000)
    {
      saveCityCodetoEEP(&web_cc);
      readCityCodefromEEP(&web_cc);
      cityCode = web_cc;
      UpdateWeater_en = 1;
      weaterTime = 0;
    }
    if(web_lcdbl>0 && web_lcdbl<=100)
    {
      EEPROM.write(BL_addr, web_lcdbl);
      EEPROM.commit();
      delay(5);
      LCD_BL_PWM = EEPROM.read(BL_addr);
      delay(5);
      analogWrite(LCD_BL_PIN, 1023 - (LCD_BL_PWM*10));
    }
    if(web_upt > 0 && web_upt <= 60)
    {
      EEPROM.write(UpWeT_addr, web_upt);
      EEPROM.commit();
      delay(5);
      updateweater_time = web_upt;
    }

    EEPROM.write(DHT_addr, web_dhten);
    EEPROM.commit();
    delay(5);
    if(web_dhten != DHT_img_flag)
    {
      DHT_img_flag = web_dhten;
      tft.fillScreen(0x0000);
      indoorAvatarDirty = 1;
      LCD_reflash(1);
      UpdateWeater_en = 1;
      TJpgDec.drawJpg(15,183,temperature, sizeof(temperature));
      TJpgDec.drawJpg(15,213,humidity, sizeof(humidity));
    }

    EEPROM.write(Ro_addr, web_setro);
    EEPROM.commit();
    delay(5);
    if(web_setro != LCD_Rotation)
    {
      LCD_Rotation = web_setro;
      tft.setRotation(LCD_Rotation);
      tft.fillScreen(0x0000);
      indoorAvatarDirty = 1;
      LCD_reflash(1);
      UpdateWeater_en = 1;
      TJpgDec.drawJpg(15,183,temperature, sizeof(temperature));
      TJpgDec.drawJpg(15,213,humidity, sizeof(humidity));
    }
  }

  String content = "<html><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'><style>";
  content += "html,body{background:#0f172a;color:#e2e8f0;font-size:22px;font-family:Arial,sans-serif;margin:0;line-height:1.6;}";
  content += ".wrap{width:100%;padding:16px;box-sizing:border-box;}fieldset{border:1px solid #334155;border-radius:10px;padding:14px;margin:12px 0;}legend{color:#7dd3fc;font-size:24px;font-weight:700;}label{display:block;margin-top:6px;}input,select{width:100%;box-sizing:border-box;padding:12px;margin:8px 0;background:#111827;color:#fff;border:1px solid #334155;border-radius:8px;font-size:22px;}.btn{width:100%;padding:14px;border:0;border-radius:8px;background:#06b6d4;color:#fff;font-weight:bold;font-size:22px;}.ver{font-size:18px;color:#93c5fd;} progress{width:100%;height:16px;}";
  content += "</style><body><div class='wrap'><form action='/' method='POST'>";
  content += "<div id='fw_ver' class='ver'></div>";

  content += "<fieldset><legend id='legend_basic'></legend>";
  content += "<label id='label_city_name'></label><input id='web_city_name' type='text' name='web_city_name' oninput='syncCityCode()'>";
  content += "<label id='label_city'></label><input id='web_ccode' type='text' name='web_ccode' value='" + cityCode + "'>";
  content += "<label id='label_bl'></label><input type='text' name='web_bl' value='" + String(LCD_BL_PWM) + "'>";
  content += "<label id='label_upt'></label><input type='text' name='web_upwe_t' value='" + String(updateweater_time) + "'>";
  content += "</fieldset>";

  content += "<fieldset><legend id='legend_adv'></legend>";
  #if DHT_EN
  content += "<label id='label_dht'></label><select id='sel_dht' name='web_DHT11_en'><option value='0'></option><option value='1'></option></select>";
  #endif
  content += "<label id='label_rot'></label><select id='sel_rot' name='web_set_rotation'><option value='0'></option><option value='1'></option><option value='2'></option><option value='3'></option></select>";
  content += "<input id='btn_save' class='btn' type='submit' value=''></fieldset></form>";

  content += "<fieldset><legend id='legend_upload'></legend>";
  content += "<form id='bootUploadForm' action='/upload_boot' method='POST' enctype='multipart/form-data'><label id='label_boot'></label><input type='file' name='boot_file' accept='image/png,image/jpeg,.png,.jpg,.jpeg'><input id='btn_boot' class='btn' type='submit' value=''></form>";
  content += "<div id='boot_upload_status' class='ver' data-ok='" + String(bootOk ? 1 : 0) + "'></div><progress id='boot_upload_progress' max='100' value='" + String(bootOk ? 100 : 0) + "'></progress>";
  content += "<form id='runUploadForm' action='/upload_run' method='POST' enctype='multipart/form-data'><label id='label_run'></label><input type='file' name='run_file' accept='image/png,image/jpeg,.png,.jpg,.jpeg'><input id='btn_run' class='btn' type='submit' value=''></form>";
  content += "<div id='run_upload_status' class='ver' data-ok='" + String(runOk ? 1 : 0) + "'></div><progress id='run_upload_progress' max='100' value='" + String(runOk ? 100 : 0) + "'></progress>";
  content += "<div id='hint_stage' class='ver'></div>";
  if(webUploadError.length() > 0) content += "<div id='upload_error' style='color:#fda4af;font-size:18px;'>" + webUploadError + "</div>";
  content += "</fieldset>";

  content += "<script>";
  content += "const RUN_W=" + String(RUN_IMG_W) + ",RUN_H=" + String(RUN_IMG_H) + ";";
  content += "const T={basic:'\u57fa\u7840\u8bbe\u7f6e',adv:'\u9ad8\u7ea7\u8bbe\u7f6e',city_name:'\u57ce\u5e02\u540d\u79f0',city:'\u57ce\u5e02\u4ee3\u7801',bl:'\u4eae\u5ea6(1-100)',upt:'\u5929\u6c14\u5237\u65b0\u95f4\u9694(\u5206\u949f)',dht:'DHT\u663e\u793a',off:'\u5173\u95ed',on:'\u5f00\u542f',rot:'\u5c4f\u5e55\u65b9\u5411',r0:'USB\u671d\u4e0a',r1:'USB\u671d\u53f3',r2:'USB\u671d\u4e0b',r3:'USB\u671d\u5de6',save:'\u4fdd\u5b58\u8bbe\u7f6e',upload:'\u56fe\u7247\u4e0a\u4f20',boot:'\u5f00\u673aLogo(PNG/JPG/JPEG)',btnBoot:'\u4e0a\u4f20\u5e76\u5e94\u7528\u5f00\u673aLogo',run:'\u8fd0\u884c\u9875\u53f3\u4e0b\u89d2\u56fe\u7247(PNG/JPG/JPEG)',btnRun:'\u4e0a\u4f20\u5e76\u5e94\u7528\u8fd0\u884c\u56fe',wait:'\u7b49\u5f85\u4e0a\u4f20',ok:'\u4e0a\u4f20\u6210\u529f',stage:'\u4e0a\u4f20\u9636\u6bb5\uff1a\u4e0a\u4f20 / \u6539\u6a21 / \u9002\u914d / \u8bbe\u7f6e\u3002\u8fd0\u884c\u56fe\u9002\u914d\u5c3a\u5bf8\uff1a'};";
  content += "function txt(id,v){var e=document.getElementById(id);if(e){if(e.tagName==='INPUT')e.value=v;else e.textContent=v;}}";
  content += "txt('fw_ver','\\u56fa\\u4ef6\\u7248\\u672c\\uff1a" + String(Version) + "');txt('legend_basic',T.basic);txt('legend_adv',T.adv);txt('label_city_name',T.city_name);txt('label_city',T.city);txt('label_bl',T.bl);txt('label_upt',T.upt);txt('label_dht',T.dht);txt('label_rot',T.rot);txt('btn_save',T.save);txt('legend_upload',T.upload);txt('label_boot',T.boot);txt('btn_boot',T.btnBoot);txt('label_run',T.run);txt('btn_run',T.btnRun);txt('hint_stage',T.stage+RUN_W+'x'+RUN_H);var ci=document.getElementById('web_city_name');if(ci)ci.placeholder='\\u4f8b\\u5982\\uff1a\\u676d\\u5dde/\\u5317\\u4eac';";
  content += "const CITY_MAP={'\\u5317\\u4eac':'101010100','\\u4e0a\\u6d77':'101020100','\\u5e7f\\u5dde':'101280101','\\u6df1\\u5733':'101280601','\\u676d\\u5dde':'101210101','\\u5357\\u4eac':'101190101','\\u6b66\\u6c49':'101200101','\\u897f\\u5b89':'101110101','\\u91cd\\u5e86':'101040100','\\u6210\\u90fd':'101270101','\\u957f\\u6c99':'101250101','\\u73e0\\u6d77':'101280701'};";
  content += "function normCity(v){v=(v||'').replace(/\\s+/g,'').trim();if(v.endsWith('市'))v=v.slice(0,-1);return v;}";
  content += "function syncCityCode(){var n=document.getElementById('web_city_name');var c=document.getElementById('web_ccode');if(!n||!c)return;var k=normCity(n.value);if(CITY_MAP[k])c.value=CITY_MAP[k];}";
  content += "var selD=document.getElementById('sel_dht');if(selD){selD.options[0].text=T.off;selD.options[1].text=T.on;selD.value='" + String(DHT_img_flag) + "';}";
  content += "var selR=document.getElementById('sel_rot');if(selR){selR.options[0].text=T.r0;selR.options[1].text=T.r1;selR.options[2].text=T.r2;selR.options[3].text=T.r3;selR.value='" + String(LCD_Rotation) + "';}";
  content += "var bs=document.getElementById('boot_upload_status');if(bs)bs.textContent=(bs.getAttribute('data-ok')==='1')?T.ok:T.wait;";
  content += "var rs=document.getElementById('run_upload_status');if(rs)rs.textContent=(rs.getAttribute('data-ok')==='1')?T.ok:T.wait;";
  content += "function loadImageFromFile(file){return new Promise(function(resolve,reject){var r=new FileReader();r.onload=function(){var img=new Image();img.onload=function(){resolve(img);};img.onerror=function(){reject(new Error('decode'));};img.src=r.result;};r.onerror=function(){reject(new Error('read'));};r.readAsDataURL(file);});}";
  content += "function canvasToBlob(canvas,quality){return new Promise(function(resolve,reject){canvas.toBlob(function(blob){if(blob)resolve(blob);else reject(new Error('convert'));},'image/jpeg',quality||0.9);});}";
  content += "async function preprocessImage(file,statusEl,targetW,targetH,fileName){if(statusEl)statusEl.textContent='\u4e0a\u4f20\u4e2d';var img=await loadImageFromFile(file);var cvs=document.createElement('canvas');var ctx=cvs.getContext('2d');if(targetW>0&&targetH>0){if(statusEl)statusEl.textContent='\u6539\u6a21\u4e2d';cvs.width=targetW;cvs.height=targetH;ctx.fillStyle='#000';ctx.fillRect(0,0,targetW,targetH);if(statusEl)statusEl.textContent='\u9002\u914d\u4e2d';var ratio=Math.min(targetW/img.width,targetH/img.height);var dw=Math.max(1,Math.round(img.width*ratio));var dh=Math.max(1,Math.round(img.height*ratio));var dx=Math.floor((targetW-dw)/2);var dy=Math.floor((targetH-dh)/2);ctx.drawImage(img,dx,dy,dw,dh);}else{cvs.width=img.width;cvs.height=img.height;ctx.drawImage(img,0,0);}var blob=await canvasToBlob(cvs,0.9);return new File([blob],fileName,{type:'image/jpeg'});}";
  content += "function bindUpload(formId,statusId,progressId){var f=document.getElementById(formId);if(!f)return;f.addEventListener('submit',async function(e){e.preventDefault();var s=document.getElementById(statusId);var p=document.getElementById(progressId);var fi=f.querySelector('input[type=file]');var files=(fi||{}).files;if(!files||files.length===0){if(s)s.textContent='\u8bf7\u9009\u62e9\u56fe\u7247\u6587\u4ef6';return;}var file=files[0];var n=(file.name||'').toLowerCase();if(!(n.endsWith('.png')||n.endsWith('.jpg')||n.endsWith('.jpeg'))){if(s)s.textContent='\u6587\u4ef6\u7c7b\u578b\u9519\u8bef\uff0c\u4ec5\u652f\u6301PNG/JPG/JPEG';return;}if(s)s.textContent='\u4e0a\u4f20\u4e2d 0%';if(p)p.value=0;var uploadFile=file;try{uploadFile=(formId==='runUploadForm')?await preprocessImage(file,s,RUN_W,RUN_H,'run_adapt.jpg'):await preprocessImage(file,s,200,70,'boot_200x70.jpg');}catch(ex){if(s)s.textContent='\u5904\u7406\u5931\u8d25';return;}if(s)s.textContent='\u8bbe\u7f6e\u4e2d';var xhr=new XMLHttpRequest();xhr.open('POST',f.action,true);xhr.upload.onprogress=function(ev){if(ev.lengthComputable){var v=Math.min(100,Math.round(ev.loaded*100/ev.total));if(p)p.value=v;if(s)s.textContent='\u4e0a\u4f20\u4e2d '+v+'%';}};xhr.onreadystatechange=function(){if(xhr.readyState===4){if(xhr.status===200){if(p)p.value=100;if(s)s.textContent='\u8bbe\u7f6e\u5b8c\u6210';setTimeout(function(){if(s)s.textContent='\u4e0a\u4f20\u6210\u529f';if(formId==='bootUploadForm'){location.href='/?boot_upload_ok=1';}else{location.href='/?run_upload_ok=1';}},500);}else{if(s)s.textContent='\u4e0a\u4f20\u5931\u8d25';}}};var fd=new FormData();fd.append((formId==='bootUploadForm')?'boot_file':'run_file',uploadFile,uploadFile.name||'upload.jpg');xhr.send(fd);});}";
  content += "syncCityCode();bindUpload('bootUploadForm','boot_upload_status','boot_upload_progress');bindUpload('runUploadForm','run_upload_status','run_upload_progress');";
  content += "</script>";
  content += "</div></body></html>";
  server.send(200, "text/html", content);
}

//no need authentication
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

//Web服务初始化
void handleBootUpload()
{
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START)
  {
    webUploadError = "";
    webUploadBytes = 0;
    webWrittenBytes = 0;
    if(!isJpgFileName(upload.filename)) { webUploadError = "Upload failed: JPG/JPEG only"; return; }
    webUploadFile = LittleFS.open("/boot.jpg", "w");
    if(!webUploadFile) webUploadError = "Upload failed: FS open error";
  }
  else if(upload.status == UPLOAD_FILE_WRITE)
  {
    if(webUploadFile)
    {
      size_t wr = webUploadFile.write(upload.buf, upload.currentSize);
      webUploadBytes += upload.currentSize;
      webWrittenBytes += (uint32_t)wr;
    }
  }
  else if(upload.status == UPLOAD_FILE_END)
  {
    if(webUploadFile) webUploadFile.close();
    if(webUploadError.length()==0 && webWrittenBytes < webUploadBytes) webUploadError = "Upload failed: file too large or no space";
    if(webUploadError.length() == 0)
    {
      String verifyErr = "";
      if(!validateUploadedJpeg("/boot.jpg", verifyErr)) webUploadError = verifyErr;
      else {
        bootLogoDrawn = 0;
      }
    }
  }
  else if(upload.status == UPLOAD_FILE_ABORTED)
  {
    if(webUploadFile) webUploadFile.close();
    webUploadError = "Upload failed: aborted";
  }
}

void handleRunUpload()
{
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START)
  {
    webUploadError = "";
    webUploadBytes = 0;
    webWrittenBytes = 0;
    if(!isJpgFileName(upload.filename)) { webUploadError = "Upload failed: JPG/JPEG only"; return; }
    webUploadFile = LittleFS.open("/run.jpg", "w");
    if(!webUploadFile) webUploadError = "Upload failed: FS open error";
  }
  else if(upload.status == UPLOAD_FILE_WRITE)
  {
    if(webUploadFile)
    {
      size_t wr = webUploadFile.write(upload.buf, upload.currentSize);
      webUploadBytes += upload.currentSize;
      webWrittenBytes += (uint32_t)wr;
    }
  }
  else if(upload.status == UPLOAD_FILE_END)
  {
    if(webUploadFile) webUploadFile.close();
    if(webUploadError.length()==0 && webWrittenBytes < webUploadBytes) webUploadError = "Upload failed: file too large or no space";
    if(webUploadError.length() == 0)
    {
      String verifyErr = "";
      if(!validateUploadedJpeg("/run.jpg", verifyErr)) webUploadError = verifyErr;
      else {
        indoorAvatarDirty = 1;
        drawRuntimeCornerImage();
      }
    }
  }
  else if(upload.status == UPLOAD_FILE_ABORTED)
  {
    if(webUploadFile) webUploadFile.close();
    webUploadError = "Upload failed: aborted";
  }
}

void Web_Sever_Init()
{
  uint32_t counttime = 0;//记录创建mDNS的时间
  Serial.println("mDNS responder building...");
  counttime = millis();
  while (!MDNS.begin("SD3"))
  {
    if(millis() - counttime > 30000) ESP.restart();//判断超过30秒钟就重启设备
  }

  Serial.println("mDNS responder started");
  //输出连接wifi后的IP地址
  // Serial.print("本地IP： ");
  // Serial.println(WiFi.localIP());

  server.on("/", handleconfig);
  server.on("/upload_boot", HTTP_POST, [](){ if(webUploadError.length() == 0) server.send(200, "text/plain", "OK"); else server.send(400, "text/plain", webUploadError); }, handleBootUpload);
  server.on("/upload_run", HTTP_POST, [](){ if(webUploadError.length() == 0) server.send(200, "text/plain", "OK"); else server.send(400, "text/plain", webUploadError); }, handleRunUpload);
  server.onNotFound(handleNotFound);

  //开启TCP服务
  server.begin();
  Serial.println("HTTP服务器已开启");  Serial.print("????: http://");
  Serial.println(WiFi.localIP());
  //将服务器添加到mDNS
  MDNS.addService("http", "tcp", 80);
}
//Web网页设置函数
void Web_Sever()
{
  MDNS.update();
  server.handleClient();
}
//web服务打开后LCD显示登陆网址及IP
void Web_sever_Win()
{
  String ip_url = WiFi.localIP().toString();
  clk.setColorDepth(8);
  
  clk.createSprite(220, 92);//创建窗口
  clk.fillSprite(0x0000);   //填充率

  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_GREEN, 0x0000);
  clk.drawString("Service IP:",110,16,4);

  clk.setTextColor(TFT_WHITE, 0x0000); 
  clk.drawString(ip_url,110,58,4);
  clk.pushSprite(10,30);  //窗口位置
    
  clk.deleteSprite();
}
#endif

#if WM_EN
//WEB配网LCD显示函数
void Web_win()
{
  clk.setColorDepth(8);
  
  clk.createSprite(220, 110);//创建窗口
  clk.fillSprite(0x0000);   //填充率

  clk.setTextDatum(CC_DATUM);   //设置文本数据
  clk.setTextColor(TFT_GREEN, 0x0000); 
  clk.drawString("Pls Connect WiFi",110,16,4);
  clk.drawString("SSID:",110,52,4);
  clk.setTextColor(TFT_WHITE, 0x0000); 
  clk.drawString("AutoConnectAP",110,86,4);
  clk.pushSprite(10,20);  //窗口位置
    
  clk.deleteSprite();
}

//WEB配网函数
void Webconfig()
{
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP  
  
  delay(3000);
  wm.resetSettings(); // wipe settings
  
  // add a custom input field
  int customFieldLength = 40;
  
  // new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\"");
  
  // test custom html input type(checkbox)
  //  new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\" type=\"checkbox\""); // custom html type
  
  // test custom html(radio)
  // const char* custom_radio_str = "<br/><label for='customfieldid'>Custom Field Label</label><input type='radio' name='customfieldid' value='1' checked> One<br><input type='radio' name='customfieldid' value='2'> Two<br><input type='radio' name='customfieldid' value='3'> Three";
  // new (&custom_field) WiFiManagerParameter(custom_radio_str); // custom html input

  const char* set_rotation = "<br/><label for='set_rotation'>Set Rotation</label>\
                              <input type='radio' name='set_rotation' value='0' checked> One<br>\
                              <input type='radio' name='set_rotation' value='1'> Two<br>\
                              <input type='radio' name='set_rotation' value='2'> Three<br>\
                              <input type='radio' name='set_rotation' value='3'> Four<br>";
  WiFiManagerParameter  custom_rot(set_rotation); // custom html input
  WiFiManagerParameter  custom_bl("LCDBL","LCD BackLight(1-100)","10",3);
  #if DHT_EN
  WiFiManagerParameter  custom_DHT11_en("DHT11_en","Enable DHT11 sensor","0",1);
  #endif
  WiFiManagerParameter  custom_weatertime("WeaterUpdateTime","Weather Update Time(Min)","10",3);
  WiFiManagerParameter  custom_cc("CityCode","CityCode","101010100",9);
  WiFiManagerParameter  p_lineBreak_notext("<p></p>");

  // wm.addParameter(&p_lineBreak_notext);
  // wm.addParameter(&custom_field);
  wm.addParameter(&p_lineBreak_notext);
  wm.addParameter(&custom_cc);
  wm.addParameter(&p_lineBreak_notext);
  wm.addParameter(&custom_bl);
  wm.addParameter(&p_lineBreak_notext);
  wm.addParameter(&custom_weatertime);
  wm.addParameter(&p_lineBreak_notext);
  wm.addParameter(&custom_rot);
  #if DHT_EN
  wm.addParameter(&p_lineBreak_notext);
  wm.addParameter(&custom_DHT11_en);
  #endif
  wm.setSaveParamsCallback(saveParamCallback);
  
  // custom menu via array or vector
  // 
  // menu tokens, "wifi","wifinoscan","info","param","close","sep","erase","restart","exit" (sep is seperator) (if param is in menu, params will not show up in wifi page!)
  // const char* menu[] = {"wifi","info","param","sep","restart","exit"}; 
  // wm.setMenu(menu,6);
  std::vector<const char *> menu = {"wifi","restart"};
  wm.setMenu(menu);
  
  // set dark theme
  wm.setClass("invert");
  
  //set static ip
  // wm.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0)); // set static ip,gw,sn
  // wm.setShowStaticFields(true); // force show static ip fields
  // wm.setShowDnsFields(true);    // force show dns field always

  // wm.setConnectTimeout(20); // how long to try to connect for before continuing
//  wm.setConfigPortalTimeout(30); // auto close configportal after n seconds
  // wm.setCaptivePortalEnable(false); // disable captive portal redirection
  // wm.setAPClientCheck(true); // avoid timeout if client connected to softap

  // wifi scan settings
  // wm.setRemoveDuplicateAPs(false); // do not remove duplicate ap names (true)
  wm.setMinimumSignalQuality(20);  // set min RSSI (percentage) to show in scans, null = 8%
  // wm.setShowInfoErase(false);      // do not show erase button on info page
  // wm.setScanDispPerc(true);       // show RSSI as percentage not graph icons
  
  // wm.setBreakAfterConfig(true);   // always exit configportal even if wifi save fails

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
   res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  //  res = wm.autoConnect("AutoConnectAP","password"); // password protected ap
  
  while(!res);
}

String getParam(String name){
  //read parameter from server, for customhmtl input
  String value;
  if(wm.server->hasArg(name)) {
    value = wm.server->arg(name);
  }
  return value;
}

void saveParamCallback(){
  int cc;
  
  Serial.println("[CALLBACK] saveParamCallback fired");
  // Serial.println("PARAM customfieldid = " + getParam("customfieldid"));
  // Serial.println("PARAM CityCode = " + getParam("CityCode"));
  // Serial.println("PARAM LCD BackLight = " + getParam("LCDBL"));
  // Serial.println("PARAM WeaterUpdateTime = " + getParam("WeaterUpdateTime"));
  // Serial.println("PARAM Rotation = " + getParam("set_rotation"));
  // Serial.println("PARAM DHT11_en = " + getParam("DHT11_en"));
  
  //将从页面中获取的数据保存
  #if DHT_EN
  DHT_img_flag = getParam("DHT11_en").toInt();
  #endif
  updateweater_time = getParam("WeaterUpdateTime").toInt();
  cc =  getParam("CityCode").toInt();
  LCD_Rotation = getParam("set_rotation").toInt();
  LCD_BL_PWM = getParam("LCDBL").toInt();

  //对获取的数据进行处理
  //城市代码
  Serial.print("CityCode = ");
  Serial.println(cc);
  if(cc>=101000000 && cc<=102000000 || cc == 0)
  {
    saveCityCodetoEEP(&cc);
    // for(int cnum=0;cnum<5;cnum++)
    // {
    //   EEPROM.write(CC_addr+cnum,cc%100);//城市地址写入城市代码
    //   EEPROM.commit();//保存更改的数据
    //   cc = cc/100;
    //   delay(5);
    // }
    readCityCodefromEEP(&cc);
    // for(int cnum=5;cnum>0;cnum--)
    // {          
    //   CCODE = CCODE*100;
    //   CCODE += EEPROM.read(CC_addr+cnum-1); 
    //   delay(5);
    // }
    cityCode = cc;
  }
  //屏幕方向
  Serial.print("LCD_Rotation = ");
  Serial.println(LCD_Rotation);
  if(EEPROM.read(Ro_addr) != LCD_Rotation)
  {
    EEPROM.write(Ro_addr, LCD_Rotation);
    EEPROM.commit();
    delay(5);
  }
  tft.setRotation(LCD_Rotation);
  tft.fillScreen(0x0000);
  Web_win();
  loadNum--;
  loading(1);
  if(EEPROM.read(BL_addr) != LCD_BL_PWM)
  {
    EEPROM.write(BL_addr, LCD_BL_PWM);
    EEPROM.commit();
    delay(5);
  }
  //屏幕亮度
  Serial.printf("亮度调整为：");
  analogWrite(LCD_BL_PIN, 1023 - (LCD_BL_PWM*10));
  Serial.println(LCD_BL_PWM);
  //天气更新时间
  Serial.printf("天气更新时间调整为：");
  Serial.println(updateweater_time);

  #if DHT_EN
  //是否使用DHT11传感器
  Serial.printf("DHT11传感器：");
  EEPROM.write(DHT_addr, DHT_img_flag);
  EEPROM.commit();//保存更改的数据
  Serial.println((DHT_img_flag?"已启用":"未启用"));
  #endif
}
#endif





void setup()
{
  Serial.begin(115200);
  EEPROM.begin(1024);
  if(!LittleFS.begin()) Serial.println("LittleFS init failed");
  // WiFi.forceSleepWake();
  // wm.resetSettings();    //在初始化中使wifi重置，需重新配置WiFi
  
 #if DHT_EN
  dht.begin();
  //从eeprom读取DHT传感器使能标志
  DHT_img_flag = EEPROM.read(DHT_addr);
 #endif
 //从eeprom读取背光亮度设置
  if(EEPROM.read(BL_addr)>0&&EEPROM.read(BL_addr)<100)
    LCD_BL_PWM = EEPROM.read(BL_addr); 
  pinMode(LCD_BL_PIN, OUTPUT);
  analogWrite(LCD_BL_PIN, 1023 - (LCD_BL_PWM*10));
  //从eeprom读取屏幕方向设置
  if(EEPROM.read(Ro_addr)>=0&&EEPROM.read(Ro_addr)<=3)
    LCD_Rotation = EEPROM.read(Ro_addr);
  
  //从eeprom读取天气更新时间
  updateweater_time = EEPROM.read(UpWeT_addr);
  
  

  tft.begin(); /* TFT init */
  tft.invertDisplay(1);//反转所有显示颜色：1反转，0正常
  tft.setRotation(LCD_Rotation);
  tft.fillScreen(0x0000);
  indoorAvatarDirty = 1;
  tft.setTextColor(TFT_BLACK, bgColor);

  targetTime = millis() + 1000; 
  readwificonfig();//读取存储的wifi信息
  Serial.print("正在连接WIFI ");
  Serial.println(wificonf.stassid);
  WiFi.begin(wificonf.stassid, wificonf.stapsw);
  
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);

  while (WiFi.status() != WL_CONNECTED) 
  {
    loading(30);  
      
    if(loadNum>=194)
    {
      //使能web配网后自动将smartconfig配网失效
      #if WM_EN
      Web_win();
      Webconfig();
      #endif

      #if !WM_EN
      SmartConfig();
      #endif   
      break;
    }
  }
  delay(10); 
  while(loadNum < 194) //让动画走完
  { 
    loading(1);
  }

  if(WiFi.status() == WL_CONNECTED)
  {
    // Serial.print("SSID:");
    // Serial.println(WiFi.SSID().c_str());
    // Serial.print("PSW:");
    // Serial.println(WiFi.psk().c_str());
    strcpy(wificonf.stassid,WiFi.SSID().c_str());//名称复制
    strcpy(wificonf.stapsw,WiFi.psk().c_str());//密码复制
    savewificonfig();
    readwificonfig();
    #if WebSever_EN
    //开启web服务器初始化
    Web_Sever_Init();
    Web_sever_Win();
    delay(10000);
    #endif
  }

  // Serial.print("本地IP： ");
  // Serial.println(WiFi.localIP());
  Serial.println("启动UDP");
  Udp.begin(localPort);
  Serial.println("等待同步...");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);

  

  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);
  
  int CityCODE = 0;
  readCityCodefromEEP(&CityCODE);
  // for(int cnum=5;cnum>0;cnum--)
  // {          
  //   CityCODE = CityCODE*100;
  //   CityCODE += EEPROM.read(CC_addr+cnum-1); 
  //   delay(5);
  // }
  if(CityCODE>=101000000 && CityCODE<=102000000) 
    cityCode = CityCODE;  
  else
    getCityCode();  //获取城市代码
   
  tft.fillScreen(TFT_BLACK);//清屏
  indoorAvatarDirty = 1;
  
  TJpgDec.drawJpg(15,183,temperature, sizeof(temperature));  //温度图标
  TJpgDec.drawJpg(15,213,humidity, sizeof(humidity));  //湿度图标

  getCityWeater();
#if DHT_EN
  if(DHT_img_flag != 0)
  IndoorTem();
#endif
#if !WebSever_EN
  WiFi.forceSleepBegin(); //wifi off
  Serial.println("WIFI休眠......");
  Wifi_en = 0;
#endif
}



void loop()
{
  #if WebSever_EN
  Web_Sever();
  #endif
  LCD_reflash(0);
  Serial_set();
}

void LCD_reflash(int en)
{
  if (now() != prevDisplay || en == 1) 
  {
    prevDisplay = now();
    digitalClockDisplay(en);
    prevTime=0;  
  }
  
  //两秒钟更新一次
  if(second()%2 ==0&& prevTime == 0 || en == 1){
#if DHT_EN
    if(DHT_img_flag != 0)
    IndoorTem();
#endif
    scrollBanner();
  }
#if imgAst_EN
  if(DHT_img_flag == 0)
  {
    if(LittleFS.exists("/run.jpg"))
    {
      if(indoorAvatarDirty)
      {
        drawRuntimeCornerImage();
        indoorAvatarDirty = 0;
      }
    }
    else
    {
      imgAnim();
    }
  }
#endif


  if(millis() - weaterTime > (60000*updateweater_time) || en == 1 || UpdateWeater_en != 0){ //10分钟更新一次天气
    if(Wifi_en == 0)
    {
      WiFi.forceSleepWake();//wifi on
      Serial.println("WIFI恢复......");
      Wifi_en = 1;
    }

    if(WiFi.status() == WL_CONNECTED)
    {
      // Serial.println("WIFI已连接");
      getCityWeater();
      if(UpdateWeater_en != 0) UpdateWeater_en = 0;
      weaterTime = millis();
      // while(!getNtpTime());
      getNtpTime();
      #if !WebSever_EN
      WiFi.forceSleepBegin(); // Wifi Off
      Serial.println("WIFI休眠......");
      Wifi_en = 0;
      #endif
    }
  }
}

// 发送HTTP请求并且将服务器响应通过串口输出
void getCityCode(){
 String URL = "http://wgeo.weather.com.cn/ip/?_="+String(now());
  //创建 HTTPClient 对象
  HTTPClient httpClient;
 
  //配置请求地址。此处也可以不使用端口号和PATH而单纯的
  httpClient.begin(wificlient,URL); 
  
  //设置请求头中的User-Agent
  httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
  httpClient.addHeader("Referer", "http://www.weather.com.cn/");
 
  //启动连接并发送HTTP请求
  int httpCode = httpClient.GET();
  Serial.print("Send GET request to URL: ");
  Serial.println(URL);
  
  //如果服务器响应OK则从服务器获取响应体信息并通过串口输出
  if (httpCode == HTTP_CODE_OK) {
    String str = httpClient.getString();
    
    int aa = str.indexOf("id=");
    if(aa>-1)
    {
       //cityCode = str.substring(aa+4,aa+4+9).toInt();
       cityCode = str.substring(aa+4,aa+4+9);
       Serial.println(cityCode); 
       getCityWeater();
    }
    else
    {
      Serial.println("获取城市代码失败");  
    }
    
    
  } else {
    Serial.println("请求城市代码错误：");
    Serial.println(httpCode);
  }
 
  //关闭ESP8266与服务器连接
  httpClient.end();
}



// 获取城市天气
void getCityWeater(){
 //String URL = "http://d1.weather.com.cn/dingzhi/" + cityCode + ".html?_="+String(now());//新
 String URL = "http://d1.weather.com.cn/weather_index/" + cityCode + ".html?_="+String(now());
  //创建 HTTPClient 对象
  HTTPClient httpClient;
  
  httpClient.begin(URL); 
  
  //设置请求头中的User-Agent
  httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
  httpClient.addHeader("Referer", "http://www.weather.com.cn/");
 
  //启动连接并发送HTTP请求
  int httpCode = httpClient.GET();
  Serial.println("正在获取天气数据");
  Serial.println(URL);
  
  //如果服务器响应OK则从服务器获取响应体信息并通过串口输出
  if (httpCode == HTTP_CODE_OK) {

    String str = httpClient.getString();
    int indexStart = str.indexOf("weatherinfo\":");
    int indexEnd = str.indexOf("};var alarmDZ");

    String jsonCityDZ = str.substring(indexStart+13,indexEnd);
    //Serial.println(jsonCityDZ);

  //气象预警不同时间会发布不同的预警信息，只会显示最新的一个，显示多个也只是显示最新时间的前一个预警，没必要了
    indexStart = str.indexOf("alarmDZ ={\"w\":[");
    indexEnd = str.indexOf("]};var dataSK");
    String jsonDataWarn1 = str.substring(indexStart+15,indexEnd);
    Serial.println("1="+jsonDataWarn1);
    if(jsonDataWarn1.length() >= 40) {
      Warn_Flag = 1;
    }
    else {
      Warn_Flag = 0;
    }

    indexStart = str.indexOf("dataSK =");
    indexEnd = str.indexOf(";var dataZS");
    String jsonDataSK = str.substring(indexStart+8,indexEnd);
    //Serial.println(jsonDataSK);

    
    indexStart = str.indexOf("\"f\":[");
    indexEnd = str.indexOf(",{\"fa");
    String jsonFC = str.substring(indexStart+5,indexEnd);
    //Serial.println(jsonFC);


    
    weaterData(&jsonCityDZ,&jsonDataSK,&jsonFC,&jsonDataWarn1);
    Serial.println("获取成功");
    
  } else {
    Serial.println("请求城市天气错误：");
    Serial.print(httpCode);
  }

 
  //关闭ESP8266与服务器连接
  httpClient.end();
}


String scrollText[7];//天气信息存储

//天气信息写到屏幕上
void weaterData(String *cityDZ,String *dataSK,String *dataFC,String *dataWarn1)
{
  //解析第一段JSON
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, *dataSK);
  JsonObject sk = doc.as<JsonObject>();

  //TFT_eSprite clkb = TFT_eSprite(&tft);
  
  /***绘制相关文字***/
  clk.setColorDepth(8);
  clk.loadFont(ZdyLwFont_20);
  
  //温度
  clk.createSprite(58, 24); 
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, bgColor); 
  clk.drawString(sk["temp"].as<String>()+"℃",28,13);
  clk.pushSprite(100,184);
  clk.deleteSprite();
  tempnum = sk["temp"].as<int>();
  tempnum = tempnum+10;
  if(tempnum<10)
    tempcol=0x00FF;
  else if(tempnum<28)
    tempcol=0x0AFF;
  else if(tempnum<34)
    tempcol=0x0F0F;
  else if(tempnum<41)
    tempcol=0xFF0F;
  else if(tempnum<49)
    tempcol=0xF00F;
  else
  {
    tempcol=0xF00F;
    tempnum=50;
  }
  tempWin();
  
  //湿度
  clk.createSprite(58, 24); 
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, bgColor); 
  clk.drawString(sk["SD"].as<String>(),28,13);
  //clk.drawString("100%",28,13);
  clk.pushSprite(100,214);
  clk.deleteSprite();
  //String A = sk["SD"].as<String>();
  huminum = atoi((sk["SD"].as<String>()).substring(0,2).c_str());
  
  if(huminum>90)
    humicol=0x00FF;
  else if(huminum>70)
    humicol=0x0AFF;
  else if(huminum>40)
    humicol=0x0F0F;
  else if(huminum>20)
    humicol=0xFF0F;
  else
    humicol=0xF00F;
  humidityWin();

  
  //城市名称
  clk.createSprite(94, 30); 
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, bgColor); 
  clk.drawString(sk["cityname"].as<String>(),44,16);
  clk.pushSprite(15,15);
  clk.deleteSprite();

  //PM2.5空气指数
  uint16_t pm25BgColor = tft.color565(156,202,127);//优
  String aqiTxt = "优";
  int pm25V = sk["aqi"];
  if(pm25V>200){
    pm25BgColor = tft.color565(136,11,32);//重度
    aqiTxt = "重度";
  }else if(pm25V>150){
    pm25BgColor = tft.color565(186,55,121);//中度
    aqiTxt = "中度";
  }else if(pm25V>100){
    pm25BgColor = tft.color565(242,159,57);//轻
    aqiTxt = "轻度";
  }else if(pm25V>50){
    pm25BgColor = tft.color565(247,219,100);//良
    aqiTxt = "良";
  }
  clk.createSprite(56, 24); 
  clk.fillSprite(bgColor);
  clk.fillRoundRect(0,0,50,24,4,pm25BgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(0x0000); 
  clk.drawString(aqiTxt,25,13);
  clk.pushSprite(104,18);
  clk.deleteSprite();
  
  scrollText[0] = "实时天气 "+sk["weather"].as<String>();
  scrollText[1] = "空气质量 "+aqiTxt;
  scrollText[2] = "风向 "+sk["WD"].as<String>()+sk["WS"].as<String>();

  //scrollText[6] = atoi((sk["weathercode"].as<String>()).substring(1,3).c_str()) ;

  //天气图标
  wrat.printfweather(170,15,atoi((sk["weathercode"].as<String>()).substring(1,3).c_str()));

  
  //左上角滚动字幕
  //解析第二段JSON
  deserializeJson(doc, *cityDZ);
  JsonObject dz = doc.as<JsonObject>();
  //Serial.println(sk["ws"].as<String>());
  //横向滚动方式
  //String aa = "今日天气:" + dz["weather"].as<String>() + "，温度:最低" + dz["tempn"].as<String>() + "，最高" + dz["temp"].as<String>() + " 空气质量:" + aqiTxt + "，风向:" + dz["wd"].as<String>() + dz["ws"].as<String>();
  //scrollTextWidth = clk.textWidth(scrollText);
  //Serial.println(aa);
  scrollText[3] = "今日"+dz["weather"].as<String>();
  
  deserializeJson(doc, *dataFC);
  JsonObject fc = doc.as<JsonObject>();
  
  scrollText[4] = "最低温度"+fc["fd"].as<String>()+"℃";
  scrollText[5] = "最高温度"+fc["fc"].as<String>()+"℃";
  
  //Serial.println(scrollText[0]);


  deserializeJson(doc, *dataWarn1);
  JsonObject dataWarnjson1 = doc.as<JsonObject>();
  Warn_Number1 = dataWarnjson1["w4"].as<int>();
  Warn_Value1 = dataWarnjson1["w6"].as<int>();
  //Serial.println(dataWarnjson);
  Serial.println("气象预警编号1：" + String(Warn_Number1) + " 等级1：" + String(Warn_Value1));
  /*switch(Warn_Number) { // 处理图片太TM烦躁了，而且显示的效果也特别差，而且预警代码一直没找到文档
    case 7: //高温
      if(Warn_Value == 1) {TJpgDec.drawJpg(175,110,gaowen_lan, sizeof(gaowen_lan));break;}
      if(Warn_Value == 2) {TJpgDec.drawJpg(175,110,gaowen_huang, sizeof(gaowen_huang));break;}
      if(Warn_Value == 3) {TJpgDec.drawJpg(175,110,gaowen_cheng, sizeof(gaowen_cheng));break;}
      if(Warn_Value == 4) {TJpgDec.drawJpg(175,110,gaowen_hong, sizeof(gaowen_hong));break;}
    //case 9: //雷电
    //case 0: //雷暴大风
    //case 2: //暴雨
    default:TJpgDec.drawJpg(175,110,BG_Color, sizeof(BG_Color));Serial.println("NULL");break;
  }*/
  uint16_t weatherWarnBgColor1;
  switch(Warn_Value1) { //这等级把我搞蒙了，一会蓝色是0，一会又变成1
    //填充颜色
    case 1:weatherWarnBgColor1 = tft.color565(0,128,255);break; //蓝色
    case 2:weatherWarnBgColor1 = tft.color565(255,204,51);break; //黄色
    case 3:weatherWarnBgColor1 = tft.color565(255,153,0);break; //橙色
    case 4:weatherWarnBgColor1 = tft.color565(255,0,0);break; //红色
    default:Serial.println("NULL");break;
  }
  //多个气象预警显示，有空了再更新
  //if(Warn_Flag == 1) {
    if(dataWarnjson1["w5"].as<String>() != "null") {
      clk.loadFont(ZdyLwFont_20);
      clk.createSprite(90, 24); 
      clk.fillSprite(bgColor);
      clk.fillRoundRect(0,0,90,24,5,weatherWarnBgColor1);
      clk.setTextDatum(CC_DATUM);
      clk.setTextColor(TFT_WHITE); 
      clk.drawString(dataWarnjson1["w5"].as<String>(),45,14);
      //clk.drawString("预 警",45,45);
      clk.pushSprite(145,145);
      clk.deleteSprite();
      clk.unloadFont();
      clk.unloadFont();
    }
  
  clk.unloadFont();
}

int currentIndex = 0;
TFT_eSprite clkb = TFT_eSprite(&tft);

void scrollBanner(){
  //if(millis() - prevTime > 2333) //3秒切换一次
//  if(second()%2 ==0&& prevTime == 0)
//  { 
    if(scrollText[currentIndex])
    {
      clkb.setColorDepth(8);
      clkb.loadFont(ZdyLwFont_20);
      clkb.createSprite(150, 30); 
      clkb.fillSprite(bgColor);
      clkb.setTextWrap(false);
      clkb.setTextDatum(CC_DATUM);
      clkb.setTextColor(TFT_WHITE, bgColor); 
      clkb.drawString(scrollText[currentIndex],74, 16);
      clkb.pushSprite(10,45);
       
      clkb.deleteSprite();
      clkb.unloadFont();
      
      if(currentIndex>=5)
        currentIndex = 0;  //回第一个
      else
        currentIndex += 1;  //准备切换到下一个        
    }
    prevTime = 1;
//  }
}
#if imgAst_EN
void imgAnim()
{
  int x=160,y=170;
  if(millis() - AprevTime > 37) //x ms切换一次
  {
    Anim++;
    AprevTime = millis();
  }
  if(Anim==10)
    Anim=0;

  switch(Anim)
  {
    case 0:
      TJpgDec.drawJpg(x,y,i0, sizeof(i0));
      break;
    case 1:
      TJpgDec.drawJpg(x,y,i1, sizeof(i1));
      break;
    case 2:
      TJpgDec.drawJpg(x,y,i2, sizeof(i2));
      break;
    case 3:
      TJpgDec.drawJpg(x,y,i3, sizeof(i3));
      break;
    case 4:
      TJpgDec.drawJpg(x,y,i4, sizeof(i4));
      break;
    case 5:
      TJpgDec.drawJpg(x,y,i5, sizeof(i5));
      break;
    case 6:
      TJpgDec.drawJpg(x,y,i6, sizeof(i6));
      break;
    case 7:
      TJpgDec.drawJpg(x,y,i7, sizeof(i7));
      break;
    case 8: 
      TJpgDec.drawJpg(x,y,i8, sizeof(i8));
      break;
    case 9: 
      TJpgDec.drawJpg(x,y,i9, sizeof(i9));
      break;
    default:
      Serial.println("显示Anim错误");
      break;
  }
}
#endif

unsigned char Hour_sign   = 60;
unsigned char Minute_sign = 60;
unsigned char Second_sign = 60;
void digitalClockDisplay(int reflash_en)
{ 
  int timey=82;
  if(hour()!=Hour_sign || reflash_en == 1)//时钟刷新
  {
    dig.printfW3660(20,timey,hour()/10);
    dig.printfW3660(60,timey,hour()%10);
    Hour_sign = hour();
  }
  if(minute()!=Minute_sign  || reflash_en == 1)//分钟刷新
  {
    dig.printfO3660(101,timey,minute()/10);
    dig.printfO3660(141,timey,minute()%10);
    Minute_sign = minute();
  }
  if(second()!=Second_sign  || reflash_en == 1)//分钟刷新
  {
    dig.printfW1830(182,timey+30,second()/10);
    dig.printfW1830(202,timey+30,second()%10);
    Second_sign = second();
  }
  
  if(reflash_en == 1) reflash_en = 0;
  /***日期****/
  clk.setColorDepth(8);
  clk.loadFont(ZdyLwFont_20);
   
  //星期
  clk.createSprite(58, 30);
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, bgColor);
  clk.drawString(week(),29,16);
  clk.pushSprite(86,150);
  clk.deleteSprite();
  
  //月日
  clk.createSprite(95, 30);
  clk.fillSprite(bgColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE, bgColor);  
  clk.drawString(monthDay(),49,16);
  clk.pushSprite(2,150);
  clk.deleteSprite();
  
  clk.unloadFont();
  /***日期****/
}

//星期
String week()
{
  String wk[7] = {"日","一","二","三","四","五","六"};
  String s = "周" + wk[weekday()-1];
  return s;
}

//月日
String monthDay()
{
  String s = String(month());
  s = s + "月" + day() + "日";
  return s;
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP时间在消息的前48字节中
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  //Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  //Serial.print(ntpServerName);
  //Serial.print(": ");
  //Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      //Serial.println(secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR);
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // 无法获取时间时返回0
}

// 向NTP服务器发送请求
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
