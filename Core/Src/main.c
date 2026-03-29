/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "ctype.h"
#include "ssd1306.h"
#include "fonts.h"
#include "rc522.h"
#include "string.h"
#include "STM32_Keypad4x4.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct Student Student;
RTC_TimeTypeDef sTime;
RTC_DateTypeDef sDate;


/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BUFFER_SIZE 500
#define MAX_STUDENTS 20 // Số lượng học sinh tối đa

#define FIFO_SIZE 10  // Dung lượng FIFO tối đa (10 URL)
#define URL_MAX_LENGTH 256  // Độ dài tối đa của URL
#define DELAY_MS 10000  // Thời gian delay giữa các lần gửi
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
struct Student {
  int id;
  char name[30];
  char code[11];
  char msv[13] ;
  char State[5];
};

typedef struct {
    char urls[FIFO_SIZE][URL_MAX_LENGTH];
    int head;
    int tail;
    int count;
    uint8_t sending;
} FIFO_t;

FIFO_t fifo = { .head = 0, .tail = 0, .count = 0, .sending = 0 };

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c2;

RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_tx;

/* USER CODE BEGIN PV */
uint32_t lastSendTime = 0;  // Thời gian gửi URL cuối cùng
uint8_t status;
uint8_t str[MAX_LEN]; // Max_LEN = 16
uint8_t sNum[5];
char uidStr[11]; // Chuỗi để chứa UID dạng hex
char Key;

uint16_t len;
char buffer[BUFFER_SIZE];
char bufferTime[20];
char thoi_gian[20] = "";
char hour[10] = "";
uint8_t mode = 0;
char key_pad[5];

char URL [256] = " ";
char master_key[] ="C37BAE2C3A";

int studentCount = 20;

const char* Web_App_URL = "https://script.google.com/macros/s/AKfycbw715puKRx8atB5so9LJqHkDj7WYHC81akzjndNMlAhuG8Q40vDWrL2WK9HzJIMv4UkVA/exec";

