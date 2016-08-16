//------------------------------------------------------------------------------
// <copyright file="AudioExplorer.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "AudioExplorer.h"
#include "resource.h"

// For StringCch* and such
#include <strsafe.h>

// For INT_MAX
#include <limits.h>

// For M_PI and log definitions
#define _USE_MATH_DEFINES
#include <math.h>

#include "XDSP.h"

#include "WaveWriter.h"
#include "Utilities.h"


#include <iostream> 
#include <fstream>
using namespace	std;


float allsample = 0;

/// <summary>
/// Entry point for the application
/// </summary>
/// <param name="hInstance">handle to the application instance</param>
/// <param name="hPrevInstance">always 0</param>
/// <param name="lpCmdLine">command line arguments</param>
/// <param name="nCmdShow">whether to display minimized, maximized, or normally</param>
/// <returns>status</returns>
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (SUCCEEDED(hr))
    {
        {
            CAudioExplorer application;
            application.Run(hInstance, nCmdShow);
        }

        CoUninitialize();
    }

    return EXIT_SUCCESS;
}

/// <summary>
/// Constructor
/// </summary>
CAudioExplorer::CAudioExplorer() :
    m_pD2DFactory(NULL),
    m_pAudioPanel(NULL),
    m_pNuiSensor(NULL),
    m_pNuiAudioSource(NULL),
    m_pDMO(NULL),
    m_pPropertyStore(NULL),
    m_fRecording(false),
    m_pWaveWriter(NULL),
    m_pActiveVisualizer(NULL)
{
    m_rgAudioVisualizers[0] = NULL;
    m_rgAudioVisualizers[1] = NULL;
}

/// <summary>
/// Destructor
/// </summary>
CAudioExplorer::~CAudioExplorer()
{
    if (m_pNuiSensor)
    {
        m_pNuiSensor->NuiShutdown();
    }

    // clean up Direct2D renderer
    delete m_pAudioPanel;
    m_pAudioPanel = NULL;

    if (m_pWaveWriter)
    {
        m_pWaveWriter->Stop();
        delete m_pWaveWriter;
        m_pWaveWriter = NULL;
    }

    for (int i = 0; i < iCountOfVisualizers; ++i)
    {
        if (m_rgAudioVisualizers[i] != NULL)
        {
            delete m_rgAudioVisualizers[i];
            m_rgAudioVisualizers[i] = NULL;
        }

    }

    // clean up Direct2D
    SafeRelease(m_pD2DFactory);

    SafeRelease(m_pNuiSensor);
    SafeRelease(m_pNuiAudioSource);
    SafeRelease(m_pDMO);
    SafeRelease(m_pPropertyStore);
}

