# EEG Algorithm SDK for Windows - 초보자 가이드

## 📌 이 프로젝트가 무엇인가요?

이 프로젝트는 **NeuroSky 뇌파(EEG) 센서**에서 받은 데이터를 분석하기 위한 개발자 도구 모음입니다.

쉽게 말해서:
- 🧠 **뇌파 센서**: MindWave Mobile 헤드셋이라는 장치에서 뇌파 신호를 읽어옵니다
- 📊 **데이터 분석**: 그 신호들을 의미 있는 정보로 변환합니다
  - 집중도 (Attention)
  - 명상도 (Meditation)
  - 안구 깜빡임 감지 (Eye Blink)
  - 노력 정도 (Mental Effort)
  - 친숙도 (Familiarity) 등

---

## 📁 프로젝트 구조 설명

### **주요 폴더별 역할**

```
Windows Developer Tools 3.2/
├── EEG_Algo_SDK_Windows/         ← 알고리즘 SDK (뇌파 분석 엔진)
│   ├── Include/                  ← 헤더 파일 (함수 설명서)
│   ├── Libs/                     ← 라이브러리 파일 (.dll)
│   └── Algo SDK Sample/          ← 예제 프로젝트
│
├── Stream SDK for PC/            ← 통신 SDK (데이터 수신)
│   ├── libs/                     ← 라이브러리 파일
│   └── Sample Project/           ← 예제 프로젝트
│       ├── win32/                ← 32비트 버전
│       └── x64/                  ← 64비트 버전
│
└── ThinkGear_Connector/          ← 통신 관리 프로그램
```

---

## 🔧 두 가지 주요 SDK

### 1️⃣ **Stream SDK** (데이터 수신)
- **역할**: MindWave 헤드셋과 연결하여 **뇌파 신호**를 받아옵니다
- **파일명**: `thinkgear.h`, `thinkgear.dll`
- **주요 함수**:
  - `TG_Connect()` - 헤드셋에 연결
  - `TG_ReadPackets()` - 뇌파 데이터 읽기
  - `TG_Disconnect()` - 연결 해제

### 2️⃣ **Algo SDK** (데이터 분석)
- **역할**: 받은 신호를 **의미 있는 정보로 변환**합니다
- **파일명**: `NSK_Algo.h`, `AlgoSdkDll.dll`
- **주요 함수**:
  - `NSK_ALGO_Init()` - 알고리즘 초기화
  - `NSK_ALGO_Start()` - 분석 시작
  - `NSK_ALGO_GetResult()` - 결과 가져오기

---

## 🚀 빠른 시작

### **필요한 준비물**
- ✅ Windows 컴퓨터 (32bit 또는 64bit)
- ✅ Visual Studio (Express 2015 이상)
- ✅ MindWave Mobile 헤드셋 (또는 샘플 데이터)
- ✅ ThinkGear_Connector 설치 (통신용)

### **단계별 진행**

#### **1단계: 헤드셋 연결**
```
1. MindWave Mobile 헤드셋을 Bluetooth로 Windows와 페어링
2. 연결된 COM 포트 확인 (예: COM10)
3. 코드에 COM 포트 번호 입력
```

#### **2단계: 예제 프로젝트 열기**
```
1. Visual Studio 실행
2. "Algo SDK Sample.sln" 파일 열기
   경로: Algo SDK Sample\Algo SDK Sample\Algo SDK Sample.sln
```

#### **3단계: 프로젝트 빌드 및 실행**
```
1. [Build] → [Rebuild Project] 선택
2. [F5]를 눌러 프로그램 실행
```

#### **4단계: 프로그램 사용**
```
프로그램 창에서:
1. 우상단에서 분석할 알고리즘 선택
   (예: Attention, Meditation, Eye Blink)
2. [Init] 버튼 클릭 → 알고리즘 초기화
3. [Start] 버튼 클릭 → 분석 시작
4. 실시간으로 결과가 나타남
```

---

## 📊 지원하는 뇌파 분석 알고리즘

| 알고리즘 | 설명 | 범위 |
|---------|------|------|
| **Attention (집중도)** | 얼마나 집중하고 있는지 | 0-100 |
| **Meditation (명상도)** | 얼마나 진정되어 있는지 | 0-100 |
| **Eye Blink** | 눈이 깜빡인 강도 | 0-255 |
| **Mental Effort** | 정신적 노력 정도 | 0-100 |
| **Familiarity** | 친숙한 정도 | 변함 |
| **EEG Bandpower** | 뇌파 대역별 강도 | 변함 |
| **Appreciation** | 만족도 | 변함 |

---

## 🔌 온라인 모드 vs 오프라인 모드

