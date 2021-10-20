#pragma once

//
//

#include <mutex>

class Patrol
{
public:
	static HRESULT Initialize(_In_ Patrol* Context);
	static unsigned __stdcall StreamThread(_In_ PVOID ThreadParam);
	static unsigned __stdcall PatrolThread(_In_ PVOID ThreadParam);
	static VOID Run(_In_ Patrol* Pat);
	static VOID Watch(_In_ Patrol* Pat);

	BOOL IsInterrupted = FALSE;

	HANDLE StartEvent = NULL;
	HANDLE EndEvent = NULL;

	HANDLE ChanageEvent = NULL;
	HANDLE ListEvent = NULL;

	CHAR WorkBuffer[8]{};

	std::mutex Mutex;

private:

	FILETIME SavedLastWriteTime{};
	FILETIME SavedLastCheckedTime{};

	//PWSTR PatrolFolder = NULL;
	//BOOL Working = FALSE;

	uintptr_t StreamHandle = NULL;
	uintptr_t PatrolHandle = NULL;

	UINT StreamID = 0;
	UINT PatrolID = 0;

};
