#include <iostream>
#include <fstream>
#include <string>
#include <Windows.h>
#include <cmath> // ������ѧ����

const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;
const int TARGET_WIDTH = 640; // ����ΪĿ��ͼ����
const int TARGET_HEIGHT = 360; // ����ΪĿ��ͼ��߶�
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

// ��ȡ��Ļ
void CaptureScreen(HDC hdcWindow, HDC hdcMemDC) {
    BitBlt(hdcMemDC, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hdcWindow, 0, 0, SRCCOPY);
}

// ��ȡ�����¼�
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

        // ����Ŀ���С���ڴ�DC��λͼ
        HDC hdcTarget = CreateCompatibleDC(hdcWindow);
        HBITMAP hbmTarget = CreateCompatibleBitmap(hdcWindow, TARGET_WIDTH, TARGET_HEIGHT);
        SelectObject(hdcTarget, hbmTarget);

        // ����Ļ��ͼ���ŵ�Ŀ���С
        SetStretchBltMode(hdcTarget, HALFTONE); // ʹ�ø�������������ģʽ
        StretchBlt(hdcTarget, 0, 0, TARGET_WIDTH, TARGET_HEIGHT, hdcMemDC, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SRCCOPY);

        // �������ź��ͼ��
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

        // ����ͼ������
        BYTE* pixels = new BYTE[TARGET_WIDTH * TARGET_HEIGHT * 3];
        GetDIBits(hdcTarget, hbmTarget, 0, TARGET_HEIGHT, pixels, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
        imageFile.write(reinterpret_cast<char*>(pixels), bi.biSizeImage);
        delete[] pixels;
        imageFile.close();

        // �ͷ���Դ
        DeleteObject(hbmTarget);
        DeleteDC(hdcTarget);

        // ��ȡ�����¼�
        InputEvent input = CaptureInput();

        // ��¼���ݵ��ļ�
        dataFile << "frame:" << frameCount << ",keys:" << input.keys << "mouseButtons:" << input.mouseButtons
            << ",mouseX:" << input.mouseX << ",mouseY:" << input.mouseY << ",mouseWheel:" << input.mouseWheel << std::endl;

        // ����������
        if (GetAsyncKeyState(VK_MENU) && GetAsyncKeyState('Q')) {
            captureEnded = true;
        }

        frameCount++;
        Sleep(CAPTURE_DELAY);
    }

    // ���� frameCount ���ļ�
    std::ofstream frameCountFileOut("frame_count.txt", std::ios::trunc);
    frameCountFileOut << frameCount;
    frameCountFileOut.close();

    // �ر��ļ����ͷ���Դ
    dataFile.close();
    DeleteObject(hbmMemDC);
    DeleteDC(hdcMemDC);
    ReleaseDC(hwnd, hdcWindow);

    return 0;
}
