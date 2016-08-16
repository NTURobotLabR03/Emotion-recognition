# Emotion-recognition<br>
multi-modal emotion recognition<br>

實際整合後online使用程式(vc12)<br>
*libary: Kinect SDK1.8、openCV2.4.9、Eigen <br>
*使用unicode<br>
1.main function可設定要使用NN或RNN，fusion要用fscore或Fuzzy integral<br>
2.initial內讀取weight與參數初始化(注意讀檔路徑)，在emotion_vc10.cpp內的initial修改<br>
3.人臉追蹤到會有紫色框框，身體追蹤到hip有紅色圈圈<br>
4.初始化會鎖定最近的使用者直到離開<br>
5.即時顯示unimodel、multimodel fusion結果，可用getoutput取值<br>
6.使用距離約2m，高度70cm<br>
