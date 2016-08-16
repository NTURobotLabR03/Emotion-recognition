//------------------------------------------------------------------------------
// <copyright file="Utilities.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------
#include "stdafx.h"

// For M_PI and log definitions
#define _USE_MATH_DEFINES
#include "math.h"
#include "Utilities.h"

/// <summary>
///  Initializes an array to contain values corresponding to a particular computed windowing function
/// </summary>
/// <param name="window">The array to initialize with the window function</param>
/// <param name="count">Length of the window to computer</param>
/// <param name="type">Specific function to use compute the window</param>
void InitializeFFTWindow(float * window, UINT count, FFTWindowFunction type)
{
    float innerValue =  (float) (2 * M_PI/ (count - 1)); // 2PI / FFT length -1 ..  6.283185307f 
    float a0 = 0;
    float a1 = 0;
    float a2 = 0;
    float a3 = 0;
    bool fHighDynamicRangeWindow = false;

    switch (type)
    {
    case RECTANGULAR:
            for (UINT i = 0; i < count; ++i)
            {
                window[i] = 1.0f;
            }
            break;
    case HANN:
            for (UINT i = 0; i < count; ++i)
            {
                window[i] = (float) .5 * ( 1 - cosf( i * innerValue ));
            }
            break;
    case HAMMING:
            for (UINT i = 0; i < count; ++i)
            {
                window[i] = (float) (.54 - ( .46 * cosf( i * innerValue )));
            }
            break;
    case NUTTALL:
            a0 = .355768f;
            a1 = .487396f;
            a2 = .144232f;
            a3 = .012604f;

            fHighDynamicRangeWindow = true;
            break;
    case BLACKMANHARRIS:
            a0 = .35875f;
            a1 = .48829f;
            a2 = .14128f;
            a3 = .01168f;

            fHighDynamicRangeWindow = true;
            break;
    case BLACKMANNUTTALL:
            a0 = .3635819f;
            a1 = .4891775f;
            a2 = .1365995f;
            a3 = .0106411f;

            fHighDynamicRangeWindow = true;
            break;
    }

    if (fHighDynamicRangeWindow)
    {
            for (UINT i = 0; i < count; ++i)
            {
                float coef = i * innerValue;
                window[i] = (float) a0 - a1 * cosf( coef ) + a2 * cosf( coef ) - a3 * cosf( coef );
            }

    }
}

/// <summary>
/// Initializes a dropdown w/ a set of strings
/// </summary>
/// <param name="hInstance">Handle to the application instance</param>
/// <param name="hwndDialog">HWND of the control</param>
/// <param name="pEntries">Pointer to an array of ListBoxEntries</param>
/// <param name="cEntries">Number of entries</param>
void LoadDropDown(HINSTANCE hInstance, HWND hwnd, ListBoxEntry * pEntries, UINT cEntries)
{
    WCHAR szComboText[512] = { 0 };
    for (UINT i = 0; i < cEntries; ++i)
    {
        LoadStringW( hInstance, pEntries[i].string, szComboText, _countof(szComboText) );
        WPARAM index = SendMessage(hwnd, CB_INSERTSTRING , static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(szComboText));

        if (pEntries[i].fDefault)
        {
            SendMessage(hwnd, CB_SETCURSEL, index, 0);
        }
    }

}