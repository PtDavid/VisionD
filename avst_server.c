#include <stdio.h>
#include <ti_vsys.h>
#include <ti_vsys_priv.h>
#include <unistd.h>
#include <pthread.h>

#include <osa.h>
#include <osa_thr.h>
#include <osa_sem.h>
#include "common.h"
#include "Encode_interface.h"
#include "sql_operations.h"
#include "gpio_lib.h"
#include "business.h"
#include "ti_vsys.h"
#include "ti_vcap.h"
#include "ti_venc.h"
#include "ti_vdec.h"
#include "ti_vdis.h"
#include "ti_vdis_common_def.h"
#include "ti_audio.h"
#include "demos/graphic/graphic.h"
#include "avst/curl/include/curl/curl.h"
#include "avst/avst_server/demo_vcap_venc_vdis.h"
#include <sys/stat.h>  
#include "serial_app.h"
#include "tcp_app.h"


VcapVenc_Ctrl gVcapVenc_ctrl;
Demo_Info gDemo_info;
FSM_STATES gVisionFSMState = IDLE;

//add by yangyong 20170327
#if 1
size_t getcontentlengthfunc(void *ptr, size_t size, size_t nmemb, void *stream)   
{  
    int r;  
    long len = 0;  
#ifdef windows 
		 /* _snscanf() is Win32 specific */ 
    r = _snscanf(ptr, size * nmemb, "Content-Length: %ld\n", &len);
#else  
    r = sscanf((const char*)ptr, "Content-Length: %ld\n", &len);  
#endif 
    if (r) /* Microsoft: we don't read the specs */  
        *((long *) stream) = len;  
    return size * nmemb;  
}

  
/* discard downloaded data */  
size_t discardfunc(void *ptr, size_t size, size_t nmemb, void *stream)   
{  
    return size * nmemb;  
} 

 
//write data to upload  
size_t writefunc(void *ptr, size_t size, size_t nmemb, void *stream)  
{  
    return fwrite(ptr, size, nmemb, (FILE*)stream);  
}  


/* read data to upload */  
size_t readfunc(void *ptr, size_t size, size_t nmemb, void *stream)  
{  
    FILE *f = (FILE*)stream;  
    size_t n;  
    if (ferror(f))  
        return CURL_READFUNC_ABORT;  
    n = fread(ptr, size, nmemb, f) * size;  
    return n;  
}  

//�ϴ�
int upload(CURL *curlhandle, const char * remotepath, const char * localpath, long timeout, long tries)  
{  
    FILE *f;  
    long uploaded_len = 0;  
    CURLcode r = CURLE_GOT_NOTHING;  
    int c;  
    f = fopen(localpath, "rb+"); 
    if (f == NULL) {  
        perror(NULL);
        return 0;  
    }  
    curl_easy_setopt(curlhandle, CURLOPT_UPLOAD, 1L);  
    curl_easy_setopt(curlhandle, CURLOPT_URL, remotepath);  
    curl_easy_setopt(curlhandle, CURLOPT_USERPWD, "avst:avst"); //"" avst:avst
    if (timeout)  
        curl_easy_setopt(curlhandle, CURLOPT_FTP_RESPONSE_TIMEOUT, timeout);  
    curl_easy_setopt(curlhandle, CURLOPT_HEADERFUNCTION, getcontentlengthfunc);  
    curl_easy_setopt(curlhandle, CURLOPT_HEADERDATA, &uploaded_len);  
    
    curl_easy_setopt(curlhandle, CURLOPT_WRITEFUNCTION, discardfunc);  
    curl_easy_setopt(curlhandle, CURLOPT_READFUNCTION, readfunc);  
    curl_easy_setopt(curlhandle, CURLOPT_READDATA, f);  
    curl_easy_setopt(curlhandle, CURLOPT_FTPPORT, "-"); /* disable passive mode */  
    curl_easy_setopt(curlhandle, CURLOPT_FTP_CREATE_MISSING_DIRS, 1L);  
    curl_easy_setopt(curlhandle, CURLOPT_VERBOSE, 1L); 

    for (c = 0; (r != CURLE_OK) && (c < tries); c++) {  
        if (c) { /* yes */   
            curl_easy_setopt(curlhandle, CURLOPT_NOBODY, 1L);  
            curl_easy_setopt(curlhandle, CURLOPT_HEADER, 1L);  
            r = curl_easy_perform(curlhandle);  
            if (r != CURLE_OK)  
                continue;  
            curl_easy_setopt(curlhandle, CURLOPT_NOBODY, 0L);  
            curl_easy_setopt(curlhandle, CURLOPT_HEADER, 0L);  
            fseek(f, uploaded_len, SEEK_SET);  
            curl_easy_setopt(curlhandle, CURLOPT_APPEND, 1L);  
        }  
        else { /* no */  
            curl_easy_setopt(curlhandle, CURLOPT_APPEND, 0L);  
        }  
        r = curl_easy_perform(curlhandle);  
    }  
    fclose(f);  
    if (r == CURLE_OK)  {
        return 1;  
    }
    else {  
        fprintf(stderr, "%s\n", curl_easy_strerror(r));  
        return 0;  
    }  
}


