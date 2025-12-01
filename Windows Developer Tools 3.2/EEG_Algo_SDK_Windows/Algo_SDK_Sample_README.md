# Algo SDK 샘플 코드 설명

## 📌 Algo SDK란?

**Algo SDK**는 Stream SDK에서 받은 뇌파 신호를 **분석하여 의미 있는 정보**로 변환합니다.

쉽게 말해서:
- **Stream SDK**: 뇌파 신호를 받아옴 (원본 데이터)
- **Algo SDK**: 그 신호를 분석하여 집중도, 명상도 등으로 변환 (처리된 데이터)

---

## 🎯 샘플 프로젝트 위치

```
EEG_Algo_SDK_Windows
└── Algo SDK Sample
    └── Algo SDK Sample
        ├── Algo SDK Sample.cpp      ← 메인 코드
        ├── Algo SDK Sample.h
        ├── Algo SDK Sample.sln      ← Visual Studio 솔루션
        ├── Algo SDK Sample.vcxproj
        ├── ME_Easy_RawData.txt      ← 테스트 데이터
        └── ME_Easy_Baseline.txt     ← 기준 데이터
```

---

## 🔑 Algo SDK 핵심 함수

```cpp
// 1. SDK 초기화 (필수)
NSK_ALGO_Init(type, dataPath);

// 2. 콜백 함수 등록
NSK_ALGO_RegisterCallback(callbackFunc, userData);

// 3. 분석 시작
NSK_ALGO_Start(bBaseline);

// 4. 데이터 전송
NSK_ALGO_DataStream(dataType, data, length);

// 5. 일시 중지
NSK_ALGO_Pause();

// 6. 분석 재개
NSK_ALGO_Start(false);

// 7. 분석 중지
NSK_ALGO_Stop();

// 8. SDK 종료
NSK_ALGO_Uninit();

// 9. 버전 정보
NSK_ALGO_SdkVersion();      // SDK 버전
NSK_ALGO_AlgoVersion(type); // 알고리즘 버전
```

---

## 🎛️ 지원하는 알고리즘 (Algorithm Types)

```cpp
NSK_ALGO_TYPE_ATT      // 집중도 (Attention)
NSK_ALGO_TYPE_MED      // 명상도 (Meditation)
NSK_ALGO_TYPE_BLINK    // 안구 깜빡임 (Eye Blink)
NSK_ALGO_TYPE_BP       // 뇌파 대역별 강도 (EEG Bandpower)
// NSK_ALGO_TYPE_AP    // 만족도 (Appreciation) - 별도 라이선스
// NSK_ALGO_TYPE_ME    // 정신적 노력 (Mental Effort) - 별도 라이선스
```

---

## 📊 데이터 타입 (Data Stream Types)

```cpp
NSK_ALGO_DATA_TYPE_EEG      // 뇌파 신호 (Raw EEG)
NSK_ALGO_DATA_TYPE_PQ       // 신호 품질 (Poor Quality)
NSK_ALGO_DATA_TYPE_ATT      // 집중도
NSK_ALGO_DATA_TYPE_MED      // 명상도
NSK_ALGO_DATA_TYPE_BULK_EEG // 대량 뇌파 데이터 (오프라인)
```

---

## 🔄 작동 원리

### **1단계: SDK 초기화**
```cpp
// 사용할 알고리즘 선택 (비트 OR 연산)
int algorithms = NSK_ALGO_TYPE_ATT | NSK_ALGO_TYPE_MED | NSK_ALGO_TYPE_BLINK;

// SDK 초기화
NSK_ALGO_Init(algorithms, "C:\\MyData");
```

### **2단계: 콜백 함수 등록**
```cpp
// 분석 결과가 나올 때 호출될 함수
void MyCallback(sNSK_ALGO_CB_PARAM param) {
    switch (param.cbType) {
        case NSK_ALGO_CB_TYPE_STATE:     // 상태 변경
            // 상태 처리
            break;
        case NSK_ALGO_CB_TYPE_SIGNAL_LEVEL:  // 신호 품질 변경
            // 품질 처리
            break;
        case NSK_ALGO_CB_TYPE_ALGO:      // 알고리즘 결과
            // 결과 처리
            break;
    }
}

NSK_ALGO_RegisterCallback(&MyCallback, userData);
```

### **3단계: 분석 시작**
```cpp
NSK_ALGO_Start(false);  // false: 정상 분석, true: 기준 데이터 수집
```

### **4단계: 데이터 전송 (메인 루프)**
```cpp
// 실시간: Stream SDK에서 받은 데이터 전송
while (true) {
    short eegData[512];  // 512개의 샘플
    
    // Stream SDK에서 데이터 읽기
    // ...
    
    // Algo SDK로 전송
    NSK_ALGO_DataStream(NSK_ALGO_DATA_TYPE_EEG, eegData, 512);
}
```

