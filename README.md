#PW 라이브러리

## 개요

* PW 라이브러리는 C++11 표준으로 작성한 라이브러리 세트입니다.
	* 기존 QT, ACE 라이브러리와 달리 타입 재정의를 거의 사용하지 않습니다.
	* 모든 형태의 애플리캐이션 개발을 위한 프래임워크는 아니며, 매우 제한적인 형태를 목표로 하고 있습니다.
* 데몬을 개발할 때, 제법 잘 정돈한 프래임워크로, 개발자는 로직에만 집중하도록 도와줍니다. ~~집중력 향상 프로그램은 아닙니다.~~
* 멀티프로세스+싱글스래드를 기반으로 작성합니다.
	* 단, 매우 제한적으로 멀티스래드도 지원하지만, 매우 까다로울 것 같습니다.

### 사용환경

* Linux
	* CentOS >= 5
		* http://www.centos.org/
		* CentOS 5, 6 에서 devtoolset-2가 필요. (GCC-C++ 4.7/C++11 요건)
	* Fedora >= 21
		* https://getfedora.org/
	* Ubuntu >= 14.04
		* http://www.ubuntu.com/
* GCC-C++ >= 4.7
	* https://gcc.gnu.org/
* CMake >= 3.0
	* http://www.cmake.org/
* OpenSSL >= 0.9.8 (가능하면 1.0.0 이상 사용)
	* http://openssl.org/
* Zlib >= 1.0
	* http://www.zlib.net/
* URIParser >= 0.7
	* http://uriparser.sourceforge.net/
* JsonCpp >= 0.9
	* https://github.com/open-source-parsers/jsoncpp
* Sqlite >= 3.0
	* http://www.sqlite.org/
* Doxygen (문서화용)
	* 그림을 위해 Graphviz나 Dot 등 유틸리티가 더 필요할 수 있습니다.
	* http://www.stack.nl/~dimitri/doxygen/
* IDE
	IDE는 CMake에서 프로젝트 파일을 만들어 줍니다.
	* Eclipse CDT (매우 느림)
		* https://projects.eclipse.org/projects/tools.cdt
	* Kdevelop >= 4.7
		* 4.7버전부터는 CMakeLists.txt 파일을 직접 임포트 할 수 있습니다.
		* https://www.kdevelop.org/
	* Code Blocks
		* http://www.codeblocks.org/
	* Sublime Text (유료, 써보고 싶음)
		* http://www.sublimetext.com/
	* CodeLite
		* http://codelite.org/
	* Kate
		* http://kate-editor.org/

### 저장소
	git clone https://github.com/skplanet/libpw.git

### 빌드
	$ git clone https://github.com/skplanet/libpw.git
	$ cd libpw
	$ make		; build 디렉토리를 만들고, cmake를 통해 빌드를 구성합니다.
	$ cd build
	$ make		; 실제 컴파일을 하여 빌드를 시작합니다.
	
### 패키징
패키지는 RPM과 DEB를 지원합니다.

	$ cd build
	$ make libpw.rpm	; RPM을 만들며, libpw/build/rpmbuild/RPMS/[ARCH]/ 디렉토리에 생성합니다.
	$ make libpw.deb	; DEB을 만들며, libpw/build/ 디렉토리에 생성합니다.

## 분류
대략적인 분류입니다.
자세한 것은 doxygen을 통해 만들어진 문서나, 헤더 파일에 있는 주석을 참고하세요.

### 네임스페이스

네임스페이스는 **pw**를 사용하며, 추가 네임스페이스를 포함합니다.

	namespace pw {
		namespace ssl { }
		namespace crypto { }
		namespace http { }
	}

### 공용

#### common
라이브러리 전체에서 사용할 타입(구조체, 열거형 ...)을 정의합니다.

