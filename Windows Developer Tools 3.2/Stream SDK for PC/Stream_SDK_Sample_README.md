# Stream SDK 샘플 코드 설명

## 📌 Stream SDK란?

**Stream SDK**는 MindWave Mobile 헤드셋과 Windows 컴퓨터를 연결하여 **뇌파 신호를 직접 받아오는** 기능을 제공합니다.

쉽게 말해서, 헤드셋에서 오는 신호를 "그대로" 받아서 화면에 출력합니다.

---

## 🎯 샘플 프로젝트 위치

### **C 언어 샘플 (32bit)**
```
Stream SDK for PC
└── Sample Project
    └── win32
        └── thinkgear_testapp
            └── thinkgear_testapp.c
```

### **C 언어 샘플 (64bit)**
```
Stream SDK for PC
└── Sample Project
    └── x64
        └── thinkgear_testapp
            └── thinkgear_testapp.c
```

### **C# 샘플 (32bit)**
```
Stream SDK for PC
└── Sample Project
    └── win32
        └── thinkgear_testapp_csharp
            └── Program.cs
```

### **C# 샘플 (64bit)**
```
Stream SDK for PC
└── Sample Project
    └── x64
        └── thinkgear_testapp_csharp_64
            └── Program.cs
```

---

## 🔑 Stream SDK 핵심 함수

```cpp
// 1. 버전 정보 확인
int TG_GetVersion();

// 2. 새로운 연결 생성
int TG_GetNewConnectionId();

// 3. 로그 파일 설정 (선택사항)
int TG_SetStreamLog(int connectionId, const char *fileName);
int TG_SetDataLog(int connectionId, const char *fileName);

// 4. 헤드셋 연결
int TG_Connect(int connectionId, const char *comPortName, 
               int baudRate, int streamType);

// 5. 데이터 읽기 (메인 함수)
int TG_ReadPackets(int connectionId, int numPackets);

// 6. 데이터 상태 확인
int TG_GetValueStatus(int connectionId, int dataType);

// 7. 데이터 값 가져오기
double TG_GetValue(int connectionId, int dataType);

// 8. 자동 읽기 설정
int TG_EnableAutoRead(int connectionId, int enable);

// 9. 필터 설정 (MindWave Mobile 1.5)
int MWM15_setFilterType(int connectionId, int filterType);
int MWM15_getFilterType(int connectionId);

// 10. 연결 해제 및 정리
int TG_Disconnect(int connectionId);
void TG_FreeConnection(int connectionId);
```

---

## 📊 지원하는 데이터 타입

| 데이터 타입 | 설명 | 범위 |
|----------|------|------|
| `TG_DATA_RAW` | 뇌파 신호 (원본) | 0-255 |
| `TG_DATA_POOR_SIGNAL` | 신호 품질 | 0-255 (낮을수록 좋음) |
| `TG_DATA_ATTENTION` | 집중도 (eSense) | 0-100 |
| `TG_DATA_MEDITATION` | 명상도 (eSense) | 0-100 |
| `MWM15_DATA_FILTER_TYPE` | 필터 타입 | 50Hz or 60Hz |

---

## 🔌 COM 포트 설정

Windows에서 COM 포트 설정 방법:

```cpp
// COM 1~9: 직접 입력
comPortName = "COM5";

// COM 10 이상: \\\.\ 접두사 필요
comPortName = "\\\\.\\COM10";
comPortName = "\\\\.\\COM40";
```

**중요**: `\\` 대신 `\` 4개를 써야 합니다! (문자열 이스케이프)

---

## 📝 C 샘플 코드 실행 흐름

```
1. 라이브러리 버전 확인
   └─ TG_GetVersion()

2. 연결 ID 생성
   └─ TG_GetNewConnectionId()

3. 로그 파일 설정 (선택)
   ├─ TG_SetStreamLog()     (뇌파 신호를 파일에 기록)
   └─ TG_SetDataLog()       (처리된 데이터를 파일에 기록)

4. 헤드셋 연결
   └─ TG_Connect()

5. 데이터 읽기 (메인 루프)
   ├─ TG_ReadPackets()      (새 데이터 패킷 수신)
   ├─ TG_GetValueStatus()   (데이터 업데이트 확인)
   └─ TG_GetValue()         (데이터 값 추출)

6. 자동 읽기 모드
   ├─ TG_EnableAutoRead()   (자동 수신 시작)
   ├─ 필터 설정 (선택)
   └─ TG_EnableAutoRead(0)  (자동 수신 종료)

7. 정리 및 종료
   ├─ TG_Disconnect()
   └─ TG_FreeConnection()