/// <summary>
/// Creates the main window and begins processing
/// </summary>
/// <param name="hInstance">handle to the application instance</param>
/// <param name="nCmdShow">whether to display minimized, maximized, or normally</param>
int CAudioExplorer::Run(HINSTANCE hInstance, int nCmdShow)
{
    m_hInstance = hInstance;

    MSG       msg = {0};
    WNDCLASS  wc;

    // Dialog custom window class
    ZeroMemory(&wc, sizeof(wc));
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.cbWndExtra    = DLGWINDOWEXTRA;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_APP));
    wc.lpfnWndProc   = DefDlgProcW;
    wc.lpszClassName = L"AudioExplorerAppDlgWndClass";

    if (!RegisterClassW(&wc))
    {
        return 0;
    }

    // Create main application window
    HWND hWndApp = CreateDialogParamW(
        hInstance,
        MAKEINTRESOURCE(IDD_APP),
        NULL,
        (DLGPROC)CAudioExplorer::MessageRouter, 
        reinterpret_cast<LPARAM>(this));

    // Show window
    //ShowWindow(hWndApp, nCmdShow);

   // m_pActiveVisualizer->ShowOptionsWindow(hInstance, hWndApp);

    // Main message loop
    while (WM_QUIT != msg.message)
    {
        if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            // If a dialog message will be taken care of by the dialog proc
            if ((hWndApp != NULL) && IsDialogMessageW(hWndApp, &msg))
            {
                continue;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return static_cast<int>(msg.wParam);
}

/// <summary>
/// Handles window messages, passes most to the class instance to handle
/// </summary>
/// <param name="hWnd">window message is for</param>
/// <param name="uMsg">message</param>
/// <param name="wParam">message data</param>
/// <param name="lParam">additional message data</param>
/// <returns>result of message processing</returns>
LRESULT CALLBACK CAudioExplorer::MessageRouter(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CAudioExplorer* pThis = NULL;

    if (WM_INITDIALOG == uMsg)
    {
        pThis = reinterpret_cast<CAudioExplorer*>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else
    {
        pThis = reinterpret_cast<CAudioExplorer*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (pThis)
    {
        return pThis->DlgProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

ListBoxEntry Visualizations[] = 
{ 
    {0, IDS_AUDIOVIS_FFT, 0, true},
    {1, IDS_AUDIOVIS_OSC, 1, false}
};

ListBoxEntry MicrophoneModes[] = 
{ 
    {0, IDS_MICMODE_SINGLE_CHANNEL, 5, false},
    {1, IDS_MICMODE_SINGLE_CHANNEL_AEC, 0, false},
    {2, IDS_MICMODE_OPTIBEAM_ARRAY_ONLY, 2, false},
    {3, IDS_MICMODE_OPTIBEAM_ARRAY_AND_AEC, 4, true},

};

ListBoxEntry EchoModes[] = 
{
    {0, IDS_AES_OFF, 0, false},
    {1, IDS_AES_ON, 2, true}
};

/// <summary>
/// Get the name of the file where WAVE data will be stored.
/// </summary>
/// <param name="waveFileName">
/// [out] String buffer that will receive wave file name.
/// </param>
/// <param name="waveFileNameSize">
/// [in] Number of characters in waveFileName string buffer.
/// </param>
/// <returns>
/// S_OK on success, otherwise failure code.
/// </returns>
HRESULT GetWaveFileName(wchar_t *waveFileName, UINT waveFileNameSize)
{
    wchar_t *knownPath = NULL;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Music, 0, NULL, &knownPath);

    if (SUCCEEDED(hr))
    {
        // Get the time
        wchar_t timeString[MAX_PATH];
        GetTimeFormatEx(NULL, 0, NULL, L"hh'-'mm'-'ss", timeString, _countof(timeString));

        // File name will be KinectAudio-HH-MM-SS.wav
        StringCchPrintfW(waveFileName, waveFileNameSize, L"%s\\KinectAudio-%s.wav", knownPath, timeString);
    }

    CoTaskMemFree(knownPath);
    return hr;
}

/// <summary>
/// Handle windows messages for the class instance
/// </summary>
/// <param name="hWnd">window message is for</param>
/// <param name="message">message</param>
/// <param name="wParam">message data</param>
/// <param name="lParam">additional message data</param>
/// <returns>result of message processing</returns>
LRESULT CALLBACK CAudioExplorer::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            // Bind application window handle
            m_hWnd = hWnd;

            // Init Direct2D
            D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);

            // Create and initialize a new Direct2D image renderer (take a look at ImageRenderer.h)
            // We'll use this to draw the data we receive from the Kinect to the screen
            m_pAudioPanel = new AudioPanel();
            HRESULT hr = m_pAudioPanel->Initialize(GetDlgItem(m_hWnd, IDC_AUDIOVIEW), m_pD2DFactory, iDisplay);
            if (FAILED(hr))
            {
                SetStatusMessage(L"Failed to initialize the Direct2D draw device.");
                break;
            }

            UINT displayWidth = m_pAudioPanel->GetDisplayWidth();
            UINT displayHeight = m_pAudioPanel->GetDisplayHeight();

            m_rgAudioVisualizers[0] = new CEqualizerVisualizer(displayWidth, displayHeight);
            m_rgAudioVisualizers[0]->SetImage(m_pAudioPanel->GetDisplayBitmap());
            m_rgAudioVisualizers[1] = new COscilloscopeVisualizer(displayWidth, displayHeight);
            m_rgAudioVisualizers[1]->SetImage(m_pAudioPanel->GetDisplayBitmap());
            m_pActiveVisualizer = m_rgAudioVisualizers[0];

            // Look for a connected Kinect, and create it if found
            hr = CreateFirstConnected();
            if (FAILED(hr))
            {
                break;
            }

            // Set the timer so we get periodic notications to read audio and update the displays
            SetTimer(m_hWnd, iAudioReadTimerId, iAudioReadTimerInterval, NULL);
            SetTimer(m_hWnd, iEnergyRefreshTimerId, iEnergyRefreshTimerInterval, NULL);

            // Now we'll initialize our UI elements
            m_hwndCombo = GetDlgItem(m_hWnd, IDC_SELECT_VISUALIZATION);
            m_hwndMode  = GetDlgItem(m_hWnd, IDC_SELECT_MICMODE);
            m_hwndEcho  = GetDlgItem(m_hWnd, IDC_SELECT_ECHO);

            LoadDropDown(m_hInstance, m_hwndCombo, Visualizations, _countof(Visualizations));
            LoadDropDown(m_hInstance, m_hwndMode, MicrophoneModes, _countof(MicrophoneModes));
            LoadDropDown(m_hInstance, m_hwndEcho, EchoModes, _countof(EchoModes));

            break;
        }

    case WM_COMMAND:
        switch (LOWORD(wParam) )
        {
        case IDC_SELECT_VISUALIZATION:
            if ( CBN_SELCHANGE == HIWORD(wParam) )
            {
                LPARAM index = SendMessage(m_hwndCombo, CB_GETCURSEL, (WORD)0, 0L);
                m_pActiveVisualizer->HideOptionsWindow();
                m_pActiveVisualizer = m_rgAudioVisualizers[index];
                m_pActiveVisualizer->ShowOptionsWindow(m_hInstance, m_hWnd);
            }
            break;

        case IDC_SELECT_MICMODE:

            if ( CBN_SELCHANGE == HIWORD(wParam) )
            {
                LPARAM index = SendMessage(m_hwndMode, CB_GETCURSEL, (WORD)0, 0L);

                // Set AEC-MicArray DMO system mode.
                // This must be set for the DMO to work properly
                PROPVARIANT pvSysMode;
                PropVariantInit(&pvSysMode);
                pvSysMode.vt = VT_I4;
                //   SINGLE_CHANNEL_AEC = 0
                //   OPTIBEAM_ARRAY_ONLY = 2
                //   OPTIBEAM_ARRAY_AND_AEC = 4
                //   SINGLE_CHANNEL = 5
                pvSysMode.lVal = MicrophoneModes[index].value;
                m_pPropertyStore->SetValue(MFPKEY_WMAAECMA_SYSTEM_MODE, pvSysMode);
                PropVariantClear(&pvSysMode);
            }
            break;

        case IDC_SELECT_ECHO:
            if ( CBN_SELCHANGE == HIWORD(wParam) )
            {
                LPARAM index = SendMessage(m_hwndEcho, CB_GETCURSEL, (WORD)0, 0L);

                // Set the number of iterations on echo cancellations
                PROPVARIANT pvSysMode;
                PropVariantInit(&pvSysMode);
                pvSysMode.vt = VT_I4;
                // index is 0, 1 We need 0,2
                pvSysMode.lVal = EchoModes[index].value;
                m_pPropertyStore->SetValue(MFPKEY_WMAAECMA_FEATR_AES, pvSysMode);
                PropVariantClear(&pvSysMode);
            }

            break;

        case IDC_BUTTON1:
            {
                switch(HIWORD(wParam))
                {
                case BN_CLICKED: 
                    // Toggle recording state
                    if (!m_fRecording)
                    {
                        WCHAR szMessage[MAX_PATH] = { 0 };
                        WCHAR szFilename[MAX_PATH] = { 0 };
                        if (SUCCEEDED(GetWaveFileName(szFilename, _countof(szFilename))))
                        {
                            m_pWaveWriter = WaveWriter::Start(szFilename, &m_wfxOut);
                        }

                        // Note, if this is NULL, EITHER GetWaveFileName failed, OR WaveWriter::Start failed
                        if (NULL == m_pWaveWriter)
                        {
                            // This means we failed to open the file.
                            // Set an error message
                            LoadStringW( m_hInstance, IDS_RECORDING_FAILED, szMessage, _countof(szMessage) );
                            SetStatusMessage(szMessage);
                        }
                        else
                        {
                            LoadStringW( m_hInstance, IDS_STATUS, szMessage, _countof(szMessage) );
                            SetStatusMessage(szMessage);

                            // Toggle the text on the record button
                            LoadStringW( m_hInstance, IDS_RECORD_STOP, szMessage, _countof(szMessage) );
                            SendDlgItemMessage(hWnd, IDC_BUTTON1, WM_SETTEXT, 0, (LPARAM)szMessage);

                            m_fRecording = TRUE;
                        }

                    }
                    else
                    {
                        WCHAR** lppPart={NULL};
                        WCHAR szPath[MAX_PATH];
                        WCHAR szMessage[MAX_PATH];

                        LoadStringW( m_hInstance, IDS_RECORDING_SUCCESS, szMessage, _countof(szMessage) );

                        GetFullPathNameW(m_pWaveWriter->GetFileName(), MAX_PATH, szPath, lppPart);                                    
                        StringCchCatW(szMessage, MAX_PATH, szPath);

                        WaveWriter * pW = m_pWaveWriter;
                        m_pWaveWriter = NULL;
                        pW->Stop();
                        delete pW;
                        SetStatusMessage(szMessage);

                        // Toggle the text on the record button
                        LoadStringW( m_hInstance, IDS_RECORD, szMessage, _countof(szMessage) );
                        SendDlgItemMessage(hWnd, IDC_BUTTON1, WM_SETTEXT, 0, (LPARAM)szMessage);

                        m_fRecording = FALSE;
                    }
                    break;
                }
            }
        }

        // Capture all available audio and update audio panel each time timer fires
    case WM_TIMER:
        if (wParam == iAudioReadTimerId)
        {
            ProcessAudio();
        }
        else if(wParam == iEnergyRefreshTimerId)
        {
            Update();
        }
        break;

        // If the titlebar X is clicked, destroy app
    case WM_CLOSE:
        KillTimer(m_hWnd, iAudioReadTimerId);
        KillTimer(m_hWnd, iEnergyRefreshTimerId);
        DestroyWindow(hWnd);
        break;

    case WM_DESTROY:
        // Quit the main message pump
        PostQuitMessage(0);
        break;
    }

    return FALSE;
}

