#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "thinkgear.h"

/**
 * Prompts and waits for the user to press ENTER.
 * 사용자가 ENTER 키를 누를 때까지 대기하는 함수
 * 반환값: 없음
 */
void
wait() {
    printf( "\n" );
    printf( "Press the ENTER key...\n" );
    fflush( stdout );
    getc( stdin );
}

/**
 * Program which prints ThinkGear Raw Wave Values to stdout.
 * ThinkGear SDK를 사용하여 뇌파 헤드셋에서 데이터를 수신하는 메인 프로그램
 * 목적: Stream SDK를 사용하여 뇌파 신호를 읽고 처리하는 방법을 보여줌
 * 반환값: 성공시 EXIT_SUCCESS, 실패시 EXIT_FAILURE
 */
int
main( void ) {
    
    char *comPortName  = NULL;
    int   dllVersion   = 0;
    int   connectionId = 0;
    int   packetsRead  = 0;
    int   errCode      = 0;
    
    double secondsToRun = 0;
    time_t startTime    = 0;
    time_t currTime     = 0;
    char  *currTimeStr  = NULL;
	int set_filter_flag = 0;
	int count = 0;
    
    /* SDK 버전 확인 - Stream SDK의 버전 정보를 가져옴 */
    dllVersion = TG_GetVersion();
    printf( "Stream SDK for PC version: %d\n", dllVersion );
    
    /* 새 연결 ID 생성 - 헤드셋과 통신하기 위한 고유한 연결 ID 할당 */
    connectionId = TG_GetNewConnectionId();
    if( connectionId < 0 ) {
        fprintf( stderr, "ERROR: TG_GetNewConnectionId() returned %d.\n",
                connectionId );
        wait();
        exit( EXIT_FAILURE );
    }
    
    /* 뇌파 신호를 파일에 기록 - 원본 바이트 스트림 데이터를 streamLog.txt 파일에 저장 */
    errCode = TG_SetStreamLog( connectionId, "streamLog.txt" );
    if( errCode < 0 ) {
        fprintf( stderr, "ERROR: TG_SetStreamLog() returned %d.\n", errCode );
        wait();
        exit( EXIT_FAILURE );
    }
    
    /* 데이터 로그 파일 설정 - ThinkGear 값 데이터를 dataLog.txt 파일에 저장 */
    errCode = TG_SetDataLog( connectionId, "dataLog.txt" );
    if( errCode < 0 ) {
        fprintf( stderr, "ERROR: TG_SetDataLog() returned %d.\n", errCode );
        wait();
        exit( EXIT_FAILURE );
    }
    
    /* Attempt to connect the connection ID handle to serial port "COM5" */
    /* NOTE: On Windows, COM10 and higher must be preceded by \\.\, as in
     *       "\\\\.\\COM12" (must escape backslashes in strings).  COM9
     *       and lower do not require the \\.\, but are allowed to include
     *       them.  On Mac OS X, COM ports are named like
     *       "/dev/tty.MindSet-DevB-1".
     */
    comPortName = "\\\\.\\COM40";
    /* 헤드셋에 연결 - 지정된 COM 포트를 통해 뇌파 헤드셋과 연결 수립 */
    errCode = TG_Connect( connectionId,
                         comPortName,
                         TG_BAUD_57600,
                         TG_STREAM_PACKETS );
    if( errCode < 0 ) {
        fprintf( stderr, "ERROR: TG_Connect() returned %d.\n", errCode );
        wait();
        exit( EXIT_FAILURE );
    }
    
    /* Keep reading ThinkGear Packets from the connection for 5 seconds... */
    secondsToRun = 5;
    startTime = time( NULL );
    while( difftime(time(NULL), startTime) < secondsToRun ) {
        
        /* 뇌파 데이터 수신 - 연결에서 이용 가능한 모든 패킷을 읽음 */
        do {
            
            /* 뇌파 데이터 수신 - 연결에서 한 개의 패킷 읽기 */
            packetsRead = TG_ReadPackets( connectionId, 1 );
            
            /* If TG_ReadPackets() was able to read a Packet of data... */
            if( packetsRead == 1 ) {
                
                /* 데이터 업데이트 확인 - 새로운 뇌파 신호 값이 업데이트되었는지 확인 */
                if( TG_GetValueStatus(connectionId, TG_DATA_RAW) != 0 ) {
                    
                    /* Get the current time as a string */
                    currTime = time( NULL );
        			currTimeStr = ctime( &currTime );
                    
                    /* 데이터 값 추출 - 뇌파 신호 값을 읽어서 출력 */
                    fprintf( stdout, "%s: raw: %d\n", currTimeStr,
                            (int)TG_GetValue(connectionId, TG_DATA_RAW) );
                    fflush( stdout );
                    
                } /* end "If Packet contained a raw wave value..." */
                
            } /* end "If TG_ReadPackets() was able to read a Packet..." */
            
        } while( packetsRead > 0 ); /* Keep looping until all Packets read */
        
    } /* end "Keep reading ThinkGear Packets for 5 seconds..." */

	/* 자동 모드로 데이터 수신 - 자동 읽기 모드를 활성화하여 지속적으로 데이터 수신 */
	printf("auto read test begin:\n");
	fflush(stdin);
	errCode = TG_EnableAutoRead(connectionId,1);
	if(errCode == 0){
		packetsRead =0;
        errCode = MWM15_setFilterType(connectionId,MWM15_FILTER_TYPE_50HZ);//MWM15_FILTER_TYPE_60HZ
		printf("MWM15_setFilterType: %d\n",errCode);
		while(packetsRead <3000){
			/* If raw value has been updated ... */
            if( TG_GetValueStatus(connectionId, TG_DATA_RAW) != 0 ) {

                /* Get and print out the updated raw value */
                //printf( "%d ",
                 //        (int)TG_GetValue(connectionId, TG_DATA_RAW) );
                //fflush( stdout );
				if(packetsRead % 5 == 0){ // too much stdout operation will lose some data
                printf(  " %d ",
                         (int)TG_GetValue(connectionId, TG_DATA_RAW) );
				}else{
					TG_GetValue(connectionId, TG_DATA_RAW);
				}
				packetsRead ++;

	            //wait for a while to call MWM15_getFilterType
				if(packetsRead == 800 || packetsRead == 1600){// at lease 1s after MWM15_setFilterType cmd
					set_filter_flag ++;
					errCode = MWM15_getFilterType(connectionId);

					printf(  " \nMWM15_getFilterType   result: %d  index: %d\n",errCode,packetsRead );

				}

            }

			if(TG_GetValueStatus(connectionId, MWM15_DATA_FILTER_TYPE) != 0 ) {

                /* Get and print out the updated raw value */
				printf(  "\n#### @@@ Find Filter type: %d  set_filter_flag: %d  index: %d\n",
                         (int)TG_GetValue(connectionId, MWM15_DATA_FILTER_TYPE), set_filter_flag,packetsRead);
				break;
                
            } 
             
		}

		/* 자동 읽기 모드 중지 */
		errCode = TG_EnableAutoRead(connectionId,0);
		printf("auto read test stoped: %d \n",errCode);
	}else{
		printf("auto read test failed: %d \n",errCode);
	}

	/* 연결 종료 및 정리 - 헤드셋과의 연결 종료 */
	TG_Disconnect(connectionId);
    
    /* 메모리 정리 - 연결 ID 관련 리소스 해제 */
    TG_FreeConnection( connectionId );
    
    /* End program */
    wait();
    return( EXIT_SUCCESS );
}
