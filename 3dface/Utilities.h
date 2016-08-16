//------------------------------------------------------------------------------
// <copyright file="Utilities.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
// Types of windowing functions
enum FFTWindowFunction
{
    RECTANGULAR,
    HANN, 
    HAMMING,
    NUTTALL,
    BLACKMANHARRIS,
    BLACKMANNUTTALL

};

/// <summary>
/// Creates a "Window" buffer which is multiplied by a time domain signal to
/// get cleaner frequency domain results
/// </summary>
/// <param name="window">pointer to the buffer to initialize</param>
/// <param name="count">number of elements in the buffer</param>
/// <param name="type">type of window to create</param>
void InitializeFFTWindow(float * window, UINT count, FFTWindowFunction type);

struct ListBoxEntry
{
    int index;
    int string;
    int value;
    bool fDefault;
};

/// <summary>
/// Initializes a dropdown w/ a set of strings
/// </summary>
/// <param name="hInstance">Handle to the application instance</param>
/// <param name="hwndControl">HWND of the control</param>
/// <param name="pEntries">Pointer to an array of ListBoxEntries</param>
/// <param name="cEntries">Number of entries</param>
void LoadDropDown(HINSTANCE hInstance, HWND hwndControl, ListBoxEntry * pEntries, UINT cEntries);

