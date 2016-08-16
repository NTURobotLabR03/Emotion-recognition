//------------------------------------------------------------------------------
// <copyright file="WaveWriter.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include "WaveWriter.h"
#include "strsafe.h"

/// <summary>
/// Constructor
/// </summary>
/// <param name="fileName">Name of the file to write to</param>
/// <param name="WaveFormat">Pointer to a WAVEFORMATEX structure describing the audio stream</param>
WaveWriter::WaveWriter(HANDLE fileHandle, const WAVEFORMATEX* WaveFormat) :
    m_cbWritten(0)
{
    m_fileHandle = fileHandle;

    memcpy(&m_format, WaveFormat, sizeof(WAVEFORMATEX));

}

/// <summary>
/// Called instead of a constructor to acquire a WaveWriter
/// </summary>
/// <remarks>
/// Be sure to call Stop to close the object
/// </remarks>
/// <param name="fileName">Name the file to write to</param>
/// <param name="WaveFormat">Pointer to a WAVEFORMATEX structure describing the audio stream</param>
WaveWriter * WaveWriter::Start(wchar_t* fileName,  const WAVEFORMATEX *WaveFormat )
{
    HANDLE fileHandle = CreateFile(fileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 
    NULL);

    if (fileHandle == NULL)
    {
        return NULL;
    }

    WaveWriter * writer = new WaveWriter(fileHandle, WaveFormat);
    StringCchCopy(writer->m_szFilename, _countof(writer->m_szFilename), fileName);

    // Yes, we do this twice.... Once to advance the write pointer so we don't lose any samples,
    // and then once to put in the right length.
    writer->WriteFileHeader();

    return writer;
}

/// <summary>
/// Stops the recording, writes the file header, and closes the file
/// </summary>
void WaveWriter::Stop()
{
    // Come back and update the file-size information.
    WriteFileHeader();
    FlushFileBuffers(m_fileHandle);
    CloseHandle(m_fileHandle);
}

/// <summary>
/// Writes out the file header
/// </summary>
/// <remarks>
/// This should be called once at the beginning of the class usage, and once at the end.
/// The first call writes out the header, and moves the file pointer to where we can start recording audio
/// The second call is required because we don't know the filesize until we're all done, but it needs to live in the header as well
/// </remarks>
/// <returns> A flag indicating success or failure </returns>
bool WaveWriter::WriteFileHeader()
{
    //  A wave file consists of:
    //
    //  RIFF header:    8 bytes consisting of the signature "RIFF" followed by a 4 byte file length.
    //  WAVE header:    4 bytes consisting of the signature "WAVE".
    //  fmt header:     4 bytes consisting of the signature "fmt " 
    //  fmt size:       4 bytes containing the size of the upcoming WAVEFORMATEX
    //  WAVEFORMATEX:     <n> bytes containing a WAVEFORMATEX structure.
    //  DATA header:    8 bytes consisting of the signature "data" followed by a 4 byte file length.
    //  wave data:      <m> bytes containing wave data.

    // Seek to the beginning
    DWORD ret = SetFilePointer(
            m_fileHandle,
            0,
            NULL,
            FILE_BEGIN);

    if (ret == INVALID_SET_FILE_POINTER)
        return false;

    DWORD cbWritten = 0;
    if (!WriteFile(m_fileHandle, Riff, sizeof(Riff), &cbWritten, NULL))
        return false;

    if (!WriteFile(m_fileHandle, &m_cbWritten, sizeof(DWORD), &cbWritten, NULL))
        return false;

    if (!WriteFile(m_fileHandle, Wave, sizeof(Wave), &cbWritten, NULL))
        return false;

    if (!WriteFile(m_fileHandle, fmt, sizeof(fmt), &cbWritten, NULL))
        return false;

    DWORD dwFormatSize = sizeof(WAVEFORMATEX) + m_format.cbSize;
    if (!WriteFile(m_fileHandle, &dwFormatSize, sizeof(dwFormatSize), &cbWritten, NULL))
        return false;

    if (!WriteFile(m_fileHandle, &m_format, sizeof(WAVEFORMATEX) + m_format.cbSize, &cbWritten, NULL))
        return false;


    if (!WriteFile(m_fileHandle, "data", 4, &cbWritten, NULL))
        return false;

    if (!WriteFile(m_fileHandle, &m_cbWritten, sizeof(DWORD), &cbWritten, NULL))
        return false;

    return true;
}

/// <summary>
/// Writes audio data to disk
/// </summary>
/// <param name="Buffer">Pointer to the buffer containing audio data</param>
/// <param name="BufferSize">Number of bytes to write from the buffer down to disk</param>
/// <returns> A flag indicating success or failure </returns>
bool WaveWriter::WriteBytes(const BYTE *Buffer, const size_t BufferSize)
{
    DWORD bytesWritten;
    if (!WriteFile(m_fileHandle, Buffer, (DWORD)BufferSize, &bytesWritten, NULL))
    {
        return false;
    }

    m_cbWritten += bytesWritten;
    return true;
}
