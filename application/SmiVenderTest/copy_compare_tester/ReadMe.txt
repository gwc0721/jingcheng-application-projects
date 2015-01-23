========================================================================
    控制台应用程序：copy_compare_tester 项目概述
========================================================================

应用程序向导已为您创建了此 copy_compare_tester 应用程序。

本文件概要介绍组成 copy_compare_tester 应用程序的
的每个文件的内容。


copy_compare_tester.vcproj
    这是使用应用程序向导生成的 VC++ 项目的主项目文件，
    其中包含生成该文件的 Visual C++ 的版本信息，以及有关使用应用程序向导选择的平台、配置和项目功能的信息。

copy_compare_tester.cpp
    这是主应用程序源文件。

/////////////////////////////////////////////////////////////////////////////
其他标准文件：

StdAfx.h, StdAfx.cpp
    这些文件用于生成名为 copy_compare_tester.pch 的预编译头 (PCH) 文件和名为 StdAfx.obj 的预编译类型文件。

/////////////////////////////////////////////////////////////////////////////
其他注释：

应用程序向导使用“TODO:”注释来指示应添加或自定义的源代码部分。

/////////////////////////////////////////////////////////////////////////////
Release Note.
v1.0.0.1 (2015/01/13)
	initial version
v1.0.0.2 (2015/01/14)
	rename AtaTrace to ata_trace
	move IBinaryBuffer declaration from ata_trace to jcparam
v1.0.1.4 (2015/01/23)
	1. Fix power cycle issue in CStorageDeviceComm::StartStopUnit()
		- after power on, add retry for TEST UTILITY (cmd 0x00)
		- otherwise, in some platform, power on failed whill happen.
	2. Fix run duration calculation in jclogger.
	3. In jcapp, add application should specify the log config file name.
		- to avoid log config conflict when some jcapp based application run in same folder.