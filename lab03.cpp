#include <windows.h>
#include <psapi.h>
#include <string>
#include <CommCtrl.h>  
#include <tlhelp32.h>



SIZE_T physicalUsed = 0;
SIZE_T physicalTotal = 0;
SIZE_T virtualUsed = 0;
SIZE_T virtualTotal = 0;

void ShowErrorMessageBox(const wchar_t* name) {
    DWORD errorCode = GetLastError();

    if (errorCode == 0) {
        MessageBox(NULL, L"No error.", name, MB_OK | MB_ICONINFORMATION);
        return;
    }

    LPVOID errorMsg;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        errorCode,
        0,
        (LPWSTR)&errorMsg,
        0,
        NULL
    );

    if (errorMsg != NULL) {
        MessageBox(NULL, static_cast<LPCWSTR>(errorMsg),name, MB_OK | MB_ICONERROR);

        LocalFree(errorMsg);
    }
    else {
        MessageBox(NULL, L"Unable to retrieve error message.", L"Error", MB_OK | MB_ICONERROR);
    }
}

void UpdateMemoryInfo(HWND hwnd) {
    MEMORYSTATUSEX perfInfo;
    perfInfo.dwLength = sizeof(perfInfo);
    GlobalMemoryStatusEx(&perfInfo);
   // GetLastError();
    physicalUsed = perfInfo.ullTotalPhys - perfInfo.ullAvailPhys;
    physicalTotal = perfInfo.ullTotalPhys;
    virtualUsed = perfInfo.ullTotalVirtual - perfInfo.ullAvailVirtual;
    virtualTotal = perfInfo.ullTotalVirtual;

    HWND labelPhysicalUsed = GetDlgItem(hwnd, 1001);
    HWND labelPhysicalTotal = GetDlgItem(hwnd, 1002);
    HWND labelVirtualUsed = GetDlgItem(hwnd, 1003);
    HWND labelVirtualTotal = GetDlgItem(hwnd, 1004);

    SetWindowText(labelPhysicalUsed, (L"Physical Used: " + std::to_wstring(physicalUsed/1024)).c_str());
    SetWindowText(labelPhysicalTotal, (L"Physical Total: " + std::to_wstring(physicalTotal/1024)).c_str());
    SetWindowText(labelVirtualUsed, (L"Virtual Used: " + std::to_wstring(virtualUsed/1024)).c_str());
    SetWindowText(labelVirtualTotal, (L"Virtual Total: " + std::to_wstring(virtualTotal/1024)).c_str());
}


