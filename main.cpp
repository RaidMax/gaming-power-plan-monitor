#include <Windows.h>
#include <consoleapi.h>
#include <iostream>
#include <Psapi.h>
#include <algorithm>
#include <cctype>

static bool b_running;
const static int i_sleeptime = 10000;
const static std::string lpszRenderLibraries[2] =
{
	"libEGL",
	"D3D"
};

BOOL WINAPI HandlerRoutine(_In_ DWORD dwCtrlType)
{
	std::cout << "exiting...\n";
	SetConsoleCtrlHandler(HandlerRoutine, 0);
	b_running = false;
	return 0;
}

bool IsMatchCaseInsensitive(std::string str1, std::string str2)
{
	std::transform(str1.begin(), str1.end(), str1.begin(), [](unsigned char c) { return std::toupper(c); });
	std::transform(str2.begin(), str2.end(), str2.begin(), [](unsigned char c) { return std::toupper(c); });

	return str1.find(str2) != std::string::npos;
}

bool IsRenderLibraryLoaded(DWORD dwProcessId)
{
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessId);

	if (hProcess != NULL)
	{
		HMODULE lphLoadedModules[1024];
		DWORD dwModuleBytes;
		DWORD dwModuleCount;

		if (EnumProcessModulesEx(hProcess, lphLoadedModules, sizeof(lphLoadedModules), &dwModuleBytes, LIST_MODULES_ALL))
		{
			dwModuleCount = dwModuleBytes / sizeof(HMODULE);
			TCHAR szModuleName[128];

			for (unsigned int i = 0; i < dwModuleCount; i++)
			{
				if (GetModuleBaseNameA(hProcess, lphLoadedModules[i], szModuleName, sizeof(szModuleName)))
				{
					int iRenderLibraryCount = sizeof(lpszRenderLibraries) / sizeof(std::string);

					for (int j = 0; j < iRenderLibraryCount; j++)
					{
						if (IsMatchCaseInsensitive(std::string(szModuleName), std::string(lpszRenderLibraries[j])))
						{
							return true;
						}
					}
				}

				else
				{
					std::cout << "encountered error code " << GetLastError() << " while attempting to get module filename\n";
				}
			}
		}
	}

	return false;
}

int main(char* argc, char** argv)
{
	SetConsoleCtrlHandler(HandlerRoutine, 1);

	DWORD lpdwProcessIds[1024];
	DWORD dwProcessIdsCount;
	DWORD dwProcessIdsBytes;
	HINSTANCE hShellResult;
	bool bLastState = true;

	std::cout << "gaming power plan monitor v0.2\n";

	do
	{
		bool bRenderLibraryLoaded = false;

		if (EnumProcesses(lpdwProcessIds, sizeof(lpdwProcessIds), &dwProcessIdsBytes))
		{
			dwProcessIdsCount = dwProcessIdsBytes / sizeof(DWORD);

			for (unsigned int i = 0; i < dwProcessIdsCount; i++)
			{
				bRenderLibraryLoaded |= IsRenderLibraryLoaded(lpdwProcessIds[i]);
			}

			if (bLastState != bRenderLibraryLoaded || !b_running)
			{
				bRenderLibraryLoaded ?
					std::cout << "there appears to be a rendering library loaded, switching to max performance...\n" :
					std::cout << "there does not appear to be a rendering library loaded, switching to balanced...\n";
				bLastState = bRenderLibraryLoaded;

				if ((hShellResult = (bLastState ? ShellExecuteA(GetDesktopWindow(), "open", "cmd.exe", "/C powercfg /S SCHEME_MIN", 0, 0) : ShellExecuteA(GetDesktopWindow(), "open", "cmd.exe", "/C powercfg /S SCHEME_BALANCED", 0, 0))) < (HINSTANCE)31)
				{
					std::cout << "encountered hinstance error code " << hShellResult << " while attempting to change power plan\n";
				};
			}
		}

		else
		{
			std::cout << "encountered error code " << GetLastError() << " while attempting to enumerate processes\n";
		}

		Sleep(i_sleeptime);
		b_running = true;
	} while (b_running);
}