```

---

## 💾 C# 샘플 코드 실행 흐름

C 코드와 동일하지만, **NativeThinkgear 클래스**를 통해 호출합니다:

```csharp
// 예제
NativeThinkgear.TG_GetVersion()
NativeThinkgear.TG_GetNewConnectionId()
NativeThinkgear.TG_Connect(...)
```

---

## 🚀 주요 작동 원리

### **수동 모드 (Manual Mode)**
```cpp
// 한 번에 1개 패킷씩 읽기
int result = TG_ReadPackets(connectionId, 1);
if (result == 1) {  // 1개 패킷 성공적으로 읽음
    // 데이터 처리
}
```

### **자동 모드 (Auto Read Mode)**
```cpp
// 자동으로 계속 데이터 수신
TG_EnableAutoRead(connectionId, 1);  // 시작

// 원하는 만큼 데이터 읽기
while (packetsRead < 2000) {
    if (TG_GetValueStatus(connectionId, TG_DATA_RAW) != 0) {
        int rawValue = TG_GetValue(connectionId, TG_DATA_RAW);
        packetsRead++;
    }
}

TG_EnableAutoRead(connectionId, 0);  // 종료
```

---

## 📋 코드 예제 해석

### **예제 1: 기본 연결**

```cpp
// 1. 연결 ID 생성
int connectionId = TG_GetNewConnectionId();

// 2. 헤드셋 연결
TG_Connect(connectionId,      // 연결 ID
           "\\\\.\\COM10",    // COM 포트
           TG_BAUD_57600,     // 통신 속도 (57600 bps)
           TG_STREAM_PACKETS); // 패킷 스트림 방식
```

### **예제 2: 데이터 읽기**

```cpp
// 계속 데이터를 읽음 (루프)
do {
    // 1개 패킷 읽기 (블로킹)
    packetsRead = TG_ReadPackets(connectionId, 1);
    
    if (packetsRead == 1) {  // 성공적으로 읽음
        // 신호 품질 확인
        if (TG_GetValueStatus(connectionId, TG_DATA_POOR_SIGNAL) != 0) {
            int signal = TG_GetValue(connectionId, TG_DATA_POOR_SIGNAL);
            printf("Signal Quality: %d\n", signal);
        }
        
        // 뇌파 신호 읽기
        if (TG_GetValueStatus(connectionId, TG_DATA_RAW) != 0) {
            int raw = TG_GetValue(connectionId, TG_DATA_RAW);
            printf("Raw EEG: %d\n", raw);
        }
    }
} while (/* 조건 */);
```

### **예제 3: 필터 설정 (MindWave Mobile 1.5)**

```cpp
// 50Hz 필터 설정 (유럽)
MWM15_setFilterType(connectionId, MWM15_FILTER_TYPE_50HZ);

// 1초 이상 기다린 후 설정 확인
Sleep(1000);

// 필터 설정 확인
int filterType = MWM15_getFilterType(connectionId);
printf("Filter Type: %d\n", filterType);
```

---

## ⚙️ 빌드 및 실행

### **Visual Studio에서**
1. 솔루션 파일 열기 (`.sln`)
2. **Build → Rebuild Solution** 선택
3. **F5**를 눌러 실행

### **콘솔 출력 예제**
```
Stream SDK for PC version: 3
Connection ID: 1
New RAW value: 127
New RAW value: 128
New RAW value: 129
...
```

---

## 🔍 로그 파일

### **streamLog.txt**
- **용도**: 헤드셋에서 받은 **원본 바이트** 기록
- **내용**: 16진수(hex) 형태의 패킷 데이터

### **dataLog.txt**
- **용도**: 처리된 **데이터 값** 기록
- **내용**: 가독성 있는 형태로 신호, 집중도 등 기록

---

## ⚠️ 주의사항

1. **COM 포트 확인**: 실제 장치에 맞는 COM 포트를 설정하세요
2. **헤드셋 연결**: ThinkGear_Connector가 설치되어 있어야 합니다
3. **Baud Rate**: 57600 bps 고정 (변경 금지)
4. **메모리 관리**: 자동 읽기 모드 사용 시 CPU 사용률이 높아질 수 있습니다

---

## 🎓 학습 순서

1. **기본 연결 이해** - TG_Connect() 함수
2. **데이터 읽기** - TG_ReadPackets() 및 TG_GetValue()
3. **자동 모드** - TG_EnableAutoRead()
4. **필터 설정** - MWM15_setFilterType()
5. **오류 처리** - 에러 코드 확인 및 메시지 처리

---

## 📚 추가 정보

- 각 샘플 코드의 `.c` 또는 `.cs` 파일을 열어서 주석을 참고하세요
- 로그 파일이 프로젝트 폴더에 생성됩니다
- 오류 코드는 0보다 작은 음수 값입니다

Happy Coding! 🚀
