///////////////////////////////////
//
// 程式名稱:Emotion recognition  
// 作者: 李尚庭
// 日期:2016/7
// 目的: 臉部、身體、聲音model，kinect擷取，emotion fusion  (讀檔及參數在emotion_vc10.cpp的各class initial調整，getoutput取值)
// 副程式:NeuralNetwork.h、DataProcess.h、WaveWriter.h、AudioExplorer.h
// libary: opencv2.4.9、Eigen、KinectSDK1.8
// 全域變數: knModel
//
///////////////////////////////////

#include <windows.h>
#include <mmsystem.h>
#include <assert.h>
//#include <strsafe.h>
#include <iostream> 
#include <fstream>
#include <math.h>
#include "resource.h"
#include <strsafe.h>
#include "emotion_vc10.h"
#include <conio.h>
// For INT_MAX
#include <limits.h>
#include "XDSP.h"
#include "WaveWriter.h"
#include "Utilities.h"
#include <map> 

using namespace cv;

KinectModel knModel;  //kinect class

Mat colorpic(COLOR_HIGHT, COLOR_WIDTH, CV_8UC4);

//double tansig(double n)
//{
//	n = 2 / (1 + exp(-2 * n)) - 1;
//
//	return n;
//}
//
//void sigmoid(MatrixXd& a){
//	for (int i = 0, m = a.rows(); i < m; ++i){
//		for (int j = 0, n = a.cols(); j < n; ++j){
//			a(i, j) = 1 / (1 + exp(-a(i, j)));
//		}
//	}
//}

//------------------------------------------------------------------------
//獲取彩色圖像資料，並進行顯示
int KinectModel::DrawColor(HANDLE h)
{
	const NUI_IMAGE_FRAME * pImageFrame = NULL;
	HRESULT hr = NuiImageStreamGetNextFrame( h, 0, &pImageFrame );
	if( FAILED( hr ) )
	{
		cout<<"Get Color Image Frame Failed"<<endl;
		return -1;
	}
	INuiFrameTexture * pTexture = pImageFrame->pFrameTexture;
	NUI_LOCKED_RECT LockedRect;
	pTexture->LockRect( 0, &LockedRect, NULL, 0 );//提取資料幀到LockedRect中，包括兩個資料物件：pitch表示每行位元組數，pBits第一個位元組的位址
	if( LockedRect.Pitch != 0 )//如果每行位元組數不為0
	{
		BYTE * pBuffer = (BYTE*) LockedRect.pBits;//pBuffer指向資料幀的第一個位元組的位址
		//該函數的作用是在LockedRect第一個位元組開始的位址複製min(pColorFrame->GetBufferSize(), UINT(pTexture->BufferLen())）個位元組到pColorFrame->GetBuffer()所指的緩衝區
		memcpy(pColorFrame->GetBuffer(), PBYTE(LockedRect.pBits), //PBYTE表示無符號單字節數值
			min(pColorFrame->GetBufferSize(), UINT(pTexture->BufferLen())));//GetBuffer()它的作用是返回一個可寫的緩衝指標

		//knData.pColorFrame = pColorFrame;

		//OpenCV顯示彩色視頻 
		Mat temp(COLOR_HIGHT, COLOR_WIDTH, CV_8UC4, pBuffer);
		//colorpic=temp;
		//resize(colorpic,colorpic,Size(COLOR_WIDTH,COLOR_HIGHT));

		//imshow("ColorVideo",temp);
		int c = cv::waitKey(1);//按下ESC結束
		//如果在視頻介面按下ESC,q,Q都會導致整個程式退出
		if( c == 27 || c == 'q' || c == 'Q' )
		{
			SetEvent(m_hEvNuiProcessStop);
			//knData.m_hEvNuiProcessStop = m_hEvNuiProcessStop;
		}
	}
	NuiImageStreamReleaseFrame( h, pImageFrame );
	return 0;
}
//獲取深度圖像資料，並進行顯示
int KinectModel::DrawDepth(HANDLE h)
{
	const NUI_IMAGE_FRAME * pImageFrame = NULL;
	HRESULT hr = NuiImageStreamGetNextFrame( h, 0, &pImageFrame );
	if( FAILED( hr ) )
	{
		cout<<"Get Depth Image Frame Failed"<<endl;
		return -1;
	}
	INuiFrameTexture * pTexture = pImageFrame->pFrameTexture;
	NUI_LOCKED_RECT LockedRect;
	pTexture->LockRect( 0, &LockedRect, NULL, 0 );
	if( LockedRect.Pitch != 0 )
	{
		USHORT * pBuff = (USHORT*) LockedRect.pBits;//注意這裡需要轉換，因為每個資料是2個位元組，存儲的同上面的顏色資訊不一樣，這裡是2個位元組一個資訊，不能再用BYTE，轉化為USHORT
		//	 pDepthBuffer = pBuff;
		memcpy(pDepthFrame->GetBuffer(), PBYTE(LockedRect.pBits),
			min(pDepthFrame->GetBufferSize(), UINT(pTexture->BufferLen())));

		//knData.pDepthFrame = pDepthFrame;

		for(int i=0;i<DEPTH_WIDTH*DEPTH_HIGHT;i++)
		{
			BYTE index = pBuff[i] & 0x07;//提取ID資訊
			USHORT realDepth = (pBuff[i]&0xFFF8)>>3;//提取距離資訊
			BYTE scale = 255 - (BYTE)(256*realDepth/0x0fff);//因為提取的資訊時距離資訊
			DepthBuf[CHANNEL*i] = DepthBuf[CHANNEL*i+1] = DepthBuf[CHANNEL*i+2] = 0;
			switch (index) //根據ID上色
			{
			case 0:
				DepthBuf[CHANNEL*i]=scale/2;
				DepthBuf[CHANNEL*i+1]=scale/2;
				DepthBuf[CHANNEL*i+2]=scale/2;
				break;
			case 1:
				DepthBuf[CHANNEL*i]=scale;
				break;
			case 2:
				DepthBuf[CHANNEL*i+1]=scale;
				break;
			case 3:
				DepthBuf[CHANNEL*i+2]=scale;
				break;
			case 4:
				DepthBuf[CHANNEL*i]=scale;
				DepthBuf[CHANNEL*i+1]=scale;
				break;
			case 5:
				DepthBuf[CHANNEL*i]=scale;
				DepthBuf[CHANNEL*i+2]=scale;
				break;
			case 6:
				DepthBuf[CHANNEL*i+1]=scale;
				DepthBuf[CHANNEL*i+2]=scale;
				break;
			case 7:
				DepthBuf[CHANNEL*i]=255-scale/2;
				DepthBuf[CHANNEL*i+1]=255-scale/2;
				DepthBuf[CHANNEL*i+2]=255-scale/2;
				break;
			}
		}
		Mat temp(DEPTH_HIGHT,DEPTH_WIDTH,CV_8UC3,DepthBuf);
		//imshow("DepthVideo",temp);
		int c = cv::waitKey(1);//按下ESC結束
		if( c == 27 || c == 'q' || c == 'Q' )
		{
			SetEvent(m_hEvNuiProcessStop);
			//knData.m_hEvNuiProcessStop = m_hEvNuiProcessStop;
		}
	}
	NuiImageStreamReleaseFrame( h, pImageFrame );
	return 0;
}
//獲取骨骼資料，並進行顯示
int KinectModel::DrawSkeleton()
{
	vector<int> skeletoncount;

	//NUI_SKELETON_FRAME SkeletonFrame;//骨骼幀的定義
	cv::Point pt[20];
	Mat skeletonMat=Mat(SKELETON_HIGHT,SKELETON_WIDTH,CV_8UC3,Scalar(0,0,0));
	//直接從kinect中提取骨骼幀
	HRESULT hr = NuiSkeletonGetNextFrame( 0, &SkeletonFrame );
	if( FAILED( hr ) )
	{
		cout<<"Get Skeleton Image Frame Failed"<<endl;
		return -1;
	}
	bool bFoundSkeleton = false; //判斷是否追蹤到骨架
	for( int i = 0 ; i < NUI_SKELETON_COUNT ; i++ )
	{
		if( SkeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED )
		{
			bFoundSkeleton = true;
			skeletoncount.push_back(i); //將目前追蹤到的骨架放入vector
		//	cout << "up: " << i << endl;
		}
	}
	// 判斷是否有跟蹤目標，沒有目標及更新最近骨架 //
	if (skeletoncount.size() == 1)	//只追蹤到一個骨架即鎖定
	{
		skeletonID = skeletoncount[0];
	}
	else if (skeletoncount.size()>1) // 判斷是否有跟蹤目標，沒有目標及更新最近骨架
	{
		double temp_dis = SkeletonFrame.SkeletonData[skeletoncount[0]].SkeletonPositions[NUI_SKELETON_POSITION_HEAD].z;
		double temp_ID = skeletonID;
		bool same = false;

		for (int i = 0; i < skeletoncount.size(); ++i)
		{
			if (skeletoncount[i] == skeletonID) same = true; //目標還在

			if (SkeletonFrame.SkeletonData[skeletoncount[i]].SkeletonPositions[NUI_SKELETON_POSITION_HEAD].z<temp_dis)
			{
				temp_dis = SkeletonFrame.SkeletonData[skeletoncount[i]].SkeletonPositions[NUI_SKELETON_POSITION_HEAD].z;
				temp_ID = skeletoncount[i];
			}
		}
		if (!same) skeletonID = temp_ID; //沒有目標更新最近骨架
	}

	// 跟蹤到了骨架
	if( bFoundSkeleton )
	{
		NuiTransformSmooth(&SkeletonFrame,NULL);
		//knData.SkeletonFrame = SkeletonFrame;

			if (SkeletonFrame.SkeletonData[skeletonID].eTrackingState == NUI_SKELETON_TRACKED&&
				SkeletonFrame.SkeletonData[skeletonID].eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_SHOULDER_CENTER] != NUI_SKELETON_POSITION_NOT_TRACKED)
			{
				for (int j = 0; j < NUI_SKELETON_POSITION_COUNT; j++)
				{
					float fx,fy;
					NuiTransformSkeletonToDepthImage(SkeletonFrame.SkeletonData[skeletonID].SkeletonPositions[j], &fx, &fy); //轉換至顯示座標
					pt[j].x = (int) ( fx * SKELETON_WIDTH )/320;
					pt[j].y = (int) ( fy * SKELETON_HIGHT )/240;
					circle(skeletonMat,pt[j],5,CV_RGB(255,0,0));
					
					if (SkeletonFrame.SkeletonData[skeletonID].eSkeletonPositionTrackingState[j] != NUI_SKELETON_POSITION_NOT_TRACKED)//跟踪点一用有三种状态：1没有被跟踪到，2跟踪到，3根据跟踪到的估计到   
					{
						//emotionmodel.bodytracked[emotionmodel.skeletoncount] = true;
						//knData.bodyistrack = true;
						bodyistrack = true;
						//cout << "down: " << i << endl;
					}
				}
				
				//畫骨架
				//	 cv::line(skeletonMat,pt[NUI_SKELETON_POSITION_SHOULDER_CENTER],pt[NUI_SKELETON_POSITION_SPINE],CV_RGB(0,255,0));
				//	 cv::line(skeletonMat,pt[NUI_SKELETON_POSITION_SPINE],pt[NUI_SKELETON_POSITION_HIP_CENTER],CV_RGB(0,255,0));
				cv::line(skeletonMat,pt[NUI_SKELETON_POSITION_HEAD],pt[NUI_SKELETON_POSITION_SHOULDER_CENTER],CV_RGB(0,255,0));
				cv::line(skeletonMat,pt[NUI_SKELETON_POSITION_HAND_RIGHT],pt[NUI_SKELETON_POSITION_WRIST_RIGHT],CV_RGB(0,255,0));
				cv::line(skeletonMat,pt[NUI_SKELETON_POSITION_WRIST_RIGHT],pt[NUI_SKELETON_POSITION_ELBOW_RIGHT],CV_RGB(0,255,0));
				cv::line(skeletonMat,pt[NUI_SKELETON_POSITION_ELBOW_RIGHT],pt[NUI_SKELETON_POSITION_SHOULDER_RIGHT],CV_RGB(0,255,0));
				cv::line(skeletonMat,pt[NUI_SKELETON_POSITION_SHOULDER_RIGHT],pt[NUI_SKELETON_POSITION_SHOULDER_CENTER],CV_RGB(0,255,0));
				cv::line(skeletonMat,pt[NUI_SKELETON_POSITION_SHOULDER_CENTER],pt[NUI_SKELETON_POSITION_SHOULDER_LEFT],CV_RGB(0,255,0));
				cv::line(skeletonMat,pt[NUI_SKELETON_POSITION_SHOULDER_LEFT],pt[NUI_SKELETON_POSITION_ELBOW_LEFT],CV_RGB(0,255,0));
				cv::line(skeletonMat,pt[NUI_SKELETON_POSITION_ELBOW_LEFT],pt[NUI_SKELETON_POSITION_WRIST_LEFT],CV_RGB(0,255,0));
				cv::line(skeletonMat,pt[NUI_SKELETON_POSITION_WRIST_LEFT],pt[NUI_SKELETON_POSITION_HAND_LEFT],CV_RGB(0,255,0));
				m_hint3D[0].x = SkeletonFrame.SkeletonData[skeletonID].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER].x;
				m_hint3D[0].y = SkeletonFrame.SkeletonData[skeletonID].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER].y;
				m_hint3D[0].z = SkeletonFrame.SkeletonData[skeletonID].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER].z;
				m_hint3D[1].x = SkeletonFrame.SkeletonData[skeletonID].SkeletonPositions[NUI_SKELETON_POSITION_HEAD].x;
				m_hint3D[1].y = SkeletonFrame.SkeletonData[skeletonID].SkeletonPositions[NUI_SKELETON_POSITION_HEAD].y;
				m_hint3D[1].z = SkeletonFrame.SkeletonData[skeletonID].SkeletonPositions[NUI_SKELETON_POSITION_HEAD].z;

			}
				//emotionmodel.body_getfeature(i);
			//cout<<SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT].x<<"     "<<SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT].y<<endl;
	}
	skeletoncount.clear();

	//imshow("SkeletonVideo",skeletonMat);
	cv::waitKey(1);
	int c = cv::waitKey(1);//按下ESC結束
	if( c == 27 || c == 'q' || c == 'Q' )
	{
		SetEvent(m_hEvNuiProcessStop);
		//knData.m_hEvNuiProcessStop = m_hEvNuiProcessStop;
	}
	return 0;
}


