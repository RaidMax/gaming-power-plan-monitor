#include <Windows.h>
#include <consoleapi.h>
#include <iostream>
#include <Psapi.h>
#include <algorithm>
#include <cctype>
#include <vector>
#include <fstream>
#include <sstream>

static bool b_running;
static bool b_laststate = true;
const static int i_sleeptime = 10000;
std::vector<std::string> v_ignoredimages;
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
	// we're just converting the characters to uppercase so we can do a simple comparison
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
			// calculates the number of modules returned
			dwModuleCount = dwModuleBytes / sizeof(HMODULE);
			TCHAR szModuleName[128];

			for (unsigned int i = 0; i < dwModuleCount; i++)
			{
				if (GetModuleBaseNameA(hProcess, lphLoadedModules[i], szModuleName, sizeof(szModuleName)))
				{
					// calculates how many libraries to search for
					int iRenderLibraryCount = sizeof(lpszRenderLibraries) / sizeof(std::string);

					for (int j = 0; j < iRenderLibraryCount; j++)
					{
						if (IsMatchCaseInsensitive(std::string(szModuleName), std::string(lpszRenderLibraries[j])))
						{
							DWORD dwCharWrittenCount = MAX_PATH;
							char szNameBuffer[MAX_PATH];
							
							if (QueryFullProcessImageNameA(hProcess, 0, szNameBuffer, &dwCharWrittenCount)  && dwCharWrittenCount > 0)
							{
								for (std::string image : v_ignoredimages)
								{
									if (IsMatchCaseInsensitive(szNameBuffer, image))
									{
										return false;
									}
								}

								if (!b_laststate)
								{
									std::cout << "found " << szModuleName << " in " << szNameBuffer << std::endl;
								}
							}

							else
							{
								std::cout << "encountered error code "  << GetLastError() << " while getting process name for pid " << dwProcessId << std::endl;
							}

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

	std::cout << "gaming power plan monitor v0.3\n";

	std::ifstream hIgnore("ignore.cfg");
	
	for (std::string szTmp; std::getline(hIgnore, szTmp);)
	{
		v_ignoredimages.push_back(szTmp);
	}

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

			// b_running is false the first time the application runs the loop, so it should force a powerstate update
			if (b_laststate != bRenderLibraryLoaded || !b_running)
			{
				bRenderLibraryLoaded ?
					std::cout << "there appears to be a rendering library loaded, switching to max performance...\n" :
					std::cout << "there does not appear to be a rendering library loaded, switching to balanced...\n";

				b_laststate = bRenderLibraryLoaded;

				if ((hShellResult = (b_laststate ?
					ShellExecuteA(GetDesktopWindow(), "open", "cmd.exe", "/C powercfg /S SCHEME_MIN", 0, 0) :
					ShellExecuteA(GetDesktopWindow(), "open", "cmd.exe", "/C powercfg /S SCHEME_BALANCED", 0, 0))) < (HINSTANCE)31)
				{
					std::cout << "encountered error code " << hShellResult << " while attempting to change power plan\n";
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