#endif
//add by yangyong 20170327
#if 1
	void * check_img(void *arg)
	{
		sleep(1);
		printf("start a thread!\n");
    CURL *curlhandle = NULL;  
    CURL *curldwn = NULL;  
    curl_global_init(CURL_GLOBAL_ALL);  
    curlhandle = curl_easy_init();  
    curldwn = curl_easy_init();		
	while(1)
	{
		if(access("/workspace/IMG/encode_so.MJPEG",F_OK)==0)
		{ 
			// 192.168.1.111 link's ftp
			upload(curlhandle, "ftp://192.168.1.111/VisionCam.JPEG", "/workspace/IMG/encode_so.MJPEG", 1, 1);
			printf("Added by Xinyi: ftp upload done.\n");
			system("rm -rf /workspace/IMG/encode_so.MJPEG");
		}
		else
		{
			printf("waiting for Img\n");
		}
			sleep(2);
		}
//			curl_easy_cleanup(curlhandle);
//			curl_easy_cleanup(curldwn);
//			curl_global_cleanup();
//			pthread_exit(NULL);
	}
#endif

#if 0
// The function returns the corresponding GPIO input, returns -1 if error occurs.
int read_gpio_value(int gpio_number, int fd) {
	int res = -1, value = -1;
	if (gpio_number == 0) {
		res = GetInput0_Status(fd, &value);
	} else if (gpio_number == 1) {
		res = GetInput1_Status(fd, &value);
	} else {
		printf("GPIO input port number not supported!\n");
		return -1;
	}
	if (-1 == res) {
		printf("Get Input%d status failed\n", gpio_number);
		return -1;
	}

	return value;
  }

  void * check_state(void *arg)
	{
		sleep(1);
		printf("start a thread to update machine state!\n");

		//GPIO操作句柄, return value: 0 成功，-1 失败
		int fd;
		int res = gpio_open(&fd);
		if(-1 == res)
		{
			printf("gpio_open failed!\n");
		}

		while(1)
		{
			int change0 = read_gpio_value(0, fd);
			int change1 = read_gpio_value(1, fd);
			printf("GPIO INPUT VALUES %d, %d\n", change0, change1);
			if (change0 == -1 || change1 == -1) {
				printf("gpio io status read error!\n");
				continue;
			}

			int state;
			int sql_status = GetSQLState(&state);
			if (sql_status == -1) {
				printf("Read database state failure!");
				continue;
			}

			gVisionFSMState = (FSM_STATES)state;

			// State machine
			if (change0 == 1 && change1 == 1) {
			    FSM_Event_Handler_Function((FSM_EVENTS) CLEAR_BASE);
			} else if (change0 == 1) {
		        FSM_Event_Handler_Function((FSM_EVENTS) ENABLE_BASE);
			} else if (change1 == 1) {
		        FSM_Event_Handler_Function((FSM_EVENTS) ENABLE_TBS);
			}

			sql_status = UpdateSQLState((int)gVisionFSMState);
			if (sql_status == -1) {
				printf("Update database state failure!");
				continue;
			}

			usleep(100000);
		}
	}
#endif


