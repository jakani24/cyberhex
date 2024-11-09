#include <Windows.h>
#include <shlobj.h>
#include <thread>
#include <chrono>
#include <fstream>
#include <codecvt>
#include <locale>
#include <string>
#include "../client_backend/well_known.h"

#define IDM_SCAN_FILE 101
#define IDM_SCAN_FOLDER 102
#define IDM_DEEP_SCAN_FILE 103
#define IDM_DEEP_SCAN_FOLDER 104
#define IDM_QUICK_SCAN 105


std::wstring string_to_widestring(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}
void send_command(const std::string& command) {
    std::ofstream outputFile(MAIN_COM_PATH);
    if (outputFile.is_open()) {
        outputFile << command;
        outputFile.close();
    }
}

// Function to update the content of the text field with the provided text
void update_textfield(HWND hWndTextField, const std::string& text) {
    // Get the current text length
    int textLength = GetWindowTextLength(hWndTextField);

    // Set the selection to the end of the text field
    SendMessage(hWndTextField, EM_SETSEL, textLength, textLength);

    // Append the new text
    SendMessage(hWndTextField, EM_REPLACESEL, FALSE, (LPARAM)string_to_widestring(text).c_str());
}

void scan_file(HWND hWndTextField, const std::string& filePath, bool deep) {
    // Remove the answer file
    std::remove(ANSWER_COM_PATH);
    // Display the scanned file path in the window
    if(!deep)
        update_textfield(hWndTextField, "Scanning file: " + filePath + "\r\n");
    else
        update_textfield(hWndTextField, "Deep scanning file: " + filePath + "\r\n");
    bool answered = false;
    // Write command into com file
    //printf("%d\n",send_to_pipe("scanfile \"" + filePath + "\""));
    std::ofstream outputFile(MAIN_COM_PATH);
    if (outputFile.is_open()) {
        if(!deep)
			outputFile << "scanfile \"" << filePath << "\"";
		else
            outputFile << "deepscanfile \"" << filePath << "\"";
        outputFile.close();
    }
    else {
        update_textfield(hWndTextField, "Error: Unable to talk to daemon!\n");
        return;
    }
    while (!answered) {
        // Wait for answer in file
        std::ifstream inputFile(ANSWER_COM_PATH);
        // The structure of the answer file is as follows:
        // found/not_found
        // filepath
        // hash
        // action_taken/no_action_taken
        if (inputFile.is_open()) {
            std::string status, scannedFilePath, hash, action;
            if (inputFile >> status) {
                inputFile.ignore(1); // Ignore space
                inputFile.ignore(1); // Ignore starting double quote
                if (status == "found" || status == "not_found") {
                    std::getline(inputFile, scannedFilePath, '\"'); // Read until closing double quote
                    inputFile.ignore(1); // Ignore space between filepath and hash
                    inputFile.ignore(1); // Ignore starting double quote
                    std::getline(inputFile, hash, ' '); // Read until space
                    std::getline(inputFile, action); // Read until end of line

                    answered = true;

                    if (status == "found") {
                        update_textfield(hWndTextField, "Virus found in file: " + scannedFilePath + "\r\n");
                        update_textfield(hWndTextField, "Hash: " + hash + "\r\n");
                        update_textfield(hWndTextField, "Action taken: " + action + "\r\n");

                    }
                    else {
                        update_textfield(hWndTextField, "No virus found in file: " + scannedFilePath + "\r\n");
                    }
                    update_textfield(hWndTextField, "------------------------------------------------------------------------------------\r\n");
                }
            }
            else {
                answered = true;
                update_textfield(hWndTextField, "Error: Unable to talk to daemon!\n");
                update_textfield(hWndTextField, "------------------------------------------------------------------------------------\r\n");
            }
            inputFile.close();
            std::remove(ANSWER_COM_PATH);
        }
        // Wait for 1 second before checking again
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    // Remove the answer file
    std::remove(ANSWER_COM_PATH);
}

// Function to simulate folder scanning
void scan_folder(HWND hProgressBar,HWND hWndTextField, const std::string& folderPath,bool deep) {
    //set progress bar to 0
    SendMessage(hProgressBar, PBM_SETPOS, 0, 0);
    int num_of_found = 0;
    // Remove the answer file
    std::remove(ANSWER_COM_PATH);
    // Display the scanned folder path in the window
    if(!deep)
        update_textfield(hWndTextField, "Scanning folder: " + folderPath + "\r\n");
    else
        update_textfield(hWndTextField, "Deep scanning folder: " + folderPath + "\r\n");
    bool answered = false;
    // Write command into com file
    std::ofstream outputFile(MAIN_COM_PATH);
    if (outputFile.is_open()) {
        if(!deep)
            outputFile << "scanfolder \"" << folderPath << "\"";
        else
            outputFile << "deepscanfolder \"" << folderPath << "\"";
        outputFile.close();
    }
    else {
        update_textfield(hWndTextField, "Error: Unable to talk to daemon!\n");
        return;
    }
    while (!answered) {
        // Wait for answer in file
        std::ifstream inputFile(ANSWER_COM_PATH);
        // The structure of the answer file is as follows:
        // found/not_found
        // filepath
        // hash
        // action_taken/no_action_taken
        if (inputFile.is_open()) {
            std::string status, scannedFilePath, hash, action;
            while (!inputFile.eof()) {
                if (inputFile >> status) {
                    if (status == "found" || status == "not_found") {
                        inputFile.ignore(1); // Ignore space
                        inputFile.ignore(1); // Ignore starting double quote
                        std::getline(inputFile, scannedFilePath, '\"'); // Read until closing double quote
                        inputFile.ignore(1); // Ignore space between filepath and hash
                        inputFile.ignore(1); // Ignore starting double quote
                        std::getline(inputFile, hash, ' '); // Read until space
                        std::getline(inputFile, action); // Read until end of line

                        //answered = true;

                        if (status == "found") {
                            update_textfield(hWndTextField, "Virus found in file: " + scannedFilePath + "\r\n");
                            update_textfield(hWndTextField, "Hash: " + hash + "\r\n");
                            update_textfield(hWndTextField, "Action taken: " + action + "\r\n");
                            num_of_found++;
                        }
                        else {
                            update_textfield(hWndTextField, "No virus found in file: " + scannedFilePath + "\r\n");
                        }
                        update_textfield(hWndTextField, "------------------------------------------------------------------------------------\r\n");
                    }
                    else if (status == "progress") {
                        std::string progress;
                        inputFile.ignore(1); // Ignore space
						inputFile >> progress;
						SendMessage(hProgressBar, PBM_SETPOS, std::stoi(progress), 0);

                    }
                    else if (status == "start") {
                        std::string all_files;
                        inputFile.ignore(1); // Ignore space
                        inputFile >> all_files;
                        update_textfield(hWndTextField, "Folder scan started with "+ all_files +" files queued for scan\r\n\r\n");
                    }
                    else if (status == "end") {
                        answered = true;
                    }
                }
            }
            inputFile.close();
            std::ofstream(ANSWER_COM_PATH);//clear the file
            Sleep(1000);//only see for new entrys ~ once a second
        }
        // Wait for 1 second before checking again
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    update_textfield(hWndTextField, "Folder scan completed\r\n");
    update_textfield(hWndTextField, "Number of infected files: " + std::to_string(num_of_found) + "\r\n");
    update_textfield(hWndTextField, "------------------------------------------------------------------------------------\r\n");
    SendMessage(hProgressBar, PBM_SETPOS, 100, 0);
    // Remove the answer file
    std::remove(ANSWER_COM_PATH);
}

void quick_scan(HWND hProgressBar, HWND hWndTextField) {
    //set progress bar to 0
    SendMessage(hProgressBar, PBM_SETPOS, 0, 0);
    int num_of_found = 0;
    // Remove the answer file
    std::remove(ANSWER_COM_PATH);

    update_textfield(hWndTextField, "Running quick scan\r\n");
    bool answered = false;
    // Write command into com file
    std::ofstream outputFile(MAIN_COM_PATH);
    if (outputFile.is_open()) {
        outputFile << "quick_scan \"" << "_._" << "\"";
        outputFile.close();
    }
    else {
        update_textfield(hWndTextField, "Error: Unable to talk to daemon!\n");
        return;
    }

    while (!answered) {
        // Wait for answer in file
        std::ifstream inputFile(ANSWER_COM_PATH);
        // The structure of the answer file is as follows:
        // found/not_found
        // filepath
        // hash
        // action_taken/no_action_taken
        if (inputFile.is_open()) {
            std::string status, scannedFilePath, hash, action;
            while (!inputFile.eof()) {
                if (inputFile >> status) {
                    if (status == "found" || status == "not_found") {
                        inputFile.ignore(1); // Ignore space
                        inputFile.ignore(1); // Ignore starting double quote
                        std::getline(inputFile, scannedFilePath, '\"'); // Read until closing double quote
                        inputFile.ignore(1); // Ignore space between filepath and hash
                        inputFile.ignore(1); // Ignore starting double quote
                        std::getline(inputFile, hash, ' '); // Read until space
                        std::getline(inputFile, action); // Read until end of line

                        //answered = true;

                        if (status == "found") {
                            update_textfield(hWndTextField, "Virus found in file: " + scannedFilePath + "\r\n");
                            update_textfield(hWndTextField, "Hash: " + hash + "\r\n");
                            update_textfield(hWndTextField, "Action taken: " + action + "\r\n");
                            num_of_found++;
                        }
                        else {
                            update_textfield(hWndTextField, "No virus found in file: " + scannedFilePath + "\r\n");
                        }
                        update_textfield(hWndTextField, "------------------------------------------------------------------------------------\r\n");
                    }
                    else if (status == "start") {
                        std::string all_files;
                        inputFile.ignore(1); // Ignore space
                        inputFile >> all_files;
                        update_textfield(hWndTextField, "Quick scan started\r\n\r\n");
                    }
                    else if (status == "end") {
                        answered = true;
                    }
                }
            }
            inputFile.close();
            std::ofstream(ANSWER_COM_PATH);//clear the file
            Sleep(1000);//only see for new entrys ~ once a second
        }
        // Wait for 1 second before checking again
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    update_textfield(hWndTextField, "Quick scan completed\r\n");
    update_textfield(hWndTextField, "Number of infected files: " + std::to_string(num_of_found) + "\r\n");
    update_textfield(hWndTextField, "------------------------------------------------------------------------------------\r\n");
    SendMessage(hProgressBar, PBM_SETPOS, 100, 0);
    // Remove the answer file
    std::remove(ANSWER_COM_PATH);
}

std::string getFolderPath(HWND hWnd) {
    std::wstring selectedFolderPath;

    // Initialize COM
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    // Display the folder picker dialog
    BROWSEINFO browseInfo = { 0 };
    TCHAR selectedPath[MAX_PATH];
    browseInfo.hwndOwner = hWnd; // Set the owner window
    browseInfo.pidlRoot = NULL;  // Start from the desktop
    browseInfo.pszDisplayName = selectedPath;
    browseInfo.lpszTitle = TEXT("Select a folder");
    browseInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidlSelected = SHBrowseForFolder(&browseInfo);
    if (pidlSelected != NULL) {
        SHGetPathFromIDList(pidlSelected, selectedPath);

        selectedFolderPath = selectedPath;

        // Free the PIDL
        IMalloc* pMalloc;
        if (SUCCEEDED(SHGetMalloc(&pMalloc))) {
            pMalloc->Free(pidlSelected);
            pMalloc->Release();
        }
    }

    // Uninitialize COM
    CoUninitialize();

    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::string narrowSelectedFolder = converter.to_bytes(selectedFolderPath);
    return narrowSelectedFolder;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static HWND hWndTextField;
    static HWND hProgressBar;
    static HBRUSH hBackgroundBrush = CreateSolidBrush(RGB(255, 255, 255)); // White color
    RECT rect;
    GetClientRect(hWnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    switch (message) {
    case WM_CREATE:
    {

        // Create the "Scan File" button
        CreateWindowEx(NULL, L"BUTTON", L"Scan File",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            20, 10, 150, 30, hWnd, (HMENU)IDM_SCAN_FILE, GetModuleHandle(NULL), NULL);

        // Create the "Scan Folder" button
        CreateWindowEx(NULL, L"BUTTON", L"Scan Folder",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            20, 50, 150, 30, hWnd, (HMENU)IDM_SCAN_FOLDER, GetModuleHandle(NULL), NULL);

        // Create the "Deep Scan File" button
        CreateWindowEx(NULL, L"BUTTON", L"Deep Scan File",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            20, 90, 150, 30, hWnd, (HMENU)IDM_DEEP_SCAN_FILE, GetModuleHandle(NULL), NULL);

        // Create the "Deep Scan Folder" button
        CreateWindowEx(NULL, L"BUTTON", L"Deep Scan Folder",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            20, 130, 150, 30, hWnd, (HMENU)IDM_DEEP_SCAN_FOLDER, GetModuleHandle(NULL), NULL);

        //create the quick scan button
        CreateWindowEx(NULL, L"BUTTON", L"Quick Scan",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            20, 170, 150, 30, hWnd, (HMENU)IDM_QUICK_SCAN, GetModuleHandle(NULL), NULL);

        // Create a multi-line edit control for displaying text
        hWndTextField = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            190, 10, width-190-20, height-10-50, hWnd, NULL, NULL, NULL);
        update_textfield(hWndTextField, "Welcome to Cyberhex endpoint protection!\r\n\r\n");

        hProgressBar = CreateWindowEx(0, PROGRESS_CLASS, NULL,
            WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
            190, height-40, width - 190 - 20, 20, hWnd, NULL, NULL, NULL);
        SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendMessage(hProgressBar, PBM_SETSTEP, 1, 0);
    }
    break;
    case WM_SIZE:
    {
		// Resize the text field to fit the window
		MoveWindow(hWndTextField, 190, 10, width - 190 - 20, height - 10 - 50, TRUE);
        MoveWindow(hProgressBar, 190, height - 40, width - 190 - 20, 20, TRUE);
        break;
	}
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId) {
        case IDM_SCAN_FILE:
        {
            // Open file dialog to select a file
            // Call scan_file function in a separate thread
            OPENFILENAME ofn;
            WCHAR szFile[MAX_PATH] = L""; // Use WCHAR for Unicode compatibility

            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hWnd;
            ofn.lpstrFile = szFile;
            ofn.lpstrFile[0] = L'\0'; // Use wide character constant L'\0'
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = L"All Files\0*.*\0"; // Use wide character string literal L""
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.lpstrInitialDir = NULL;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileName(&ofn) == TRUE) {
                std::wstring selectedFile = ofn.lpstrFile; // Use std::wstring for wide characters
                std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
                std::string narrowSelectedFile = converter.to_bytes(selectedFile);
                std::thread(scan_file, hWndTextField, narrowSelectedFile,0).detach();
            }
        }
        break;
        case IDM_SCAN_FOLDER:
        {
            // Open folder picker dialog
            // Call scan_folder function in a separate thread
            std::string selected_folder = getFolderPath(hWnd);
            if(selected_folder!="")
                std::thread(scan_folder,hProgressBar, hWndTextField, selected_folder,0).detach();
        }
        break;
        case IDM_DEEP_SCAN_FILE:
        {
            // Open file dialog to select a file
            // Call scan_file function in a separate thread
            OPENFILENAME ofn;
            WCHAR szFile[MAX_PATH] = L""; // Use WCHAR for Unicode compatibility

            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hWnd;
            ofn.lpstrFile = szFile;
            ofn.lpstrFile[0] = L'\0'; // Use wide character constant L'\0'
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = L"All Files\0*.*\0"; // Use wide character string literal L""
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.lpstrInitialDir = NULL;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileName(&ofn) == TRUE) {
                std::wstring selectedFile = ofn.lpstrFile; // Use std::wstring for wide characters
                std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
                std::string narrowSelectedFile = converter.to_bytes(selectedFile);
                std::thread(scan_file, hWndTextField, narrowSelectedFile, 1).detach();
            }

        }
        break;
        case IDM_DEEP_SCAN_FOLDER:
        {
			// Open folder picker dialog
			// Call scan_folder function in a separate thread
			std::string selected_folder = getFolderPath(hWnd);
			if(selected_folder!="")
				std::thread(scan_folder,hProgressBar, hWndTextField, selected_folder,1).detach();
		}
        break;
        case IDM_QUICK_SCAN:
        {
            std::thread(quick_scan, hProgressBar, hWndTextField).detach();
		}
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
 
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rect;
        GetClientRect(hWnd, &rect);
        FillRect(hdc, &rect, hBackgroundBrush); // Fill the entire client area with white color
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"Cyberhex endpoint protection frontend";

    WNDCLASS wc = { };

    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hWnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Cyberhex endpoint protection",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 
        CW_USEDEFAULT, 
        640, 
        480,
        NULL,
        NULL,
        hInstance,
        NULL
    );


    if (hWnd == NULL) {
        return 0;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
