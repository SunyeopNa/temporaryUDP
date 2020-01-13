#include "PracticalSocket.h" // For UDPSocket and SocketException
#include <iostream>          // For cout and cerr
#include <cstdlib>           // For atoi()
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <opencv/cv.h>
#include <queue>
#include <pthread.h>
#include <errno.h>

// C++ std library dependencies
#include <chrono> // `std::chrono::` functions and classes, e.g. std::chrono::milliseconds
#include <thread> // std::this_thread
using namespace cv;

std::queue<cv::Mat> frameQueue;
int tcpsocket;

Mat syncFrame;


// This worker will just read and return all the jpg files in a directory
class WUserOutput : public op::WorkerConsumer<std::shared_ptr<std::vector<UserDatum>>>
{
public:
    void initializationOnThread() {}

    void workConsumer(const std::shared_ptr<std::vector<UserDatum>>& datumsPtr)
    {
        try
        {
            // User's displaying/saving/other processing here
                // datum.cvOutputData: rendered frame with pose or heatmaps
                // datum.poseKeypoints: Array<float> with the estimated pose

            if (datumsPtr != nullptr && !datumsPtr->empty())
            {
                // Show in command line the resulting pose keypoints for body, face and hands
                //op::log("\nKeypoints:");
                // Accesing each element of the keypoints
                const auto& poseKeypoints = datumsPtr->at(0).poseKeypoints;
                //op::log("Person pose keypoints:");

                //this keyPoints will be sent to client
		vector<UserKeyPoint> keyPoints;
		
		for (auto person = 0 ; person < poseKeypoints.getSize(0) ; person++)
                {
		    UserKeyPoint keyPoint;
                    //op::log("Person " + std::to_string(person) + " (x, y, score):");
                    for (auto bodyPart = 0 ; bodyPart < poseKeypoints.getSize(1) ; bodyPart++)
                    {
                	/*
		        std::string valueToPrint;
                        for (auto xyscore = 0 ; xyscore < poseKeypoints.getSize(2) ; xyscore++)
                        {
                            valueToPrint += std::to_string(poseKeypoints[{person, bodyPart, xyscore}]) + " ";
                              
                        }
			*/
			float x = poseKeypoints[{person,bodyPart,0}];
			float y = poseKeypoints[{person,bodyPart,1}];
			Pt point(x,y);
			// **extract color here!
                        // If bodyPart need color extraction and  x,y point is non zero, color extraction is processed.
                        //if( isColorNeeded(bodyPart) && x != 0 && y != 0)
                        if( x!=0 && y!=0)
			{
                            // Get current frame
                           // Mat curFrame = datumsPtr->at(0).cvOutputData;
                            Mat curFrame = syncFrame.clone();
			    Size frameSize = curFrame.size();

                            const int RANGE = 3;
                            int sRed = 0;
                            int sBlue = 0;
                            int sGreen = 0;
			    	
                            int startX = (x > RANGE) ? x-RANGE : 0;
			    int endX = (frameSize.width-x > RANGE) ? x+RANGE-1 : frameSize.width-1;
			    int startY = (y > RANGE) ? y-RANGE : 0; 
			    int endY = (frameSize.height-y > RANGE)? y+RANGE-1 : frameSize.height-1;
                            int cnt = 0;
                            
			    //cout << "x:"<<startX<<"~"<<endX<<"// y:"<<startY<<"~"<<endY<<endl;
			    //cout << "width*height:"<<frameSize.width<<"*"<<frameSize.height<<endl;
			    
			    for(auto y_ = startY; y_ < endY ; y_++)
                            {
				Vec3b* pixel = curFrame.ptr<Vec3b>(y);
                                for(auto x_ = startX; x_ < endX ; x_++)
                                {
                                    // Get RGB value about x,y point in current frame
                                    //Vec3b intensity = datumsPtr->at(0).cvOutputData.at<Vec3b>(y_,x_);                                
                                    
				    sRed += pixel[x_][2];
                                    sBlue += pixel[x_][0];
                                    sGreen += pixel[x_][1];
                                    cnt++; 
                                }
                            }	
			
			    int aRed = -1;
		  	    int aGreen = -1;
			    int aBlue = -1;
			    cout<<"cnt:"<<cnt<<endl;
			    if(cnt != 0){
				// Calculate the average RGB
				aRed = (sRed/cnt);
				aGreen =(sGreen/cnt);
				aBlue = (sBlue/cnt);
			    }
			    point.setColor(aRed,aGreen,aBlue);
                        }
                        else
                        { // If x,y point is zero, color extraction is skipped and color information is set to NULL
                           point.setColor(-1,-1,-1);
			}		
			//add point to array	
			keyPoint.addPoint(point);
                    }
		    
                    keyPoints.push_back(keyPoint);
                }
		

                int detectedPeople = keyPoints.size(); 
                if(detectedPeople != 0){
	  	    string msg = Json(keyPoints).dump()+"\r\n";
		    cout << msg <<endl;

                    //Send client the coordinates of Right hand and body 
                    if(send(tcpsocket, msg.c_str(), msg.size(), 0) < 0){
                        cout << "Send failed : " << msg << endl;
                    }
                }    

                const char key = (char)cv::waitKey(1);
                if (key == 27)
                    this->stop();
            }
        }
        catch (const std::exception& e)
        {
            op::log("Some kind of unexpected error happened.");
            this->stop();
            op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }
};

int main(int argc, char *argv[]) 
{
    int PORT_NUM;
    int FRAME= 0;
    cout << "Server v2.0" << endl;
    
    int ARGV_PORT = atoi(argv[1]);
    if (ARGV_PORT == 0)
    {
        cout << "포트 번호를 입력하세요>> "; 
    	cin >> PORT_NUM;
    }
	else
    {
        PORT_NUM = ARGV_PORT;				   
    }

    cout << "현재 포트번호: " << PORT_NUM << endl;
    //namedWindow("recv", CV_WINDOW_AUTOSIZE);
    //Parsing command line flags
    
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    // Running openPoseTutorialWrapper2
    try {
        pthread_t serverThread;
        int reuse;
        //TCP Socket
        int sockfd=socket(AF_INET,SOCK_STREAM,0);
        
        // Set socket in order to reuse its resources
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int)) == -1){
            printf("Reuse port Error: %s\n", strerror(errno));
        }

        struct sockaddr_in serverAddress;
        struct sockaddr_in clientAddress;
        memset(&serverAddress,0,sizeof(serverAddress));
        serverAddress.sin_family=AF_INET;
        serverAddress.sin_addr.s_addr=htonl(INADDR_ANY);
        serverAddress.sin_port=htons(PORT_NUM);
        if(bind(sockfd,(struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
        {
              printf("bind error: %s\n", strerror(errno));
              exit(1);
        }
        else
        {
            cout << "TCP Socket is created" << endl;
        }

        if(listen(sockfd,5) == -1)
        {
              printf("listen error: %s\n", strerror(errno));
              exit(1);
        }

        string th_str;

		socklen_t sosize  = sizeof(clientAddress);
		tcpsocket = accept(sockfd,(struct sockaddr*)&clientAddress,&sosize);
		th_str = inet_ntoa(clientAddress.sin_addr);
        
		pthread_create(&serverThread,NULL,&transfer,NULL);

        //UDP Socket
        UDPSocket sock(PORT_NUM);
        cout << "UDP Socket is created" << endl;

        char buffer[BUF_LEN]; // Buffer for echo string
        int recvMsgSize; // Size of received message
        string sourceAddress; // Address of datagram source
        unsigned short sourcePort; // Port of datagram source

        clock_t last_cycle = clock();
        while (true) 
        {
            //Block until receive message from a client
            if((recvMsgSize = sock.recvFrom(buffer, BUF_LEN, sourceAddress, sourcePort)) < 0){
                cout << "Receive time out" << endl;
                exit(1);
            }
            
            char * longbuf = new char[recvMsgSize];
            memcpy( & longbuf[0], buffer, recvMsgSize);

            //cout << "Received packet from " << sourceAddress << ":" << sourcePort << endl;
            Mat rawData = Mat(1, recvMsgSize, CV_8UC3, longbuf);
    	    Mat frame = imdecode(rawData, CV_LOAD_IMAGE_COLOR);
	    
	        flip(frame,frame,1);
            
	        if (frame.size().width == 0) 
            {
                cerr << "decode failure!" << endl;
                continue;
            }
            if(frameQueue.size() < 50 && FRAME % 20 != 0)
            {
                frameQueue.push(frame);
            }
            FRAME++;
            //imshow("recv", frame);
            //waitKey(1);
            free(longbuf);
            clock_t next_cycle = clock();
            double duration = (next_cycle - last_cycle) / (double) CLOCKS_PER_SEC;
            //cout << "\teffective FPS:" << (1 / duration) << " \tkbps:" << (PACK_SIZE * total_pack / duration / 1024 * 8) << endl;
            //cout << next_cycle - last_cycle;
            last_cycle = next_cycle;
        }
    } catch (SocketException & e) 
    {
        cerr << e.what() << endl;
        exit(1);
    }
    return 0;
}