void KinectModel::DataThread()//kinect 資料更新擷取
{
	HANDLE hEvents[4] = {m_hEvNuiProcessStop,m_hNextColorFrameEvent,
		m_hNextDepthFrameEvent,m_hNextSkeletonEvent};//內核事件

	while(1)
	{
		int nEventIdx;
		nEventIdx=WaitForMultipleObjects(sizeof(hEvents)/sizeof(hEvents[0]),
			hEvents,FALSE,100);
		if (WAIT_OBJECT_0 == WaitForSingleObject(m_hEvNuiProcessStop, 0))
		{
			break;
		}
		// Process signal events
		if (WAIT_OBJECT_0 == WaitForSingleObject(m_hNextColorFrameEvent, 0))
		{
			DrawColor(m_pColorStreamHandle);//獲取彩色圖像並進行顯示
		}
		if (WAIT_OBJECT_0 == WaitForSingleObject(m_hNextDepthFrameEvent, 0))
		{
			DrawDepth(m_pDepthStreamHandle); //獲得深度資訊並顯示
		}
		if (WAIT_OBJECT_0 == WaitForSingleObject(m_hNextSkeletonEvent, 0))
		{
			DrawSkeleton(); //獲得骨架資訊及使用者ID並顯示
		}

	}
	CloseHandle(m_hEvNuiProcessStop);
	m_hEvNuiProcessStop = NULL;
	CloseHandle( m_hNextSkeletonEvent );
	CloseHandle( m_hNextDepthFrameEvent );
	CloseHandle( m_hNextColorFrameEvent );
	//return 0;
}



//kinect設備初始化
int KinectModel::initial() 
{
	audioinitial();  //microphone 初始化

	if( hr != S_OK )//Kinect提供了兩種處理返回值的方式，就是判斷上面的函數是否執行成功。
	{
		cout<<"NuiInitialize failed"<<endl;
		return hr;
	}
	//1、Color ----打開KINECT設備的彩色圖資訊通道
	m_hNextColorFrameEvent	= CreateEvent( NULL, TRUE, FALSE, NULL );//創建一個windows事件物件，創建成功則返回事件的控制碼
	m_pDepthStreamHandle = NULL;//保存圖像資料流的控制碼，用以提取資料
	//打開KINECT設備的彩色圖資訊通道，並用m_pColorStreamHandle保存該流的控制碼，以便於以後讀取
	hr = NuiImageStreamOpen(NUI_IMAGE_TYPE_COLOR,NUI_IMAGE_RESOLUTION_640x480, 0, 2, m_hNextColorFrameEvent, &m_pColorStreamHandle);
	if( FAILED( hr ) )
	{
		cout<<"Could not open image stream video"<<endl;
		return hr;
	}
	//2、Depth -----打開Kinect設備的深度圖資訊通道
	m_hNextDepthFrameEvent	= CreateEvent( NULL, TRUE, FALSE, NULL );
	m_pDepthStreamHandle	= NULL;//保存深度資料流程的控制碼，用以提取資料
	//打開KINECT設備的深度圖資訊通道，並用m_pDepthStreamHandle保存該流的控制碼，以便於以後讀取
	hr = NuiImageStreamOpen( NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, NUI_IMAGE_RESOLUTION_320x240, 0, 2, m_hNextDepthFrameEvent, &m_pDepthStreamHandle);
	if( FAILED( hr ) )
	{
		cout<<"Could not open depth stream video"<<endl;
		return hr;
	}
	//3、Skeleton -----定義骨骼信號事件控制碼，打開骨骼跟蹤事件
	m_hNextSkeletonEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	//hr = NuiSkeletonTrackingEnable( m_hNextSkeletonEvent, NUI_SKELETON_TRACKING_FLAG_ENABLE_IN_NEAR_RANGE|NUI_SKELETON_TRACKING_FLAG_ENABLE_SEATED_SUPPORT);
	hr = NuiSkeletonTrackingEnable(m_hNextSkeletonEvent, 0);
	
	if( FAILED( hr ) )
	{
		cout<<"Could not open skeleton stream video"<<endl;
		//knData.hr = hr;
		return hr;
	}

	//4、用於結束的事件物件
	m_hEvNuiProcessStop = CreateEvent(NULL,TRUE,FALSE,NULL);
	//knData.m_hEvNuiProcessStop = CreateEvent(NULL, TRUE, FALSE, NULL);

	//5、開啟一個執行緒---用於讀取彩色、深度、骨骼資料,該執行緒用於調用執行緒函數KinectDataThread，執行緒函數對深度讀取彩色和深度圖像並進行骨骼跟蹤，同時進行顯示
	//m_hProcesss = CreateThread(NULL, 0, this->KinectDataThread, 0, 0, 0);
	
	////////////////////////////////////////////////////////////////////////
	m_hint3D[0] = FT_VECTOR3D(0, 0, 0);//頭中心座標，初始化
	m_hint3D[1] = FT_VECTOR3D(0, 0, 0);//肩膀的中心座標
	pColorFrame = FTCreateImage();//彩色圖像資料，資料類型為IFTImage*
	pDepthFrame = FTCreateImage();//深度圖像資料

}


void KinectModel::audioinitial() // 參數初始化
{
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	m_pNuiSensor = NULL;
	m_pNuiAudioSource = NULL;
	m_pDMO = NULL;
	m_pPropertyStore = NULL;

	hr = CreateFirstConnected(); //設備連接
}

// SpeechModel初始化
void SpeechModel::initial(const int type)  //input: 1:nn, 2:rnn
{
	//learning model初始化
	output_nn = MatrixXd::Zero(1, 6);
	vector<MatrixXd> weight;
	vector<MatrixXd> bias;
	weight.push_back(MatrixXd());
	weight.push_back(MatrixXd());
	bias.push_back(MatrixXd());
	bias.push_back(MatrixXd());

	// learning model type NN or RNN, weight讀檔
	if (type == 1)
	{
		nntype = 1;  //NN

		read_binary("parameter/face/NN/67.37/W0", weight[0]);
		read_binary("parameter/face/NN/67.37/W1", weight[1]);
		read_binary("parameter/face/NN/67.37/b0", bias[0]);
		read_binary("parameter/face/NN/67.37/b1", bias[1]);
		read_binary("parameter/face/NN/67.37/ave", ave);
		read_binary("parameter/face/NN/67.37/std", Std);
		read_binary("parameter/face/NN/67.37/fscore", fscore);

		net = nn::NeuralNetwork(weight, bias);
	}
	else if (type == 2)
	{
		nntype = 2;   //RNN
		MatrixXd weight_memory;

		dp::loadCSV("parameter/speech/RNN/Wi.csv", weight[0]);
		dp::loadCSV("parameter/speech/RNN/Wo.csv", weight[1]);
		dp::loadCSV("parameter/speech/RNN/bh.csv", bias[0]);
		dp::loadCSV("parameter/speech/RNN/bo.csv", bias[1]);
		dp::loadCSV("parameter/speech/RNN/Wh.csv", weight_memory);
		dp::loadCSV("parameter/speech/RNN/mean.csv", ave);
		dp::loadCSV("parameter/speech/RNN/std.csv", Std);
		dp::loadCSV("parameter/speech/RNN/fscore.csv", fscore);

		rnet = rnn::RecurrentNN(weight, bias, weight_memory);
	}

	net = nn::NeuralNetwork(weight, bias); //建立learning model

	//參數初始化
	size_t storage_size = sizeof(XDSP::XVECTOR) * wBinsForFFT / 4;
	// Note, the XVECTORS need to be aligned on 32 bit boundaries, so we need to use _aligned_malloc
	m_pxvBinsFFTRealStorage = (XDSP::XVECTOR*)_aligned_malloc(storage_size, 32);
	m_pxvBinsFFTImaginaryStorage = (XDSP::XVECTOR*)_aligned_malloc(storage_size, 32);
	m_pxvUnityTable = (XDSP::XVECTOR*)_aligned_malloc(sizeof(XDSP::XVECTOR) * wBinsForFFT, 32);

	m_pfltBinsFFTReal = (float *)m_pxvBinsFFTRealStorage;
	m_pfltBinsFFTImaginary = (float *)m_pxvBinsFFTImaginaryStorage;

	ZeroMemory(m_fltBinsFFTDisplay, sizeof(m_fltBinsFFTDisplay));
	ZeroMemory(m_rgfltAudioInputForFFT, sizeof(m_rgfltAudioInputForFFT));
	ZeroMemory(m_pfltBinsFFTReal, storage_size);
	ZeroMemory(m_pfltBinsFFTImaginary, storage_size);

	XDSP::FFTInitializeUnityTable((FLOAT32*)m_pxvUnityTable, wBinsForFFT);

	InitializeFFTWindow(m_rgfltWindow, wBinsForFFT, HANN);  //FFT window


	// MFCC BANK //
	mfcc_filter[0] = 9;
	mfcc_filter[1] = 12;
	mfcc_filter[2] = 15;
	mfcc_filter[3] = 18;
	mfcc_filter[4] = 21;
	mfcc_filter[5] = 25;
	mfcc_filter[6] = 29;
	mfcc_filter[7] = 33;
	mfcc_filter[8] = 38;
	mfcc_filter[9] = 43;
	mfcc_filter[10] = 49;
	mfcc_filter[11] = 54;
	mfcc_filter[12] = 61;
	mfcc_filter[13] = 68;
	mfcc_filter[14] = 75;
	mfcc_filter[15] = 84;
	mfcc_filter[16] = 93;
	mfcc_filter[17] = 102;
	mfcc_filter[18] = 113;
	mfcc_filter[19] = 124;
	mfcc_filter[20] = 136;
	mfcc_filter[21] = 150;
	mfcc_filter[22] = 164;
	mfcc_filter[23] = 180;
	mfcc_filter[24] = 196;
	mfcc_filter[25] = 215;
	mfcc_filter[26] = 235;
	mfcc_filter[27] = 256;
}

// SpeechModel RNN output
void SpeechModel::RNNinference()
{
	static MatrixXd y;
	if (feature_nn.size()==40) //window size
	{
		for (int i = 0; i<feature_nn.size(); ++i)
			feature_nn[i] = ((feature_nn[i] - ave.transpose()).array() / Std.transpose().array()).matrix(); //normalize

		output_nn = rnet.predict(feature_nn);  //predict

		dp::vec2class(output_nn, y);

		feature_nn.erase(feature_nn.begin());
	}
}

