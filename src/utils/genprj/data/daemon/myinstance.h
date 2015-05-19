#include "./mycommon.h"

#ifndef MYINSTANCE_H
#define MYINSTANCE_H

class MyInstance :  pw::InstanceInterface
{
public:
	//----------------------------------------------------------------------
	// Enums...

	//! \brief Timer enums.
	enum class CRON
	{
		ONE_MIN,
		FIVE_MIN,
		TEN_MIN,
		ONE_HOUR,
		ONE_DAY,
	};

public:
	//! \brief Instance!
	inline static MyInstance& s_getInstance(void) { static MyInstance inst(g_name); return inst; }

public:
	MyInstance(const char* appname);
	virtual ~MyInstance();

public:
	//----------------------------------------------------------------------
	// Custom variables...

	// \TODO Delete demo code...
    string m_test_string;
    int m_test_integer;

public:
	//----------------------------------------------------------------------
	// Custom methods...

private:
	//----------------------------------------------------------------------
	// Event handlers

	void eventTimer(int id, void* param) override;
	bool eventConfig(bool isDefault, bool isReload) override;
	bool eventInitLog(void) override;
	bool eventInitChannel(void) override;
	bool eventInitListenerSingle(void) override;
	bool eventInitListenerParent(void) override;
	bool eventInitListenerChild(void) override;
	//bool eventInitListener(void) override;
	bool eventInitTimer(void) override;
	//bool eventInitExtras(void) override;
	//bool eventInitChild(void) override;

	//void eventExit(void) override;
	//void eventExitChild(size_t index, pid_t pid, int exit_status, void* param) override;
	//void eventEndTurn(void) override;

	//----------------------------------------------------------------------
	// Clean-up event handlers
	//
	//void eventForkChild(size_t index, void* param) override;
	void eventForkCleanUpChannel(size_t index, void* param) override;
	//void eventForkCleanUpListener(size_t index, void* param) override;
	//void eventForkCleanUpTimer(size_t index, void* param) override;
	//void eventForkCleanUpExtras(size_t index, void* param) override;
	//void eventForkCleanUpPoller(size_t index, void* param) override;

};


//! \brief Poller macro
#define POLLER (INST.getPoller())

//! \brief Instance
extern MyInstance& INST;

//! \brief Job manager
extern pw::JobManager& JOBMAN;

//! \brief Configuration
extern pw::Ini& CONF;

//! \brief Command log
extern pw::Log& CMDLOG;

//! \brief Error log
extern pw::Log& ERRLOG;

#endif // MYINSTANCE_H
