#include <iostream>
#include <fstream>
#include <string>
#include <Windows.h>
#include <cmath> // 用于数学计算

const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;
const int TARGET_WIDTH = 640; // 更改为目标图像宽度
const int TARGET_HEIGHT = 360; // 更改为目标图像高度
const int FRAMES_PER_SECOND = 24;
const DWORD CAPTURE_DELAY = 1000 / FRAMES_PER_SECOND;

bool captureStarted = false;
bool captureEnded = false;
std::ofstream dataFile;

struct InputEvent {
    std::string keys;
    std::string mouseButtons;
    int mouseX, mouseY;
    int mouseWheel;
};

// 截取屏幕
void CaptureScreen(HDC hdcWindow, HDC hdcMemDC) {
    BitBlt(hdcMemDC, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hdcWindow, 0, 0, SRCCOPY);
}

// 截取输入事件
InputEvent CaptureInput() {
    InputEvent event;
    event.keys = "";
    event.mouseButtons = "";
    event.mouseWheel = 0;

    bool keyPressed = false;
    for (int key = 0x01; key <= 0xFE; key++) {
        if (GetAsyncKeyState(key) & 0x8000) {
            event.keys += std::to_string(key) + ",";
            keyPressed = true;
        }
    }
    if (!keyPressed) {
        event.keys = "none,";
    }

    for (int button = 0x01; button <= 0x07; button++) {
        if (GetAsyncKeyState(button) & 0x8000) {
            event.mouseButtons += std::to_string(button) + ",";
        }
    }

    POINT mousePos;
    GetCursorPos(&mousePos);
    event.mouseX = mousePos.x;
    event.mouseY = mousePos.y;
    event.mouseWheel = (SHORT)GetAsyncKeyState(0x08);

    return event;
}

int main() {
    HWND hwnd = GetDesktopWindow();
    HDC hdcWindow = GetDC(hwnd);
    HDC hdcMemDC = CreateCompatibleDC(hdcWindow);
    HBITMAP hbmMemDC = CreateCompatibleBitmap(hdcWindow, SCREEN_WIDTH, SCREEN_HEIGHT);
    SelectObject(hdcMemDC, hbmMemDC);

    std::ifstream frameCountFile("frame_count.txt");
    int frameCount = 0;
    if (frameCountFile.is_open()) {
        frameCountFile >> frameCount;
        frameCountFile.close();
    }

    std::cout << "Press F12 to start capture..." << std::endl;
    while (!GetAsyncKeyState(VK_F12)) {
        Sleep(100);
    }
    captureStarted = true;

    dataFile.open("game_data.txt", std::ios::out | std::ios::app);

    while (!captureEnded) {
        CaptureScreen(hdcWindow, hdcMemDC);

        // 创建目标大小的内存DC和位图
        HDC hdcTarget = CreateCompatibleDC(hdcWindow);
        HBITMAP hbmTarget = CreateCompatibleBitmap(hdcWindow, TARGET_WIDTH, TARGET_HEIGHT);
        SelectObject(hdcTarget, hbmTarget);

        // 将屏幕截图缩放到目标大小
        SetStretchBltMode(hdcTarget, HALFTONE); // 使用更高质量的缩放模式
        StretchBlt(hdcTarget, 0, 0, TARGET_WIDTH, TARGET_HEIGHT, hdcMemDC, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SRCCOPY);

        // 保存缩放后的图像
        std::string fileName = "frame_" + std::to_string(frameCount) + ".bmp";
        std::ofstream imageFile(fileName, std::ios::binary);
        BITMAPINFOHEADER bi;
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = TARGET_WIDTH;
        bi.biHeight = -TARGET_HEIGHT;  // Negative height to indicate top-down bitmap
        bi.biPlanes = 1;
        bi.biBitCount = 24;
        bi.biCompression = BI_RGB;
        bi.biSizeImage = 0;
        bi.biXPelsPerMeter = 0;
        bi.biYPelsPerMeter = 0;
        bi.biClrUsed = 0;
        bi.biClrImportant = 0;

        BITMAPFILEHEADER bmfh;
        bmfh.bfType = 0x4D42;  // "BM"
        bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        bmfh.bfSize = bmfh.bfOffBits + bi.biWidth * bi.biHeight * 3;  // 3 bytes per pixel
        bmfh.bfReserved1 = 0;
        bmfh.bfReserved2 = 0;
        imageFile.write(reinterpret_cast<char*>(&bmfh), sizeof(BITMAPFILEHEADER));
        imageFile.write(reinterpret_cast<char*>(&bi), sizeof(BITMAPINFOHEADER));

        // 保存图像数据
        BYTE* pixels = new BYTE[TARGET_WIDTH * TARGET_HEIGHT * 3];
        GetDIBits(hdcTarget, hbmTarget, 0, TARGET_HEIGHT, pixels, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
        imageFile.write(reinterpret_cast<char*>(pixels), bi.biSizeImage);
        delete[] pixels;
        imageFile.close();

        // 释放资源
        DeleteObject(hbmTarget);
        DeleteDC(hdcTarget);

        // 截取输入事件
        InputEvent input = CaptureInput();

        // 记录数据到文件
        dataFile << "frame:" << frameCount << ",keys:" << input.keys << "mouseButtons:" << input.mouseButtons
            << ",mouseX:" << input.mouseX << ",mouseY:" << input.mouseY << ",mouseWheel:" << input.mouseWheel << std::endl;

        // 检测结束条件
        if (GetAsyncKeyState(VK_MENU) && GetAsyncKeyState('Q')) {
            captureEnded = true;
        }

        frameCount++;
        Sleep(CAPTURE_DELAY);
    }

    // 保存 frameCount 到文件
    std::ofstream frameCountFileOut("frame_count.txt", std::ios::trunc);
    frameCountFileOut << frameCount;
    frameCountFileOut.close();

    // 关闭文件和释放资源
    dataFile.close();
    DeleteObject(hbmMemDC);
    DeleteDC(hdcMemDC);
    ReleaseDC(hwnd, hdcWindow);

    return 0;
}