// SpeechModel NN output
void SpeechModel::NNinference()
{
	static MatrixXd y;
	MatrixXd norFeature;
	if (feature_nn.size()>0)
	{
		for (int i = 0; i < feature_nn.size(); ++i)
		{
			norFeature = ((feature_nn[i] - ave).array() / Std.array()).matrix(); //normalize
			output_nn = net.predict(norFeature); //predict
		}
		dp::vec2class(output_nn, y);
		//cout << "speech:" << y << endl;
		feature_nn.erase(feature_nn.begin());
	}

}

//SpeechModel feature extraction
void SpeechModel::getFeature(BYTE * pProduced, DWORD cbProduced)
{
	//參數設定
	static const float Invert = 1 / (float)MAXSHORT;
	static const float Decay = 0.7f;
	float m_fltAccumulatedSquareSum = 0;
	const float cEnergyNoiseFloor = 0.2f;
	double energy_max = 0;
	double energy_min = 99999;
	int zerocrossnumber = 0;
	float tempAudioinput[wBinsForFFT / 2] = { 0 };

	// Calculate FFT from audio
	for (UINT i = 0; i < cbProduced; i += 2)
	{
		short audioSample = static_cast<short>(pProduced[i] | (pProduced[i + 1] << 8));

		m_fltAccumulatedSquareSum += audioSample * audioSample;
		if (audioSample*audioSample>energy_max) energy_max = audioSample*audioSample;
		//if ((audioSample*audioSample<energy_min) && audioSample*audioSample>0) energy_min = audioSample*audioSample;
		if (audioSample == 0) zerocrossnumber++;

		float audioSampleFloat = Invert * audioSample;

		m_rgfltAudioInputForFFT[m_iAccumulatedSampleCount] = audioSampleFloat;

		if (m_iAccumulatedSampleCount >= 256)   //紀錄後256個sample overlap用
			tempAudioinput[m_iAccumulatedSampleCount - 256] = audioSampleFloat;

		m_iAccumulatedSampleCount++;

		if (m_iAccumulatedSampleCount < wBinsForFFT)   //累積512個smaple
		{
			continue;
		}


		// Each energy value will represent the logarithm of the mean of the
		// sum of squares of a group of audio samples.
		float meanSquare = m_fltAccumulatedSquareSum / wBinsForFFT;
		float amplitude = log(meanSquare) / log(static_cast<float>(INT_MAX));
		//energy_max = log(energy_max) / log(static_cast<float>(INT_MAX));
		//energy_min = log(energy_min) / log(static_cast<float>(INT_MAX));

		// Truncate portion of signal below noise floor
		float amplitudeAboveNoise = max(0.0f, amplitude - cEnergyNoiseFloor);
		// Renormalize signal above noise floor to [0,1] range.
		float EnergyBuffer = amplitudeAboveNoise / (1 - cEnergyNoiseFloor);

		// At this point we have enough samples to do our FFT
		// First, copy the samples across to the XVector storage
		for (UINT iSample = 0; iSample < wBinsForFFT; ++iSample)
		{
			m_pfltBinsFFTReal[iSample] = m_rgfltAudioInputForFFT[iSample] * m_rgfltWindow[iSample];
		}

		int lb2FFT = 0;
		int temp = wBinsForFFT;
		while (0 == (temp & 1))
		{
			++lb2FFT;
			temp = temp >> 1;
		}
		// Pass off to the FFT library to do the heavy lifting.  Before this call, the m_pfltBinsFFTReal array holds the time domain signal
		//  and the m_pfltBinsFFTImaginary array is initialized to 0
		// After this call, m_pfltBinsFFTReal will contain the real components of the frequency domain data, and m_pfltBinsFFTImaginary
		//  will hold the imaginary components
		// NOTE:  There is a newer FFT library from the DirectX team that does the FFT on the GPU. 
		// See: http://msdn.microsoft.com/en-us/library/windows/desktop/ff476274(v=vs.85).aspx
		XDSP::FFT((XDSP::XVECTOR *)m_pfltBinsFFTReal, (XDSP::XVECTOR *)m_pfltBinsFFTImaginary, m_pxvUnityTable, wBinsForFFT);
		XDSP::FFTUnswizzle(m_rgfltAudioInputForFFT, m_pfltBinsFFTReal, lb2FFT);
		memcpy_s(m_pfltBinsFFTReal, sizeof(XDSP::XVECTOR) * wBinsForFFT / 4, m_rgfltAudioInputForFFT, sizeof(float)* wBinsForFFT);
		XDSP::FFTUnswizzle(m_rgfltAudioInputForFFT, m_pfltBinsFFTImaginary, lb2FFT);
		memcpy_s(m_pfltBinsFFTImaginary, sizeof(XDSP::XVECTOR) * wBinsForFFT / 4, m_rgfltAudioInputForFFT, sizeof(float)* wBinsForFFT);

		float fre_max = 0;
		float f0 = 0;
		float fre_temp = 0;
		bool first = true;

		for (UINT iBin = 0; iBin < wBinsForFFT / 2; ++iBin)
		{
			float imaginary = m_pfltBinsFFTImaginary[iBin];
			float real = m_pfltBinsFFTReal[iBin];

			// Calculating the magnitude requires considering both the real and imaginary components.
			// We could convert to log scale, but this works well without it.
			float magnitude = sqrt(imaginary * imaginary + real * real);

			// This next operation will smooth out the results a little and prevent the display from jumping around wildly.
			// You can play with this by changing the "Decay" parameter above.
			float decayedOldValue = m_fltBinsFFTDisplay[iBin] * Decay;
			m_fltBinsFFTDisplay[iBin] = max(magnitude, decayedOldValue);


			if (m_fltBinsFFTDisplay[iBin] > fre_temp)  //數值最大的頻率
			{
				fre_temp = m_fltBinsFFTDisplay[iBin];
				fre_max = iBin;
			}

			if (m_fltBinsFFTDisplay[iBin] > 0.001)  //基頻
			{
				if (first)
				{
					f0 = iBin;
					first = false;
				}
			}

		}

		MFCC(m_fltBinsFFTDisplay);    //MFCC

		// feature extraction
		MatrixXd feature(1,21);
		vector<double> tempFeature;
		//cout << "confidence: " << sourceConfidence << endl;
		tempFeature.push_back(EnergyBuffer);
		tempFeature.push_back(energy_max);
		tempFeature.push_back(zerocrossnumber );
		tempFeature.push_back(180.0 * beamAngle / M_PI);
		tempFeature.push_back(180 * sourceAngle / M_PI);
		tempFeature.push_back((float)sourceConfidence);
		tempFeature.push_back(f0);
		tempFeature.push_back(fre_max);

			for (int i = 0; i < 13; i++){
				tempFeature.push_back(mfcc_coef[i] );
			}

			for (int i = 0; i < tempFeature.size(); ++i)
			{
				feature(0,i) = tempFeature[i];
			}
		
			//cout << "audio"<<aa++ << endl<<endl;
			feature_nn.push_back(feature);

			//依據 learning model進行推論
			switch (nntype)
			{
			case NN:
				NNinference();
				break;
			case RNN:
				RNNinference();
				break;
			default:
				break;
			}

		// We're all done with our FFT so we'll clean up and get ready for next time.
		m_iAccumulatedSampleCount = 256;
		memset(m_rgfltAudioInputForFFT, 0, sizeof(float)* wBinsForFFT);
		memset(m_pfltBinsFFTReal, 0, sizeof(float)* wBinsForFFT);
		memset(m_pfltBinsFFTImaginary, 0, sizeof(float)* wBinsForFFT);
		m_fltAccumulatedSquareSum = 0;
		energy_max = 0;
		energy_min = 99999;
		zerocrossnumber = 0;

		for (int i = 0; i < 256; i++) //overlap
			m_rgfltAudioInputForFFT[i] = tempAudioinput[i];

	}
}


//kinect microphone array connect
HRESULT KinectModel::CreateFirstConnected() 
{
	INuiSensor * pNuiSensor;
	
	int iSensorCount = 0;
	hr = NuiGetSensorCount(&iSensorCount);
	if (FAILED(hr))
	{
		cout << "Failed to enumerate sensors!" << endl;
		return hr;
	}

	// Look at each Kinect sensor
	for (int i = 0; i < iSensorCount; ++i)
	{
		// Create the sensor so we can check status, if we can't create it, move on to the next
		hr = NuiCreateSensorByIndex(i, &pNuiSensor);
		if (FAILED(hr))
		{
			continue;
			cout << "1" << endl;
		}

		// Get the status of the sensor, and if connected, then we can initialize it
		hr = pNuiSensor->NuiStatus();
		if (S_OK == hr)
		{
			m_pNuiSensor = pNuiSensor;
			break;
			cout << "2" << endl;
		}

		// This sensor wasn't OK, so release it since we're not using it
		pNuiSensor->Release();
	}

	if (NULL != m_pNuiSensor)
	{
		// Initialize the Kinect and specify that we'll be using audio signal
		hr = m_pNuiSensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX | NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_SKELETON | NUI_INITIALIZE_FLAG_USES_AUDIO);
		if (SUCCEEDED(hr))
		{
			hr = InitializeAudioSource(); //連接到進行初始化
			cout << "3" << endl;
		}
	}
	else
	{
		cout << "No ready Kinect found!" << endl;
		hr = E_FAIL;
	}

	return hr;
}


//kinect microphone array initial
HRESULT KinectModel::InitializeAudioSource()
{
	// Get the audio source
	HRESULT hr = m_pNuiSensor->NuiGetAudioSource(&m_pNuiAudioSource);
	if (FAILED(hr))
	{
		cout << "Failed to get Audio Source!" << endl;
		return hr;
	}

	hr = m_pNuiAudioSource->QueryInterface(IID_IMediaObject, (void**)&m_pDMO);
	if (FAILED(hr))
	{
		cout << "Failed to access the DMO!";
		return hr;
	}

	hr = m_pNuiAudioSource->QueryInterface(IID_IPropertyStore, (void**)&m_pPropertyStore);
	if (FAILED(hr))
	{
		cout << "Failed to access the Audio Property store!";
		return hr;
	}

	// Set AEC-MicArray DMO system mode.
	// This must be set for the DMO to work properly
	PROPVARIANT pvSysMode;
	PropVariantInit(&pvSysMode);
	pvSysMode.vt = VT_I4;
	//   SINGLE_CHANNEL_AEC = 0
	//   OPTIBEAM_ARRAY_ONLY = 2
	//   OPTIBEAM_ARRAY_AND_AEC = 4
	//   SINGLE_CHANNEL_NSAGC = 5
	pvSysMode.lVal = (LONG)(4);
	m_pPropertyStore->SetValue(MFPKEY_WMAAECMA_SYSTEM_MODE, pvSysMode);
	PropVariantClear(&pvSysMode);

	// echo
	//PROPVARIANT pvSysMode;
	PropVariantInit(&pvSysMode);
	pvSysMode.vt = VT_I4;
	// index is 0, 1 We need 0,2
	pvSysMode.lVal = 2;
	m_pPropertyStore->SetValue(MFPKEY_WMAAECMA_FEATR_AES, pvSysMode);
	PropVariantClear(&pvSysMode);


	// Set DMO output format
	WAVEFORMATEX fmt = { AudioFormat, AudioChannels, AudioSamplesPerSecond, AudioAverageBytesPerSecond, AudioBlockAlign, AudioBitsPerSample, 0 };
	memcpy_s(&m_wfxOut, sizeof(WAVEFORMATEX), &fmt, sizeof(WAVEFORMATEX));
	DMO_MEDIA_TYPE mt = { 0 };
	hr = MoInitMediaType(&mt, sizeof(WAVEFORMATEX));
	if (FAILED(hr))
	{
		return hr;
	}
	mt.majortype = MEDIATYPE_Audio;
	mt.subtype = MEDIASUBTYPE_PCM;
	mt.lSampleSize = 0;
	mt.bFixedSizeSamples = TRUE;
	mt.bTemporalCompression = FALSE;
	mt.formattype = FORMAT_WaveFormatEx;
	memcpy_s(mt.pbFormat, sizeof(WAVEFORMATEX), &m_wfxOut, sizeof(WAVEFORMATEX));

	hr = m_pDMO->SetOutputType(0, &mt, 0);
	MoFreeMediaType(&mt);

	//knData.m_pNuiAudioSource = m_pNuiAudioSource;
	//knData.m_pDMO = m_pDMO;
	return hr;
}

