// AlgEYE - Master Program (PC Based)

#include<opencv/cvaux.h>
#include<opencv/highgui.h>
#include<opencv/cxcore.h>
#include <opencv/cv.h>

#include<stdio.h>
#include<stdlib.h>
#include <fstream>

#include <time.h>

#include"Serial.h"

using namespace std;

//This function is a work around for the issues that arise when using the CSerial library
//It essentially just loops and waits until the right number of chars are received
//This is to avoid dropped chars
char readCharLoop(){
	CSerial serial2;
	char lpBuffer='0';
	
	if(serial2.Open(3,9600))
	{
			int num=0;
			do{
				num= serial2.ReadData(&lpBuffer, 1);
			}
			while(num==0);
			
			serial2.Close();
	}

	return lpBuffer;

}

///////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) {
	
	//Initialize OpenCV
	CvSize size640x480 = cvSize(640, 480);
	CvCapture* p_capWebcam;	
	IplImage* p_imgOriginal;
	IplImage* hsv;		
	
	//Initialize globals used throughout the program
	ofstream dataFile;
	int netTime = 0;
	int intervalTime = 1000;
	int hourCount = 1;

	int avgCounter = 1; float avgH =0, avgS =0, avgV =0, avgH2 =0, avgS2 =0, avgV2 =0; 

	//More misc initialization for OpenCV and the program
	CvMemStorage* p_strStorage;	
	char charCheckForEscKey;
	p_capWebcam = cvCaptureFromCAM(0);
	
	//Safety check to make sure the camera is there
	if(p_capWebcam == NULL) {
		printf("error: capture is NULL \n");
		getchar();
		return(-1);	
	}

	//Declare video feed window
	cvNamedWindow("Original", CV_WINDOW_AUTOSIZE);

	//Create new image that will be the converted hsv values
	hsv = cvCreateImage(size640x480, IPL_DEPTH_8U, 3);
	
	//Infinite loop (until esc sequence)
	while(1) {
		//Get the current frame
		p_imgOriginal = cvQueryFrame(p_capWebcam);
		
		//Convert RGB to HSV in a separate image
		cvCvtColor( p_imgOriginal, hsv, CV_BGR2HSV );
		
		//Safety check to make sure camera actually captured stuff
		if(p_imgOriginal == NULL) {
			printf("error: frame is NULL \n");
			getchar();
			break;
		}
		
		//Initilization of vars for the average algorithm
		float H=0, S=0, V=0, R=0, G=0, B=0, pixCount =1;
		float H2=0, S2=0, V2=0, R2=0, G2=0, B2=0, pixCount2 = 1;
		CvScalar curValHSV;
		CvScalar curValRGB;

		//Average algorithm that takes the average for 2 reactors and 2 sets of values (RGB and HSV)
		for(int row=190; row<290; row++){
			
			for(int col=150; col<250; col++){
				curValHSV = cvGet2D(hsv,row,col); 
				curValRGB = cvGet2D(p_imgOriginal,row,col);

				H=(H*(pixCount-1)+curValHSV.val[0])/pixCount;
				S=(S*(pixCount-1)+curValHSV.val[1])/pixCount;
				V=(V*(pixCount-1)+curValHSV.val[2])/pixCount;

				B=(B*(pixCount-1)+curValRGB.val[0])/pixCount;
				G=(G*(pixCount-1)+curValRGB.val[1])/pixCount;
				R=(R*(pixCount-1)+curValRGB.val[2])/pixCount;

				pixCount++;
			}

			for(int col=390; col<490; col++){
				curValHSV = cvGet2D(hsv,row,col); 
				curValRGB = cvGet2D(p_imgOriginal,row,col);

				H2=(H2*(pixCount-1)+curValHSV.val[0])/pixCount;
				S2=(S2*(pixCount-1)+curValHSV.val[1])/pixCount;
				V2=(V2*(pixCount-1)+curValHSV.val[2])/pixCount;

				B2=(B2*(pixCount-1)+curValRGB.val[0])/pixCount;
				G2=(G2*(pixCount-1)+curValRGB.val[1])/pixCount;
				R2=(R2*(pixCount-1)+curValRGB.val[2])/pixCount;

				pixCount2++;
			}

		}

		printf("H1=%f S1=%f V1=%f\n", H, S, V);
		printf("H2=%f S2=%f V2=%f\n", H2, S2, V2);

		//Draw circles of the average colour of each reactor
		cvCircle(p_imgOriginal,	cvPoint(213,500), 50, CV_RGB(R, G, B), CV_FILLED);
		cvCircle(p_imgOriginal, cvPoint(426,500), 50, CV_RGB(R2, G2, B2), CV_FILLED);

		//Draw helper "cross hair" rectangles
		cvRectangle(p_imgOriginal, cvPoint(150,190), cvPoint(250,290), CV_RGB(255,0,0), 3);	
		cvRectangle(p_imgOriginal, cvPoint(390,190), cvPoint(490,290), CV_RGB(255,0,0), 3);
		
		//Allocate necessary memory
		p_strStorage = cvCreateMemStorage(0);

		//Display video feed
		cvShowImage("Original", p_imgOriginal);
			
		//Start serial communication
		CSerial serial;

		//If opening the serial port is succesful
		if (serial.Open(3, 9600))
		{
			//If the algae are green and are unsaturated/dying
			if( (H>28 && H<44) && S<230){
				//Send the microcontroller the message to turn the motor
				static char szMessage = '1';
				serial.SendData(&szMessage, 1);
				serial.Close(); 	
			
				char * outputc = new char[6];
			 
				outputc[6]=0;
				
				//Wait for servo success code and temperature
				for(int i=0; i<6; i++){
						outputc[i]=readCharLoop();
				}
			
				printf("Temperature: %s degrees C \n\n", outputc);
				int tens = outputc[2]-48;
				int ones = outputc[3]-48;
				int dec = outputc[5]-48;
				float num = tens*10+ones+0.1*dec;
				 
			}
			else{
				static char szMessage = '0';
				serial.SendData(&szMessage, 1);
				serial.Close(); 

				char * outputc = new char[5];
			
				outputc[5]=0;
			
				//Wait for temperature to come in
				for(int i=0; i<5; i++){
						 outputc[i]=readCharLoop();
				}
			
				printf("Temperature: %s degrees C \n\n", outputc);
				int tens = outputc[1]-48;
				int ones = outputc[2]-48;
				int dec = outputc[4]-48;
				float num = tens*10+ones+0.1*dec;
				
			}
		}
		else
			printf("Failed to open port!\n\n\n");


		//Release memory
		cvReleaseMemStorage(&p_strStorage);	
		
		//If esc key pressed, then break loop
		charCheckForEscKey = cvWaitKey(10);
		if(charCheckForEscKey == 27) break;		

		//Placeholder time interval component (replace with time.h functions)
		Sleep(intervalTime); 
		netTime += intervalTime;
		
		//Save data locally to a .txt file
		if(netTime/1000/60/60 >= hourCount)
		{
			avgH=(avgH*(avgCounter-1)+H)/avgCounter;
			avgS=(avgS*(avgCounter-1)+S)/avgCounter;
			avgV=(avgV*(avgCounter-1)+V)/avgCounter;

			avgH2=(avgH2*(avgCounter-1)+H2)/avgCounter;
			avgS2=(avgS2*(avgCounter-1)+S2)/avgCounter;
			avgV2=(avgV2*(avgCounter-1)+V2)/avgCounter;
			
			if (avgCounter >=5){

			dataFile.open("data.txt", std::ios_base::app);
			dataFile << "Time since beginning: "<< hourCount << ":00 hours \n" ;
			dataFile << "HSV (SD): " << avgH << ", " << avgS  << ", "<< avgV << "\n";
			dataFile << "HSV (C): " << avgH2 << ", " << avgS2  << ", "<< avgV2 << "\n\n";
			dataFile.close();

			hourCount +=1;
			avgCounter=0;
			avgH =0; avgS =0; avgV =0; avgH2 =0; avgS2 =0; avgV2 =0;
			}

			avgCounter+=1;
		}

	} // end while

	//Release more memory and destroy the window when done
	cvReleaseCapture(&p_capWebcam);
	cvDestroyWindow("Original");

	return(0);

}