void HandleListViewClick(HWND hwnd, LPARAM lParam) {
    LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
    if (pnmv->iItem < 0)
    {
        return;
    }
    wchar_t processID[256];
    ListView_GetItemText(hwnd, pnmv->iItem, 1, processID, sizeof(processID) / sizeof(processID[0]));
}
void CreateProcTable(HWND hwndTable) {
    LVCOLUMN lvColumn;
    lvColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    lvColumn.cx = 100;  
    lvColumn.pszText = (LPWSTR)L"Process Name"; 
    lvColumn.iSubItem = 0;

    CreateWindow(WC_LISTVIEW, L"", WS_VISIBLE | WS_CHILD, 10, 130, 150, 30, hwndTable, (HMENU)1007, GetModuleHandle(NULL), NULL);
    ShowErrorMessageBox(L"CW createProc");

    SendMessage(hwndTable, LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);
    ShowErrorMessageBox(L"SM");


    const wchar_t* columnHeaders[] = { L"Process ID", L"Parent Process ID", L"Base Priority", L"Thread Count", L"Module Count" };
    for (int i = 1; i <= 5; ++i) {
        LVCOLUMN lvColumn;
        lvColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        lvColumn.cx = 100;  
        lvColumn.pszText = (LPWSTR)columnHeaders[i - 1];
        lvColumn.iSubItem = i;

        SendMessage(hwndTable, LVM_INSERTCOLUMN, i, (LPARAM)&lvColumn);
    }


    SendMessage(hwndTable, LVM_DELETEALLITEMS, 0, 0);

    HANDLE const hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == hSnapshot) {
        return;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32)) {
        int i = 0;
        LVITEM lvItem;
        lvItem.mask = LVIF_TEXT;
        do {

            lvItem.iItem = i;
            lvItem.iSubItem = 0;
            lvItem.pszText = pe32.szExeFile;
            ListView_InsertItem(hwndTable, &lvItem);


            lvItem.iSubItem = 1;
            std::wstring w;
            w = std::to_wstring(pe32.th32ProcessID);
            lvItem.pszText = w.data();
            ListView_SetItem(hwndTable, &lvItem);

            lvItem.iSubItem = 2;
            w = std::to_wstring(pe32.th32ParentProcessID);
            lvItem.pszText = w.data();
            ListView_SetItem(hwndTable, &lvItem);

            lvItem.iSubItem = 3;
            w = std::to_wstring(pe32.pcPriClassBase);
            lvItem.pszText = w.data();
            ListView_SetItem(hwndTable, &lvItem);

            lvItem.iSubItem = 4;
            w = std::to_wstring(pe32.cntThreads);
            lvItem.pszText = w.data();
            ListView_SetItem(hwndTable, &lvItem);

            lvItem.iSubItem = 5;
            w = std::to_wstring(pe32.cntUsage);
            lvItem.pszText = w.data();
            ListView_SetItem(hwndTable, &lvItem);

            i++;
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
}

LRESULT CALLBACK LVProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
       CreateProcTable(hwnd);
        break;
    case WM_NOTIFY: {
        LPNMHDR pnmhdr = (LPNMHDR)lParam;
        if (pnmhdr->code == NM_CLICK) {
          /*TODO logic*/

        }
        break;
    }
    default: 
    {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    }
    return 0;
}

void HandleListViewDoubleClick(HWND hwnd, int itemIndex) {
    LVITEM lvItem;
    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = itemIndex;
    lvItem.iSubItem = 0;
    lvItem.pszText = new wchar_t[MAX_PATH];
    lvItem.cchTextMax = MAX_PATH;
    ListView_GetItem(hwnd, &lvItem);
    delete[] lvItem.pszText;
}


void InitProcessTableWindow(HWND pHwnd) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = LVProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"ProcessTableClass";
    RegisterClass(&wc);
    ShowErrorMessageBox(L"RegClass");

    HWND hwndTable = CreateWindow(L"ProcessTableClass", L"ProcessTableClass", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 300, NULL, NULL, GetModuleHandle(NULL), NULL);
    ShowWindow(hwndTable, SW_SHOWNORMAL);
    UpdateWindow(hwndTable);
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        CreateWindow(L"BUTTON", L"Show Process Table", WS_VISIBLE | WS_CHILD, 10, 130, 150, 30, hwnd, (HMENU)1005, NULL, NULL);
        CreateWindow(L"STATIC", L"Physical Used:", WS_VISIBLE | WS_CHILD, 10, 10, 300, 20, hwnd, (HMENU)1001, NULL, NULL);
        CreateWindow(L"STATIC", L"Physical Total:", WS_VISIBLE | WS_CHILD, 10, 40, 300, 20, hwnd, (HMENU)1002, NULL, NULL);
        CreateWindow(L"STATIC",L"Virtual Used:", WS_VISIBLE | WS_CHILD, 10, 70, 300, 20, hwnd, (HMENU)1003, NULL, NULL);
        CreateWindow(L"STATIC", L"Virtual Total:", WS_VISIBLE | WS_CHILD, 10, 100, 300, 20, hwnd, (HMENU)1004, NULL, NULL);
        SetTimer(hwnd, 1, 1000, NULL);
        break;
    }
    case WM_TIMER: {

        UpdateMemoryInfo(hwnd);
        break;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 1005) {
            InitProcessTableWindow(hwnd);
        }
        break;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"MemoryInfoWindowClass";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, L"MemoryInfoWindowClass", L"Memory Info", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 200, NULL, NULL, GetModuleHandle(NULL), NULL);

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