//https://script.google.com/macros/s/AKfycbwssVUpCzr_4ROx3tVCOe5E2DRa5P1MQEhmhk9Ac9i-iJqP1feV4W1Mz4AtUMRZseK9Gg/exec
Student students[11] = {
		{1,"kien","C37BAE2C3A","22a1701d0137",""},
		{2,"vu","01A11702B5","22a1701d0138",""},
		{3,"an","35983A0295","22a1701d0139",""},
		{4,"tuan","BED13F0252","22a1701d0140",""},
		{5,"hieu","544C46025C","22a1701d0141",""},
		{6,"quang","4EBC4302B3","22a1701d0142",""},
		{7,"khanh","F70B3802C6","22a1701d0143",""},
		{8,"lam"," ","22a1701d0144",""},
		{9,"nguyen"," ","22a1701d0145",""},
		{10,"hehe"," ","22a1701d0146",""}

};
uint8_t H_in = 0;
uint8_t M_in = 0;
uint8_t s_in = 0;
char timeBuffer [7] = {'0','0','0','0','0','0'};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
static void MX_I2C2_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_RTC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


	uint32_t getCurrentTimeMs() {

		HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
		HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);  // Bắt buộc phải đọc Date để cập nhật Time

		return (sTime.Seconds * 1000);  // Chỉ lấy số giây

	}
	void addURLToFIFO(char *url) {
	    if (fifo.count < FIFO_SIZE) {
	        strncpy(fifo.urls[fifo.head], url, URL_MAX_LENGTH);
	        fifo.head = (fifo.head + 1) % FIFO_SIZE;
	        fifo.count++;
	    }
	}
	void sendURLToESP32_DMA() {
	    if (fifo.count > 0 && fifo.sending == 0) {
	        char *urlBuffer = fifo.urls[fifo.tail];
	        fifo.sending = 1;
	        HAL_UART_Transmit_DMA(&huart1, (uint8_t *)urlBuffer, strlen(urlBuffer));
	    }
	}
	void processSendURL() {
	    uint32_t currentTime = getCurrentTimeMs();
	    if (fifo.count > 0 && !fifo.sending && (currentTime - lastSendTime >= DELAY_MS)) {
	        sendURLToESP32_DMA();
	    }
	}



	void buzzer() {
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
		HAL_Delay(200);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
	}

	void convertUIDToString(uint8_t *uid, char *str) {
	    sprintf(str, "%02X%02X%02X%02X%02X", uid[0], uid[1], uid[2], uid[3], uid[4]);
	}
	void wait();
	void settingkey(void);
	void updateTime();



	char* getStudentNameById(char* uid) {
	  for (int i = 0; i < studentCount; i++) {
	    if (strcmp(students[i].code, uid) == 0) {
	      return students[i].name;
	    }
	  }
	  return NULL; // Trả về nullptr nếu không tìm thấy
	}

	char* getStudentMsvById(char* uid) {
	  for (int i = 0; i < studentCount; i++) {
	    if (strcmp(students[i].code, uid) == 0) {
	      return students[i].msv;
	    }
	  }
	  return NULL; // Trả về nullptr nếu không tìm thấy
	}

	char* getStudentStateById(char* uid) {
		  for (int i = 0; i < studentCount; i++) {
		    if (strcmp(students[i].code, uid) == 0) {
		      return students[i].State;
		    }
		  }
		  return NULL; // Trả về nullptr nếu không tìm thấy
		}

	void changeInOut (char* uid){

	      for (int i = 0; i < studentCount; i++) {
			if (strcmp(students[i].code, uid) == 0) {
				if(strcmp(students[i].State, "vao") == 0) {
					  strcpy(students[i].State, "ra");
				  }else {strcpy(students[i].State, "vao");}
			}
	      }
	}
	void urlencode(const char *str, char *encodedString, size_t maxSize) {
	    size_t j = 0;
	    for (size_t i = 0; str[i] != '\0' && j < maxSize - 1; i++) {
	        char c = str[i];
	        if (c == ' ') {
	            encodedString[j++] = '+';
	        } else if ( isalnum(c)) {
	            encodedString[j++] = c;
	        } else {
	            if (j + 3 < maxSize - 1) {
	                snprintf(&encodedString[j], 4, "%%%02X", (unsigned char)c);
	                j += 3;
	            } else {
	                break;
	            }
	        }
	    }
	    encodedString[j] = '\0';
	}

	const char* checkLate() {

	    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);  // Lấy thời gian hiện tại từ RTC

	    if (sTime.Hours > H_in ||
	       (sTime.Hours == H_in && sTime.Minutes > M_in) ||
	       (sTime.Hours == H_in && sTime.Minutes == M_in && sTime.Seconds > s_in)) {
	        return "DI MUON";  // Sinh viên quẹt thẻ sau giờ vào lớp
	    }
	    return "";  // Sinh viên đến đúng giờ hoặc sớm hơn
	}

	void CreadURL(){
	    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
	    sprintf(hour, "%02d:%02d:%02d",sTime.Hours, sTime.Minutes, sTime.Seconds);

//		char URL [256] = " ";

	  	char charArray[11];
		memcpy(charArray, uidStr, 11);
	  	char* studentName = getStudentNameById(charArray);
	  	char* studentMsv = getStudentMsvById(charArray);
	  	char* State = getStudentStateById(charArray);
	  	changeInOut(charArray);
	  	const char* vaomuon;
		if (strcmp(State, "vao") == 0){
				vaomuon = checkLate();  // Kiểm tra sinh viên có đi muộn không
		}else vaomuon = " ";

	  	if (studentName != NULL ) {
	 		SSD1306_Clear();
	 		SSD1306_GotoXY (0,0);
	 		SSD1306_Puts ("XAC NHAN", &Font_7x10, 1);
	 		SSD1306_GotoXY (0,20);
	 		SSD1306_Puts ("DIEM DANH", &Font_7x10, 1);
	 		SSD1306_GotoXY (0,40);
	 		SSD1306_Puts (studentMsv, &Font_7x10, 1);
	 		SSD1306_UpdateScreen();
	 		HAL_Delay(500);

	 		 char name[50] = " ";
	 		 char state[6] = " ";
	 		 char vaolop[20]= " ";
	 		 char time[15] = " ";
	 		urlencode(hour,time,sizeof(time));
	 		urlencode(studentName,name,sizeof(name));
	 		urlencode(State,state,sizeof(state));
	 		urlencode(vaomuon,vaolop,sizeof(vaolop));



			sprintf(URL, "%s?sts=writelog&uid=%s&name=%s&inout=%s&msv=%s&note=%s&time=%s",Web_App_URL, uidStr, name,state, studentMsv, vaolop, time );


			addURLToFIFO(URL);  // Lưu URL vào FIFO
			HAL_Delay(200);
			SSD1306_GotoXY (0,0);
			SSD1306_Puts ("        ", &Font_7x10, 1);
			SSD1306_GotoXY (0,0);
			SSD1306_Puts ("DANG LUU...", &Font_7x10, 1);
			SSD1306_UpdateScreen();
			HAL_Delay(200);

			wait();
	 	 }else {
	 		  SSD1306_Clear();
	 		  SSD1306_GotoXY (0,0);
	 		  SSD1306_Puts ("not found", &Font_7x10, 1);
	 		  SSD1306_GotoXY (0,20);
	 		  SSD1306_Puts (uidStr, &Font_7x10, 1);
	 		  SSD1306_UpdateScreen();
	 		 HAL_Delay(500);
	 		 wait();
	 	 }
		return ;
	}
	void print_oled (char *Prt,int x,int y){
		SSD1306_Clear();
		SSD1306_GotoXY (x,y);
		SSD1306_Puts ( Prt, &Font_7x10, 1);
		SSD1306_UpdateScreen();
	}
	void readUID(){

		  status = MFRC522_Request(PICC_REQIDL, str);
		  status = MFRC522_Anticoll(str);
		  if(status == 0){
			  buzzer();
			  HAL_Delay(200);
			 // memset(URL, 0, sizeof(URL));

			  convertUIDToString(str, uidStr);
			  HAL_Delay(500);
			  if (strcmp(uidStr, master_key) == 0){

			// vào chế độ cài đặt với thẻ bằng bàn phím
				  settingkey();

			//

			  }else{CreadURL();}
		  }

	}
	void delete_card () {
		SSD1306_Clear();
		SSD1306_GotoXY (0,0);
		SSD1306_Puts ("DELETE CARD...", &Font_7x10, 1);
		SSD1306_UpdateScreen();

		while (1) {
			status = MFRC522_Request(PICC_REQIDL, str);
			status = MFRC522_Anticoll(str);
			if(status == 0){
				  buzzer();
				  HAL_Delay(200);
				 // memset(URL, 0, sizeof(URL));

				  convertUIDToString(str, uidStr);
				  HAL_Delay(500);
				  if (strcmp(uidStr, master_key) == 0){

					  break;
				  }
				  for (int i = 0; i < studentCount; i++) {
						if (strcmp(students[i].code, uidStr) == 0) {
							strcpy(students[i].code, " ");

						}
				  }

					SSD1306_GotoXY (0,10);
					SSD1306_Puts ("DA XOA!!!", &Font_7x10, 1);
					SSD1306_UpdateScreen();
					HAL_Delay(2000);
					SSD1306_GotoXY (0,10);
					SSD1306_Puts ("          ", &Font_7x10, 1);
					SSD1306_UpdateScreen();

			}
		}
		return;
	}

	void add_card() {
		SSD1306_Clear();
		SSD1306_GotoXY (0,0);
		SSD1306_Puts ("ADD CARD MODE", &Font_7x10, 1);
		SSD1306_UpdateScreen();
		HAL_Delay(500);
		SSD1306_GotoXY (0,11);
		SSD1306_Puts ("DANH SACH SV:", &Font_7x10, 1);
		SSD1306_UpdateScreen();
		HAL_Delay(1000);
		SSD1306_Clear();

		char numberID[3] = "";
		int i = 0;
		char state;

		while(i < studentCount){

		  if (strcmp(students[i].code, " ") == 0) {

				SSD1306_GotoXY (0,0);
				SSD1306_Puts ("ID: ", &Font_7x10, 1);
				SSD1306_GotoXY (30,0);
				SSD1306_Puts ("          ", &Font_7x10, 1);
				SSD1306_GotoXY (30,0);
				sprintf(numberID, "%d",students[i].id);
				SSD1306_Puts (numberID, &Font_7x10, 1);
				SSD1306_GotoXY (0,10);
				SSD1306_Puts ("NAME: ", &Font_7x10, 1);
				SSD1306_GotoXY (40,10);
				SSD1306_Puts ("              ", &Font_7x10, 1);
				SSD1306_GotoXY (40,10);
				SSD1306_Puts (students[i].name, &Font_7x10, 1);
				SSD1306_GotoXY (0,20);
				SSD1306_Puts ("MSV: ", &Font_7x10, 1);
				SSD1306_GotoXY (30,20);
				SSD1306_Puts ("                ", &Font_7x10, 1);
				SSD1306_GotoXY (30,20);
				SSD1306_Puts (students[i].msv, &Font_7x10, 1);
				SSD1306_UpdateScreen();
			}else if (i == studentCount - 1) {
				i = 0;
				}else{
					i++;
					continue;
				}

			state = KEYPAD_Read();
			if (state){
			switch(state) {
				case '*' :


					break;
				case '#' :

					i++;
					break;
				default:
					break;
				}

			}
			if (state == '1') {

				break;
			}
		}
		int numID = atoi(numberID) - 1;
		SSD1306_Clear();
		SSD1306_GotoXY (0,0);
		SSD1306_Puts (numberID, &Font_7x10, 1);
		SSD1306_UpdateScreen();
		HAL_Delay(2000);



		SSD1306_Clear();
		wait();
		int bien_quet = 1;
		while (bien_quet) {
			status = MFRC522_Request(PICC_REQIDL, str);
			status = MFRC522_Anticoll(str);
			if(status == 0){
				  buzzer();
				  HAL_Delay(200);
				 // memset(URL, 0, sizeof(URL));
				  convertUIDToString(str, uidStr);
				  HAL_Delay(500);
				  if (strcmp(uidStr, master_key) == 0){
					  break;
				  }
				  int i = 0;
				  for (i = 0; i < studentCount; i++) {
						if (strcmp(students[i].code, uidStr) == 0) {
							SSD1306_Clear();
							SSD1306_GotoXY (0,0);
							SSD1306_Puts ("THE DA CO DATA", &Font_7x10, 1);
							SSD1306_UpdateScreen();
							HAL_Delay(1000);
							SSD1306_Clear();
							SSD1306_GotoXY (0,0);
							SSD1306_Puts ("HAY QUET THE KHAC", &Font_7x10, 1);
							SSD1306_UpdateScreen();
							HAL_Delay(1000);
							break;
						}
						if(i == studentCount - 1) {
							SSD1306_Clear();
							SSD1306_GotoXY (0,0);
							SSD1306_Puts ("DA THEM THE", &Font_7x10, 1);
						    strcpy(students[numID].code, uidStr);
						    SSD1306_UpdateScreen();
						    HAL_Delay(1000);
						    SSD1306_Clear();
						    bien_quet = 0;
						}
				  }




//					SSD1306_GotoXY (0,10);
//					SSD1306_Puts ("DA XOA!!!", &Font_7x10, 1);
//					SSD1306_UpdateScreen();
//					HAL_Delay(2000);
//					SSD1306_GotoXY (0,10);
//					SSD1306_Puts ("          ", &Font_7x10, 1);
//					SSD1306_UpdateScreen();

			}
		}
		return;
	}

	void setTimeFromKeypad() {


//	    // Chuyển chuỗi thành số nguyên
		H_in = (timeBuffer[0] - '0') * 10 + (timeBuffer[1] - '0');
		M_in = (timeBuffer[2] - '0') * 10 + (timeBuffer[3] - '0');
		s_in = (timeBuffer[4] - '0') * 10 + (timeBuffer[5] - '0');


	}






	void set_time_in() {

		uint8_t index = 0;
		char tg;
		char str[2];
		uint8_t count = 0;
		uint8_t x = 0;


		SSD1306_Clear();
//		SSD1306_GotoXY (0,0);
//		SSD1306_Puts ("00:00:00", &Font_7x10, 1);
		SSD1306_GotoXY (0,0);
		str[0] = timeBuffer[0];
		str[1] = '\0';
		SSD1306_Puts (str, &Font_7x10, 1);
		str[0] = timeBuffer[1];
		str[1] = '\0';
		SSD1306_Puts (str, &Font_7x10, 1);
		SSD1306_Puts (":", &Font_7x10, 1);
		str[0] = timeBuffer[2];
		str[1] = '\0';
		SSD1306_Puts (str, &Font_7x10, 1);
		str[0] = timeBuffer[3];
		str[1] = '\0';
		SSD1306_Puts (str, &Font_7x10, 1);
		SSD1306_Puts (":", &Font_7x10, 1);
		str[0] = timeBuffer[4];
		str[1] = '\0';
		SSD1306_Puts (str, &Font_7x10, 1);
		str[0] = timeBuffer[5];
		str[1] = '\0';
		SSD1306_Puts (str, &Font_7x10, 1);
		SSD1306_UpdateScreen();

		SSD1306_GotoXY (0,0);
		while(1){
			tg = KEYPAD_Read();  // Đọc phím

			if (tg) {
				x = index > 0 ? index*7 : 0;
				SSD1306_GotoXY (x,0);
				buzzer();
				if (tg >= '0' && tg <= '9' && count < 6) {
					str[0] = tg;
					str[1] = '\0';  // Ký tự kết thúc chuỗi
//					x = index > 0 ? index*7 : 0;
//					SSD1306_GotoXY (x,0);
					index++;
					SSD1306_Puts (str, &Font_7x10, 1);
					SSD1306_UpdateScreen();
					timeBuffer[count++] = tg;  // Lưu số vào buffer
					if (index == 2 || index == 5) {
						SSD1306_Puts (":", &Font_7x10, 1);
						SSD1306_UpdateScreen();
						index++;
					}
				} else if (tg == '*') {
					// Xóa nhập lại
					if (index == 0) {
						index = 0;
						--count;
					}else {
						if (index == 3 || index == 6) {
							index -=2 ;
						}else{
							--index;
						}

						--count;
					}

					x = index > 0 ? index*7 : 0;
					SSD1306_GotoXY (x,0);
					SSD1306_Puts ("0", &Font_7x10, 1);
					SSD1306_UpdateScreen();


				} else if (tg == '#' || count == 6 ) {
					timeBuffer[6] = '\0';  // Kết thúc chuỗi
					setTimeFromKeypad();  // Cập nhật RTC
					count = 0;  // Reset buffer
					index = 0;
					break;
				}
			}
		}

	}