// MFCC (input float[])
void SpeechModel::MFCC(float FFTbins[])
{
	for (int i = 0; i < 26; i++)
	{
		float filterenergy = 0;

		int length1 = mfcc_filter[i + 1] - mfcc_filter[i];
		int length2 = mfcc_filter[i + 2] - mfcc_filter[i + 1];
		//cout << length1 << " " << length2 << " ";

		for (int j = mfcc_filter[i]; j <= mfcc_filter[i + 2]; j++)
		{
			if (j <= mfcc_filter[i + 1])
				filterenergy += FFTbins[j] * (j - mfcc_filter[i]) / length1;
			else
				filterenergy += FFTbins[j] * (1 - ((j - mfcc_filter[i + 1]) / length2));

		}

		bank_energy.push_back(log(filterenergy));
		//cout << bank_energy[bank_energy.size() - 1] << " ";
	}
	dct(bank_energy, mfcc_coef, 0); //DCT

	bank_energy.clear();

}


// SpeechModel emotion recognition
void SpeechModel::recognition()
{
	if (knModel.hr == S_OK)
	{
			// Bottom portion of computed energy signal that will be discarded as noise.
			// Only portion of signal above noise floor will be displayed.
			
			ULONG cbProduced = 0;
			BYTE *pProduced = NULL;
			DWORD dwStatus = 0;
			DMO_OUTPUT_DATA_BUFFER outputBuffer = { 0 };
			outputBuffer.pBuffer = &knModel.m_captureBuffer;

			HRESULT hr = S_OK;

			do
			{
				knModel.m_captureBuffer.Init(0);
				outputBuffer.dwStatus = 0;
				hr = knModel.m_pDMO->ProcessOutput(0, 1, &outputBuffer, &dwStatus);
				if (FAILED(hr))
				{
					cout << "Failed to process audio output." << endl;
					break;
				}

				if (S_FALSE == hr)
				{
					cbProduced = 0;
				}
				else
				{
					knModel.m_captureBuffer.GetBufferAndLength(&pProduced, &cbProduced);
				}
				allsample += cbProduced;

				/*cout << "all sample: " << allsample << endl;
				cout << "capture sample number: " << cbProduced << endl;*/

				if (cbProduced > 0)
				{
				
					// Obtain beam angle from INuiAudioBeam afforded by microphone array
					knModel.m_pNuiAudioSource->GetBeam(&beamAngle);
					knModel.m_pNuiAudioSource->GetPosition(&sourceAngle, &sourceConfidence);

					// Pass off data to each visualizer to process
					// Note, these are super fast, so no need to only pass to one
					getFeature(pProduced, cbProduced);  //feature extraction

				}

			} while (outputBuffer.dwStatus & DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE);

			cv::waitKey(10);
		
	}
}


//BodyModel 初始化
void BodyModel::initial(const int type) //input: 1:nn, 2:rnn
{ 
	
	//參數初始化
	 user_index = -1;
	 framecount = 1;

	 // learning model 初始化
	 output_nn = MatrixXd::Zero(1,6);
	 vector<MatrixXd> weight;
	 vector<MatrixXd> bias;
	 weight.push_back(MatrixXd());
	 weight.push_back(MatrixXd());
	 bias.push_back(MatrixXd());
	 bias.push_back(MatrixXd());

	 // learning model type NN or RNN, weight讀檔
	 if (type == 1)
	 {
		 nntype = 1;   //NN
		 opl;
		 read_binary("parameter/body/NN/85.7/W0", weight[0]);
		 read_binary("parameter/body/NN/85.7/W1", weight[1]);
		 read_binary("parameter/body/NN/85.7/b0", bias[0]);
		 read_binary("parameter/body/NN/85.7/b1", bias[1]);
		 read_binary("parameter/body/NN/85.7/ave", ave);
		 read_binary("parameter/body/NN/85.7/std", Std);
		 read_binary("parameter/body/NN/85.7/fscore", fscore);

		 net = nn::NeuralNetwork(weight, bias);
	 }

	 else if (type == 2)
	 {
		 nntype = 2;  //RNN
		 MatrixXd weight_memory;

		 dp::loadCSV("parameter/body/RNN/Wi.csv", weight[0]);
		 dp::loadCSV("parameter/body/RNN/Wo.csv", weight[1]);
		 dp::loadCSV("parameter/body/RNN/bh.csv", bias[0]);
		 dp::loadCSV("parameter/body/RNN/bo.csv", bias[1]);
		 dp::loadCSV("parameter/body/RNN/Wh.csv", weight_memory);
		 dp::loadCSV("parameter/body/RNN/mean.csv", ave);
		 dp::loadCSV("parameter/body/RNN/std.csv", Std);
		 dp::loadCSV("parameter/body/RNN/fscore.csv", fscore);

		 rnet = rnn::RecurrentNN(weight, bias, weight_memory);
	 }


	head.clear();
	hand_l.clear();
	hand_r.clear();
	shoulder_l.clear();
	shoulder_r.clear();
	shoulder_c.clear();
	hip_c.clear();
	foot_l.clear();
	foot_r.clear();
	v_l_hand.clear();
	v_r_hand.clear();
	v_hip_c.clear();

}

// BodyModel position feature
void BodyModel::getpos(const int i)  //input: ID
{
	Data temp;
	Data dis_temp1;
	Data dis_temp2;
	Data area_temp;

	/*const NUI_SKELETON_DATA & skeleton = SkeletonFrame.SkeletonData[i];

	NUI_SKELETON_BONE_ORIENTATION boneOrientations[NUI_SKELETON_POSITION_COUNT];
	NuiSkeletonCalculateBoneOrientations(&skeleton, boneOrientations);

	head_orientation = boneOrientations[NUI_SKELETON_POSITION_HEAD].absoluteRotation.rotationQuaternion;
	hip_orientation = boneOrientations[NUI_SKELETON_POSITION_HIP_CENTER].absoluteRotation.rotationQuaternion;
	cout << "w:" << head_orientation.w << "  x:" << head_orientation.x << " y:" << head_orientation.y << " z:" << head_orientation.z<<endl;*/

	NUI_SKELETON_FRAME SkeletonFrame = knModel.SkeletonFrame;

	//head coordinate data
	if (SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HEAD].x != 0 && SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HEAD].y != 0 && SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HEAD].z != 0){
		temp.x = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HEAD].x;
		temp.y = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HEAD].y;
		temp.z = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HEAD].z;
		head.push_back(temp);
		//cout<<head_x[k]<<"   "<<head_y[k]<<"   "<<k<<endl;
		//cout<<k<<endl;
	}
	//left hand coordinate data
	if (SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HAND_LEFT].x != 0 && SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HAND_LEFT].y != 0 && SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HAND_LEFT].z != 0){
		temp.x = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HAND_LEFT].x;
		temp.y = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HAND_LEFT].y;
		temp.z = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HAND_LEFT].z;
		hand_l.push_back(temp);
		//cout<<k<<endl;
	}
	//right hand coordinate data
	if (SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT].x != 0 && SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT].y != 0 && SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT].z != 0){
		temp.x = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT].x;
		temp.y = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT].y;
		temp.z = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT].z;
		hand_r.push_back(temp);
		//cout<<"hand x: "<<hand_r[hand_r.size()-1].x<<endl;
	}
	//center shoulder coordinate data
	if (SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER].x != 0 && SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER].y != 0 && SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER].z != 0){
		temp.x = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER].x;
		temp.y = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER].y;
		temp.z = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER].z;
		shoulder_c.push_back(temp);

	}
	//left shoulder coordinate data
	if (SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_LEFT].x != 0 && SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_LEFT].y != 0 && SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_LEFT].z != 0){
		temp.x = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_LEFT].x;
		temp.y = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_LEFT].y;
		temp.z = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_LEFT].z;
		shoulder_l.push_back(temp);
	}
	//right shoulder coordinate data
	if (SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_RIGHT].x != 0 && SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_RIGHT].y != 0 && SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_RIGHT].z != 0){
		temp.x = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_RIGHT].x;
		temp.y = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_RIGHT].y;
		temp.z = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_RIGHT].z;
		shoulder_r.push_back(temp);
	}
	//hip center coordinate data
	if (SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HIP_CENTER].x != 0 && SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HIP_CENTER].y != 0 && SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HIP_CENTER].z != 0){
		temp.x = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HIP_CENTER].x;
		temp.y = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HIP_CENTER].y;
		temp.z = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_HIP_CENTER].z;
		hip_c.push_back(temp);
		//cout << hip_c_x[k] << "   " << hip_c_y[k] << "   " << k << endl;
	}

	//left foot coordinate data
	if (SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_FOOT_LEFT].x != 0 && SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_FOOT_LEFT].y != 0 && SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_FOOT_LEFT].z != 0){
		temp.x = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_FOOT_LEFT].x;
		temp.y = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_FOOT_LEFT].y;
		temp.z = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_FOOT_LEFT].z;
		foot_l.push_back(temp);
		//cout<<head_x[k]<<"   "<<head_y[k]<<"   "<<k<<endl;
		//cout<<k<<endl;
	}

	//Right foot coordinate data
	if (SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_FOOT_RIGHT].x != 0 && SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_FOOT_RIGHT].y != 0 && SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_FOOT_RIGHT].z != 0){
		temp.x = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_FOOT_RIGHT].x;
		temp.y = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_FOOT_RIGHT].y;
		temp.z = SkeletonFrame.SkeletonData[i].SkeletonPositions[NUI_SKELETON_POSITION_FOOT_RIGHT].z;
		foot_r.push_back(temp);
		//cout<<head_x[k]<<"   "<<head_y[k]<<"   "<<k<<endl;
		//cout<<k<<endl;
	}


	//distance between left hand and hip center
	dis_temp1.x = hip_c[hip_c.size() - 1].x - hand_l[hand_l.size() - 1].x;
	dis_temp1.y = hip_c[hip_c.size() - 1].y - hand_l[hand_l.size() - 1].y;
	dis_temp1.z = hip_c[hip_c.size() - 1].z - hand_l[hand_l.size() - 1].z;
	dis_hl_hc = (sqrt((dis_temp1.x*dis_temp1.x) + (dis_temp1.y*dis_temp1.y) + (dis_temp1.z*dis_temp1.z)));

	//cout << "dis_hl_hc_x: " << dis_hl_hc_x[k] << endl;

	//distance between right hand and hip center
	dis_temp2.x = hip_c[hip_c.size() - 1].x - hand_r[hand_r.size() - 1].x;
	dis_temp2.y = hip_c[hip_c.size() - 1].y - hand_r[hand_r.size() - 1].y;
	dis_temp2.z = hip_c[hip_c.size() - 1].z - hand_r[hand_r.size() - 1].z;
	dis_hr_hc = (sqrt((dis_temp2.x*dis_temp2.x) + (dis_temp2.y*dis_temp2.y) + (dis_temp2.z*dis_temp2.z)));
	//cout << "hand to hip: " << dis_hr_hc << endl;

	//area of two hands and hip_c
	//cross product of two vector
	area_temp.x = (dis_temp1.y*dis_temp2.z) - (dis_temp2.y*dis_temp1.z);
	area_temp.y = -1 * ((dis_temp1.x* dis_temp2.z) - (dis_temp2.x* dis_temp1.z));
	area_temp.z = (dis_temp1.x* dis_temp2.y) - (dis_temp2.x* dis_temp1.y);
	//note the area
	area_hipc_2h = (sqrt((area_temp.x*area_temp.x) + (area_temp.y*area_temp.y) + (area_temp.z*area_temp.z))*0.5);

	//distance between shoulder center and hip center
	temp.x = hip_c[hip_c.size() - 1].x - shoulder_c[shoulder_c.size() - 1].x;
	temp.y = hip_c[hip_c.size() - 1].y - shoulder_c[shoulder_c.size() - 1].y;
	temp.z = hip_c[hip_c.size() - 1].z - shoulder_c[shoulder_c.size() - 1].z;
	dis_sc_hc = (sqrt((temp.x*temp.x) + (temp.y*temp.y) + (temp.z*temp.z)));

	//the distance of two shoulders at y-axis(whether it's horizontal)
	delta_shoulder_y = (abs(shoulder_r[shoulder_r.size() - 1].y - shoulder_l[shoulder_l.size() - 1].y));

	//distance between two hands;
	temp.x = hand_r[hand_r.size() - 1].x - hand_l[hand_l.size() - 1].x;
	temp.y = hand_r[hand_r.size() - 1].y - hand_l[hand_l.size() - 1].y;
	temp.z = hand_r[hand_r.size() - 1].z - hand_l[hand_l.size() - 1].z;
	dis_hl_hr = (sqrt((temp.x*temp.x) + (temp.y*temp.y) + (temp.z*temp.z)));

	//distance between head and left hand
	dis_temp1.x = hand_l[hand_l.size() - 1].x - head[head.size() - 1].x;
	dis_temp1.y = hand_l[hand_l.size() - 1].y - head[head.size() - 1].y;
	dis_temp1.z = hand_l[hand_l.size() - 1].z - head[head.size() - 1].z;
	dis_h_hl = (sqrt((dis_temp1.x*dis_temp1.x) + (dis_temp1.y*dis_temp1.y) + (dis_temp1.z*dis_temp1.z)));

	//distance between head and right hand
	dis_temp2.x = hand_r[hand_r.size() - 1].x - head[head.size() - 1].x;
	dis_temp2.y = hand_r[hand_r.size() - 1].y - head[head.size() - 1].y;
	dis_temp2.z = hand_r[hand_r.size() - 1].z - head[head.size() - 1].z;
	dis_h_hr = (sqrt((dis_temp2.x*dis_temp2.x) + (dis_temp2.y*dis_temp2.y) + (dis_temp2.z*dis_temp2.z)));


	//area of head and two hands
	//cross product h_hl and h_hr
	area_temp.x = (dis_temp2.y*dis_temp1.z) - (dis_temp1.y*dis_temp2.z);
	area_temp.y = -1 * (dis_temp2.x*dis_temp1.z) - (dis_temp1.x*dis_temp2.z);
	area_temp.z = (dis_temp2.x*dis_temp1.y) - (dis_temp1.x*dis_temp2.y);
	//area
	area_h_2h = (sqrt((area_temp.x*area_temp.x) + (area_temp.y*area_temp.y) + (area_temp.z*area_temp.z))*0.5);



	//distance between left foot and hip center
	dis_temp1.x = hip_c[hip_c.size() - 1].x - foot_l[foot_l.size() - 1].x;
	dis_temp1.y = hip_c[hip_c.size() - 1].y - foot_l[foot_l.size() - 1].y;
	dis_temp1.z = hip_c[hip_c.size() - 1].z - foot_l[foot_l.size() - 1].z;
	dis_hl_hc = (sqrt((dis_temp1.x*dis_temp1.x) + (dis_temp1.y*dis_temp1.y) + (dis_temp1.z*dis_temp1.z)));

	//cout << "dis_hl_hc_x: " << dis_hl_hc_x[k] << endl;

	//distance between right foot and hip center
	dis_temp2.x = hip_c[hip_c.size() - 1].x - foot_r[foot_r.size() - 1].x;
	dis_temp2.y = hip_c[hip_c.size() - 1].y - foot_r[foot_r.size() - 1].y;
	dis_temp2.z = hip_c[hip_c.size() - 1].z - foot_r[foot_r.size() - 1].z;
	dis_hr_hc = (sqrt((dis_temp2.x*dis_temp2.x) + (dis_temp2.y*dis_temp2.y) + (dis_temp2.z*dis_temp2.z)));
	//cout << "hand to hip: " << dis_hr_hc << endl;

	//area of two foot and hip_c
	//cross product of two vector
	area_temp.x = (dis_temp1.y*dis_temp2.z) - (dis_temp2.y*dis_temp1.z);
	area_temp.y = -1 * ((dis_temp1.x* dis_temp2.z) - (dis_temp2.x* dis_temp1.z));
	area_temp.z = (dis_temp1.x* dis_temp2.y) - (dis_temp2.x* dis_temp1.y);
	//note the area
	area_hip_2f = (sqrt((area_temp.x*area_temp.x) + (area_temp.y*area_temp.y) + (area_temp.z*area_temp.z))*0.5);


	temp.x = foot_r[foot_r.size() - 1].x - foot_l[foot_l.size() - 1].x;
	temp.y = foot_r[foot_r.size() - 1].y - foot_l[foot_l.size() - 1].y;
	temp.z = foot_r[foot_r.size() - 1].z - foot_l[foot_l.size() - 1].z;
	dis_fl_fr = (sqrt((temp.x*temp.x) + (temp.y*temp.y) + (temp.z*temp.z)));

	//cout << "  pos" << endl;

}