int main()
{

//	// Added by devin
//    Undistort_Map uv;
//
//	getUndistortMatrix(uv.u, uv.v);
//	printf("go 1111\r\n");
//    uv.u[0]=122, uv.v[0]=144;

//	  ////////////////////////////test
//
//	  pos* i;
//  printf("firstadd i=%p\r\n",&i);
//	  i=(pos *)malloc(sizeof(i)*1600*400);
//	  if(i==NULL)
//	  {
//		  printf("require failed\n\r");
//	  }
//	  else
//	  {
//		  printf("success:");
//
//	  }
//
//    struct axy{
//    	float x;
//    	float y;
//    }*a;
//    int k;
//    for(k=0;k<100;k++)
//    {
//    float *a=(float*)malloc(sizeof(float)*1920000);
//    printf("k=%d\r\n",k);
//    if(a)
//    {
//    	printf("success ");
//    }
//    else
//    {
//    	printf("Not Enough memory!\n");
//    }
//    }

//
//
//
//
//	  /////////////////////////////////



	int wait=0;
    //led_open();
    serial_thread_start();
    tcp_link_host_port_setting("192.168.1.111",0x3000);
    	tcp_link_thread_start();
    business_thread_start();


	VSYS_PARAMS_S vsysParams;
	VCAP_PARAMS_S vcapParams;
	VENC_PARAMS_S vencParams;
	VDEC_PARAMS_S vdecParams;
	VDIS_PARAMS_S vdisParams;

	Vsys_params_init(&vsysParams);
	Vcap_params_init(&vcapParams);
	Venc_params_init(&vencParams);
	Vdec_params_init(&vdecParams);
	Vdis_params_init(&vdisParams);


	vcapParams.numChn          = 4;
	vencParams.numPrimaryChn   = 4;
	vencParams.numSecondaryChn = 0;
	vdecParams.numChn          = 0;

	vdisParams.numChannels = 1;
	vdisParams.enableConfigExtThsFilter = TRUE; //FALSE; //
	vdisParams.enableConfigExtVideoEncoder = FALSE;
	//vdisParams.tiedDevicesMask = VDIS_VENC_HDMI | VDIS_VENC_HDCOMP;
	vdisParams.deviceParams[VDIS_DEV_DVO2].enable = FALSE;
	vdisParams.deviceParams[VDIS_DEV_SD].enable = FALSE;//TRUE; //
	vdisParams.deviceParams[VDIS_DEV_HDCOMP].enable = FALSE;//TRUE; //
	vdisParams.deviceParams[VDIS_DEV_HDMI].enable = TRUE;
	//vdisParams.deviceParams[VDIS_DEV_HDMI].resolution	= VSYS_STD_1080P_60;
	vdisParams.deviceParams[VDIS_DEV_HDMI].outputInfo.vencNodeNum = VDIS_VENC_HDMI;
	
	vsysParams.numChs  = 4;
	vsysParams.systemUseCase = VSYS_USECASE_MULTICHN_PROGRESSIVE_VCAP_VDIS_VENC_VDEC;
	vsysParams.enableSecondaryOut = FALSE;
	vsysParams.enableNsf	   = FALSE;
	vsysParams.enableCapture = TRUE;
	vsysParams.enableNullSrc = FALSE;

	vsysParams.numDisplays	 = 1;
	vsysParams.enableAVsync  = TRUE;
	Vsys_enableFastUsecaseSwitch(FALSE);

	Vsys_init(&vsysParams);
	Vcap_init(&vcapParams);
	Venc_init(&vencParams);
	Vdis_init(&vdisParams);

#if 0
	//TODO: transfer params to DSP

	System_linkControl(
			SYSTEM_LINK_ID_DSP,
			SYSTEM_COMMON_CMD_SET_DSP_VALUE,
			&uv,
			sizeof(uv),
			TRUE
			);
#endif
	printf("go 2222\r\n");

	// Added by Xinyi 20180405
	int useCase = 2;
    VcapVenc_ipcFramesCreate(useCase);
    VcapVenc_ipcFramesInSetCbInfo();

    Vsys_configureDisplay();

//add by yangyong 20170322 
#if 1
	Media_G2_bitsWriteCreate();//�����źź���
#endif

//add by yangyong 20170327
#if 1
	pthread_t ID;
	pthread_attr_t attr;
	pthread_attr_init(&attr);//��ʼ���߳�����
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);//��������
	pthread_create(&ID,&attr,check_img,NULL);//�����߳�
	pthread_attr_destroy(&attr);//
#endif


//add by xinyi 201800515
#if 0
	pthread_t ID_state;
	pthread_create(&ID_state,NULL,check_state,NULL);//�����߳�
#endif

#if 1 
	  Vsys_create();
	#if 1
		while (wait<2)
		{
//			printf("Added by Xinyi: wait = %d minutes.\n", wait);
//			wait++;
//			sleep(1);
		}

	#endif
	    VcapVenc_ipcFramesStop();

	    //VcapVenc_bitsWriteDelete();
	    VcapVenc_ipcFramesDelete();

	  Vsys_delete();
	 // sqlite3_close(dp);
#endif

	  serial_thread_stop();
	  tcp_link_thread_stop();
     business_thread_stop();
    // led_close();

	return 0;

}
