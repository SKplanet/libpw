#include <pw/pwlib.h>
#include "myinstance.h"

MyInstance& INST(MyInstance::s_getInstance());
pw::JobManager& JOBMAN(INST.m_job.man);
pw::Ini& CONF(INST.m_config.conf);
pw::Log& CMDLOG(INST.m_log.cmd);
pw::Log& ERRLOG(INST.m_log.err);

MyInstance::MyInstance(const char* appname) : InstanceInterface(appname)
{

}

MyInstance::~MyInstance()
{

}