/// <summary>
/// Create the first connected Kinect found.
/// </summary>
/// <returns>S_OK on success, otherwise failure code.</returns>
HRESULT CAudioExplorer::CreateFirstConnected()
{
    INuiSensor * pNuiSensor;
    HRESULT hr;

    int iSensorCount = 0;
    hr = NuiGetSensorCount(&iSensorCount);
    if (FAILED(hr))
    {
        SetStatusMessage(L"Failed to enumerate sensors!");
        return hr;
    }

    // Look at each Kinect sensor
    for (int i = 0; i < iSensorCount; ++i)
    {
        // Create the sensor so we can check status, if we can't create it, move on to the next
        hr = NuiCreateSensorByIndex(i, &pNuiSensor);
        if (FAILED(hr))
        {
            continue;
        }

        // Get the status of the sensor, and if connected, then we can initialize it
        hr = pNuiSensor->NuiStatus();
        if (S_OK == hr)
        {
            m_pNuiSensor = pNuiSensor;
            break;
        }

        // This sensor wasn't OK, so release it since we're not using it
        pNuiSensor->Release();
    }

    if (NULL != m_pNuiSensor)
    {
        // Initialize the Kinect and specify that we'll be using audio signal
        hr = m_pNuiSensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_AUDIO); 
        if (SUCCEEDED(hr))
        {
            hr = InitializeAudioSource();
        }
    }
    else
    {
        SetStatusMessage(L"No ready Kinect found!");
        hr = E_FAIL;
    }

    return hr;
}