* HTTP응답코드을 빌려 라이브러리 응답코드(ResultCode)로 정의하였습니다.
* 호스트 정보(host_type)나, 포인터와 길이를 관리하는 구조체(blob_type) 등을 정의하였습니다.
* 기타 표준 라이브러리를 사용하여, 자주 사용하는 타입을 정의하였습니다.
	* 가능하면 미리 정의한 타입을 사용하여, 뻥튀기(code-bloat)를 줄여봅시다!
	* 그러나 요즘 C++ 컴파일러는 -O3 옵션에 최적화를 제법 잘 수행하니, 너무 스트레스는 받지 마세요.

### 시스템
공용에 가깝지만, 다소 애매하여 달리 분류하였습니다.

#### exception
예외 클래스를 정의하였습니다.

#### log
간단한 로그 시스템입니다.

* 날짜별, 시간별로 로그파일을 생성합니다.
	* 최근 로그는 하드링크를 생성하니, 로그 감시 스크립트 만들 때 잘 써봅시다.
* 나름 레벨에 따른 출력을 지원합니다.

#### timer
간단한 타이머입니다.

* 각 타이머 이벤트 객체(pw::Timer::Event)를 상속 받아, 각종 로직를 구현할 수 있습니다.

#### sysinfo
시스템 정보를 확인합니다.

* OS, 호스트 이름, CPU, NIC, 메모리, 파일시스템 정보를 간단히 확인할 수 있습니다.
* 이러한 잘 변경하지 않는 정보를 기반으로, 해시를 생성하여 암호화쪽에 응용할 수 있습니다.

#### module
공유객체(Shared object, 윈도우즈 환경에서 DLL로 불리우는 그것이죠)를 관리합니다.

* 프래임워크에 가깝습니다.
	* 미리 정의한 함수 이름으로 SO 파일을 만들어, 플러그인 등으로 응용할 수 있습니다.

### 문자열
문자열을 다루는 유틸리티입니다.

#### string
std::string, std::stream 객체를 적극 활용한 잡다한 문자열 유틸리티입니다.

#### tokenizer
문자열을 특정 문자로 자르는 기능입니다.

#### ini
INI 파일 형식을 간단하게 읽고 씁니다.

* 읽을 때, 주석 등은 가볍게 무시하니 주의하세요.
* XML/DOM처럼 전체를 읽고, 섹션과 아이템 이름으로 접근할 수 있습니다.
* XML/SAX처럼 이벤트 방식도 제공합니다.

#### encode
문자열 인코딩/디코딩을 지원합니다.

URL 인코딩을 지원합니다.
Hexa 인코딩을 지원합니다.
Base64 인코딩을 지원합니다.
Escape 인코딩을 지원합니다.

#### date
날짜와 시간 문자열을 다룹니다.

* 단순히 ASN.1 포맷 날짜를 time_t 형태로 변환합니다.

#### key
std::string과 비슷한 문자열 클래스입니다.

* 멀티스래드를 위한 락이 없습니다.
* std::map 등에서 키로 사용할 때, std::string보다 좋은 성능을 꿈꾸어봅니다.

#### compress
Zlib 래핑 클래스입니다.

#### region
국가별 전화번호 기반 사전입니다.

#### uri
URIParser 래핑 클래스입니다.

* RFC 3986을 직접 구현하기엔...

#### strfltr
문자열 필터를 지원합니다.

* 정규식을 지원하지만, std::regex는 아직(GCC-4.9) 제대로 동작하지 않아 부득이 하게 boost::regex나 POSIX C의 regex를 사용할 수 있습니다.

### 네트워크
소켓과 멀티플렉서를 다룹니다.

#### iopoller
멀티플렉서 인터패이스입니다.

* 현재는 POSIX select와 Linux epoll만 다룹니다.
* 멀티스래드 환경에 전혀 안전하지 않습니다.

#### socket
여러가지 소켓 유틸리티를 지원합니다.

* pw::IoPoller::Event를 상속 받아, 멀티플랙서와 연동할 수 있습니다.
	* pw::ChannelInterface의 부모 클래스입니다.
* connect할 때, 도메인네임으로 호출할 경우, 적절한 캐시데몬(nscd)를 사용하지 않았을 경우, DNS에 부하를 줄 수 있습니다.
	* 적당히 /etc/hosts에 등록하는 것도 방법입니다.

