//------------------------------------------------------------------------------
// <copyright file="AudioVisualizer.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include "AudioVisualizer.h"
#include "stdafx.h"
#include "AudioExplorer.h"
#include "resource.h"

#include "Utilities.h"
#include "float.h"

#include <iostream> 
#include <fstream>
using namespace	std;


ListBoxEntry WindowingFunctions[] =
{
    {0, IDS_WINDOW_RECT, RECTANGULAR, false},
    {0, IDS_WINDOW_HANN, HANN, true},
    {0, IDS_WINDOW_HAMMING,HAMMING, false},
    {0, IDS_WINDOW_NUTTALL, NUTTALL, false},
    {0, IDS_WINDOW_BLACKMANHARRIS, BLACKMANHARRIS, false},
    {0, IDS_WINDOW_BLACKMANNUTTALL, BLACKMANNUTTALL, false}
};

/// <summary>
///   Constructor
/// </summary>
/// <param name="displayWidth">Width of the display for this visualizer</param>
/// <param name="displayHeight">Height of the display for this visualizer</param>
CAudioVisualizer::CAudioVisualizer(UINT displayWidth, UINT displayHeight) :
    m_uiDisplayWidth(displayWidth),
    m_uiDisplayHeight(displayHeight)
{

    const int cBytesPerPixel = 4;
    const int cMaxPixelChannelIntensity = 0xff;

    // Allocate background and set to white
    m_uiBackgroundStride = cBytesPerPixel * m_uiDisplayWidth;
    int numBackgroundBytes = m_uiBackgroundStride * m_uiDisplayHeight * 2;
    m_pBackground = new BYTE[numBackgroundBytes];
    memset(m_pBackground, cMaxPixelChannelIntensity, numBackgroundBytes);

    // Allocate foreground and set to blue/violet color
    m_uiForegroundStride = cBytesPerPixel;
    m_pForeground = new BYTE[cBytesPerPixel * m_uiDisplayHeight * 2];
    UINT *pPixels = reinterpret_cast<UINT*>(m_pForeground);
    for (UINT iPixel = 0; iPixel < m_uiDisplayHeight * 2; ++iPixel)
    {
        pPixels[iPixel] = 0x8A2BE2;
    }

};

/// <summary>
///   Destructor
/// </summary>
CAudioVisualizer::~CAudioVisualizer()
{
    if (m_pBackground)
    {
        delete m_pBackground;
        m_pBackground = NULL;
    }

    if (m_pForeground)
    {
        delete m_pForeground;
        m_pForeground = NULL;
    }
};



/// <summary>
///   Constructor
/// </summary>
/// <param name="displayWidth">Width of the display for this visualizer</param>
/// <param name="displayHeight">Height of the display for this visualizer</param>
CEqualizerVisualizer::CEqualizerVisualizer(UINT displayWidth, UINT displayHeight) : 
    CAudioVisualizer(displayWidth, displayHeight),
    m_iAccumulatedSampleCount(0),
    m_fAdaptiveScaling(false),
    m_hInstance(NULL),
    m_hwndOptions(NULL)
{
    size_t storage_size = sizeof(XDSP::XVECTOR) * wBinsForFFT / 4;

    // Note, the XVECTORS need to be aligned on 32 bit boundaries, so we need to use _aligned_malloc
    m_pxvBinsFFTRealStorage = (XDSP::XVECTOR*)_aligned_malloc(storage_size, 32);
    m_pxvBinsFFTImaginaryStorage = (XDSP::XVECTOR*)_aligned_malloc(storage_size, 32);
    m_pxvUnityTable = (XDSP::XVECTOR*)_aligned_malloc(sizeof(XDSP::XVECTOR) * wBinsForFFT , 32);

    m_pfltBinsFFTReal = (float *)m_pxvBinsFFTRealStorage;
    m_pfltBinsFFTImaginary = (float *)m_pxvBinsFFTImaginaryStorage;

    ZeroMemory(m_fltBinsFFTDisplay,   sizeof(m_fltBinsFFTDisplay));
    ZeroMemory(m_rgfltAudioInputForFFT, sizeof(m_rgfltAudioInputForFFT));
    ZeroMemory(m_pfltBinsFFTReal,      storage_size);
    ZeroMemory(m_pfltBinsFFTImaginary, storage_size);

    XDSP::FFTInitializeUnityTable ((FLOAT32*)m_pxvUnityTable, wBinsForFFT);

    InitializeFFTWindow(m_rgfltWindow, wBinsForFFT, HANN);
}