/// <summary>
/// Initialize Kinect audio capture/control objects.
/// </summary>
/// <returns>
/// S_OK on success, otherwise failure code.
/// </returns>
HRESULT CAudioExplorer::InitializeAudioSource()
{
    // Get the audio source
    HRESULT hr = m_pNuiSensor->NuiGetAudioSource(&m_pNuiAudioSource);
    if (FAILED(hr))
    {
        SetStatusMessage(L"Failed to get Audio Source!");
        return hr;
    }

    hr = m_pNuiAudioSource->QueryInterface(IID_IMediaObject, (void**)&m_pDMO);
    if (FAILED(hr))
    {
        SetStatusMessage(L"Failed to access the DMO!");
        return hr;
    }

    hr = m_pNuiAudioSource->QueryInterface(IID_IPropertyStore, (void**)&m_pPropertyStore);
    if (FAILED(hr))
    {
        SetStatusMessage(L"Failed to access the Audio Property store!");
        return hr;
    }

    // Set AEC-MicArray DMO system mode.
    // This must be set for the DMO to work properly
    PROPVARIANT pvSysMode;
    PropVariantInit(&pvSysMode);
    pvSysMode.vt = VT_I4;
    //   SINGLE_CHANNEL_AEC = 0
    //   OPTIBEAM_ARRAY_ONLY = 2
    //   OPTIBEAM_ARRAY_AND_AEC = 4
    //   SINGLE_CHANNEL_NSAGC = 5
    pvSysMode.lVal = (LONG)(4);
    m_pPropertyStore->SetValue(MFPKEY_WMAAECMA_SYSTEM_MODE, pvSysMode);
    PropVariantClear(&pvSysMode);

    // Set DMO output format
    WAVEFORMATEX fmt = {AudioFormat, AudioChannels, AudioSamplesPerSecond, AudioAverageBytesPerSecond, AudioBlockAlign, AudioBitsPerSample, 0};
    memcpy_s (&m_wfxOut, sizeof(WAVEFORMATEX), &fmt, sizeof(WAVEFORMATEX));
    DMO_MEDIA_TYPE mt = {0};
    hr = MoInitMediaType(&mt, sizeof(WAVEFORMATEX));
    if (FAILED(hr))
    {
        return hr;
    }
    mt.majortype = MEDIATYPE_Audio;
    mt.subtype = MEDIASUBTYPE_PCM;
    mt.lSampleSize = 0;
    mt.bFixedSizeSamples = TRUE;
    mt.bTemporalCompression = FALSE;
    mt.formattype = FORMAT_WaveFormatEx;	
    memcpy_s(mt.pbFormat, sizeof(WAVEFORMATEX), &m_wfxOut, sizeof(WAVEFORMATEX));

    hr = m_pDMO->SetOutputType(0, &mt, 0); 
    MoFreeMediaType(&mt);

    return hr;
}

