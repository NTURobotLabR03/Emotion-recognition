//------------------------------------------------------------------------------
// <copyright file="AudioVisualizer.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#pragma once

#include "AudioPanel.h"
#include "resource.h"

// For IMediaObject and related interfaces
#include <dmo.h>

// For configuring DMO properties
#include <wmcodecdsp.h>

// For WAVEFORMATEX
#include <mmreg.h>

// For FORMAT_WaveFormatEx and such
#include <uuids.h>

// For Kinect SDK APIs
#include <NuiApi.h>

#include "XDSP.h"

/// <summary>
///  A base class for visualizers. Doesn't really do much right now.
/// </summary>
class CAudioVisualizer
{
protected:
    ID2D1Bitmap * m_pBitmap;

    UINT                        m_uiDisplayWidth;
    UINT                        m_uiDisplayHeight;
    BYTE*                       m_pBackground;
    UINT                        m_uiBackgroundStride;
    BYTE*                       m_pForeground;
    UINT                        m_uiForegroundStride;

public:
    /// <summary>
    ///   Constructor
    /// </summary>
    /// <param name="displayWidth">Width of the display for this visualizer</param>
    /// <param name="displayHeight">Height of the display for this visualizer</param>
    CAudioVisualizer(UINT displayWidth, UINT displayHeight);

    /// <summary>
    ///   Destructor
    /// </summary>
    virtual ~CAudioVisualizer();

    /// <summary>
    /// This function will be called periodically to pass the visualizer the audio data stream.  
    /// It is imperative that the visuallizer processes the data quickly
    /// Note, the base implementation is empty
    /// </summary>
    /// <param name="pAudio">Pointer to a BYTE array containing audio data</param>
    /// <param name="cb">Number of bytes that should be read from the array</param>
    virtual void ProcessAudio(BYTE * , DWORD ) {};


    /// <summary>
    /// This function is called when the visual representation needs to be updated.
    /// In this visualizer, this will produce a normalized output based on the FFT, grouping close frequencies into wider bands.
    /// Note, the base implementation is empty
    /// </summary>
    virtual void Update() {};

    /// <summary>
    ///   Displays the options window for this visualizer
    /// </summary>
    /// <param name="hInstance">HINSTANCE for the Application</param>
    /// <param name="hInstance">HWND for the main window</param>
    virtual bool ShowOptionsWindow(HINSTANCE hInstance, HWND hwndParent) { return false; };

    /// <summary>
    ///   Hides the options window for this visualizer
    /// </summary>
    virtual void HideOptionsWindow() {  };

    /// <summary>
    ///   Sets the bitmap for this visualizer to render into
    /// </summary>
    /// <param name="hInstance">Bitmap for this visualizer to render into</param>
    virtual void SetImage (ID2D1Bitmap * pBitmap)
    {
        m_pBitmap = pBitmap;
    }

};

/// <summary>
///  An FFT Based visualizer
/// </summary>
class CEqualizerVisualizer : public CAudioVisualizer
{
private:
    HINSTANCE m_hInstance;

    // Buckets -- This is how many frequency bins the FFT will divide into... 
    // That also means it's the number of samples which will be considered for one FFT
    // NOTE:  This MUST be a power of 2.
    static const WORD       wBinsForFFT = 512;

    // Buffer used to store audio stream data for FFT calculation
    float                   m_rgfltAudioInputForFFT[wBinsForFFT];

    // Buffer used to store real and imaginary values for FFT
    // We allocate XVectors for storage to ensure correct memory alignment
    // The FFT implmenetation leverages 128 bit numbers, which is really just a way to move 4 32 bit numbers around quickly.
    // When we read and write to these buffers, it will be as floats, which is why we define the float pointers which will be initialized 
    // to point at the XVector storage
    XDSP::XVECTOR *         m_pxvBinsFFTRealStorage;
    XDSP::XVECTOR *         m_pxvBinsFFTImaginaryStorage;
    float *                 m_pfltBinsFFTReal;
    float *                 m_pfltBinsFFTImaginary;

    // This is used internally in the FFT implementation.  We provide storage, but leverage a provided function to initialize it, 
    // and never do anything with it but pass it back to the FFT implementation
    XDSP::XVECTOR *			m_pxvUnityTable;

    // This is an array that will hold a "Window" function, which will be multiplied against the audio data to limit artifacts in the FFT.
    // For more information, see http://en.wikipedia.org/wiki/Window_function
    float                   m_rgfltWindow[wBinsForFFT];

    // Buffer used to store the display values for the FFT
    // Note, we only need to display the 1st half of the bins, as the second half contain
    // redundant information.
    float					m_fltBinsFFTDisplay [wBinsForFFT / 2];

    // Number of audio samples accumulated so far
    int                     m_iAccumulatedSampleCount;

    // Should we back off our scaling factor so we show smaller impulses?
    bool                    m_fAdaptiveScaling;

