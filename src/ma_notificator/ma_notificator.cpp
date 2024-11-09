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
std::string widestring_to_string(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(wstr);
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
        int argc;
        LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        // Create a multi-line edit control for displaying text
        hWndTextField = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            10, 10, width - 20, height - 20, hWnd, NULL, NULL, NULL);

        if (argc >= 2) {
			for (int i = 1; i < argc; i++) {
				update_textfield(hWndTextField, widestring_to_string(argv[i]) + "\r\n");
			}
        }
        else {
			update_textfield(hWndTextField, "No notification to display.");
        }
    }
    break;
    case WM_SIZE:
    {
        // Resize the text field to fit the window
        MoveWindow(hWndTextField, 10, 10, width - 20, height - 20 , TRUE);
        break;
    }
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId) {
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

    const wchar_t CLASS_NAME[] = L"Cyberhex endpoint protection notificator";

    WNDCLASS wc = { };

    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int windowWidth = 640;
    int windowHeight = 480;

    // Calculate the center position
    int xPos = (screenWidth - windowWidth) / 2;
    int yPos = (screenHeight - windowHeight) / 2;

    HWND hWnd = CreateWindowEx(
        WS_EX_TOPMOST,                         // Extended window style (TOPMOST)
        CLASS_NAME,                            // Window class name
        L"Cyberhex Endpoint Protection - Notificator", // Window title
        WS_OVERLAPPEDWINDOW,                   // Window style
        xPos,                                  // X position
        yPos,                                  // Y position
        windowWidth,                           // Width
        windowHeight,                          // Height
        NULL,                                  // Parent window
        NULL,                                  // Menu
        hInstance,                             // Instance handle
        NULL                                   // Additional application data
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