/// <summary>
/// Capture new audio data.
/// </summary>
void CAudioExplorer::ProcessAudio()
{
    // Bottom portion of computed energy signal that will be discarded as noise.
    // Only portion of signal above noise floor will be displayed.
	fstream test;
	test.open("test_s.txt", ios::out | ios::app);

    ULONG cbProduced = 0;
    BYTE *pProduced = NULL;
    DWORD dwStatus = 0;
    DMO_OUTPUT_DATA_BUFFER outputBuffer = {0};
    outputBuffer.pBuffer = &m_captureBuffer;

    HRESULT hr = S_OK;

    do
    {
        m_captureBuffer.Init(0);
        outputBuffer.dwStatus = 0;
        hr = m_pDMO->ProcessOutput(0, 1, &outputBuffer, &dwStatus);
        if (FAILED(hr))
        {
            SetStatusMessage(L"Failed to process audio output.");
            break;
        }

        if (S_FALSE == hr)
        {
            cbProduced = 0;
        }
        else
        {
            m_captureBuffer.GetBufferAndLength(&pProduced, &cbProduced);
        }
		allsample += cbProduced;
		
		cout << "\nall sample: " << allsample << endl;
		cout << "capture sample number: "<<cbProduced<<endl;

        if (cbProduced > 0)
        {
            double beamAngle, sourceAngle, sourceConfidence;

            // Obtain beam angle from INuiAudioBeam afforded by microphone array
            m_pNuiAudioSource->GetBeam(&beamAngle);
            m_pNuiAudioSource->GetPosition(&sourceAngle, &sourceConfidence);

			test << 180.0 * beamAngle / M_PI << " " << 180.0 * sourceAngle / M_PI << " " << (float)(sourceConfidence) << endl;
			cout << "beamAngle: " << 80.0 * beamAngle / M_PI << "    sourceAngle: " << 180.0 * sourceAngle / M_PI << "    sourceConfidence: " << sourceConfidence << endl;


            // Convert angles to degrees and set values in audio panel
            m_pAudioPanel->SetBeam(static_cast<float>((180.0 * beamAngle) / M_PI));
            m_pAudioPanel->SetSoundSource(static_cast<float>((180.0 * sourceAngle) / M_PI), static_cast<float>(sourceConfidence));

            // Pass off data to each visualizer to process
            // Note, these are super fast, so no need to only pass to one
            for (int iVisualizer = 0; iVisualizer < iCountOfVisualizers; ++iVisualizer)
            {
                m_rgAudioVisualizers[iVisualizer]->ProcessAudio(pProduced, cbProduced);
            }

            // If we have a recording object and we're actively recording, 
            // go ahead and write down to disk
            if (m_pWaveWriter && m_fRecording)
            {
                m_pWaveWriter->WriteBytes(pProduced, cbProduced);
            }
        }

    } while (outputBuffer.dwStatus & DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE);
}

/// <summary>
/// Display latest audio data.
/// </summary>
void CAudioExplorer::Update()
{
    m_pActiveVisualizer->SetImage(m_pAudioPanel->GetDisplayBitmap());
    m_pActiveVisualizer->Update();
    m_pAudioPanel->Draw();
}

/// <summary>
/// Set the status bar message
/// </summary>
/// <param name="szMessage">message to display</param>
void CAudioExplorer::SetStatusMessage(WCHAR * szMessage)
{
    SendDlgItemMessageW(m_hWnd, IDC_STATUS, WM_SETTEXT, 0, (LPARAM)szMessage);
}