/// <summary>
///   Displays the options window for this visualizer
/// </summary>
/// <param name="hInstance">HINSTANCE for the Application</param>
/// <param name="hInstance">HWND for the main window</param>
bool CEqualizerVisualizer::ShowOptionsWindow(HINSTANCE hInstance, HWND hwndApp)
{
    if (NULL == m_hwndOptions)
    {
        m_hInstance = hInstance;

        WNDCLASS  wc;

        // Dialog custom window class
        ZeroMemory(&wc, sizeof(wc));
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.cbWndExtra    = DLGWINDOWEXTRA;
        wc.hInstance     = hInstance;
        wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
        wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_APP));
        wc.lpfnWndProc   = DefDlgProcW;
        wc.lpszClassName = L"AudioExplorerEqualizerOptionsDlgWndClass";

        if (!RegisterClassW(&wc))
        {
            return false;
        }

        // Create options window
        m_hwndOptions = CreateDialogParamW(
            hInstance,
            MAKEINTRESOURCE(IDD_DIALOG1),
            hwndApp,
            (DLGPROC)CEqualizerVisualizer::MessageRouter, 
            reinterpret_cast<LPARAM>(this));

    }

    // Show window
    ShowWindow(m_hwndOptions, SW_SHOW);
    return true;
}

/// <summary>
///   Hides the options window for this visualizer
/// </summary>
void CEqualizerVisualizer::HideOptionsWindow()
{
    if (m_hwndOptions)
        ShowWindow(m_hwndOptions, SW_HIDE);
}


/// <summary>
/// Handles window messages, passes most to the class instance to handle
/// </summary>
/// <param name="hWnd">window message is for</param>
/// <param name="uMsg">message</param>
/// <param name="wParam">message data</param>
/// <param name="lParam">additional message data</param>
/// <returns>result of message processing</returns>
LRESULT CALLBACK CEqualizerVisualizer::MessageRouter(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CEqualizerVisualizer* pThis = NULL;

    if (WM_INITDIALOG == uMsg)
    {
        pThis = reinterpret_cast<CEqualizerVisualizer*>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else
    {
        pThis = reinterpret_cast<CEqualizerVisualizer*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (pThis)
    {
        return pThis->DlgProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}


/// <summary>
/// Handle windows messages for the class instance
/// </summary>
/// <param name="hWnd">window message is for</param>
/// <param name="uMsg">message</param>
/// <param name="wParam">message data</param>
/// <param name="lParam">additional message data</param>
/// <returns>result of message processing</returns>
LRESULT CALLBACK CEqualizerVisualizer::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        LoadDropDown(m_hInstance, GetDlgItem(hWnd, IDC_WINDOW_FUNCTION), WindowingFunctions, _countof(WindowingFunctions));
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam) )
        {
        case IDC_WINDOW_FUNCTION:

            if (CBN_SELCHANGE == HIWORD(wParam))
            {
                LPARAM index = SendDlgItemMessage(hWnd, IDC_WINDOW_FUNCTION, CB_GETCURSEL, (WORD)0, 0L);
                InitializeFFTWindow(m_rgfltWindow, wBinsForFFT, (FFTWindowFunction) WindowingFunctions[index].value);
            }
            break;
        }

    case IDC_CHECK1:
        if (BN_CLICKED == HIWORD(wParam))
        {
            // Toggle out internal state for adaptive scaling
            m_fAdaptiveScaling = !m_fAdaptiveScaling;
        }
        break;

        // If the titlebar X is clicked, just hide the options window
    case WM_CLOSE:
        HideOptionsWindow();
        break;
    }
    return FALSE;
}

/// <remarks>
///   Destructor
/// </remarks>
CEqualizerVisualizer::~CEqualizerVisualizer()
{
    _aligned_free(m_pxvBinsFFTRealStorage);
    _aligned_free(m_pxvBinsFFTImaginaryStorage);
    _aligned_free(m_pxvUnityTable);
}

