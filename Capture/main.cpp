#include <iostream>
#include <fstream>
#include <Windows.h>
#include <vector>
#include "jpeglib.h"
#include <string>

const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;
const int TARGET_WIDTH = 640;
const int TARGET_HEIGHT = 360;
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

void CaptureScreen(HDC hdcWindow, HDC hdcMemDC) {
    BitBlt(hdcMemDC, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, hdcWindow, 0, 0, SRCCOPY);
}

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

void SaveJpeg(const char* filename, BYTE* imageData, int width, int height) {
    FILE* outfile;
    if (fopen_s(&outfile, filename, "wb") != 0) {
        std::cerr << "Error opening output JPEG file!" << std::endl;
        return;
    }

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 90, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    JSAMPROW row_pointer[1];
    int row_stride = width * 3;

    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = &imageData[cinfo.next_scanline * row_stride];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    fclose(outfile);
    jpeg_destroy_compress(&cinfo);
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

        HDC hdcTarget = CreateCompatibleDC(hdcWindow);
        HBITMAP hbmTarget = CreateCompatibleBitmap(hdcWindow, TARGET_WIDTH, TARGET_HEIGHT);
        SelectObject(hdcTarget, hbmTarget);

        SetStretchBltMode(hdcTarget, HALFTONE);
        StretchBlt(hdcTarget, 0, 0, TARGET_WIDTH, TARGET_HEIGHT, hdcMemDC, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SRCCOPY);

        std::string fileName = "frame_" + std::to_string(frameCount) + ".jpg";
        std::vector<BYTE> imageData(TARGET_WIDTH * TARGET_HEIGHT * 3);

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
        bmi.bmiHeader.biWidth = TARGET_WIDTH;
        bmi.bmiHeader.biHeight = -TARGET_HEIGHT; // Negative height to indicate top-down bitmap
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 24;
        bmi.bmiHeader.biCompression = BI_RGB;

        GetDIBits(hdcTarget, hbmTarget, 0, TARGET_HEIGHT, imageData.data(), &bmi, DIB_RGB_COLORS);

        SaveJpeg(fileName.c_str(), imageData.data(), TARGET_WIDTH, TARGET_HEIGHT);

        DeleteObject(hbmTarget);
        DeleteDC(hdcTarget);

        InputEvent input = CaptureInput();

        dataFile << "frame:" << frameCount << ",keys:" << input.keys << "mouseButtons:" << input.mouseButtons
            << ",mouseX:" << input.mouseX << ",mouseY:" << input.mouseY << ",mouseWheel:" << input.mouseWheel << std::endl;

        if (GetAsyncKeyState(VK_MENU) && GetAsyncKeyState('Q')) {
            captureEnded = true;
        }

        frameCount++;
        Sleep(CAPTURE_DELAY);
    }

    std::ofstream frameCountFileOut("frame_count.txt", std::ios::trunc);
    frameCountFileOut << frameCount;
    frameCountFileOut.close();

    dataFile.close();
    DeleteObject(hbmMemDC);
    DeleteDC(hdcMemDC);
    ReleaseDC(hwnd, hdcWindow);

    return 0;
}