// BodyModel velocity feature
void BodyModel::getvel()
{
	Data temp;

	//velocity of l_hand;
	temp.x = hand_l[hand_l.size() - 1].x - hand_l[hand_l.size() - 2].x;
	temp.y = hand_l[hand_l.size() - 1].y - hand_l[hand_l.size() - 2].y;
	temp.z = hand_l[hand_l.size() - 1].z - hand_l[hand_l.size() - 2].z;
	v_l_hand.push_back(sqrt((temp.x*temp.x) + (temp.y*temp.y) + (temp.z*temp.z)));

	//velocity of r_hand;

	temp.x = hand_r[hand_r.size() - 1].x - hand_r[hand_r.size() - 2].x;
	temp.y = hand_r[hand_r.size() - 1].y - hand_r[hand_r.size() - 2].y;
	temp.z = hand_r[hand_r.size() - 1].z - hand_r[hand_r.size() - 2].z;
	v_r_hand.push_back(sqrt((temp.x*temp.x) + (temp.y*temp.y) + (temp.z*temp.z)));
	//cout << "vel hand : " << v_r_hand[v_r_hand.size() - 1] << endl;

	//velocity of hip_c;

	temp.x = hip_c[hip_c.size() - 1].x - hip_c[hip_c.size() - 2].x;
	temp.y = hip_c[hip_c.size() - 1].y - hip_c[hip_c.size() - 2].y;
	temp.z = hip_c[hip_c.size() - 1].z - hip_c[hip_c.size() - 2].z;
	v_hip_c.push_back(sqrt((temp.x*temp.x) + (temp.y*temp.y) + (temp.z*temp.z)));


	//veclocity of foot_r
	temp.x = foot_r[foot_r.size() - 1].x - foot_r[foot_r.size() - 2].x;
	temp.y = foot_r[foot_r.size() - 1].y - foot_r[foot_r.size() - 2].y;
	temp.z = foot_r[foot_r.size() - 1].z - foot_r[foot_r.size() - 2].z;
	v_r_foot.push_back(sqrt((temp.x*temp.x) + (temp.y*temp.y) + (temp.z*temp.z)));


	//veclocity of foot_l
	temp.x = foot_l[foot_l.size() - 1].x - foot_l[foot_l.size() - 2].x;
	temp.y = foot_l[foot_l.size() - 1].y - foot_l[foot_l.size() - 2].y;
	temp.z = foot_l[foot_l.size() - 1].z - foot_l[foot_l.size() - 2].z;
	v_l_foot.push_back(sqrt((temp.x*temp.x) + (temp.y*temp.y) + (temp.z*temp.z)));

	//cout << "  vel" << endl;
}

//  BodyModel accelation feature
void BodyModel::getacc()
{
	//acceleration of l_hand;
	a_l_hand.push_back((abs(v_l_hand[v_l_hand.size() - 1] - v_l_hand[v_l_hand.size() - 2])));

	//acceleration of r_hand;
	a_r_hand.push_back((abs(v_r_hand[v_r_hand.size() - 1] - v_r_hand[v_r_hand.size() - 2])));

	//acceleration of hip_c;
	a_hip_c.push_back((abs(v_hip_c[v_hip_c.size() - 1] - v_hip_c[v_hip_c.size() - 2])));

	//acceleration of r_foot;
	a_r_foot.push_back((abs(v_r_foot[v_r_foot.size() - 1] - v_r_foot[v_r_foot.size() - 2])));

	//acceleration of l_foot;
	a_l_foot.push_back((abs(v_l_foot[v_l_foot.size() - 1] - v_l_foot[v_l_foot.size() - 2])));

	//cout << "  acc" << endl;
}

//  BodyModel jerk feature
void BodyModel::getjerk()
{
	//acceleration of l_hand;
	j_l_hand = ((abs(a_l_hand[a_l_hand.size() - 1] - a_l_hand[a_l_hand.size() - 2])));

	//acceleration of r_hand;
	j_r_hand = ((abs(a_r_hand[a_r_hand.size() - 1] - a_r_hand[a_r_hand.size() - 2])));

	//acceleration of hip_c;
	j_hip_c = ((abs(a_hip_c[a_hip_c.size() - 1] - a_hip_c[a_hip_c.size() - 2])));


	//acceleration of r_foot;
	j_r_foot = ((abs(a_r_foot[a_r_foot.size() - 1] - a_r_foot[a_r_foot.size() - 2])));

	//acceleration of l_foot;
	j_l_foot = ((abs(a_l_foot[a_l_foot.size() - 1] - a_l_foot[a_l_foot.size() - 2])));
	//cout << "  jerk" << endl;
}

//BodyModel feature extraction
void BodyModel::getFeature()
{
	// check user ID, 不同ID清除先前資訊
	if (user_index != knModel.get_userID())
	{
		framecount = 1;
		user_index = knModel.get_userID();

		feature_nn.clear();
		output_nn = MatrixXd::Zero(1, 6);

		head.clear();
		hand_l.clear();
		hand_r.clear();
		shoulder_l.clear();
		shoulder_r.clear();
		shoulder_c.clear();
		hip_c.clear();
		foot_l.clear();
		foot_r.clear();

		v_l_hand.clear();
		v_r_hand.clear();
		v_hip_c.clear();
		v_l_foot.clear();
		v_r_foot.clear();
		a_hip_c.clear();
		a_l_hand.clear();
		a_r_hand.clear();
		a_l_foot.clear();
		a_r_foot.clear();

	}

	// 目前累積資訊frame數
	switch (framecount)
	{
	case 1:
		getpos(user_index);
		framecount++;
		break;

	case 2:
		getpos(user_index);
		getvel();
		framecount++;
		break;
	case 3:
		getpos(user_index);
		getvel();
		getacc();
		framecount++;

		break;
	case 4:
		getpos(user_index);
		getvel();
		getacc();
		getjerk();
		framecount++;
	case 5:
		getpos(user_index);
		getvel();
		getacc();
		getjerk();

		head.erase(head.begin());
		hand_l.erase(hand_l.begin());
		hand_r.erase(hand_r.begin());
		shoulder_l.erase(shoulder_l.begin());
		shoulder_r.erase(shoulder_r.begin());
		shoulder_c.erase(shoulder_c.begin());
		hip_c.erase(hip_c.begin());
		foot_l.erase(foot_l.begin());
		foot_r.erase(foot_r.begin());
		v_l_hand.erase(v_l_hand.begin());
		v_r_hand.erase(v_r_hand.begin());
		v_hip_c.erase(v_hip_c.begin());
		v_l_foot.erase(v_l_foot.begin());
		v_r_foot.erase(v_r_foot.begin());
		a_hip_c.erase(a_hip_c.begin());
		a_l_hand.erase(a_l_hand.begin());
		a_r_hand.erase(a_r_hand.begin());
		a_l_foot.erase(a_l_foot.begin());
		a_r_foot.erase(a_r_foot.begin());
		break;

	default:
		break;
	}



}

// BodyModel RNN output
void BodyModel::RNNinference()
{
	static MatrixXd y;
	if (feature_nn.size()==25) //window size
	{
		for (int i = 0; i<feature_nn.size();++i)
			feature_nn[i] = ((feature_nn[i] - ave.transpose()).array() / Std.transpose().array()).matrix();			// normalization

		output_nn = rnet.predict(feature_nn); //prediction
	
		dp::vec2class(output_nn, y);
		//cout << "body:" << y << endl;
		feature_nn.erase(feature_nn.begin());
	}
}

// BodyModel NN output
void BodyModel::NNinference()
{
	static MatrixXd y;
	MatrixXd norFeature;
	if (feature_nn.size()>0)
	{
		for (int i = 0; i < feature_nn.size(); ++i)
		{
			norFeature = ((feature_nn[i] - ave).array() / Std.array()).matrix();		// normalization
			output_nn = net.predict(norFeature); //presiction
		}

		dp::vec2class(output_nn, y);
		//cout << "body:" << y << endl;
		feature_nn.erase(feature_nn.begin());
	}
}