/// <summary>
/// This function will be called periodically to pass the visualizer the audio data stream.  
/// It is imperitave that the visuallizer processes the data quickly
/// In this visualizer, this is where the FFT processing is done
/// </summary>
/// <param name="pAudio">Pointer to a BYTE array containing audio data</param>
/// <param name="cb">Number of bytes that should be read from the array</param>
void CEqualizerVisualizer::ProcessAudio(BYTE * pProduced, DWORD cbProduced)
{
	fstream feq;
	feq.open("feq.txt", ios::out | ios::app);
	fstream eng;
	eng.open("eng.txt", ios::out | ios::app);

    static const float Invert = 1 / (float) MAXSHORT;
    static const float Decay = 0.7f;
	float m_fltAccumulatedSquareSum = 0;
	const float cEnergyNoiseFloor = 0.2f;
	double energy_max = 0;
	double energy_min = 99999;
	int zerocrossnumber = 0;

    // Calculate FFT from audio
    for (UINT i = 0; i < cbProduced; i += 2)
    {
        short audioSample = static_cast<short>(pProduced[i] | (pProduced[i+1] << 8));

		m_fltAccumulatedSquareSum += audioSample * audioSample;
		if (audioSample*audioSample>energy_max) energy_max = audioSample*audioSample;
		//if ((audioSample*audioSample<energy_min) && audioSample*audioSample>0) energy_min = audioSample*audioSample;
		if (audioSample == 0) zerocrossnumber++;

        float audioSampleFloat = Invert * audioSample;

        m_rgfltAudioInputForFFT[m_iAccumulatedSampleCount++] = audioSampleFloat;

        if (m_iAccumulatedSampleCount < wBinsForFFT)
        {
            continue;
        }


		// Each energy value will represent the logarithm of the mean of the
		// sum of squares of a group of audio samples.
		float meanSquare = m_fltAccumulatedSquareSum / wBinsForFFT;
		float amplitude = log(meanSquare) / log(static_cast<float>(INT_MAX));
		energy_max = log(energy_max) / log(static_cast<float>(INT_MAX));
		//energy_min = log(energy_min) / log(static_cast<float>(INT_MAX));

		// Truncate portion of signal below noise floor
		float amplitudeAboveNoise = max(0.0f, amplitude - cEnergyNoiseFloor);
		// Renormalize signal above noise floor to [0,1] range.
		float EnergyBuffer = amplitudeAboveNoise / (1 - cEnergyNoiseFloor);

		eng << EnergyBuffer << " " << energy_max << " " << zerocrossnumber << endl;
		cout << "\n----energy----\n" << "aveg: " << EnergyBuffer << "    max: " << energy_max << "     zerocross: " << zerocrossnumber << endl;

        // At this point we have enough samples to do our FFT
        // First, copy the samples across to the XVector storage
        for (UINT iSample = 0; iSample < wBinsForFFT; ++iSample)
        {
            m_pfltBinsFFTReal[iSample] = m_rgfltAudioInputForFFT[iSample] * m_rgfltWindow[iSample];
        }

        int lb2FFT = 0;
        int temp = wBinsForFFT;
        while (0 == (temp & 1))
        {
            ++lb2FFT;
            temp = temp >> 1;
        }
        // Pass off to the FFT library to do the heavy lifting.  Before this call, the m_pfltBinsFFTReal array holds the time domain signal
        //  and the m_pfltBinsFFTImaginary array is initialized to 0
        // After this call, m_pfltBinsFFTReal will contain the real components of the frequency domain data, and m_pfltBinsFFTImaginary
        //  will hold the imaginary components
        // NOTE:  There is a newer FFT library from the DirectX team that does the FFT on the GPU. 
        // See: http://msdn.microsoft.com/en-us/library/windows/desktop/ff476274(v=vs.85).aspx
        XDSP::FFT ((XDSP::XVECTOR *)m_pfltBinsFFTReal, (XDSP::XVECTOR *)m_pfltBinsFFTImaginary, m_pxvUnityTable, wBinsForFFT);
        XDSP::FFTUnswizzle(m_rgfltAudioInputForFFT, m_pfltBinsFFTReal, lb2FFT);
        memcpy_s(m_pfltBinsFFTReal, sizeof(XDSP::XVECTOR) * wBinsForFFT / 4, m_rgfltAudioInputForFFT, sizeof(float) * wBinsForFFT);
        XDSP::FFTUnswizzle(m_rgfltAudioInputForFFT, m_pfltBinsFFTImaginary, lb2FFT);
        memcpy_s(m_pfltBinsFFTImaginary, sizeof(XDSP::XVECTOR) * wBinsForFFT / 4, m_rgfltAudioInputForFFT, sizeof(float) * wBinsForFFT);
		
		//cout << "\n -----frequency-----\n";
        for (UINT iBin = 0; iBin < wBinsForFFT/2; ++iBin)
        {
            float imaginary = m_pfltBinsFFTImaginary[iBin];
            float real = m_pfltBinsFFTReal[iBin];

            // Calculating the magnitude requires considering both the real and imaginary components.
            // We could convert to log scale, but this works well without it.
            float magnitude = sqrt(imaginary * imaginary + real * real);

            // This next operation will smooth out the results a little and prevent the display from jumping around wildly.
            // You can play with this by changing the "Decay" parameter above.
            float decayedOldValue = m_fltBinsFFTDisplay[iBin] * Decay;
            m_fltBinsFFTDisplay[iBin] = max(magnitude, decayedOldValue);
			feq << m_fltBinsFFTDisplay[iBin] << " ";
			//cout << m_fltBinsFFTDisplay[iBin] << " ";
        }
		feq << "\n";


        // We're all done with our FFT so we'll clean up and get ready for next time.
        m_iAccumulatedSampleCount = 0;
        memset (m_rgfltAudioInputForFFT, 0, sizeof(float) * wBinsForFFT);
        memset (m_pfltBinsFFTReal, 0, sizeof(float) * wBinsForFFT);
        memset (m_pfltBinsFFTImaginary, 0, sizeof(float) * wBinsForFFT);
		m_fltAccumulatedSquareSum = 0;
		energy_max = 0;
		energy_min = 99999;
		zerocrossnumber = 0;
    }

	feq.close();
	eng.close();
}