void settingkey(void) {
		int i = 0;
		SSD1306_Clear();
		SSD1306_GotoXY (0,0);
		SSD1306_Puts ("1.DELETE CARD", &Font_7x10, 1);
		SSD1306_GotoXY (0,10);
		SSD1306_Puts ("2.ADD CARD", &Font_7x10, 1);
		SSD1306_GotoXY (0,20);
		SSD1306_Puts ("3.CAP NHAT TIME", &Font_7x10, 1);
		SSD1306_GotoXY (0,30);
		SSD1306_Puts ("4.SET TIME VAO LOP", &Font_7x10, 1);
		SSD1306_GotoXY (0,40);
		SSD1306_Puts ("ANY_KEY_FOR_EXIT", &Font_7x10, 1);
		SSD1306_UpdateScreen();
		HAL_Delay(20);
		while (i < 1) {
			Key = KEYPAD_Read();

			if (Key){
				  buzzer();
//				  key_pad[i] = Key;
//				  i++;
//			}
//				/* Read Keypad Every 100ms */
//				HAL_Delay(20);
			switch(Key) {
				case '1' :
					delete_card();
					break;
				case '2' :
					add_card();
					break;
				case '3' :
					updateTime();
					break;
				case '4' :
					set_time_in();
					break;
				default:
					break;

				}
			wait();
			return;
			}
		}

}



	void wait(){
		  SSD1306_Clear();
		  SSD1306_GotoXY (0,20);
		  SSD1306_Puts ("___HAY QUET THE____", &Font_7x10, 1);

		  SSD1306_UpdateScreen();

	}


	void set_RTC_from_string() {
		int year, month, day, hour, minute, second;
		sscanf(bufferTime, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);

	    sTime.Hours = (hour);
	    sTime.Minutes = (minute);
	    sTime.Seconds = (second+3);
	    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

	    sDate.Year = (year - 2000);
	    sDate.Month = (month);
	    sDate.Date = (day);
	    HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);



	}

	void Time(void) {
	    // Lấy thời gian từ RTC
	    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

	    // Định dạng chuỗi thời gian
	    sprintf(thoi_gian, "%02d:%02d:%02d",sTime.Hours, sTime.Minutes, sTime.Seconds);

	    // Hiển thị lên OLED

		SSD1306_GotoXY (36,45);
		SSD1306_Puts (thoi_gian, &Font_7x10, 1);

		  SSD1306_UpdateScreen();
	}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */


  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
  HAL_Delay(5000);
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_I2C2_Init();
  MX_USART1_UART_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */

  MFRC522_Init();
  HAL_Delay(1000);
  SSD1306_Init();

