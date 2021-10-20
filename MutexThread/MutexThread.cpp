#include <Windows.h>
#include <crtdbg.h>
#include <stdio.h>
#include <process.h>

#include "Patrol.h"

//!
//! unsigned __stdcall 
//!
unsigned __stdcall Patrol::StreamThread(_In_ PVOID ThreadParam)
{
	UINT ret = 0;

	Patrol* pf = (Patrol*)ThreadParam;
	pf->Run(pf);

	_endthreadex(ret);
	return ret;
}

//!
//! unsigned __stdcall 
//!
unsigned __stdcall Patrol::PatrolThread(_In_ PVOID ThreadParam)
{
	UINT ret = 0;

	Patrol* pf = (Patrol*)ThreadParam;
	pf->Watch(pf);

	_endthreadex(ret);
	return ret;
}

HRESULT Patrol::Initialize(_In_ Patrol* Context)
{
	HRESULT result = S_OK;

	Context->StartEvent = CreateEvent(
		NULL,               // default security attributes
		TRUE,               // manual-reset event
		FALSE,              // initial state is nonsignaled
		NULL                // object name
	);
	if (Context->StartEvent == NULL) {
		fprintf(stderr, "CreateEvent StartEvent failed (%d)\n", GetLastError());
		return E_ABORT;
	}

	Context->EndEvent = CreateEvent(
		NULL,               // default security attributes
		TRUE,               // manual-reset event
		FALSE,              // initial state is nonsignaled
		NULL                // object name
	);
	if (Context->EndEvent == NULL) {
		fprintf(stderr, "CreateEvent EndEvent failed (%d)\n", GetLastError());
		return E_ABORT;
	}

	Context->ChanageEvent = CreateEvent(
		NULL,               // default security attributes
		TRUE,               // manual-reset event
		FALSE,              // initial state is nonsignaled
		NULL                // object name
	);
	if (Context->ChanageEvent == NULL) {
		fprintf(stderr, "CreateEvent ChanageEvent failed (%d)\n", GetLastError());
		return E_ABORT;
	}

	Context->ListEvent = CreateEvent(
		NULL,               // default security attributes
		TRUE,               // manual-reset event
		FALSE,              // initial state is nonsignaled
		NULL                // object name
	);
	if (Context->ListEvent == NULL) {
		fprintf(stderr, "CreateEvent WaitEvent failed (%d)\n", GetLastError());
		return E_ABORT;
	}

	Context->StreamHandle = _beginthreadex(
		NULL,			// [in] Pointer to a SECURITY_ATTRIBUTES structure
		0,				// [in] Initial size of the stack, in bytes
						//		If this parameter is zero, the new thread uses the default size for the executable
		StreamThread,   // [in] Pointer to the application-defined function to be executed
						//    unsigned ( __stdcall *start_address )( void * ),
		Context,		// [in] Pointer to a variable to be passed to the thread
		0,				// [in] Flags that control the creation of the thread
						//    If this value is zero, the thread runs immediately after creation
		&Context->StreamID	// [out] Pointer to a variable that receives the thread identifier
	);
	_ASSERT(Context->StreamHandle);

	Context->PatrolHandle = _beginthreadex(
		NULL,			// [in] Pointer to a SECURITY_ATTRIBUTES structure
		0,				// [in] Initial size of the stack, in bytes
						//		If this parameter is zero, the new thread uses the default size for the executable
		PatrolThread,   // [in] Pointer to the application-defined function to be executed
						//    unsigned ( __stdcall *start_address )( void * ),
		Context,		// [in] Pointer to a variable to be passed to the thread
		0,				// [in] Flags that control the creation of the thread
						//    If this value is zero, the thread runs immediately after creation
		&Context->PatrolID	// [out] Pointer to a variable that receives the thread identifier
	);
	_ASSERT(Context->PatrolHandle);

	return result;
}