//BodyModel emotion recognition
void BodyModel::recognition()
{

	if (knModel.bodyistrack == true) //user tracked
		{
		    // draw hip position
			float bx, by;
			LONG colorx, colory;
			NuiTransformSkeletonToDepthImage(knModel.SkeletonFrame.SkeletonData[knModel.get_userID()].SkeletonPositions[NUI_SKELETON_POSITION_HIP_CENTER], &bx, &by);
			NuiImageGetColorPixelCoordinatesFromDepthPixel(NUI_IMAGE_RESOLUTION_640x480, 0, bx, by, 0, &colorx, &colory);
			Point hippoint = { colorx, colory };
			circle(colorpic, hippoint, 5, Scalar(0, 0, 255), 5);


			getFeature();  //getFeature
			knModel.bodyistrack = false;

			if (framecount == 5)// frame>5 進行辨識
			{
				// combine all feature
				vector<double> tempFeature;
				MatrixXd feature(1,36);

					tempFeature.push_back( hip_c[hip_c.size() - 1].z );
					tempFeature.push_back(head[head.size() - 1].x - hip_c[hip_c.size() - 1].x);
					tempFeature.push_back(head[head.size() - 1].y - hip_c[hip_c.size() - 1].y);
					tempFeature.push_back(head[head.size() - 1].z - hip_c[hip_c.size() - 1].z);
					tempFeature.push_back(hand_l[hand_l.size() - 1].x - hip_c[hip_c.size() - 1].x);
					tempFeature.push_back(hand_l[hand_l.size() - 1].y - hip_c[hip_c.size() - 1].y);
					tempFeature.push_back(hand_l[hand_l.size() - 1].z - hip_c[hip_c.size() - 1].z);
					tempFeature.push_back(hand_r[hand_r.size() - 1].x - hip_c[hip_c.size() - 1].x);
					tempFeature.push_back(hand_r[hand_r.size() - 1].y - hip_c[hip_c.size() - 1].y);
					tempFeature.push_back(hand_r[hand_r.size() - 1].z - hip_c[hip_c.size() - 1].z);
					tempFeature.push_back( dis_h_hl );
					tempFeature.push_back( dis_h_hr );
					tempFeature.push_back( dis_hl_hc );
					tempFeature.push_back( dis_hr_hc );
					//tempFeature.push_back( dis_fl_hc); //
					//tempFeature.push_back( dis_fr_hc ); //
					tempFeature.push_back( dis_sc_hc );
					tempFeature.push_back( dis_hl_hr );
					tempFeature.push_back( dis_fl_fr );
					tempFeature.push_back( delta_shoulder_y) ;
					tempFeature.push_back( area_h_2h );
					tempFeature.push_back( area_hipc_2h) ;
					tempFeature.push_back( area_hip_2f);
					tempFeature.push_back( v_l_hand[v_l_hand.size() - 1]);
					tempFeature.push_back( v_r_hand[v_r_hand.size() - 1] );
					tempFeature.push_back( v_hip_c[v_hip_c.size() - 1]);
					tempFeature.push_back( v_l_foot[v_l_foot.size() - 1]) ;
					tempFeature.push_back( v_r_foot[v_r_foot.size() - 1]) ;
					tempFeature.push_back( a_l_hand[a_l_hand.size() - 1]);
					tempFeature.push_back( a_r_hand[a_r_hand.size() - 1]) ;
					tempFeature.push_back( a_hip_c[a_hip_c.size() - 1] );
					tempFeature.push_back( a_l_foot[a_l_foot.size() - 1]) ;
					tempFeature.push_back( a_r_foot[a_r_foot.size() - 1] );
					tempFeature.push_back( j_l_hand );
					tempFeature.push_back( j_r_hand );
					tempFeature.push_back( j_hip_c );
					tempFeature.push_back( j_l_foot) ;
					tempFeature.push_back( j_r_foot );

					for (int i = 0; i < tempFeature.size(); ++i)
					{
						feature(0, i) = tempFeature[i];
					}
					feature_nn.push_back(feature);
			}

		}
		else
		{
			output_nn = MatrixXd::Zero(1, 6);
			feature_nn.clear();
			//if (nntype==2)
				//memory
		}
		
		//emotion recognition 
		switch (nntype)
		{
			case NN:
				NNinference();
				break;
			case RNN:
				RNNinference();
				break;
			default:
				break;
		}

		cv::waitKey(10);

}
// FaceModel RNN output
void FaceModel::RNNinference()
{
	static MatrixXd y;
	if (feature_nn.size()==16) //window size
	{
		for (int i = 0; i<feature_nn.size();++i)
			feature_nn[i] = ((feature_nn[i] - ave.transpose()).array() / Std.transpose().array()).matrix(); //normalize

		output_nn = rnet.predict(feature_nn); //predict
	
		dp::vec2class(output_nn, y);
		//cout << "body:" << y << endl;
		feature_nn.erase(feature_nn.begin());
	}
}

//FaceModel NN output
void FaceModel::NNinference()
{ 
	static MatrixXd y;
	MatrixXd norFeature;
	if (feature_nn.size()>0)
	{
		for (int i = 0; i < feature_nn.size(); ++i)
		{
			norFeature = ((feature_nn[i] - ave).array() / Std.array()).matrix(); //normalize
			output_nn = net.predict(norFeature); //predict
		}
			
			dp::vec2class(output_nn, y);
			//cout << "face: " << y << endl;
			feature_nn.erase(feature_nn.begin());
	}
		
}