    // Handle to our options window
    HWND                    m_hwndOptions;

public:
    /// <summary>
    ///   Constructor
    /// </summary>
    /// <param name="displayWidth">Width of the display for this visualizer</param>
    /// <param name="displayHeight">Height of the display for this visualizer</param>
    CEqualizerVisualizer(UINT displayWidth, UINT displayHeight);

    /// <remarks>
    ///   Destructor
    /// </remarks>
    ~CEqualizerVisualizer();

    /// <summary>
    /// This function will be called periodically to pass the visualizer the audio data stream.  
    /// It is imperitave that the visuallizer processes the data quickly
    /// In this visualizer, this is where the FFT processing is done
    /// </summary>
    /// <param name="pAudio">Pointer to a BYTE array containing audio data</param>
    /// <param name="cb">Number of bytes that should be read from the array</param>
    virtual void ProcessAudio(BYTE * pAudio, DWORD cb);

    /// <summary>
    /// This function is called when the visual representation needs to be updated.
    /// In this visualizer, this will produce a normalized output based on the FFT, grouping close frequencies into wider bands.
    /// It also establishes a noise floor, and adapts to the dynamic range seen in the signal... 
    /// </summary>
    virtual void Update();

    /// <summary>
    ///   Displays the options window for this visualizer
    /// </summary>
    /// <param name="hInstance">HINSTANCE for the Application</param>
    /// <param name="hInstance">HWND for the main window</param>
    bool ShowOptionsWindow(HINSTANCE hInstance, HWND hwndParent);

    /// <summary>
    ///   Hides the options window for this visualizer
    /// </summary>
    void HideOptionsWindow();

private:
    /// <summary>
    /// Handles window messages, passes most to the class instance to handle
    /// </summary>
    /// <param name="hWnd">window message is for</param>
    /// <param name="uMsg">message</param>
    /// <param name="wParam">message data</param>
    /// <param name="lParam">additional message data</param>
    /// <returns>result of message processing</returns>
    static LRESULT CALLBACK MessageRouter(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


    /// <summary>
    /// Handle windows messages for the class instance
    /// </summary>
    /// <param name="hWnd">window message is for</param>
    /// <param name="uMsg">message</param>
    /// <param name="wParam">message data</param>
    /// <param name="lParam">additional message data</param>
    /// <returns>result of message processing</returns>
    LRESULT CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


};

/// <summary>
///  A Wave Form visualizer
/// </summary>
class COscilloscopeVisualizer : public CAudioVisualizer
{
private:
    // Number of audio samples captured from Kinect audio stream accumulated into a single
    // energy measurement that will get displayed
    static const int        iAudioSamplesPerEnergySample = 40;

    // Number of energy samples that will be visible in display at any given time.
    static const int        iEnergySamplesToDisplay = 780;

    // Number of energy samples that will be stored in the circular buffer.
    // Always keep it higher than the energy display length to avoid overflow.
    static const int        iEnergyBufferLength = 1000;

    // Buffer used to store audio stream energy data as we read audio
    float                   m_rgfltEnergyBuffer[iEnergyBufferLength];

    // Buffer used to store audio stream energy data ready to be displayed
    float                   m_rgfltEnergyDisplayBuffer[iEnergySamplesToDisplay];

    // Sum of squares of audio samples being accumulated to compute the next energy value
    float                   m_fltAccumulatedSquareSum;

    // Error between time slice we wanted to display and time slice that we ended up
    // displaying, given that we have to display in integer pixels.
    float					m_fltEnergyError;

    // Number of audio samples accumulated so far to compute the next energy value
    int                     m_iAccumulatedSampleCount;

    // Index of next element available in audio energy buffer
    int                     m_iEnergyIndex;

    // Number of newly calculated audio stream energy values that have not yet been displayed.
    int						m_iNewEnergyAvailable;

    // Index of first energy element that has never (yet) been displayed to screen.
    int						m_iEnergyRefreshIndex;

    // Last time energy visualization was rendered to screen.
    DWORD					m_dwLastEnergyRefreshTime;  

public:

    /// <summary>
    ///   Constructor
    /// </summary>
    /// <param name="displayWidth">Width of the display for this visualizer</param>
    /// <param name="displayHeight">Height of the display for this visualizer</param>
    COscilloscopeVisualizer(UINT displayWidth, UINT displayHeight);

    /// <summary>
    /// This function will be called periodically to pass the visualizer the audio data stream.  
    /// It is imperitave that the visuallizer processes the data quickly
    /// In this visualizer, this is where the energy of the signal is calculated using RMS
    /// </summary>
    /// <param name="pAudio">Pointer to a BYTE array containing audio data</param>
    /// <param name="cb">Number of bytes that should be read from the array</param>
    virtual void ProcessAudio(BYTE * pAudio, DWORD cb);

    /// <summary>
    /// This function is called when the visual representation needs to be updated.  
    /// In this visualizer, this produces a waveform (or oscilloscope) view.
    /// </summary>
    virtual void Update();

};