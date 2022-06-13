#pragma once

//#define TestVersion          //Release Need Mark This Line

#define MF_VERSION_MAJOR	1
#define MF_VERSION_MINOR	0
#define MF_VERSION_PATCH	1
#define MF_VERSION_DETAIL   1   //Test Version

#define MF_STR_HELPER(x) #x
#define MF_STR(x) MF_STR_HELPER(x)

#define MF_VERSION_COMPANY	"Kenxen Limited"
#define MF_VERSION			MF_VERSION_MAJOR,MF_VERSION_MINOR,MF_VERSION_PATCH,0
#define MF_VERSION_STR		MF_STR(MF_VERSION_MAJOR.MF_VERSION_MINOR.MF_VERSION_PATCH)
#define MF_VERSION_CR		"Copyright (c) 2018 Kenxen Limited.  All rights reserved."
#define MF_VERSION_PNAME	"KUVC"