#### iobuffer
채널에서 사용하는 버퍼입니다.

#### sockaddr
소켓 주소를 관리하는 기능입니다.

* IPv4, IPv6, UNIX 타입을 지원합니다.

#### iprange
IP 주소를 범위별로 묶어서 관리하는 기능입니다.

* IPv4/IPv6를 지원합니다.
* 간단한 ACL이나 GeoIP 등으로 응용할 수 있습니다.

#### packet_if
기본 패킷 인터패이스입니다.

#### listener_if
리스너 인터패이스입니다.

#### msgpacket, msgchannel
자체 프로토콜인 메시지 포맷을 구현한 것입니다.

#### httppacket, httpchannel
HTTP/1.x를 구현한 것입니다.

#### apnspacket, apnschannel
APNS(Apple Push Notification Service) 프로토콜을 구현한 것입니다.

#### redispacket, redischannel
REDIS 프로토콜을 구현한 것입니다.

#### simplechpool
간단한 채널 풀을 구현한 것입니다.

### 암호화

#### digest
OpenSSL EVP MD(Message Digest) 래핑 클래스입니다. 해시를 담당하죠.

#### crypto
OpenSSL EVP Cipher 래핑 클래스입니다.

#### ssl
OpenSSL SSL과 EVP 래핑 클래스입니다.

* 인증서, 비밀키, 공개키 등을 다룹니다.
* SSL 세션을 다룹니다.

### 인스턴스
데몬 개발 프래임워크이며, 이를 pw::Instance라고 이름 붙였습니다.

#### jobmanager
Job(트랜잭션)을 관리하는 객체입니다.

#### multichannel_if
서버끼리 접속 유지 연결을 할 경우, 클라이언트쪽에서 사용하는 채널입니다.

#### concurrentqueue_if
멀티스래드에서 생산자와 소비자 사이에 요청과 응답을 보내기 위한 파이프라인입니다.

#### instance_if
데몬 프로세스 프래임워크입니다.

* 싱글톤 객체로 데몬이 시작해서, 환경설정 읽기, 객체 초기화, 리스너 개방, 로그 관리, 타이머 관리, 트랜잭션 관리 등을 하는 객체입니다.

## 프래임워크

### 인스턴스

1. 인스턴스는 main() 함수에서 start() 함수를 호출하는 것으로 시작한다.
2. 라이브러리를 초기화한다.
	1. OpenSSL 라이브러리 등을 초기화 한다.
3. 현재 프로세스가 자식인지 확인한다. 이 작업은 향후 자식 프로세스를 생성하고, start() 함수를 다시 호출할 때, 부모 프로세스에서만 초기화할 내용을 포함한다.
	1. 부모 프로세스라면 시그널 콜백을 설정한다.
	2. 시스템 정보를 읽어온다.
4. loadConfig() 함수를 호출하여 환경설정 파일을 열고, 설정 파일을 읽는다.
	1. eventConfig() 가상함수를 호출하며, 최초 호출인지 여부를 인자로 넘겨준다.
	2. 사용자는 가상함수를 상속 받아 구현한다.
5. eventInitLog() 가상함수를 호출한다.
	1. 사용자는 가상함수를 상속 받아 추가로그 파일을 초기화한다.
	2. initializeLog() 함수를 이용하면 일정한 형태로 쉽게 로그를 초기화할 수 있다.
	3. 기본으로 커맨드로그와 에러로그를 초기화하며, 에러로그는 Log::s_setLibrary() 함수 호출을 통해 라이브러리용 로그로 자동 설정한다.
6. 멀티플렉서 IoPoller를 생성한다.
	1. 환경설정에 주어진 값으로 초기화한다.
	2. 주어지지 않았을 경우, epoll 환경에서 epoll을 사용하며, 그외는 모두 select를 사용한다.
