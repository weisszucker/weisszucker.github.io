//------------------------------------------------------------------------------
// compile command: cl.exe /Ox /Zi /EHsc capture-dump.cc
//------------------------------------------------------------------------------

#include <Windows.h>
#include <iostream>
#include <mutex>
#include <thread>

#pragma comment(lib, "dbghelp")
#include <DbgHelp.h>  // Must be included after <Windows.h>

constexpr MINIDUMP_TYPE kDumpType = static_cast<MINIDUMP_TYPE>(
    MiniDumpWithUnloadedModules | MiniDumpWithProcessThreadData |
    MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo);
constexpr const wchar_t* kJankDumpFileName = L"jank.dmp";
constexpr const wchar_t* kCrashDumpFileName = L"crash.dmp";

std::mutex g_dbghelp_lock;  // Dbghelp is not thread-safe.
LPTOP_LEVEL_EXCEPTION_FILTER g_previous_filter = NULL;

void WriteDump(const wchar_t* filename,
               MINIDUMP_EXCEPTION_INFORMATION* exception_info) {
  HANDLE file_handle =
      ::CreateFileW(filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (file_handle == INVALID_HANDLE_VALUE) {
    std::cerr << "CreateFile failed: " << ::GetLastError() << std::endl;
    return;
  }

  bool result = false;
  {
    std::lock_guard<std::mutex> lg{g_dbghelp_lock};
    result =
        ::MiniDumpWriteDump(::GetCurrentProcess(), ::GetCurrentProcessId(),
                            file_handle, kDumpType, exception_info, NULL, NULL);
  }
  ::CloseHandle(file_handle);

  if (!result) {
    std::cerr << "MiniDumpWriteDump failed: " << ::GetLastError() << std::endl;
  } else {
    std::wcout << L"Dump created: " << filename << std::endl;
  }
}

void TakeJankSnapshot(DWORD thread_id) {
  HANDLE thread_handle = ::OpenThread(THREAD_SUSPEND_RESUME, false, thread_id);
  if (!thread_handle) {
    std::cerr << "OpenThread failed: " << ::GetLastError() << std::endl;
    return;
  }

  if (::SuspendThread(thread_handle) == static_cast<DWORD>(-1)) {
    std::cerr << "SuspendThread failed: " << ::GetLastError() << std::endl;
    ::CloseHandle(thread_handle);
    return;
  }

  WriteDump(kJankDumpFileName, NULL);

  if (::ResumeThread(thread_handle) == static_cast<DWORD>(-1)) {
    std::cerr << "ResumeThread failed: " << ::GetLastError() << std::endl;
  }
  ::CloseHandle(thread_handle);
}

LONG WINAPI HandleExceptionFilter(EXCEPTION_POINTERS* exception_pointers) {
  MINIDUMP_EXCEPTION_INFORMATION exception_info;
  exception_info.ThreadId = ::GetCurrentThreadId();
  exception_info.ExceptionPointers = exception_pointers;
  WriteDump(kCrashDumpFileName, &exception_info);

  if (g_previous_filter) {
    return g_previous_filter(exception_pointers);
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

void SimulateJank() {
  ::Sleep(3000);
}

void SimulateCrash() {
  *((volatile int*)nullptr) = 42;
}

int main() {
  g_previous_filter = ::SetUnhandledExceptionFilter(&HandleExceptionFilter);

  std::thread watchdog([thread_id = ::GetCurrentThreadId()] {
    ::Sleep(0);
    TakeJankSnapshot(thread_id);
  });
  SimulateJank();
  watchdog.join();

  SimulateCrash();

  return 0;
}