//! static Run
VOID
Patrol::Run(_In_ Patrol* Pat)
{
	DWORD waitResult;
	enum { eventCount = 2 };
	HANDLE waitHandles[eventCount];
	//LARGE_INTEGER lastTime, currentTime, ElapsedMicroseconds;

	printf("Run\n");

	waitHandles[0] = Pat->StartEvent;
	waitHandles[1] = Pat->EndEvent;

	do {
		//printf("\n");
		//printf("Wait for<%ws>\n", Pat->TargetFilePath);
		//fflush(stdout);

		waitResult = WaitForMultipleObjects(
			eventCount,
			waitHandles,
			FALSE,
			1000 * 5 /*INFINITE*/  //dwMilliseconds
		);

		if (waitResult == 0) {
			printf("waitResult == 0 (Start) ID=%lu\n", GetCurrentThreadId());
			ResetEvent(Pat->StartEvent);

			Pat->Mutex.lock();

			Pat->WorkBuffer[0] = 'W';
			Pat->WorkBuffer[1] = '\0';
			if (Pat->IsInterrupted) {
				printf("IsInterrupted W\n");
				Pat->Mutex.unlock();
				return;
			}
			Sleep(1000 * 1);
			if (Pat->IsInterrupted) {
				printf("IsInterrupted W 1\n");
				Pat->Mutex.unlock();
				return;
			}
			Pat->WorkBuffer[1] = 'O';
			Pat->WorkBuffer[2] = '\0';
			if (Pat->IsInterrupted) {
				printf("IsInterrupted O\n");
				Pat->Mutex.unlock();
				return;
			}
			Sleep(1000 * 1);
			if (Pat->IsInterrupted) {
				printf("IsInterrupted O 1\n");
				Pat->Mutex.unlock();
				return;
			}
			Pat->WorkBuffer[2] = 'R';
			Pat->WorkBuffer[3] = '\0';
			if (Pat->IsInterrupted) {
				printf("IsInterrupted R\n");
				Pat->Mutex.unlock();
				return;
			}
			Sleep(1000 * 1);
			if (Pat->IsInterrupted) {
				printf("IsInterrupted R 1\n");
				Pat->Mutex.unlock();
				return;
			}
			Pat->WorkBuffer[3] = 'K';
			Pat->WorkBuffer[4] = '\0';
			if (Pat->IsInterrupted) {
				printf("IsInterrupted K\n");
				Pat->Mutex.unlock();
				return;
			}

			printf("Run: <%s> ID=%lu\n", Pat->WorkBuffer, GetCurrentThreadId());

			Pat->Mutex.unlock();
		}
		else if (waitResult == 1) {
			printf("waitResult == 1 (End) ID=%lu\n", GetCurrentThreadId());
			ResetEvent(Pat->EndEvent);
		}
		else {
			printf("waitResult == %lu ID=%lu\n", waitResult, GetCurrentThreadId());
		}
	}
	while (!Pat->IsInterrupted);
}

//! static Patrol
VOID
Patrol::Watch(_In_ Patrol* Pat)
{
	DWORD waitResult;
	enum { eventCount = 3 };
	HANDLE waitHandles[eventCount];
	//LARGE_INTEGER lastTime, currentTime, ElapsedMicroseconds;

	printf("Patrol\n");

	waitHandles[0] = Pat->ChanageEvent;
	waitHandles[1] = Pat->ListEvent;
	waitHandles[2] = Pat->EndEvent;

	do {
		//printf("\n");
		//printf("Wait for<%ws>\n", Pat->TargetFilePath);

		//fflush(stdout);

		waitResult = WaitForMultipleObjects(
			eventCount,
			waitHandles,
			FALSE,
			1000 * 5 /*INFINITE*/  //dwMilliseconds
		);

		if (waitResult == 0) {
			printf("waitResult == 0 (Change) ID=%lu\n", GetCurrentThreadId());
			ResetEvent(Pat->ChanageEvent);
		}
		else if (waitResult == 1) {
			printf("waitResult == 1 (Wait) ID=%lu\n", GetCurrentThreadId());
			ResetEvent(Pat->ListEvent);
			printf("<%s>\n", Pat->WorkBuffer);
		}
		else if (waitResult == 2) {
			printf("waitResult == 1 (END) ID=%lu\n", GetCurrentThreadId());
			ResetEvent(Pat->EndEvent);
			break;
		}
		else {
			printf("waitResult == %lu ID=%lu\n", waitResult, GetCurrentThreadId());
		}
	}
	while (!Pat->IsInterrupted);
}

//!
int main()
{
	WCHAR line[MAX_PATH];
	WCHAR* p;
	BOOL ReopenDone = TRUE;
	Patrol pat{};
	Patrol* pf = &pat;

	printf("Hello Reopen Thread=%lu!\n", GetCurrentThreadId());

	pf->Initialize(pf);

	do {
		memset(line, 0, MAX_PATH);

		printf("Filename (to change), or Q[uit]? ");

		(void)fgetws(line, MAX_PATH, stdin);
		line[wcslen(line) - 1] = L'\0';

		p = &line[0];
		while (isspace(*p)) {
			p++;
		}
		switch (toupper(*p)) {

		case 'Q':
			pf->IsInterrupted = TRUE;
			Sleep(1000);

			goto EXIT;
			break;

		case 'S':
			if (!SetEvent(pf->StartEvent)) {
				printf("SetEvent failed (%d)\n", GetLastError());
				return 0;
			}
			break;

		case 'E':
			if (!SetEvent(pf->EndEvent)) {
				printf("SetEvent failed (%d)\n", GetLastError());
				return 0;
			}
			break;

		case 'L':
			if (!SetEvent(pf->ListEvent)) {
				printf("SetEvent failed (%d)\n", GetLastError());
				return 0;
			}
			break;

		case 'C':
		default:
			if (!SetEvent(pf->ChanageEvent)) {
				printf("SetEvent failed (%d)\n", GetLastError());
				return  0;
			}

			pf->Mutex.lock();

			pf->WorkBuffer[0] = 'C';
			pf->WorkBuffer[1] = '\0';
			printf("<%s>\n", pf->WorkBuffer);

			pf->Mutex.unlock();

			break;
		}
	} while (ReopenDone);

EXIT:

	return 0;
}