/// <summary>
/// This function is called when the visual representation needs to be updated.
/// In this visualizer, this will produce a normalized output based on the FFT, grouping close frequencies into wider bands.
/// It also establishes a noise floor, and adapts to the dynamic range seen in the signal... 
/// </summary>
void CEqualizerVisualizer::Update()
{
    // Because we're called from the timer loop, we get hits before the display is actually ready.
    // In that case, just return
    if (!m_pBitmap)
        return;

    BYTE * pBackground = m_pBackground;
    UINT backgroundStride = m_uiBackgroundStride;
    BYTE * pForeground = m_pForeground;
    UINT foregroundStride = m_uiForegroundStride;

    // This is a variable we will use to track the dynamic range which we want to render.
    static float maxscaler = 100;

    D2D1_SIZE_U size =  m_pBitmap->GetPixelSize();

    // This is the number of visual bands which we will show.  Note, this directly drives how bands in the data will be combined for render
    UINT Bands = wBinsForFFT/4;
    UINT BinCount = wBinsForFFT / 2;
    UINT cCollapse = BinCount / Bands;

    UINT Width = (size.width - 10) / (2 * Bands);

    // Clear whole display to background color
    m_pBitmap->CopyFromMemory(NULL, pBackground, backgroundStride);

    float maxVal = 0;
    float minVal = FLT_MAX;

    // This loop is determining the range of values that we see across the FFT.
    // This will be used to establish a noise floor, and to drive the range we render
    for (UINT i = 0; i < BinCount; ++i)
    {

        float val = 0;

        val += m_fltBinsFFTDisplay[i];
        while (((i + 1) % cCollapse) != 0 && i < BinCount)
        {
            val += m_fltBinsFFTDisplay[++i];
        }
        maxVal = max(maxVal, val);
        minVal = min(minVal, val);
    }

    // This is the amount that we need to multiply by to scale the current signal to a range of 0-1.
    // Note, we subtract minVal, as that is the noise floor we will be pulling out of the whole results.
    float scaler = 1 / (maxVal - minVal);

    // This is where we adapt the dynamic range over time. 
    if (scaler > 0)
    {
        // We first scale the remembered scaler by a small amount so that the system will slowly adapt back to low noise levels after
        // a period of quiet.
        if (m_fAdaptiveScaling)
            maxscaler = float(1.02 * maxscaler);

        // See whether our remebered range is wider or smaller than the required range to show the current data set.
        // Pick the larger of the two ranges, and use that as both the range this time, and the remembered range for the future.
        maxscaler = min (scaler, maxscaler); //max(min (scaler, maxscaler), 1.0f);
        scaler = maxscaler;
    }

    // Draw each frequency band as a centered vertical bar, where the length of each bar is
    // proportional to the amount of energy it represents.
    UINT iBar = 0;
    for (UINT i = 0; i < BinCount; ++i)
    {

        // Combine the energy data from a collection of bands for clearer display
        float val = 0;

        val += m_fltBinsFFTDisplay[i];
        while ((i + 1) % cCollapse != 0 && i < BinCount)
        {
            val += m_fltBinsFFTDisplay[++i];
        }

        // Establish a noise floor
        val = val - minVal;


        const int cHalfImageHeight = size.height / 2;

        // Each bar has a minimum height of 1 (to get a steady signal down the middle) and a maximum height
        // equal to the bitmap height.
        int barHeight = static_cast<int>(max(1.0f, (scaler * val * size.height) ));

        // Center bar vertically on image
        int top = max(cHalfImageHeight - (barHeight / 2), 0);
        int bottom = min(top + barHeight, (int)(size.height - 1));


        D2D1_RECT_U barRect = D2D1::RectU(Width * 2 * iBar, top, Width * 2 * iBar + Width, bottom);

        // Draw bar in foreground color
        m_pBitmap->CopyFromMemory(&barRect, pForeground, foregroundStride);
        ++iBar;
    }
}

