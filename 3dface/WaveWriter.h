//------------------------------------------------------------------------------
// <copyright file="WaveWriter.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#pragma once

#include "StdAfx.h"

// For WAVEFORMATEX
#include <mmreg.h>

//
//  WAV file writer.
//
//  This is a VERY simple .WAV file writer.
//

//
//  A wave file consists of:
//
//  RIFF header:    8 bytes consisting of the signature "RIFF" followed by a 4 byte file length.
//  WAVE header:    4 bytes consisting of the signature "WAVE".
//  fmt header:     4 bytes consisting of the signature "fmt " 
//  fmt size:       4 bytes containing the size of the upcoming WAVEFORMATEX
//  WAVEFORMATEX:     <n> bytes containing a WAVEFORMATEX structure.
//  DATA header:    8 bytes consisting of the signature "data" followed by a 4 byte file length.
//  wave data:      <m> bytes containing wave data.
//
//
//  Header for a WAV file - we define a structure describing the first few fields in the header for convenience.
//

//  Static RIFF header, we'll append the format to it.

const BYTE Riff[] = 
{
    'R', 'I', 'F', 'F'
};

const BYTE Wave[] = 
{
    'W',   'A',   'V',   'E'
};

const BYTE fmt[] = 
{
    'f',   'm',   't',   ' '
};

//  Static wave DATA tag.
const BYTE WaveData[] = { 'd', 'a', 't', 'a'};

static const UINT FileHeaderSize = 8 + 4 + 4 + 4 + sizeof(WAVEFORMATEX) + 8 + 4;

/// <summary>
/// A simple helper class for creating a wave file
/// </summary>
/// <remarks>
/// This class can be used to stream live audio to disk
/// </remarks>
class WaveWriter
{
private:
    
    // Format data which describes the audio stream
    WAVEFORMATEX m_format;

    // Handle to the file we'll stream to
    HANDLE m_fileHandle;

    // How much data has been written
    UINT m_cbWritten;

    // Filename we're actually writing to
    WCHAR m_szFilename[MAX_PATH];

    /// <summary>
    /// Constructor
    /// </summary>
    /// <param name="fileHandle">Handle to the file to write to</param>
    /// <param name="WaveFormat">Pointer to a WAVEFORMATEX structure describing the audio stream</param>
    WaveWriter(HANDLE fileHandle, const WAVEFORMATEX* WaveFormat);
    
    /// <summary>
    /// Writes out the file header
    /// </summary>
    /// <remarks>
    /// This should be called once at the beginning of the class usage, and once at the end.
    /// The first call writes out the header, and moves the file pointer to where we can start recording audio
    /// The second call is required because we don't know the filesize until we're all done, but it needs to live in the header as well
    /// </remarks>
    /// <returns> A flag indicating success or failure </returns>
    bool WriteFileHeader();

public:
    
    /// <summary>
    /// Called instead of a constructor to acquire a WaveWriter
    /// </summary>
    /// <remarks>
    /// Be sure to call Stop to close the object
    /// </remarks>
    /// <param name="fileName">Name the file to write to</param>
    /// <param name="WaveFormat">Pointer to a WAVEFORMATEX structure describing the audio stream</param>
    static WaveWriter * Start(wchar_t* fileName,  const WAVEFORMATEX *WaveFormat);

    /// <summary>
    /// Stops the recording, writes the file header, and closes the file
    /// </summary>
    void Stop();

    /// <summary>
    /// Writes audio data to disk
    /// </summary>
    /// <param name="Buffer">Pointer to the buffer containing audio data</param>
    /// <param name="BufferSize">Number of bytes to write from the buffer down to disk</param>
    /// <returns> A flag indicating success or failure </returns>
    bool WriteBytes(const BYTE *Buffer, const size_t BufferSize);

    const WCHAR * GetFileName() { return m_szFilename; };
};

