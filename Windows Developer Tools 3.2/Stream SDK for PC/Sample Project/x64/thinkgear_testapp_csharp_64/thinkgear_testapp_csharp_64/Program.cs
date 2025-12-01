// Stream SDK를 사용하여 ThinkGear 헤드셋과 통신하는 C# 예제 프로그램 (64비트 버전)
// ThinkGear는 뇌파 신호를 수집하는 웨어러블 EEG 장치입니다.
// 이 프로그램은 뇌파 데이터를 읽고 필터 설정을 하는 방법을 보여줍니다. (64비트 시스템용)

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using libStreamSDK;  // ThinkGear Stream SDK 라이브러리를 사용하기 위한 네임스페이스

namespace thinkgear_testapp_csharp_64
{
    // ThinkGear Stream SDK 사용 예제 프로그램 (64비트 버전)
    class Program
    {
        // 프로그램의 메인 진입점
        // ThinkGear 헤드셋과의 연결을 설정하고 뇌파 데이터를 수집하는 전체 과정을 수행합니다.
        static void Main(string[] args)
        {
            // NativeThinkgear 클래스의 인스턴스 생성
            // 이 객체를 통해 Stream SDK의 모든 기능을 사용할 수 있습니다.
            NativeThinkgear thinkgear = new NativeThinkgear();

            // ========================================
            // 1단계: Stream SDK 버전 출력
            // ========================================
            // ThinkGear 드라이버의 현재 설치된 버전 정보를 출력합니다.
            // 이는 호환성 확인 및 디버깅에 유용합니다.
            Console.WriteLine("Version: " + NativeThinkgear.TG_GetVersion());

            // ========================================
            // 2단계: 새로운 연결 ID 생성
            // ========================================
            // ThinkGear 장치와의 통신을 위한 고유한 연결 ID를 생성합니다.
            // 이 ID는 이후의 모든 통신에서 사용됩니다.
            int connectionID = NativeThinkgear.TG_GetNewConnectionId();
            Console.WriteLine("Connection ID: " + connectionID);

            // 연결 ID 생성 실패 처리
            if (connectionID < 0)
            {
                Console.WriteLine("ERROR: TG_GetNewConnectionId() returned: " + connectionID);
                return;
            }

            int errCode = 0;
            
            // ========================================
            // 3단계: 로그 파일 설정
            // ========================================
            // 원본 바이트 스트림 로그 파일 설정
            // 실시간으로 받는 모든 원본 데이터를 파일에 저장하여 디버깅과 분석에 사용합니다.
            errCode = NativeThinkgear.TG_SetStreamLog(connectionID, "streamLog.txt");
            Console.WriteLine("errCode for TG_SetStreamLog : " + errCode);
            // 스트림 로그 설정 실패 처리
            if (errCode < 0)
            {
                Console.WriteLine("ERROR: TG_SetStreamLog() returned: " + errCode);
                return;
            }

            // ThinkGear에서 처리된 뇌파 데이터 로그 파일 설정
            // 분석된 뇌파 수치(주의력, 명상도 등)를 파일에 저장합니다.
            errCode = NativeThinkgear.TG_SetDataLog(connectionID, "dataLog.txt");
            Console.WriteLine("errCode for TG_SetDataLog : " + errCode);
            // 데이터 로그 설정 실패 처리
            if (errCode < 0)
            {
                Console.WriteLine("ERROR: TG_SetDataLog() returned: " + errCode);
                return;
            }

            // ========================================
            // 4단계: 헤드셋 연결 (COM 포트)
            // ========================================
            // COM 포트 이름 설정 (ThinkGear 헤드셋이 연결된 포트)
            // Windows에서는 "COM40" 형식으로 포트를 지정합니다.
            string comPortName = "\\\\.\\COM40";

            // 지정된 COM 포트를 통해 ThinkGear 헤드셋과 연결을 시도합니다.
            // 연결 설정:
            // - 보드레이트(통신 속도): 57600 bps
            // - 데이터 형식: ThinkGear 패킷 스트림 형식
            errCode = NativeThinkgear.TG_Connect(connectionID,
                          comPortName,
                          NativeThinkgear.Baudrate.TG_BAUD_57600,
                          NativeThinkgear.SerialDataFormat.TG_STREAM_PACKETS);
            // 연결 실패 처리
            if (errCode < 0)
            {
                Console.WriteLine("ERROR: TG_Connect() returned: " + errCode);
                return;
            }

            // ========================================
            // 5단계: 패킷 읽기 및 뇌파 데이터 추출
            // ========================================
            // 첫 번째 테스트: 수동으로 10개의 패킷을 읽습니다.
            // 각 패킷은 ThinkGear 헤드셋에서 보낸 뇌파 신호 묶음입니다.
            int packetsRead = 0;
            while (packetsRead < 10)
            {
                // COM 포트에서 1개의 데이터 패킷을 읽으려고 시도합니다.
                // 반환값: 1 = 완전한 패킷 읽음, 0 = 아직 완전하지 않음, <0 = 오류
                errCode = NativeThinkgear.TG_ReadPackets(connectionID, 1);
                Console.WriteLine("TG_ReadPackets returned: " + errCode);
                
                // 성공적으로 완전한 패킷을 읽었는지 확인
                if (errCode == 1)
                {
                    packetsRead++;

                    // ========================================
                    // 6단계: 데이터 상태 확인
                    // ========================================
                    // 원본 뇌파 데이터(TG_DATA_RAW)가 새로 업데이트 되었는지 확인합니다.
                    // 0이 아닌 값 = 새로운 데이터가 도착했음을 의미
                    if (NativeThinkgear.TG_GetValueStatus(connectionID, NativeThinkgear.DataType.TG_DATA_RAW) != 0)
                    {
                        // 업데이트된 원본 뇌파 값을 가져와 콘솔에 출력합니다.
                        // 이 값은 12비트 해상도의 실시간 뇌파 신호입니다.
                        Console.WriteLine("New RAW value: : " + (int)NativeThinkgear.TG_GetValue(connectionID, NativeThinkgear.DataType.TG_DATA_RAW));

                    } /* 데이터 업데이트 확인 종료 */

                } /* 패킷 읽기 성공 종료 */

            } /* 10개 패킷 읽기 종료 */

            // ========================================
            // 7단계: 자동 읽기 모드 활성화
            // ========================================
            // 자동 읽기 모드 테스트 시작 안내
            Console.WriteLine("auto read test begin:");

            // 자동 읽기 모드를 활성화합니다 (매개변수 1 = 활성화)
            // 이 모드에서는 백그라운드에서 자동으로 데이터를 계속 읽습니다.
            errCode = NativeThinkgear.TG_EnableAutoRead(connectionID, 1);
            if (errCode == 0)
            {
                packetsRead = 0;
                
                // ========================================
                // 8단계: 필터 타입 설정
                // ========================================
                // MWM15_FILTER_TYPE_60HZ 필터 설정: 60Hz 전원 간섭 제거
                // 이 필터는 전원선 간섭(60Hz)을 제거하여 신호 품질을 개선합니다.
                // (이 버전은 60Hz 필터를 사용합니다 - 미국 등에서 사용)
                errCode = NativeThinkgear.MWM15_setFilterType(connectionID, NativeThinkgear.FilterType.MWM15_FILTER_TYPE_60HZ);
                Console.WriteLine("MWM15_setFilterType called: " + errCode);
                
                // 2000개 패킷을 처리합니다 (대량 데이터 테스트 및 시간 측정용)
                while (packetsRead < 2000)
                {
                    // 새로운 원본 뇌파 데이터가 도착했는지 확인
                    if (NativeThinkgear.TG_GetValueStatus(connectionID, NativeThinkgear.DataType.TG_DATA_RAW) != 0)
                    {
                        // ========================================
                        // 8-1단계: 필터 타입 확인
                        // ========================================
                        // 필터 타입 설정 결과를 확인
                        if (NativeThinkgear.TG_GetValueStatus(connectionID, NativeThinkgear.DataType.MWM15_DATA_FILTER_TYPE) != 0)
                        {
                            Console.WriteLine(" Find Filter Type:  " + NativeThinkgear.TG_GetValue(connectionID, NativeThinkgear.DataType.MWM15_DATA_FILTER_TYPE) + " index: " + packetsRead);
                        }

                        // 업데이트된 원본 뇌파 값을 가져옵니다.
                        // 이 값은 12비트 해상도의 실시간 뇌파 신호입니다.
                        NativeThinkgear.TG_GetValue(connectionID, NativeThinkgear.DataType.TG_DATA_RAW);
                       
                        packetsRead++;

                        // 약 1초 간격으로(512 패킷마다) 필터 설정을 다시 확인합니다.
                        // 800번째(약 1.56초)와 1600번째(약 3.12초) 패킷에서 확인
                        if (packetsRead == 800 || packetsRead == 1600)
                        {
                            errCode = NativeThinkgear.MWM15_getFilterType(connectionID);
                            Console.WriteLine(" MWM15_getFilterType called: " + errCode);
                        }

                    }
                }

                // ========================================
                // 9단계: 자동 읽기 모드 종료
                // ========================================
                // 자동 읽기 모드를 비활성화합니다 (매개변수 0 = 비활성화)
                errCode = NativeThinkgear.TG_EnableAutoRead(connectionID, 0);
                Console.WriteLine("auto read test stoped: "+ errCode);
            }
            else
            {
                Console.WriteLine("auto read test failed: " + errCode);
            }

            // ========================================
            // 10단계: 연결 종료 및 정리
            // ========================================
            // ThinkGear 헤드셋과의 연결을 종료합니다.
            NativeThinkgear.TG_Disconnect(connectionID);

            // 할당된 연결 ID와 관련된 리소스를 정리합니다.
            // 이 작업은 메모리 누수를 방지하고 시스템 리소스를 해제합니다.
            NativeThinkgear.TG_FreeConnection(connectionID);

            // 프로그램 종료
            // Enter 키를 눌러야 콘솔 창이 닫힙니다.
            Console.ReadLine();

        }
    }
}