// FaceModel初始化
int FaceModel::initial(const int type)  //input: 1:nn, 2:rnn
{
	  pColorDisplay = FTCreateImage();
	  // learning model 初始化
	  output_nn = MatrixXd::Zero(1,6);
	  vector<MatrixXd> weight;
	  vector<MatrixXd> bias;
	  weight.push_back(MatrixXd());
	  weight.push_back(MatrixXd());
	  //weight.push_back(MatrixXd());
	  bias.push_back(MatrixXd());
	  bias.push_back(MatrixXd());
	 // bias.push_back(MatrixXd());

	  // learning model type NN or RNN, weight讀檔
	  if (type == 1)
	  {
		  nntype = 1;  //NN

		  read_binary("parameter/face/NN/67.37/W0", weight[0]);
		  read_binary("parameter/face/NN/67.37/W1", weight[1]);
		  read_binary("parameter/face/NN/67.37/b0", bias[0]);
		  read_binary("parameter/face/NN/67.37/b1", bias[1]);
		  read_binary("parameter/face/NN/67.37/ave", ave);
		  read_binary("parameter/face/NN/67.37/std", Std);
		  read_binary("parameter/face/NN/67.37/fscore", fscore);

		  net = nn::NeuralNetwork(weight, bias);
	  }
		 else if (type == 2)
		 {
			 nntype = 2;  //RNN
			 MatrixXd weight_memory;

			 dp::loadCSV("parameter/face/RNN/Wi.csv", weight[0]);
			 dp::loadCSV("parameter/face/RNN/Wo.csv", weight[1]);
			 dp::loadCSV("parameter/face/RNN/bh.csv", bias[0]);
			 dp::loadCSV("parameter/face/RNN/bo.csv", bias[1]);
			 dp::loadCSV("parameter/face/RNN/Wh.csv", weight_memory);
			 dp::loadCSV("parameter/face/RNN/mean.csv", ave);
			 dp::loadCSV("parameter/face/RNN/std.csv", Std);
			 dp::loadCSV("parameter/face/RNN/fscore.csv", fscore);

			 rnet = rnn::RecurrentNN(weight, bias, weight_memory);
		 }


	  hr = knModel.hr;

	  knModel.pColorFrame = FTCreateImage();
	  knModel.pDepthFrame = FTCreateImage();


	  //----------1、創建一個人臉跟蹤實例-----------------------
	  pFT = FTCreateFaceTracker();//返回資料類型為IFTFaceTracker*
	  if (!pFT)
	  {
		  return -1;// Handle errors
	  }
	  //初始化人臉跟蹤所需要的資料資料-----FT_CAMERA_CONFIG包含彩色或深度感測器的資訊（長，寬，焦距）
	   myCameraConfig = { COLOR_WIDTH, COLOR_HIGHT, NUI_CAMERA_COLOR_NOMINAL_FOCAL_LENGTH_IN_PIXELS }; // width, height, focal length
	   depthConfig = { DEPTH_WIDTH, DEPTH_HIGHT, NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS };
	  //depthConfig.FocalLength = NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS;
	  //depthConfig.Width	 = DEPTH_WIDTH;
	  //depthConfig.Height	 = DEPTH_HIGHT;//這裡一定要填，而且要填對才行！！
	  //IFTFaceTracker的初始化, --- 人臉跟蹤主要介面IFTFaceTracker的初始化
	  hr = pFT->Initialize(&myCameraConfig, &depthConfig, NULL, NULL);
	  if (FAILED(hr))
	  {
		  return -2;// Handle errors
	  }
	  // 2、----------創建一個實例接受3D跟蹤結果----------------
	  pFTResult = NULL;
	  hr = pFT->CreateFTResult(&pFTResult);
	  if (FAILED(hr))
	  {
		  return -11;
	  }
	  // prepare Image and SensorData for 640x480 RGB images
	  if (!knModel.pColorFrame)
	  {
		  return -12;// Handle errors
	  }
	  // Attach assumes that the camera code provided by the application
	  // is filling the buffer cameraFrameBuffer
	  //申請記憶體空間
	  pColorDisplay->Allocate(COLOR_WIDTH, COLOR_HIGHT, FTIMAGEFORMAT_UINT8_B8G8R8X8);
	  hr = knModel.pColorFrame->Allocate(COLOR_WIDTH, COLOR_HIGHT, FTIMAGEFORMAT_UINT8_B8G8R8X8);
	  if (FAILED(hr))
	  {
		  return hr;
	  }
	  hr = knModel.pDepthFrame->Allocate(DEPTH_WIDTH, DEPTH_HIGHT, FTIMAGEFORMAT_UINT16_D13P3);
	  if (FAILED(hr))
	  {
		  return hr;
	  }

	  //填充FT_SENSOR_DATA結構，包含用於人臉追蹤所需要的所有輸入資料
	  POINT	 point;
	  sensorData.ZoomFactor = 1.0f;
	  point.x = 0;
	  point.y = 0;
	  sensorData.ViewOffset = point;//POINT(0,0)


}
// facemodel emotion recognition
int FaceModel::recognition()
{

	// 跟蹤人臉
	
		sensorData.pVideoFrame = knModel.pColorFrame;//彩色圖像資料
		sensorData.pDepthFrame = knModel.pDepthFrame;//深度圖像資料
		//初始化追蹤，比較耗時
		if (!faceisTracked)//為false  
		{
			//會耗費較多cpu計算資源，開始跟蹤
			hr = pFT->StartTracking(&sensorData, NULL, knModel.m_hint3D, pFTResult);//輸入為彩色圖像，深度圖像，人頭和肩膀的三維座標
			if(SUCCEEDED(hr) && SUCCEEDED(pFTResult->GetStatus()))
			{
				faceisTracked = true;
			}
			else
			{
				faceisTracked = false;
				output_nn = MatrixXd::Zero(1, 6);
				feature_nn.clear();
			}
		}
		else
		{
			//繼續追蹤，很迅速，它一般使用一個已大概知曉的人臉模型，所以它的調用不會消耗多少cpu計算，pFTResult存放跟蹤的結果
			hr = pFT->ContinueTracking(&sensorData, knModel.m_hint3D, pFTResult);
			if(FAILED(hr) || FAILED (pFTResult->GetStatus()))
			{
				// 跟丟
				faceisTracked = false;
				output_nn = MatrixXd::Zero(1, 6);
				feature_nn.clear();
			}
		}
		int bStop;
		if (faceisTracked)
		{
			IFTModel*	ftModel;//三維人臉模型
			HRESULT	 hr = pFT->GetFaceModel(&ftModel);//得到三維人臉模型
			FLOAT*	 pSU = NULL;
			UINT	 numSU;
			BOOL	 suConverged;
			pFT->GetShapeUnits(NULL, &pSU, &numSU, &suConverged);

			//cout << "SU num: " << numSU << " , SU point " << pSU[9] << endl;

			POINT viewOffset = {0, 0};
			knModel.pColorFrame->CopyTo(pColorDisplay, NULL, 0, 0);//將彩色圖像pColorFrame複製到pColorDisplay中，然後對pColorDisplay進行直接處理
			hr = getFeature(pColorDisplay, ftModel, &myCameraConfig, pSU, 1.0, viewOffset, pFTResult, 0x00FFFF00);// feature extraction
			if(FAILED(hr))
				printf("顯示失敗！！\n");

			Mat tempMat(COLOR_HIGHT, COLOR_WIDTH, CV_8UC4, pColorDisplay->GetBuffer());
			colorpic = tempMat.clone();

			//histogram_pic = tempMat.clone();

			//imshow("faceTracking", tempMat);
			bStop = cv::waitKey(1);//按下ESC結束

		}
		else	// -----------當isTracked = false時，則值顯示獲取到的彩色圖像資訊
		{			
			knModel.pColorFrame->CopyTo(pColorDisplay, NULL, 0, 0);
			Mat tempMat(COLOR_HIGHT,COLOR_WIDTH,CV_8UC4,pColorDisplay->GetBuffer());
			colorpic = tempMat.clone();
			//histogram_pic = tempMat.clone();
			//imshow("faceTracking", tempMat);
			bStop = cv::waitKey(1);
		}
		// learning model output
		switch (nntype)
		{
		case NN:
			NNinference();
			break;
		case RNN:
			RNNinference();
			break;
		default:
			break;
		}
		cv::waitKey(16);

	return 0;
	
}
//顯示網狀人臉,初始化人臉模型,抽取特徵
HRESULT FaceModel::getFeature(IFTImage* pColorImg, IFTModel* pModel, FT_CAMERA_CONFIG const* pCameraConfig, FLOAT const* pSUCoef,
	FLOAT zoomFactor, POINT viewOffset, IFTResult* pAAMRlt, UINT32 color)//zoomFactor = 1.0f viewOffset = POINT(0,0) pAAMRlt為跟蹤結果
{

	Mat fpoint(3*COLOR_HIGHT,3*COLOR_WIDTH,CV_8UC4);
	if (!pColorImg || !pModel || !pCameraConfig || !pSUCoef || !pAAMRlt)
	{
		return E_POINTER;
	}
	HRESULT hr = S_OK;
	UINT vertexCount = pModel->GetVertexCount();//面部特徵點的個數
	FT_VECTOR2D* pPts2D = reinterpret_cast<FT_VECTOR2D*>(_malloca(sizeof(FT_VECTOR2D) * vertexCount));//二維向量 reinterpret_cast強制類型轉換符 _malloca在堆疊上分配記憶體
	//複製_malloca(sizeof(FT_VECTOR2D) * vertexCount)個位元組到pPts2D，用於存放面部特徵點，該步相當於初始化
	if (pPts2D)
	{
		FLOAT *pAUs;
		UINT auCount;//UINT類型在WINDOWS API中有定義，它對應於32位不帶正負號的整數
		hr = pAAMRlt->GetAUCoefficients(&pAUs, &auCount);

		if (SUCCEEDED(hr))
		{
			//rotationXYZ人臉旋轉角度！
			FLOAT scale, rotationXYZ[3], translationXYZ[3];
			hr = pAAMRlt->Get3DPose(&scale, rotationXYZ, translationXYZ);

			if (SUCCEEDED(hr))
			{
				hr = pModel->GetProjectedShape(pCameraConfig, zoomFactor, viewOffset, pSUCoef, pModel->GetSUCount(), pAUs, auCount,
					scale, rotationXYZ, translationXYZ, pPts2D, vertexCount);
				//這裡獲取了vertexCount個面部特徵點，存放在pPts2D指標陣列中
				if (SUCCEEDED(hr))
				{
					POINT* p3DMdl = reinterpret_cast<POINT*>(_malloca(sizeof(POINT) * vertexCount));
				
					MatrixXd feature(1,54);
					vector<double> tempFeature;
					tempFeature.reserve(54);
					Point2f fpp[22] = {0};
					int n = 0;

					if (p3DMdl)
					{
						Point2f fshift;

						for (UINT i = 0; i < vertexCount; ++i)
						{
							p3DMdl[i].x = LONG(pPts2D[i].x + 0.5f);
							p3DMdl[i].y = LONG(pPts2D[i].y + 0.5f);
							// seclect feature points
							if (i == 0 || i == 7 || i == 8 || i == 10 || i == 15 || i == 16 || i == 17 || i == 20 || i == 21 || i == 22 || i == 23 || i == 27 || i == 31 || i == 48 || i == 49 || i == 50 || i == 53 || i == 54 || i == 55 || i == 56 || i == 60 || i == 64)
							{
								fpp[n].x = pPts2D[i].x;
								fpp[n].y = pPts2D[i].y;
									//circle(colorpic, fpp[n], 1, cv::Scalar(0, 0, 255), 2);
								n++;
							}
							if (i == 5)  //nose
							{
								fshift.x = pPts2D[i].x;
								fshift.y = pPts2D[i].y;
							}
						}
						// shift related to nose
						for (int i = 0; i < n; i++)
						{
							fpp[i] = fpp[i] - fshift;
							tempFeature.push_back(fpp[i].x);
							tempFeature.push_back(fpp[i].y);
							//cout << "fppdis " << i << ": " << fpp[i].x << ", " << fpp[i].y << endl;
						}

						//cout << "-----head pose-----" << endl << "  x: " << translationXYZ[0] << "  y: " << translationXYZ[1] << "  z: " << translationXYZ[2] << endl << "  pitch: " << rotationXYZ[0] << "  yaw: " << rotationXYZ[1] << "  roll: " << rotationXYZ[2] << endl << endl;

						for (int i = 0; i < auCount; i++){
							tempFeature.push_back(pAUs[i]);
						}
						tempFeature.push_back(translationXYZ[2]);
						tempFeature.push_back(rotationXYZ[0]);
						tempFeature.push_back(rotationXYZ[1]);
						tempFeature.push_back(rotationXYZ[2]);

						for (int i = 0, n = tempFeature.size(); i < n; ++i){
							feature(0,i) = tempFeature[i];
						}

						feature_nn.push_back(feature);

						cv::waitKey(1);

						FT_TRIANGLE* pTriangles;
						UINT triangleCount;
						hr = pModel->GetTriangles(&pTriangles, &triangleCount);

						/////////face mask/////////
						if (SUCCEEDED(hr))
						{
							
							struct EdgeHashTable
							{
								UINT32* pEdges;
								UINT edgesAlloc;
								void Insert(int a, int b)
								{ 
									UINT32 v = (min(a, b) << 16) | max(a, b);
									UINT32 index = (v + (v << 8)) * 49157, i;
									for (i = 0; i < edgesAlloc - 1 && pEdges[(index + i) & (edgesAlloc - 1)] && v != pEdges[(index + i) & (edgesAlloc - 1)]; ++i)
									{
									}
									pEdges[(index + i) & (edgesAlloc - 1)] = v;
								}
							} eht;
							eht.edgesAlloc = 1 << UINT(log(2.f * (1 + vertexCount + triangleCount)) / log(2.f));
							eht.pEdges = reinterpret_cast<UINT32*>(_malloca(sizeof(UINT32) * eht.edgesAlloc));
							//if (eht.pEdges)
							//{
							//	ZeroMemory(eht.pEdges, sizeof(UINT32) * eht.edgesAlloc);
							//	for (UINT i = 0; i < triangleCount; ++i)
							//	{
							//		eht.Insert(pTriangles[i].i, pTriangles[i].j);
							//		eht.Insert(pTriangles[i].j, pTriangles[i].k);
							//		eht.Insert(pTriangles[i].k, pTriangles[i].i);
							//	}
							//	for (UINT i = 0; i < eht.edgesAlloc; ++i)
							//	{
							//		if(eht.pEdges[i] != 0)
							//		{
							//			pColorImg->DrawLine(p3DMdl[eht.pEdges[i] >> 16], p3DMdl[eht.pEdges[i] & 0xFFFF], color, 1);
							//		}
							//	}
							//	_freea(eht.pEdges);
							//}

							// 畫出人臉矩形框
							RECT rectFace;

							hr = pAAMRlt->GetFaceRect(&rectFace);//得到人臉矩形
							if (SUCCEEDED(hr))
							{
								POINT leftTop = { rectFace.left, rectFace.top };//左上角
								POINT rightTop = { rectFace.right - 1, rectFace.top };//右上角
								POINT leftBottom = { rectFace.left, rectFace.bottom - 1 };//左下角
								POINT rightBottom = { rectFace.right - 1, rectFace.bottom - 1 };//右下角
								UINT32 nColor = 0xff00ff;
								SUCCEEDED(hr = pColorImg->DrawLine(leftTop, rightTop, nColor, 1)) &&
								SUCCEEDED(hr = pColorImg->DrawLine(rightTop, rightBottom, nColor, 1)) &&
								SUCCEEDED(hr = pColorImg->DrawLine(rightBottom, leftBottom, nColor, 1)) &&
								SUCCEEDED(hr = pColorImg->DrawLine(leftBottom, leftTop, nColor, 1));
							}
						}
						_freea(p3DMdl);
					}
					else
					{
						hr = E_OUTOFMEMORY;
					}
				}
			}
		}
		_freea(pPts2D);
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}
	return hr;
}
//emotion class 建構
EmotionClass::EmotionClass()
{
	////初始化為1，但是在main裡面會重新設定
	//臉部、身體、聲音、fsuion
	Facemode = 1;
	Bodymode = 1;
	Speechmode = 1;
	Fusionmode = 1;
}
//emotion class 初始化
void EmotionClass::initial()
{
	output_fusion = MatrixXd::Zero(1, 6);
	dp::loadCSV("parameter/lambda.csv", fuzzy_lambda);  //lambda value
	//進行uni-model初始化，讀檔及參數
	face.initial(Facemode);
	body.initial(Bodymode);
	speech.initial(Speechmode);
}

void  EmotionClass::fusion_Fscore()			//// Fscore 作為 weight 直接相加，進行fusion
{
	static MatrixXd y;
	output_fusion = face.get_fscore().array()*face.get_output().array() + body.get_fscore().array()*body.get_output().array()+speech.get_fscore().array()*speech.get_output().array();

	dp::vec2class(output_fusion, y);
	//cout << "fusion:"<<y<<"  -> " << output_fusion << endl<<endl
}

void  EmotionClass::fusion_Fuzzy()		//// fuzzy integral fusion
{
	for (int i = 0; i < 6; ++i) //六種情緒
	{
		multimap <double, double, std::greater<double>> fmap; //以map儲存uni-model辨識值及fscore
		fmap.insert(pair<double, double>(face.get_output()(0,i), face.get_fscore()(0,i)));
		fmap.insert(pair<double, double>(body.get_output()(0, i), body.get_fscore()(0, i)));
		fmap.insert(pair<double, double>(speech.get_output()(0, i), speech.get_fscore()(0, i)));
		
		MatrixXd temp(3, 2);
		multimap<double, double>::iterator it = fmap.begin();

		double g_lambda = (*it).second;
		temp(0, 0) = (*it).first;
		temp(0, 1) = g_lambda;
		++it;

		for (int i=1; it != fmap.end(); ++it,++i)
		{
			g_lambda = g_lambda + (*it).second + fuzzy_lambda(0, i)* g_lambda* (*it).second;
			temp(i, 0) = (*it).first;
			temp(i, 1) = g_lambda;
		}
		output_fusion(0,i)=temp.rowwise().minCoeff().maxCoeff(); //fusion result
	}

	//dp::vec2class(output_fusion, y);
	//cout << "fusion:"<<y<<"  -> " << output_fusion << endl<<endl;
}

