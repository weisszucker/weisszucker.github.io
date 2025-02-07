//------------------------------------------------------------------------------
// compile command:
//   (normal) cl.exe /Ox /Zi /EHsc /std:c++17 capture-stack.cc
//   (oninline) cl.exe /Ox /Ob0 /Zi /EHsc /std:c++17 capture-stack.cc
//------------------------------------------------------------------------------

#include <Windows.h>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>

#pragma comment(lib, "dbghelp")
#include <DbgHelp.h>  // Must be included after <Windows.h>

constexpr const wchar_t* kJankStackFileName = L"jank.txt";
constexpr const wchar_t* kCrashStackFileName = L"crash.txt";

std::mutex g_dbghelp_lock;  // Dbghelp is not thread-safe.
LPTOP_LEVEL_EXCEPTION_FILTER g_previous_filter = NULL;

using Modules = std::map<DWORD64, std::pair<std::wstring, DWORD64>>;

BOOL CALLBACK EnumModulesCallback(PCWSTR name,
                                  DWORD64 base,
                                  ULONG size,
                                  PVOID context) {
  Modules* modules = reinterpret_cast<Modules*>(context);
  const wchar_t* p = wcsrchr(name, L'\\');
  modules->emplace(base,
                   std::make_pair(std::wstring(p ? (p + 1) : name), size));
  return TRUE;
}

void WriteStack(const wchar_t* filename, CONTEXT* context_record) {
  Modules modules;
  {
    std::lock_guard<std::mutex> lg{g_dbghelp_lock};
    if (!::EnumerateLoadedModulesW64(::GetCurrentProcess(), EnumModulesCallback,
                                     &modules)) {
      std::cerr << "EnumerateLoadedModulesW64 failed: " << ::GetLastError()
                << std::endl;
      return;
    }
  }

  // From chromium base/debug/stack_trace_win.cc
  STACKFRAME64 stack_frame;
  memset(&stack_frame, 0, sizeof(stack_frame));
#if defined(_M_X64) || defined(__x86_64__)
  DWORD machine_type = IMAGE_FILE_MACHINE_AMD64;
  stack_frame.AddrPC.Offset = context_record->Rip;
  stack_frame.AddrFrame.Offset = context_record->Rbp;
  stack_frame.AddrStack.Offset = context_record->Rsp;
#elif defined(_M_IX86) || defined(__i386__)
  DWORD machine_type = IMAGE_FILE_MACHINE_I386;
  stack_frame.AddrPC.Offset = context_record->Eip;
  stack_frame.AddrFrame.Offset = context_record->Ebp;
  stack_frame.AddrStack.Offset = context_record->Esp;
#elif defined(_M_ARM64) || defined(__aarch64__)
  DWORD machine_type = IMAGE_FILE_MACHINE_ARM64;
  stack_frame.AddrPC.Offset = context_record->Pc;
  stack_frame.AddrFrame.Offset = context_record->Fp;
  stack_frame.AddrStack.Offset = context_record->Sp;
#else
#error Unsupported Windows Arch
#endif
  stack_frame.AddrPC.Mode = AddrModeFlat;
  stack_frame.AddrFrame.Mode = AddrModeFlat;
  stack_frame.AddrStack.Mode = AddrModeFlat;

  std::wofstream ofs(filename, std::ios::out);
  size_t count = 0;
  while (true) {
    {
      std::lock_guard<std::mutex> lg{g_dbghelp_lock};
      if (!::StackWalk64(machine_type, ::GetCurrentProcess(),
                         ::GetCurrentThread(), &stack_frame, context_record,
                         NULL, &::SymFunctionTableAccess64,
                         &::SymGetModuleBase64, NULL)) {
        break;
      }
    }

    DWORD64 offset = stack_frame.AddrPC.Offset;
    std::wstring module = L"???";
    for (const auto& [base, pair] : modules) {
      if (offset >= base && offset < base + pair.second) {
        module = pair.first;
        offset -= base;
        break;
      }
    }
    ofs << module << L"!0x" << std::hex << offset << std::endl;
    ++count;
  }
  std::wcout << L"Wrote " << count << L" frames: " << filename << std::endl;
}

void TakeJankSnapshot(DWORD thread_id) {
  HANDLE thread_handle = ::OpenThread(
      THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, false, thread_id);
  if (!thread_handle) {
    std::cerr << "OpenThread failed: " << ::GetLastError() << std::endl;
    return;
  }

  if (::SuspendThread(thread_handle) == static_cast<DWORD>(-1)) {
    std::cerr << "SuspendThread failed: " << ::GetLastError() << std::endl;
    ::CloseHandle(thread_handle);
    return;
  }

  CONTEXT context;
  memset(&context, 0, sizeof(context));
  context.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL;

  if (!::GetThreadContext(thread_handle, &context)) {
    std::cerr << "GetThreadContext failed: " << ::GetLastError() << std::endl;

    if (::ResumeThread(thread_handle) == static_cast<DWORD>(-1)) {
      std::cerr << "ResumeThread failed: " << ::GetLastError() << std::endl;
    }
    ::CloseHandle(thread_handle);
    return;
  }

  WriteStack(kJankStackFileName, &context);

  if (::ResumeThread(thread_handle) == static_cast<DWORD>(-1)) {
    std::cerr << "ResumeThread failed: " << ::GetLastError() << std::endl;
  }
  ::CloseHandle(thread_handle);
}

LONG WINAPI HandleExceptionFilter(EXCEPTION_POINTERS* exception_pointers) {
  CONTEXT context;
  memcpy(&context, exception_pointers->ContextRecord, sizeof(context));
  context.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL;

  WriteStack(kCrashStackFileName, &context);

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
  const HANDLE current_process = ::GetCurrentProcess();
  if (!::SymInitialize(current_process, NULL, TRUE)) {
    std::cerr << "SymInitialize failed: " << ::GetLastError() << std::endl;
    return 1;
  }

  g_previous_filter = SetUnhandledExceptionFilter(&HandleExceptionFilter);

  std::thread watchdog([thread_id = ::GetCurrentThreadId()] {
    ::Sleep(0);
    TakeJankSnapshot(thread_id);
  });
  SimulateJank();
  watchdog.join();

  SimulateCrash();

  ::SymCleanup(current_process);
  return 0;
}