### **5단계: 분석 중지 및 종료**
```cpp
NSK_ALGO_Stop();
NSK_ALGO_Uninit();
```

---

## 🎯 콜백 함수 이해하기

콜백 함수는 **분석 결과가 나올 때 자동으로 호출**됩니다.

### **상태 콜백 (State Callback)**
```cpp
case NSK_ALGO_CB_TYPE_STATE:
    eNSK_ALGO_STATE state = param.param.state & NSK_ALGO_STATE_MASK;
    
    switch (state) {
    case NSK_ALGO_STATE_INITED:           // 초기화 완료
    case NSK_ALGO_STATE_RUNNING:          // 실행 중
    case NSK_ALGO_STATE_PAUSE:            // 일시 중지
    case NSK_ALGO_STATE_STOP:             // 정지
    case NSK_ALGO_STATE_UNINTIED:         // 미초기화
    case NSK_ALGO_STATE_COLLECTING_BASELINE_DATA:  // 기준 데이터 수집 중
    case NSK_ALGO_STATE_ANALYSING_BULK_DATA:       // 오프라인 분석 중
    }
    break;
```

### **신호 품질 콜백**
```cpp
case NSK_ALGO_CB_TYPE_SIGNAL_LEVEL:
    eNSK_ALGO_SIGNAL_QUALITY sq = param.param.sq;
    
    switch (sq) {
    case NSK_ALGO_SQ_GOOD:           // 좋음
    case NSK_ALGO_SQ_MEDIUM:         // 중간
    case NSK_ALGO_SQ_POOR:           // 나쁨
    case NSK_ALGO_SQ_NOT_DETECTED:   // 감지 안 됨
    }
    break;
```

### **알고리즘 결과 콜백**
```cpp
case NSK_ALGO_CB_TYPE_ALGO:
    
    // 결과 타입 확인
    switch (param.param.index.type) {
    case NSK_ALGO_TYPE_ATT:  // 집중도
        int attention = param.param.index.value.group.att_index;
        printf("집중도: %d\n", attention);  // 0-100
        break;
        
    case NSK_ALGO_TYPE_MED:  // 명상도
        int meditation = param.param.index.value.group.med_index;
        printf("명상도: %d\n", meditation);  // 0-100
        break;
        
    case NSK_ALGO_TYPE_BLINK:  // 안구 깜빡임
        int blink = param.param.index.value.group.eye_blink_strength;
        printf("깜빡임 강도: %d\n", blink);  // 0-255
        break;
        
    case NSK_ALGO_TYPE_BP:  // 뇌파 대역
        float delta = param.param.index.value.group.bp_index.delta_power;
        float theta = param.param.index.value.group.bp_index.theta_power;
        float alpha = param.param.index.value.group.bp_index.alpha_power;
        float beta = param.param.index.value.group.bp_index.beta_power;
        float gamma = param.param.index.value.group.bp_index.gamma_power;
        printf("Delta: %.2f, Theta: %.2f, Alpha: %.2f\n", delta, theta, alpha);
        break;
    }
    break;
```

---

## 🖥️ 샘플 코드 구조

### **GUI 컴포넌트**
```
┌─────────────────────────────────────────────┐
│ [Init]  [Version]  [Med] [Att] [BP]         │  <- 버튼과 체크박스
│                                             │
│ [Start] [Data]     [Blink] [AP]             │
│                                             │
│ [Stop]              [ME] [ME2] [F] [F2]     │
│                                             │
│ ┌───────────────────────────────────────┐  │
│ │  분석 결과 출력 (Real-time Output)    │  │  <- 결과 표시
│ │  [ATT = 75]                           │  │
│ │  [MED = 42]                           │  │
│ │  [Eye blink strength = 128]           │  │
│ └───────────────────────────────────────┘  │
│                                             │
│ State: Running  [Signal: Good]              │  <- 상태 표시
└─────────────────────────────────────────────┘
```

### **주요 변수**

| 변수 | 설명 |
|-----|------|
| `bInited` | SDK 초기화 완료 여부 |
| `bRunning` | 분석 실행 중 여부 |
| `lSelectedAlgos` | 선택된 알고리즘 (비트 플래그) |
| `connectionId` | 헤드셋 연결 ID |
| `hThreadHandle` | 데이터 읽기 스레드 |

---

## 🔄 실행 흐름

