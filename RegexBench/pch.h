// pch.h

#ifndef PCH_H
#define PCH_H

// 여기에 미리 컴파일하려는 헤더 추가
#include "framework.h"

// 정규 표현식 라이브러리
#include <re2/re2.h>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

// 윈도우 관련 헤더
#include <gdiplus.h>
#include <atlconv.h> // CString 변환 매크로 포함
#include <chrono>
#include <Shlwapi.h>
#include <afx.h>

// STL 관련 헤더
#include <vector>
#include <deque>
#include <string>
#include <memory>
#include <utility>
#include <thread>
#include <atomic>
#include <mutex>
#include <future>
#include <random>
#include <algorithm>
#include <fstream>
#include <ctime>
#include <sstream>
#include <filesystem>
#include <numeric>

// 공용 헤더 파일
#include "Pattern.h"

#endif
