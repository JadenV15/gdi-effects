// Compile with:
// cl /EHsc /nologo /DUNICODE /D_UNICODE gdi.cpp user32.lib gdi32.lib

#include <Windows.h>
#include <iostream>
#include <cstdlib>
#include <cstring>

typedef union _MY_RGBQUAD {
    COLORREF rgb;
    struct {
        BYTE r;
        BYTE g;
        BYTE b;
        BYTE Reserved;
    };
} MY_RGBQUAD;

#define PRGBQUAD MY_RGBQUAD*

using ShaderFunction = void (*)(PRGBQUAD pixels, int width, int height);

// Shaders

static void Shader1(PRGBQUAD pixels, int width, int height)
{
    for (int i = 0; i < width * height; ++i) {
        int x = i % width;
        int y = i / width;

        pixels[i].rgb += x + y;
    }
}

static void Shader2(PRGBQUAD pixels, int width, int height)
{
    for (int i = 0; i < width * height; ++i) {
        int x = i % width;
        int y = i / width;

        pixels[i].rgb += x * y;
    }
}

static void Shader3(PRGBQUAD pixels, int width, int height)
{
    for (int i = 0; i < width * height; ++i) {
        pixels[i].rgb += 360;
    }
}

static void Shader4(PRGBQUAD pixels, int width, int height)
{
    for (int i = 0; i < width * height; ++i) {
        int x = i % width;
        int y = i / width;

        pixels[i].rgb += x ^ y;
    }
}

static void Shader5(PRGBQUAD pixels, int width, int height)
{
    for (int i = 0; i < width * height; ++i) {
        pixels[i].rgb =
            (pixels[i].rgb * 2) % RGB(255, 255, 255);
    }
}

// Shader lookup

static ShaderFunction GetShader(int shaderNumber)
{
    switch (shaderNumber) {
    case 1: return Shader1;
    case 2: return Shader2;
    case 3: return Shader3;
    case 4: return Shader4;
    case 5: return Shader5;
    default: return nullptr;
    }
}

// Run shader

static bool RunShader(
    ShaderFunction shader,
    DWORD timeoutMs)
{
    HDC hdcScreen = GetDC(NULL);
    if (!hdcScreen) {
        std::cerr << "Failed to get screen DC.\n";
        return false;
    }

    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    if (!hdcMem) {
        ReleaseDC(NULL, hdcScreen);
        std::cerr << "Failed to create memory DC.\n";
        return false;
    }

    const int width = GetSystemMetrics(SM_CXSCREEN);
    const int height = GetSystemMetrics(SM_CYSCREEN);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = height;

    PRGBQUAD pixels = nullptr;

    HBITMAP bitmap = CreateDIBSection(
        hdcScreen,
        &bmi,
        DIB_RGB_COLORS,
        reinterpret_cast<void**>(&pixels),
        NULL,
        0
    );

    if (!bitmap) {
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);
        std::cerr << "Failed to create DIB section.\n";
        return false;
    }

    HGDIOBJ oldBitmap = SelectObject(hdcMem, bitmap);

    const DWORD start = GetTickCount();

    while (GetTickCount() - start < timeoutMs) {

        // Capture desktop.
        if (!BitBlt(
            hdcMem,
            0, 0,
            width, height,
            hdcScreen,
            0, 0,
            SRCCOPY))
        {
            std::cerr << "BitBlt failed.\n";
            break;
        }

        // Apply shader.
        shader(pixels, width, height);

        // Shader 5 has its own special output behavior.
        if (shader == Shader5) {
            BitBlt(
                hdcScreen,
                0, 0,
                width, height,
                hdcMem,
                -30, 0,
                SRCCOPY
            );

            BitBlt(
                hdcScreen,
                0, 0,
                width, height,
                hdcMem,
                width - 30, 0,
                SRCCOPY
            );

            BitBlt(
                hdcScreen,
                0, 0,
                width, height,
                hdcMem,
                0, -30,
                SRCCOPY
            );

            BitBlt(
                hdcScreen,
                0, 0,
                width, height,
                hdcMem,
                0, height - 30,
                SRCCOPY
            );
        }
        else {
            BitBlt(
                hdcScreen,
                0, 0,
                width, height,
                hdcMem,
                0, 0,
                SRCCOPY
            );
        }
    }

    SelectObject(hdcMem, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    return true;
}

// Command line

static void PrintUsage(const char* program)
{
    std::cout
        << "Collection of GDI effects based on pankoza's work.\n"
        << "WARNING: For educational purposes only. Not suitable for those with epilepsy.\n\n"
        << "Usage: " << program << " <shader> <seconds> [-r]\n\n"
        << "Shaders:\n"
        << "  1\n"
        << "  2\n"
        << "  3\n"
        << "  4\n"
        << "  5\n\n"
        << "Options:\n"
        << "  -r: Repaint the corrupted screen after running the shader.\n"
        << "      NOTE: currently this opens a dialog window.\n\n"
        << "Examples:\n"
        << "  " << program << " 3 10\n"
        << "  " << program << " 3 10 -r\n";
}

/* TODO
static HWND CreateTinyWindow()
{
    const wchar_t className[] = L"TinyWindow";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = className;

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW,
        className,
        L"",
        WS_POPUP,
        0, 0, // x, y
        100, 100, // width, height
        NULL,
        NULL,
        wc.hInstance,
        NULL
    );

    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
    UpdateWindow(hwnd);

    return hwnd;
}
*/

int main(int argc, char* argv[])
{
    SetProcessDpiAwarenessContext(
        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
    );

    if (argc < 3 || argc > 4) {
        PrintUsage(argv[0]);
        return 1;
    }

    const int shaderNumber = std::atoi(argv[1]);
    const int seconds = std::atoi(argv[2]);

    if (shaderNumber < 1 || shaderNumber > 5) {
        std::cerr << "Invalid shader number. Use 1-5.\n";
        return 1;
    }

    if (seconds <= 0) {
        std::cerr << "Invalid duration. Use a positive number of seconds.\n";
        return 1;
    }

    bool repaint = false;

    if (argc == 4) {
        if (std::strcmp(argv[3], "-r") == 0) {
            repaint = true;
        }
        else {
            std::cerr << "Unknown option: " << argv[3] << "\n";
            return 1;
        }
    }

    ShaderFunction shader = GetShader(shaderNumber);

    if (!shader) {
        std::cerr << "Invalid shader.\n";
        return 1;
    }

    std::cout
        << "Running shader "
        << shaderNumber
        << " for "
        << seconds
        << " seconds...\n";

    RunShader(
        shader,
        static_cast<DWORD>(seconds) * 1000
    );

    if (repaint) {
        std::cout << "Repainting desktop...\n";
        
        InvalidateRect(NULL, NULL, TRUE);

        // TODO: proper repaint mechanism
        // Currently, showing a dialog seems to clear the screen of the mess
        MessageBoxW(NULL, L"Finished", L"", MB_OK | MB_ICONINFORMATION);
    }

    std::cout << "Finished.\n";

    return 0;
}
