#include <Windows.h>
#include <consoleapi.h>
#include <iostream>
#include <Psapi.h>

static bool b_Running;
const static int iSleepTime = 10000;
const static int iRenderLibraryCount = 2;
const static std::string lpszRenderLibraries[2] =
{
	"libEGL",
	"D3DX"
};

BOOL WINAPI HandlerRoutine(_In_ DWORD dwCtrlType)
{
	std::cout << "exiting...\n";
	SetConsoleCtrlHandler(HandlerRoutine, 0);
	b_Running = false;
	return 0;
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
					for (int j = 0; j < iRenderLibraryCount; j++)
					{
						if (std::string(szModuleName).find(lpszRenderLibraries[j]) != std::string::npos)
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
	b_Running = true;

	DWORD lpdwProcessIds[1024];
	DWORD dwProcessIdsCount;
	DWORD dwProcessIdsBytes;
	HINSTANCE hShellResult;
	bool bLastState = true;

	std::cout << "gaming power plan monitor v0.1\n";

	while (b_Running)
	{
		bool bRenderLibraryLoaded = false;

		if (EnumProcesses(lpdwProcessIds, sizeof(lpdwProcessIds), &dwProcessIdsBytes))
		{
			dwProcessIdsCount = dwProcessIdsBytes / sizeof(DWORD);

			for (unsigned int i = 0; i < dwProcessIdsCount; i++)
			{
				bRenderLibraryLoaded |= IsRenderLibraryLoaded(lpdwProcessIds[i]);
			}

			if (bLastState != bRenderLibraryLoaded)
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

		Sleep(iSleepTime);
	}
}
