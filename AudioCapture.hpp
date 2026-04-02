#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include <mutex>

#include "ProcessLoopbackCapture/ProcessLoopbackCapture.h"

namespace AudioCapture
{
    ProcessLoopbackCapture* loopbackCapture;
    std::vector<unsigned char> audioData;

    bool first = true;
    std::mutex audioMtx;
    std::condition_variable audioCv;

    inline void OnDataCapture(const std::vector<unsigned char>::iterator& i1, const std::vector<unsigned char>::iterator& i2, void* pUserData)
    {
        if (first)
        {
            std::unique_lock lock{ audioMtx };
            first = false;
            audioCv.notify_all();
            return;
        }

        audioData.insert(audioData.end(), i1, i2);
    }

    inline void Start(int sampleRate)
    {
        audioData.reserve(16 * 1000 * 1000);

        loopbackCapture = new ProcessLoopbackCapture;
        loopbackCapture->SetCaptureFormat(sampleRate, 16, 2, WAVE_FORMAT_PCM);
        loopbackCapture->SetTargetProcess(GetCurrentProcessId(), true);
        loopbackCapture->SetCallback(&OnDataCapture);
        loopbackCapture->SetIntermediateThreadEnabled(false);
        loopbackCapture->StartCapture();

        first = true;

        std::unique_lock lock{ audioMtx };
        audioCv.wait(lock); // wait for the first data to be captured and only then return
    }

    inline void Stop(const char* filename)
    {
        loopbackCapture->StopCapture();
        WAVEFORMATEX format{};
        loopbackCapture->CopyCaptureFormat(format);

        HANDLE hFile = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        DWORD header[]
        {
            1179011410,
            0,
            1163280727,
            544501094,
            sizeof(WAVEFORMATEX)
        };

        DWORD dwBytesWritten = 0;
        DWORD dwHeaderSize = 0;
        WriteFile(hFile, header, sizeof(header), &dwBytesWritten, NULL);
        dwHeaderSize += dwBytesWritten;

        WriteFile(hFile, &format, sizeof(WAVEFORMATEX), &dwBytesWritten, NULL);
        dwHeaderSize += dwBytesWritten;

        DWORD data[] = { 1635017060, 0 };
        WriteFile(hFile, data, sizeof(data), &dwBytesWritten, NULL);
        dwHeaderSize += dwBytesWritten;

        WriteFile(hFile, audioData.data(), audioData.size(), &dwBytesWritten, NULL);

        DWORD dwSize = (DWORD)audioData.size();

        SetFilePointer(hFile, dwHeaderSize - sizeof(DWORD), NULL, FILE_BEGIN);
        WriteFile(hFile, &dwSize, sizeof(DWORD), &dwBytesWritten, NULL);

        SetFilePointer(hFile, sizeof(DWORD), NULL, FILE_BEGIN);

        DWORD cbTotalSize = dwSize + dwHeaderSize - 8;
        WriteFile(hFile, &cbTotalSize, sizeof(DWORD), &dwBytesWritten, NULL);

        CloseHandle(hFile);

        delete loopbackCapture;

        audioData.clear();
        audioData.shrink_to_fit();
    }
};

#endif