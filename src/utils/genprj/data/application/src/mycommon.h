/*
 * Generated by PWGenPrj
 * Generated at __PWTEMPLATE_DATE__
 */

/*!
 * \file mycommon.h
 * \brief Local common header
 */

#ifndef MYCOMMON_H
#define MYCOMMON_H

#include <pw/pwlib.h>
// for your finger health...
using namespace pw;

#ifdef VERSION
constexpr auto g_version = VERSION;
constexpr auto g_version_major = VERSION_MAJOR;
constexpr auto g_version_minor = VERSION_MINOR;
constexpr auto g_version_patch = VERSION_PATCH;
constexpr auto g_version_release = VERSION_RELEASE;
#else
constexpr auto g_version = "1.0.0";
constexpr auto g_version_major = 1;
constexpr auto g_version_minor = 0;
constexpr auto g_version_patch = 0;
constexpr auto g_version_release = "1";
#endif

//! \brief Application name.
constexpr auto g_name = "__PWTEMPLATE_NAME__";

//! \brief Start timestamp.
constexpr auto g_start = "__PWTEMPLATE_DATE__";

#endif // MYCOMMON_H
