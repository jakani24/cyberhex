/*
This file contains the implementation of the create_task function. This function creates a task in the Windows Task Scheduler that runs the Cyberhex application on system boot. The task is created with the highest privilege level and is set to run on system boot. The task is set to run with the SYSTEM account and is set to run even if the system is on batteries. The task is set to have an execution time limit of zero seconds, meaning that it will run indefinitely. The task is registered with the Task Scheduler and saved to the system.

Functions:
	- create_task: Creates a task in the Windows Task Scheduler that runs the Cyberhex application on system boot.
*/

#include "service.h"
#include <iostream>
#include <string>
#include <Windows.h>
#include <taskschd.h>
#include <comdef.h>

using namespace std;


int create_task()
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        cout << "CoInitializeEx failed: " << hex << hr << endl;
        return 1;
    }

    hr = CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        0,
        NULL);

    if (FAILED(hr))
    {
        cout << "CoInitializeSecurity failed: " << hex << hr << endl;
        CoUninitialize();
        return 1;
    }

    LPCWSTR wszTaskName = L"CyberhexBackgroundTask";
    wstring wstrExecutablePath = L"c:\\program files\\cyberhex\\secure\\app\\cyberhex.exe";

    ITaskService* pService = NULL;
    hr = CoCreateInstance(CLSID_TaskScheduler,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ITaskService,
        (void**)&pService);
    if (FAILED(hr))
    {
        cout << "Failed to create an instance of ITaskService: " << hex << hr << endl;
        CoUninitialize();
        return 1;
    }

    hr = pService->Connect(_variant_t(), _variant_t(),
        _variant_t(), _variant_t());
    if (FAILED(hr))
    {
        cout << "ITaskService::Connect failed: " << hex << hr << endl;
        pService->Release();
        CoUninitialize();
        return 1;
    }

    ITaskFolder* pRootFolder = NULL;
    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr))
    {
        cout << "Cannot get Root Folder pointer: " << hex << hr << endl;
        pService->Release();
        CoUninitialize();
        return 1;
    }

    pRootFolder->DeleteTask(_bstr_t(wszTaskName), 0);

    ITaskDefinition* pTask = NULL;
    hr = pService->NewTask(0, &pTask);

    pService->Release();
    if (FAILED(hr))
    {
        cout << "Failed to create a task definition: " << hex << hr << endl;
        pRootFolder->Release();
        CoUninitialize();
        return 1;
    }

    IPrincipal* pPrincipal = NULL;
    hr = pTask->get_Principal(&pPrincipal);
    if (FAILED(hr))
    {
        cout << "Cannot get principal pointer: " << hex << hr << endl;
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    hr = pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);
    pPrincipal->Release();
    if (FAILED(hr))
    {
        cout << "Cannot set highest privilege level: " << hex << hr << endl;
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    ITaskSettings* pSettings = NULL;
    hr = pTask->get_Settings(&pSettings);
    if (FAILED(hr))
    {
        cout << "Cannot get settings pointer: " << hex << hr << endl;
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    hr = pSettings->put_ExecutionTimeLimit(_bstr_t(L"PT0S")); // Set execution time limit to zero seconds
    pSettings->Release();
    if (FAILED(hr))
    {
        cout << "Cannot set execution time limit: " << hex << hr << endl;
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return 1;
    }


    hr = pSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE); // Set to allow start on batteries
    pSettings->Release();
    if (FAILED(hr))
    {
        cout << "Cannot set start on batteries: " << hex << hr << endl;
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

   
	hr = pSettings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
	pSettings->Release();
	if (FAILED(hr))
	{
		cout << "Cannot set dont stop on batteries: " << hex << hr << endl;
		pRootFolder->Release();
		pTask->Release();
		CoUninitialize();
		return 1;
	}
    
	

    ITriggerCollection* pTriggerCollection = NULL;
    hr = pTask->get_Triggers(&pTriggerCollection);
    if (FAILED(hr))
    {
        cout << "Cannot get trigger collection: " << hex << hr << endl;
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    ITrigger* pTrigger = NULL;
    hr = pTriggerCollection->Create(TASK_TRIGGER_BOOT, &pTrigger);
    pTriggerCollection->Release();
    if (FAILED(hr))
    {
        cout << "Cannot create the trigger: " << hex << hr << endl;
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    IActionCollection* pActionCollection = NULL;
    hr = pTask->get_Actions(&pActionCollection);
    if (FAILED(hr))
    {
        cout << "Cannot get Task collection pointer: " << hex << hr << endl;
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    IAction* pAction = NULL;
    hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
    pActionCollection->Release();
    if (FAILED(hr))
    {
        cout << "Cannot create the action: " << hex << hr << endl;
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    IExecAction* pExecAction = NULL;
    hr = pAction->QueryInterface(
        IID_IExecAction, (void**)&pExecAction);
    pAction->Release();
    if (FAILED(hr))
    {
        cout << "QueryInterface call failed for IExecAction: " << hex << hr << endl;
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    hr = pExecAction->put_Path(_bstr_t(wstrExecutablePath.c_str()));
    pExecAction->Release();
    if (FAILED(hr))
    {
        cout << "Cannot set path of executable: " << hex << hr << endl;
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    IRegisteredTask* pRegisteredTask = NULL;
    VARIANT varPassword;
    varPassword.vt = VT_EMPTY;
    hr = pRootFolder->RegisterTaskDefinition(
        _bstr_t(wszTaskName),
        pTask,
        TASK_CREATE_OR_UPDATE,
        _variant_t(L"SYSTEM"),
        varPassword,
        TASK_LOGON_SERVICE_ACCOUNT,
        _variant_t(L""),
        &pRegisteredTask);
    if (FAILED(hr))
    {
        cout << "Error saving the Task : " << hex << hr << endl;
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    cout << "Success! Task successfully registered." << endl;

    pRootFolder->Release();
    pTask->Release();
    pRegisteredTask->Release();
    CoUninitialize();
    return 0;
}