7. eventInitChannel() 가상함수를 호출하여, 미리 접속해야할 채널을 초기화한다.
	1. 사용자는 가상함수를 상속받아 구현한다.
	2. 보통 MultiChannelInterface를 상속받은 채널을 초기화한다.
8. eventInitListener() 가상함수를 호출하여, 리스너를 초기화한다.
	1. 프로세스 구현에 따라 구분하기 편하도록 eventInitListener() 가상함수는 아래 세 가상함수를 선별하여 호출한다.
		1. eventInitListenerSingle() 싱글 프로세스일 경우 호출한다.
		2. eventInitListenerParent() 부모 프로세스일 경우 호출한다.
		3. eventInitListenerChild() 자식 프로세스일 경우 호출한다.
	2. 사용자는 가능하면 위 세 가상함수를 상속받아 구현한다.
9. 자식 프로세스가 필요한지 확인하여, 자식 프로세스를 초기화하는 작업을 시작한다.
	1. eventInitChild() 가상함수를 호출하여, 자식 프로세스를 fork한다.
		1. InstanceInterface::fork() 함수는 시스템 fork()함수를 호출하며, 자식과 부모사이에 통신을 위해 socketpair를 생성한다.
		2. 자식 프로세스에서는 eventForkChild() 가상함수 호출한다.
			1. eventForChild()는 cleanUpForChild()를 호출하여, eventForkCleanUp으로 시작하는 가상함수를 호출한다.
			2. 이들이 기본으로 하는 일은 부모로부터 복제한 각종 파일 디스크립터 등 리소스를 정리하는 일이다.
		3. start() 함수를 호출하여, 자식 프로세스는 3번부터 재시작한다. (순서도에서 Child loop...로 표현하였다)
		4. 자식 프로세스를 정상 종료할 경우, eventExitChild()를 호출한다.
10. eventInitTimer() 가상함수를 호출하여, 타이머 이벤트를 초기화한다.
	1. 사용자는 가상함수를 상속받아 구현한다.
	2. 전역에서 사용할 타이머를 등록하는 것이 좋다.
11. eventInitExtras() 가상함수를 호출하여, 프레임워크에서 미리 정의하지 않은 기타 모듈을 초기화한다.
	1. 사용자는 가상함수를 상속받아 구현한다.
12. 실행 플래그가 참인지 확인한다.
	1. 거짓이라면, eventExit() 가상함수를 호출하고 프로세스를 종료한다.
	2. USR1, USR2 시그널을 받으면, 실행 플래그을 거짓으로 설정하여 프로세스 종료를 꾀한다.
13. 자식 프로세스가 죽었는지 확인한다.
	1. CHLD 시그널을 받으면, 자식 프로세스에 대해 시스템 wait()을 호출하여 죽은 프로세스를 모두 검출한다.
	2. 죽은 프로세스에 물려 있던 socketpair를 정리한다.
	3. fork()-eventForkChild()-... 등을 통해 자식 프로세스를 다시 만든다. (9번 참조)
14. 환경설정을 다시 읽어야하는지 확인한다.
	1. 타이머가 일정시간마다 플래그를 설정한다.
	2. HUP 시그널을 받으면 플래그를 설정한다.
	3. eventConfig() 가상함수를 호출한다.
15. 멀티플렉서에 이벤트를 확인하고, 이벤트를 해당 객체에 전달한다.
	1. IoPoller::Client::eventIo() 가상함수를 호출한다.
	2. 대부분의 사용자는 이 가상함수를 상속받을 일이 없다.
16. Job객체에서 타임아웃이 발생했는지 확인한다.
	1. Job::eventTimeout() 가상함수를 호출한다.
17. 타이머 이벤트를 확인하고, 이벤트를 해당 객체에 전달한다.
	1. Timer::Event::eventTimer() 가상함수를 호출한다.
18. eventEndTurn() 가상함수를 호출하여 루프를 한 번 실행했다는 것을 알린다.
	1. 사용자는 가상함수를 상속받아 구현한다.
19. 12번으로 이동한다.

복잡해 보이지만, 사용자는 이 전부를 이해할 필요가 없습니다~