//  HAL_UART_Receive_IT(&huart1, (uint8_t*)&len, sizeof(len));



	  SSD1306_GotoXY (0,0);
	  SSD1306_Puts ("////////////////", &Font_7x10, 1);
	  SSD1306_GotoXY (0, 30);
	  SSD1306_Puts ("Xin Chao", &Font_7x10, 1);
	  SSD1306_UpdateScreen();
	  HAL_Delay (1000);

	  SSD1306_ScrollRight(0,7);
	  HAL_Delay(3000);
	  SSD1306_ScrollLeft(0,7);
	  HAL_Delay(3000);
	  SSD1306_Stopscroll();
	  SSD1306_Clear();

	SSD1306_Fill(1);
	SSD1306_UpdateScreen();
	HAL_Delay(1000);
	SSD1306_Clear();
	wait();

	HAL_UART_Transmit(&huart1, (uint8_t*)"time", sizeof("time"), HAL_MAX_DELAY);
	HAL_Delay(500);
	HAL_UART_Receive_IT(&huart1, (uint8_t*)bufferTime, 19);
	HAL_Delay(2000);
	set_RTC_from_string();
	HAL_Delay(1000);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
//  	  SSD1306_GotoXY (0,0);
//  	  SSD1306_Puts (" ", &Font_8x8, 1);

	  readUID();
	  Time();
	  processSendURL();

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 400000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef DateToUpdate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
  hrtc.Init.OutPut = RTC_OUTPUTSOURCE_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;

  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  DateToUpdate.WeekDay = RTC_WEEKDAY_MONDAY;
  DateToUpdate.Month = RTC_MONTH_JANUARY;
  DateToUpdate.Date = 0x1;
  DateToUpdate.Year = 0x0;

  if (HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5
                          |GPIO_PIN_6, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB12 PB13 PB14 PB15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB3 PB4 PB5 PB6 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void updateTime() {
	SSD1306_Clear();
	SSD1306_GotoXY (0,0);
	SSD1306_Puts ("UPDATE TIME", &Font_7x10, 1);
	SSD1306_UpdateScreen();
	HAL_Delay(100);
	HAL_UART_Transmit(&huart1, (uint8_t*)"time", sizeof("time"), HAL_MAX_DELAY);
	HAL_Delay(200);
	HAL_UART_Receive_IT(&huart1, (uint8_t*)bufferTime, 19);
	HAL_Delay(2000);
	set_RTC_from_string();
//	HAL_Delay(1000);
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
    if (huart->Instance == huart1.Instance) {  // Kiểm tra UART nào gọi callback

//        // 2. Kiểm tra nếu độ dài quá lớn
//        if (len >= BUFFER_SIZE) len = BUFFER_SIZE - 1;
//
//        // 3. Nhận đúng số byte dữ liệu
//        HAL_UART_Receive_IT(&huart1, (uint8_t*)buffer, len);
//
//        // 4. Kết thúc chuỗi để tránh lỗi in ra màn hình
//        buffer[len] = '\0';
//    	readData(buffer,len);
//        HAL_UART_Transmit(&huart1,(uint8_t*)buffer, len , HAL_MAX_DELAY);

//        HAL_UART_Receive_IT(&huart1, (uint8_t*)&len, sizeof(len));  // Khởi động lại nhận



    }
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
    if (huart->Instance == huart1.Instance) {  // Kiểm tra UART nào gọi callback

        fifo.sending = 0;
        fifo.tail = (fifo.tail + 1) % FIFO_SIZE;
        fifo.count--;

        // Cập nhật thời gian gửi cuối cùng
        lastSendTime = getCurrentTimeMs();

    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