/// <summary>
///   Constructor
/// </summary>
/// <param name="displayWidth">Width of the display for this visualizer</param>
/// <param name="displayHeight">Height of the display for this visualizer</param>
COscilloscopeVisualizer::COscilloscopeVisualizer(UINT displayWidth, UINT displayHeight) : CAudioVisualizer(displayWidth, displayHeight),
    m_fltAccumulatedSquareSum(0.0),
    m_fltEnergyError(0.0),
    m_iAccumulatedSampleCount(0),
    m_iEnergyIndex(0),
    m_iEnergyRefreshIndex(0),
    m_iNewEnergyAvailable(0),
    m_dwLastEnergyRefreshTime(NULL)
{
    ZeroMemory(m_rgfltEnergyBuffer, sizeof(m_rgfltEnergyBuffer));
    ZeroMemory(m_rgfltEnergyDisplayBuffer, sizeof(m_rgfltEnergyDisplayBuffer));
}

/// <summary>
/// This function will be called periodically to pass the visualizer the audio data stream.  
/// It is imperitave that the visuallizer processes the data quickly
/// In this visualizer, this is where the energy of the signal is calculated using RMS
/// </summary>
/// <param name="pAudio">Pointer to a BYTE array containing audio data</param>
/// <param name="cb">Number of bytes that should be read from the array</param>
void COscilloscopeVisualizer::ProcessAudio(BYTE * pProduced, DWORD cbProduced)
{
    // Bottom portion of computed energy signal that will be discarded as noise.
    // Only portion of signal above noise floor will be displayed.

	fstream test;
	test.open("test_o.txt", ios::out|ios::app );

    const float cEnergyNoiseFloor = 0.2f;

    // Calculate energy from audio
    for (UINT i = 0; i < cbProduced; i += 2)
    {
        // compute the sum of squares of audio samples that will get accumulated
        // into a single energy value.
        short audioSample = static_cast<short>(pProduced[i] | (pProduced[i+1] << 8));
        m_fltAccumulatedSquareSum += audioSample * audioSample;
        ++m_iAccumulatedSampleCount;

        if (m_iAccumulatedSampleCount < iAudioSamplesPerEnergySample)
        {
            continue;
        }

        // Each energy value will represent the logarithm of the mean of the
        // sum of squares of a group of audio samples.
        float meanSquare = m_fltAccumulatedSquareSum / iAudioSamplesPerEnergySample;
        float amplitude = log(meanSquare) / log(static_cast<float>(INT_MAX));

        // Truncate portion of signal below noise floor
        float amplitudeAboveNoise = max(0.0f, amplitude - cEnergyNoiseFloor);

        // Renormalize signal above noise floor to [0,1] range.
        m_rgfltEnergyBuffer[m_iEnergyIndex] = amplitudeAboveNoise / (1 - cEnergyNoiseFloor);

		test << m_rgfltEnergyBuffer[m_iEnergyIndex]<<" ";

        m_iNewEnergyAvailable++;
        m_iEnergyIndex = (m_iEnergyIndex + 1) % iEnergyBufferLength;

        m_fltAccumulatedSquareSum = 0;
        m_iAccumulatedSampleCount = 0;
		
    }

	test << "\n";
	test.close();
}