### **온라인 모드 (Realtime Mode)**
- 실제 헤드셋에서 **실시간으로** 데이터 수신
- 헤드셋이 연결되어 있어야 함

### **오프라인 모드 (Offline Mode)**
- 미리 저장된 데이터 파일(`.txt`)을 읽어서 분석
- 헤드셋 없이도 테스트 가능
- 예제 데이터: `ME_Easy_RawData.txt`, `ME_Easy_Baseline.txt`

---

## 💾 파일별 설명

### **Include 폴더 (헤더 파일)**
```
NSK_Algo.h              - 알고리즘 함수 정의
NSK_Algo_Defines.h      - 알고리즘 상수 정의
NSK_Algo_ReturnCodes.h  - 반환 코드 (성공/실패)
NSK_Algo_Helper.h       - 도우미 함수
Platform_defines.h      - 플랫폼별 정의
```

### **Libs 폴더 (라이브러리)**
```
AlgoSdkDll.dll          - 32비트 알고리즘 라이브러리
AlgoSdkDll64.dll        - 64비트 알고리즘 라이브러리
```

### **Stream SDK 라이브러리**
```
thinkgear.h             - 데이터 수신 함수 정의
```

---

## 🎯 예제 프로젝트로 배우기

### **Algo SDK Sample**
- **위치**: `Algo SDK Sample\Algo SDK Sample\`
- **파일**: `Algo SDK Sample.cpp`
- **하는 일**:
  - MindWave 헤드셋과 통신
  - 뇌파 데이터 수신
  - Algo SDK로 처리
  - 결과 표시

### **Stream SDK Sample** (C/C++)
- **위치**: `Stream SDK for PC\Sample Project\`
- **파일**: `thinkgear_testapp.c` (32bit), `thinkgear_testapp.c` (64bit)
- **하는 일**: 순수하게 데이터만 수신

### **Stream SDK Sample** (C#)
- **위치**: `Stream SDK for PC\Sample Project\win32\thinkgear_testapp_csharp\`
- **파일**: `Program.cs`, `NativeThinkgear.cs`
- **하는 일**: C#으로 데이터 수신하기

---

## 🛠️ 개발자용 주요 정보

### **버전 정보**
- **최신**: v2.9.2 (2016-02-03)
- **기능**: EEG Bandpower 지원

### **함수 호출 순서**
```cpp
1. NSK_ALGO_Init()      // 초기화
2. NSK_ALGO_Start()     // 시작
3. NSK_ALGO_Process()   // 데이터 처리 (반복)
4. NSK_ALGO_Pause()     // 일시 중지
5. NSK_ALGO_Stop()      // 정지
```

### **콜백 함수**
- 분석 결과가 나오면 **콜백 함수**가 호출됩니다
- 결과를 받아서 화면에 표시하거나 저장할 수 있습니다

---

## 📚 더 알아보기

### **공식 문서**
- 📖 `eeg_algorithm_sdk_for_windows_development_guide.pdf` 참조
- 🌐 http://developer.neurosky.com

### **지원 받기**
- 💬 지원 포럼: http://support.neurosky.com
- 📧 이메일: support@neurosky.com
- 👥 커뮤니티: http://www.linkedin.com/groups/NeuroSky-Brain-Computer-Interface-Technology-3572341

---

## ⚠️ 주의사항

1. **64bit vs 32bit**: 사용하는 Visual Studio와 Windows 버전에 맞는 DLL을 사용하세요
2. **COM 포트**: 올바른 COM 포트를 설정하지 않으면 연결이 안 됩니다
3. **Baseline 데이터**: 첫 사용 시 baseline 데이터 수집이 필요할 수 있습니다
4. **ThinkGear_Connector**: 헤드셋 통신을 위해 반드시 설치하세요

---

## 🎓 학습 순서 추천

**초급 → 중급 → 고급** 순서로 학습하세요:

### **초급 (이해)**
1. Stream SDK 개념 이해
2. 헤드셋 연결 방법 학습
3. 샘플 프로젝트 실행

### **중급 (구현)**
1. Stream SDK를 사용한 데이터 수신 코드 작성
2. Algo SDK 기본 함수 학습
3. Algo SDK Sample 분석

### **고급 (응용)**
1. 여러 알고리즘 조합 사용
2. 실시간 데이터 처리
3. 자신의 애플리케이션에 통합

---

## 📝 라이선스

**NeuroSky Inc.** 저작권 - 상업 또는 연구 목적으로 사용 시 라이선스 확인 필요

---

**Happy Coding! 🚀**

이 README가 도움이 되었나요? 더 궁금한 점이 있으면 공식 문서를 참조하세요.