//圖像顯示即時uni-model與fusion結果
void EmotionClass::plot_fusionpic()
{
	///   face part   ///
	Mat plot = Mat(250, 800, CV_8UC3,Scalar(0,0,0));

	putText(plot, "Face", Point(0,30), FONT_HERSHEY_COMPLEX, 1, Scalar(255, 0, 0));
	putText(plot, "neu", Point(0, 70), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));
	putText(plot, "hap", Point(0, 100), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));
	putText(plot, "sur", Point(0, 130), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));
	putText(plot, "sad", Point(0, 160), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));
	putText(plot, "fea", Point(0, 190), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));
	putText(plot, "ang", Point(0, 220), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));

	line(plot, Point(50 + 0.2 * 150, 60), Point(50 + 0.2 * 150, 230), Scalar(150, 150, 150), 1);
	line(plot, Point(50 + 0.4 * 150, 60), Point(50 + 0.4 * 150, 230), Scalar(150, 150, 150), 1);
	line(plot, Point(50 + 0.6 * 150, 60), Point(50 + 0.6 * 150, 230), Scalar(150, 150, 150), 1);
	line(plot, Point(50 + 0.8 * 150, 60), Point(50 + 0.8 * 150, 230), Scalar(150, 150, 150), 1);

	MatrixXd foutput_nn = face.get_output();

	if (foutput_nn.sum() != 0)
	{
		MatrixXd foutput = (foutput_nn / foutput_nn.sum());
		//cout << foutput;
		line(plot, Point(50, 70), Point((50 + foutput(0, 0) * 150), 70), Scalar(0, 0, 255), 4);
		line(plot, Point(50, 100), Point(50 + foutput(0, 1) * 150, 100), Scalar(0, 0, 255), 4);
		line(plot, Point(50, 130), Point(50 + foutput(0, 2) * 150, 130), Scalar(0, 0, 255), 4);
		line(plot, Point(50, 160), Point(50 + foutput(0, 3) * 150, 160), Scalar(0, 0, 255), 4);
		line(plot, Point(50, 190), Point(50 + foutput(0, 4) * 150, 190), Scalar(0, 0, 255), 4);
		line(plot, Point(50, 220), Point(50 + foutput(0, 5) * 150, 220), Scalar(0, 0, 255), 4);
	}

	///   body part   ///
	putText(plot, "Body", Point(200, 30), FONT_HERSHEY_COMPLEX, 1, Scalar(255, 0, 0));
	putText(plot, "neu", Point(200, 70), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));
	putText(plot, "hap", Point(200, 100), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));
	putText(plot, "sur", Point(200, 130), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));
	putText(plot, "sad", Point(200, 160), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));
	putText(plot, "fea", Point(200, 190), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));
	putText(plot, "ang", Point(200, 220), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));

	line(plot, Point(250 + 0.2 * 150, 60), Point(250 + 0.2 * 150, 230), Scalar(150, 150, 150), 1);
	line(plot, Point(250 + 0.4 * 150, 60), Point(250 + 0.4 * 150, 230), Scalar(150, 150, 150), 1);
	line(plot, Point(250 + 0.6 * 150, 60), Point(250 + 0.6 * 150, 230), Scalar(150, 150, 150), 1);
	line(plot, Point(250 + 0.8 * 150, 60), Point(250 + 0.8 * 150, 230), Scalar(150, 150, 150), 1);

	MatrixXd boutput_nn = body.get_output();

	if (boutput_nn.sum() != 0)
	{
		MatrixXd boutput = (boutput_nn / boutput_nn.sum());
		//cout << boutput;
		line(plot, Point(250, 70), Point((250 + boutput(0, 0) * 150), 70), Scalar(0, 0, 255), 4);
		line(plot, Point(250, 100), Point(250 + boutput(0, 1) * 150, 100), Scalar(0, 0, 255), 4);
		line(plot, Point(250, 130), Point(250 + boutput(0, 2) * 150, 130), Scalar(0, 0, 255), 4);
		line(plot, Point(250, 160), Point(250 + boutput(0, 3) * 150, 160), Scalar(0, 0, 255), 4);
		line(plot, Point(250, 190), Point(250 + boutput(0, 4) * 150, 190), Scalar(0, 0, 255), 4);
		line(plot, Point(250, 220), Point(250 + boutput(0, 5) * 150, 220), Scalar(0, 0, 255), 4);
	}

	///   speech part   ///
	putText(plot, "Speech", Point(400, 30), FONT_HERSHEY_COMPLEX, 1, Scalar(255, 0, 0));
	putText(plot, "neu", Point(400, 70), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));
	putText(plot, "hap", Point(400, 100), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));
	putText(plot, "sur", Point(400, 130), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));
	putText(plot, "sad", Point(400, 160), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));
	putText(plot, "fea", Point(400, 190), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));
	putText(plot, "ang", Point(400, 220), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));

	line(plot, Point(450 + 0.2 * 150, 60), Point(450 + 0.2 * 150, 230), Scalar(150, 150, 150), 1);
	line(plot, Point(450 + 0.4 * 150, 60), Point(450 + 0.4 * 150, 230), Scalar(150, 150, 150), 1);
	line(plot, Point(450 + 0.6 * 150, 60), Point(450 + 0.6 * 150, 230), Scalar(150, 150, 150), 1);
	line(plot, Point(450 + 0.8 * 150, 60), Point(450 + 0.8 * 150, 230), Scalar(150, 150, 150), 1);
	
	MatrixXd soutput_nn = speech.get_output();

	if (soutput_nn.sum() != 0)
	{
		MatrixXd soutput = (soutput_nn / soutput_nn.sum());
		//cout << output;
		line(plot, Point(450, 70), Point((450 + soutput(0, 0) * 150), 70), Scalar(0, 0, 255), 4);
		line(plot, Point(450, 100), Point(450 + soutput(0, 1) * 150, 100), Scalar(0, 0, 255), 4);
		line(plot, Point(450, 130), Point(450 + soutput(0, 2) * 150, 130), Scalar(0, 0, 255), 4);
		line(plot, Point(450, 160), Point(450 + soutput(0, 3) * 150, 160), Scalar(0, 0, 255), 4);
		line(plot, Point(450, 190), Point(450 + soutput(0, 4) * 150, 190), Scalar(0, 0, 255), 4);
		line(plot, Point(450, 220), Point(450 + soutput(0, 5) * 150, 220), Scalar(0, 0, 255), 4);
	}
	
	///   fusion part   ///

	putText(plot, "Fusion", Point(600, 30), FONT_HERSHEY_COMPLEX, 1, Scalar(255, 0, 0));
	putText(plot, "neu", Point(600, 70), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));
	putText(plot, "hap", Point(600, 100), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));
	putText(plot, "sur", Point(600, 130), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));
	putText(plot, "sad", Point(600, 160), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));
	putText(plot, "fea", Point(600, 190), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));
	putText(plot, "ang", Point(600, 220), FONT_HERSHEY_COMPLEX, 0.5, Scalar(255, 255, 255));

	line(plot, Point(650 + 0.2 * 150, 60), Point(650 + 0.2 * 150, 230), Scalar(150, 150, 150), 1);
	line(plot, Point(650 + 0.4 * 150, 60), Point(650 + 0.4 * 150, 230), Scalar(150, 150, 150), 1);
	line(plot, Point(650 + 0.6 * 150, 60), Point(650 + 0.6 * 150, 230), Scalar(150, 150, 150), 1);
	line(plot, Point(650 + 0.8 * 150, 60), Point(650 + 0.8 * 150, 230), Scalar(150, 150, 150), 1);

	if (output_fusion.sum() != 0)
	{
		MatrixXd output = (output_fusion / output_fusion.sum());
		//cout << output;
		line(plot, Point(650, 70), Point((650 + output(0, 0) * 150), 70), Scalar(0, 0, 255), 4);
		line(plot, Point(650, 100), Point(650 + output(0, 1) * 150, 100), Scalar(0, 0, 255), 4);
		line(plot, Point(650, 130), Point(650 + output(0, 2) * 150, 130), Scalar(0, 0, 255), 4);
		line(plot, Point(650, 160), Point(650 + output(0, 3) * 150, 160), Scalar(0, 0, 255), 4);
		line(plot, Point(650, 190), Point(650 + output(0, 4) * 150, 190), Scalar(0, 0, 255), 4);
		line(plot, Point(650, 220), Point(650 + output(0, 5) * 150, 220), Scalar(0, 0, 255), 4);
	}

	cv::imshow("pic", plot);
}

//multi-model emotion recognition main function
void  EmotionClass::emotionloop()
{
	//keybord event online使用可以記錄辨識結果，以confusion matrix呈現
	int type = 0;
	bool record = 0;
	MatrixXd::Index   maxIndex;
	Eigen::MatrixXd confu_face = MatrixXd::Zero(6, 6);
	Eigen::MatrixXd confu_body = MatrixXd::Zero(6, 6);
	Eigen::MatrixXd confu_speech = MatrixXd::Zero(6, 6);
	Eigen::MatrixXd confu_fusion = MatrixXd::Zero(6, 6);

	cout << "======keybord event======" << endl;
	cout << "g: record"<<endl;
	cout << "s: stop" << endl;
	cout << "0-5: type" << endl;
	cout << "p: show confusion matrix" << endl;
	cout << "o: save confusion matrix" << endl;
	cout << "i: load confusion matrix" << endl;
	cout << "r: reset confusion matrix" << endl;
	cout << "=========================" << endl<<endl;
	while (1)
	{
	
		// 開thread執行，聲音與臉身體平行
		// uni-model emotion recognition
		#pragma omp parallel sections
		{
			#pragma omp section
			{ speech.recognition(); }

			#pragma omp section
			{ 
				face.recognition();
				body.recognition();
			}
		}
		#pragma omp barrier

		//WaitForSingleObject(shandle, INFINITE);
	
		// 將uni-model output進行fusion
		switch (Fusionmode)
		{
			case FSCORE:
				fusion_Fscore();
				break;
			case FUZZY_INTEGRAL:
				fusion_Fuzzy();
				break;
			default:
				break;
		}

		//keybord event online使用可以記錄辨識結果，以confusion matrix呈現
		if (_kbhit())
		{
			int a = _getch();
			if (a == 'g')    record = 1;
			else if (a == 's')  record = 0;
			else if (a == '0')  type = 0;
			else if (a == '1')  type = 1;
			else if (a == '2')  type = 2;
			else if (a == '3')  type = 3;
			else if (a == '4')  type = 4;
			else if (a == '5')  type = 5;
			else if (a == 'p') 
			{
				cout << "face" << endl << confu_face << endl << "accuracy: " << confu_face.trace() / confu_face.sum() << endl << endl;
				cout << "body" << endl << confu_body << endl << "accuracy: " << confu_body.trace() / confu_body.sum() << endl << endl;
				cout << "speech" << endl << confu_speech << endl << "accuracy: " << confu_speech.trace() / confu_speech.sum() << endl << endl;
				cout << "fusion" << endl << confu_fusion << endl << "accuracy: " << confu_fusion.trace() / confu_fusion.sum() << endl << endl;
				cout << "--------------------------" << endl;
			}
			else if (a == 'o')
			{
				dp::saveCSV("confu_face.csv", confu_face); 
				dp::saveCSV("confu_body.csv", confu_body); 
				dp::saveCSV("confu_speech.csv", confu_speech); 
				dp::saveCSV("confu_fusion.csv", confu_fusion);  
				cout << "save!" << endl; 
			}
			else if (a == 'i')
			{ 
				dp::loadCSV("confu_face.csv", confu_face);
				dp::loadCSV("confu_body.csv", confu_body);
				dp::loadCSV("confu_speech.csv", confu_speech);
				dp::loadCSV("confu_fusion.csv", confu_fusion);  
				cout << "load.." << endl;
			}
			else if (a == 'r')
			{
				confu_face = MatrixXd::Zero(6, 6);
				confu_body = MatrixXd::Zero(6, 6);
				confu_speech = MatrixXd::Zero(6, 6);
				confu_fusion = MatrixXd::Zero(6, 6);
			}
		}
		// 紀錄confusion matrix
		if (record)
		{
			if (face.get_output().sum() != 0)
			{
				face.get_output().row(0).maxCoeff(&maxIndex);
				confu_face(maxIndex, type)++;
			}
			if (body.get_output().sum() != 0)
			{
				body.get_output().row(0).maxCoeff(&maxIndex);
				confu_body(maxIndex, type)++;
			}
			if (speech.get_output().sum() != 0)
			{
				speech.get_output().row(0).maxCoeff(&maxIndex);
				confu_speech(maxIndex, type)++;
			}
			if (output_fusion.sum() != 0)
			{
				output_fusion.row(0).maxCoeff(&maxIndex);
				confu_fusion(maxIndex, type)++;
			}
		}

		plot_fusionpic();  // 顯示uni-model、multi-model辨識結果
		cv::imshow("emotion", colorpic);
		cv::waitKey(10);
	}

}