/// <summary>
/// This function is called when the visual representation needs to be updated.  
/// In this visualizer, this produces a waveform (or oscilloscope) view.
/// </summary>
void COscilloscopeVisualizer::Update()
{
    // Because we're called from the timer loop, we get hits before the display is actually ready.
    // In that case, just return

    if (!m_pBitmap)
        return;

    DWORD previousRefreshTime = m_dwLastEnergyRefreshTime;
    DWORD now = GetTickCount();
    m_dwLastEnergyRefreshTime  = now;

    // No need to refresh if there is no new energy available to render
    if(m_iNewEnergyAvailable <= 0)
    {
        return;
    }

    if (previousRefreshTime != NULL)
    {
        //Check if there is an overflow in the circular buffer and reset the index
        if(m_iNewEnergyAvailable > (iEnergyBufferLength-iEnergySamplesToDisplay))
        {
            m_iEnergyRefreshIndex = m_iEnergyIndex;
            m_iNewEnergyAvailable = 0;
            m_fltEnergyError = 0.0;
        }
        else
        {
            float energyToAdvance = m_fltEnergyError +(((now - previousRefreshTime) * AudioSamplesPerSecond/1000.0f) / iAudioSamplesPerEnergySample); 
            int energySamplesToAdvance = min(m_iNewEnergyAvailable, (int)(energyToAdvance));
            m_fltEnergyError = energyToAdvance - energySamplesToAdvance;
            m_iEnergyRefreshIndex = (m_iEnergyRefreshIndex + energySamplesToAdvance) % iEnergyBufferLength;
            m_iNewEnergyAvailable -= energySamplesToAdvance;
        }
    }
    // Copy energy samples into buffer to be displayed, taking into account that energy
    // wraps around in a circular buffer.
    int baseIndex = (m_iEnergyRefreshIndex + iEnergyBufferLength - iEnergySamplesToDisplay) % iEnergyBufferLength;
    int samplesUntilEnd = iEnergyBufferLength - baseIndex;
    if(samplesUntilEnd>iEnergySamplesToDisplay)
    {
        memcpy_s(m_rgfltEnergyDisplayBuffer, iEnergySamplesToDisplay*sizeof(float),m_rgfltEnergyBuffer + baseIndex, iEnergySamplesToDisplay*sizeof(float));
    }
    else
    {
        int samplesFromBeginning = iEnergySamplesToDisplay-samplesUntilEnd;
        memcpy_s(m_rgfltEnergyDisplayBuffer, iEnergySamplesToDisplay*sizeof(float), m_rgfltEnergyBuffer + baseIndex, samplesUntilEnd*sizeof(float));
        memcpy_s(m_rgfltEnergyDisplayBuffer + samplesUntilEnd, (iEnergySamplesToDisplay - samplesUntilEnd)*sizeof(float), m_rgfltEnergyBuffer, samplesFromBeginning*sizeof(float));
    }

    BYTE * pBackground = m_pBackground;
    UINT backgroundStride = m_uiBackgroundStride;
    BYTE * pForeground = m_pForeground;
    UINT foregroundStride = m_uiForegroundStride;

    D2D1_SIZE_U size =  m_pBitmap->GetPixelSize();    

    float *pEnergy = m_rgfltEnergyDisplayBuffer; 
    UINT energyLength = iEnergySamplesToDisplay;

    // Clear whole display to background color
    m_pBitmap->CopyFromMemory(NULL, pBackground, backgroundStride);

    // Draw each energy sample as a centered vertical bar, where the length of each bar is
    // proportional to the amount of energy it represents.
    // Time advances from left to right, with current time represented by the rightmost bar.
    for (UINT i = 0; i < min(energyLength, size.width); ++i)
    {
        const int cHalfImageHeight = size.height / 2;

        // Each bar has a minimum height of 1 (to get a steady signal down the middle) and a maximum height
        // equal to the bitmap height.
        int barHeight = static_cast<int>(max(1.0f, (pEnergy[i] * size.height)));

        // Center bar vertically on image
        int top = cHalfImageHeight - (barHeight / 2);
        int bottom = top + barHeight;
        D2D1_RECT_U barRect = D2D1::RectU(i, top, i + 1, bottom);

        // Draw bar in foreground color
        m_pBitmap->CopyFromMemory(&barRect, pForeground, foregroundStride);
    }
}