```
프로그램 시작
    ↓
DLL 로드 (AlgoSdkDll.dll)
    ↓
함수 포인터 설정
    ↓
헤드셋 연결 (COM 포트)
    ↓
데이터 읽기 스레드 생성
    ↓
사용자 버튼 클릭 대기
    ↓
[Init] 버튼 클릭
    ↓
알고리즘 선택 확인
    ↓
NSK_ALGO_Init() 호출
    ↓
[Start] 버튼 클릭
    ↓
NSK_ALGO_Start() 호출
    ↓
데이터 수신 및 분석 (스레드)
    ↓
콜백 함수 호출 (결과 출력)
    ↓
[Stop] 버튼 클릭
    ↓
NSK_ALGO_Stop() 호출
    ↓
[Init] 버튼 (Uninit 표시) 클릭
    ↓
NSK_ALGO_Uninit() 호출
    ↓
프로그램 종료
```

---

## 📌 온라인 vs 오프라인 모드

### **온라인 모드 (실시간)**
```cpp
// 헤드셋에서 실시간으로 데이터 받기
NSK_ALGO_Start(false);  // false = 실시간 모드

// ThreadReadPacket()에서 계속 데이터 전송
NSK_ALGO_DataStream(NSK_ALGO_DATA_TYPE_EEG, rawData, 512);
```

### **오프라인 모드 (파일 분석)**
```cpp
// 저장된 파일에서 데이터 읽기
NSK_ALGO_Start(false);

// 파일의 모든 데이터를 한 번에 전송
NSK_ALGO_DataStream(NSK_ALGO_DATA_TYPE_BULK_EEG, raw_data, total_count);

// 자동으로 분석 완료 후 콜백 호출
```

---

## 🚀 테스트 데이터

프로젝트 폴더에 포함된 테스트 파일:

- **ME_Easy_RawData.txt**: 뇌파 신호 샘플 데이터
- **ME_Easy_Baseline.txt**: 기준 데이터 (개인별 보정용)

### **[Data] 버튼으로 테스트**
1. [Init] 버튼으로 SDK 초기화
2. [Data] 버튼을 클릭하여 테스트 파일 로드
3. 자동으로 오프라인 분석 실행
4. 콜백 함수에서 결과 출력

---

## ⚠️ 주의사항

1. **COM 포트**: `MWM_COM` 매크로에서 올바른 포트 설정 (기본값: COM7)
2. **DLL 버전**: 32bit/64bit에 맞는 DLL 사용
3. **기준 데이터**: 첫 사용 시 기준 데이터 수집 필요 (2-3초)
4. **메모리**: 대량 데이터 분석 시 메모리 관리 중요

---

## 🎓 학습 순서

1. **기본 구조 이해**
   - SDK 초기화 함수
   - 콜백 함수 개념

2. **알고리즘 선택**
   - 필요한 알고리즘만 활성화
   - 비트 플래그 조합

3. **데이터 전송**
   - NSK_ALGO_DataStream() 사용법
   - 배열 크기와 데이터 길이

4. **결과 처리**
   - 콜백 함수 구현
   - 결과 해석

5. **실시간 vs 오프라인**
   - 양쪽 모드 테스트
   - 장단점 이해

---

## 📝 코드 예제

### **예제 1: SDK 초기화**

```cpp
// 현재 디렉토리 가져오기
wchar_t ReadBuffer[1024] = {0};
char readBuf[1024] = {0};
GetCurrentDirectory(1024, ReadBuffer);
wcstombs_s(NULL, readBuf, ReadBuffer, 1024);

// 사용할 알고리즘 선택
int lSelectedAlgos = NSK_ALGO_TYPE_ATT | NSK_ALGO_TYPE_MED;

// 콜백 함수 등록
NSK_ALGO_RegisterCallback(&AlgoSdkCallback, hWnd);

// SDK 초기화
eNSK_ALGO_RET ret = NSK_ALGO_Init(lSelectedAlgos, readBuf);
if (ret == NSK_ALGO_RET_SUCCESS) {
    printf("초기화 성공!\n");
} else {
    printf("초기화 실패: %d\n", ret);
}
```

### **예제 2: 분석 시작 및 일시 중지**

```cpp
// 분석 시작
eNSK_ALGO_RET ret = NSK_ALGO_Start(false);

// 나중에 일시 중지
NSK_ALGO_Pause();

// 다시 시작
NSK_ALGO_Start(false);

// 완전히 중지
NSK_ALGO_Stop();
```

### **예제 3: 기준 데이터 수집**

```cpp
// 기준 데이터 수집 모드로 시작
NSK_ALGO_Start(true);  // true = 기준 데이터 수집 모드

// 2-3초 동안 데이터 수집
// 자동으로 기준 데이터 저장

NSK_ALGO_Stop();
```

---

## 📚 추가 리소스

- Algo SDK Sample.cpp 파일의 주석 참고
- `NSK_Algo.h` 헤더 파일에서 함수 정의 확인
- `NSK_Algo_Defines.h`에서 상수 정의 확인

Happy Coding! 🚀
