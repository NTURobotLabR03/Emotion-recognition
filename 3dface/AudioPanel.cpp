﻿//------------------------------------------------------------------------------
// <copyright file="AudioPanel.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "AudioPanel.h"

/// <summary>
/// Constructor
/// </summary>
AudioPanel::AudioPanel() : 
    m_hWnd(0),
    m_pD2DFactory(NULL), 
    m_pRenderTarget(NULL),
    m_RenderTargetTransform(D2D1::Matrix3x2F::Identity()),
    m_uiDisplayWidth(0),
    m_uiDisplayHeight(0),
    m_pBackground(NULL),
    m_uiBackgroundStride(0),
    m_pDisplay(NULL),
    m_displayPosition(D2D1::RectF()),
    m_pBeamGauge(NULL),
    m_pBeamGaugeFill(NULL),
    m_pBeamNeedle(NULL),
    m_pBeamNeedleFill(NULL),
    m_BeamNeedleTransform(D2D1::Matrix3x2F::Identity()),
    m_pSourceGauge(NULL),
    m_pSourceGaugeFill(NULL),
    m_SourceGaugeTransform(D2D1::Matrix3x2F::Identity()),
    m_pPanelOutline(NULL),
    m_pPanelOutlineStroke(NULL)
{
}

/// <summary>
/// Destructor
/// </summary>
AudioPanel::~AudioPanel()
{
    DiscardResources();
    SafeRelease(m_pD2DFactory);
}

/// <summary>
/// Set the window to draw to as well as the video format
/// Implied bits per pixel is 32
/// </summary>
/// <param name="hWnd">window to draw to</param>
/// <param name="pD2DFactory">already created D2D factory object</param>
/// <param name="energyToDisplay">Number of energy samples to display at any given time.</param>
/// <returns>S_OK on success, otherwise failure code.</returns>
HRESULT AudioPanel::Initialize(const HWND hWnd, ID2D1Factory* pD2DFactory, const UINT energyToDisplay)
{
    if (NULL == pD2DFactory)
    {
        return E_INVALIDARG;
    }

    m_hWnd = hWnd;

    // One factory for the entire application so save a pointer here
    m_pD2DFactory = pD2DFactory;

    m_pD2DFactory->AddRef();

    m_uiDisplayWidth = energyToDisplay;
    m_uiDisplayHeight = m_uiDisplayWidth / 4;

    return EnsureResources();
}

/// <summary>
/// Draws audio panel.
/// </summary>
/// <returns>S_OK on success, otherwise failure code.</returns>
HRESULT AudioPanel::Draw()
{
    // create the resources for this draw device. They will be recreated if previously lost.
    HRESULT hr = EnsureResources();

    if ( FAILED(hr) )
    {
        return hr;
    }
    
    m_pRenderTarget->BeginDraw();
    m_pRenderTarget->SetTransform(m_RenderTargetTransform);

    m_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

    // Draw energy display
    m_pRenderTarget->DrawBitmap(m_pDisplay, m_displayPosition);

    // Draw audio beam gauge
    m_pRenderTarget->FillGeometry(m_pBeamGauge, m_pBeamGaugeFill, NULL);
    m_pRenderTarget->SetTransform(m_BeamNeedleTransform * m_RenderTargetTransform);
    m_pRenderTarget->FillGeometry(m_pBeamNeedle, m_pBeamNeedleFill, NULL);
    m_pRenderTarget->SetTransform(m_RenderTargetTransform);

    // Draw sound source gauge
    m_pSourceGaugeFill->SetTransform(m_SourceGaugeTransform);
    m_pRenderTarget->FillGeometry(m_pSourceGauge, m_pSourceGaugeFill, NULL);

    // Draw panel outline
    m_pRenderTarget->DrawGeometry(m_pPanelOutline, m_pPanelOutlineStroke, 0.001f);
            
    hr = m_pRenderTarget->EndDraw();

    // Device lost, need to recreate the render target. We'll dispose it now and retry drawing.
    if (D2DERR_RECREATE_TARGET == hr)
    {
        hr = S_OK;
        DiscardResources();
    }

    return hr;
}

void AudioPanel::SetBeam(const float & beamAngle)
{
    if (NULL == m_pRenderTarget)
    {
        return;
    }

    m_BeamNeedleTransform = D2D1::Matrix3x2F::Rotation(-beamAngle, D2D1::Point2F(0.5f,0.0f));
}

void AudioPanel::SetSoundSource(const float soundSourceAngle, const float soundSourceConfidence)
{
    if (NULL == m_pRenderTarget)
    {
        return;
    }

    // Maximum possible confidence corresponds to this gradient width
    const float cMinGradientWidth = 0.04f;

    // Set width of mark based on confidence.
    // A confidence of 0 would give us a gradient that fills whole area diffusely.
    // A confidence of 1 would give us the narrowest allowed gradient width.
    float width = max((1 - soundSourceConfidence), cMinGradientWidth);

    // Update the gradient representing sound source position to reflect confidence
    CreateSourceGaugeFill(width);

    m_SourceGaugeTransform = D2D1::Matrix3x2F::Rotation(-soundSourceAngle, D2D1::Point2F(0.5f,0.0f));
}


/// <summary>
/// Dispose of Direct2d resources.
/// </summary>
void AudioPanel::DiscardResources()
{
    if (m_pBackground != NULL)
    {
        delete []m_pBackground;
    }
    
    SafeRelease(m_pRenderTarget);
    SafeRelease(m_pDisplay);
    SafeRelease(m_pBeamGauge);
    SafeRelease(m_pBeamGaugeFill);
    SafeRelease(m_pBeamNeedle);
    SafeRelease(m_pBeamNeedleFill);
    SafeRelease(m_pSourceGauge);
    SafeRelease(m_pSourceGaugeFill);
    SafeRelease(m_pPanelOutline);
    SafeRelease(m_pPanelOutlineStroke);
}

/// <summary>
/// Ensure necessary Direct2d resources are created
/// </summary>
/// <returns>indicates success or failure</returns>
HRESULT AudioPanel::EnsureResources()
{
    HRESULT hr = S_OK;

    if (NULL == m_pRenderTarget)
    {
        // Get the panel size
        RECT panelRect = {0};
        GetWindowRect(m_hWnd, &panelRect);
        UINT panelWidth = panelRect.right - panelRect.left;
        UINT panelHeight = panelRect.bottom - panelRect.top;
        D2D1_SIZE_U size = D2D1::SizeU(panelWidth, panelHeight);

        m_RenderTargetTransform = D2D1::Matrix3x2F::Scale(D2D1::SizeF((FLOAT)panelWidth, (FLOAT)panelWidth));

        D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
        rtProps.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE);
        rtProps.usage = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;

        // Create a hWnd render target, in order to render to the window set in initialize
        hr = m_pD2DFactory->CreateHwndRenderTarget(
            rtProps,
            D2D1::HwndRenderTargetProperties(m_hWnd, size),
            &m_pRenderTarget
            );

        if (SUCCEEDED(hr))
        {
            hr = CreateDisplay();

            if (SUCCEEDED(hr))
            {
                hr = CreateBeamGauge();

                if (SUCCEEDED(hr))
                {
                    hr = CreateSourceGauge();

                    if (SUCCEEDED(hr))
                    {
                        hr = CreatePanelOutline();
                    }
                }
            }
        }
    }

    if ( FAILED(hr) )
    {
        DiscardResources();
    }

    return hr;
}

/// <summary>
    /// Create the bitmap for visualizers to write into
/// </summary>
/// <returns>S_OK on success, otherwise failure code.</returns>
HRESULT AudioPanel::CreateDisplay()
{
    const int cBytesPerPixel = 4;
    const int cMaxPixelChannelIntensity = 0xff;

    D2D1_SIZE_U size = D2D1::SizeU(m_uiDisplayWidth, m_uiDisplayHeight);
    HRESULT hr = S_OK;

    // Allocate background and set to white
    m_uiBackgroundStride = cBytesPerPixel * m_uiDisplayWidth;
    int numBackgroundBytes = m_uiBackgroundStride * m_uiDisplayHeight * 2;
    m_pBackground = new BYTE[numBackgroundBytes];
    memset(m_pBackground, cMaxPixelChannelIntensity, numBackgroundBytes);

    // Specify layout position for energy display
    m_displayPosition = D2D1::RectF(0.13f, 0.0353f, 0.87f, 0.2203f);

    hr = m_pRenderTarget->CreateBitmap(
                size, 
                D2D1::BitmapProperties( D2D1::PixelFormat( DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE) ),
                &m_pDisplay 
                );

    if (SUCCEEDED(hr))
    {
        m_pDisplay->CopyFromMemory(NULL, m_pBackground, m_uiBackgroundStride);
    }

    return hr;
}

/// <summary>
/// Create gauge (with needle) used to display beam angle.
/// </summary>
/// <returns>S_OK on success, otherwise failure code.</returns>
HRESULT AudioPanel::CreateBeamGauge()
{
    HRESULT hr = m_pD2DFactory->CreatePathGeometry(&m_pBeamGauge);

    // Create gauge background shape
    if (SUCCEEDED(hr))
    {
        ID2D1GeometrySink *pGeometrySink = NULL;
        hr = m_pBeamGauge->Open(&pGeometrySink);

        if (SUCCEEDED(hr))
        {
            pGeometrySink->BeginFigure(D2D1::Point2F(0.1503f,0.2832f), D2D1_FIGURE_BEGIN_FILLED);
            pGeometrySink->AddLine(D2D1::Point2F(0.228f,0.2203f));
            pGeometrySink->AddArc(D2D1::ArcSegment(D2D1::Point2F(0.772f,0.2203f), D2D1::SizeF(0.35f,0.35f), 102, D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
            pGeometrySink->AddLine(D2D1::Point2F(0.8497f,0.2832f));
            pGeometrySink->AddArc(D2D1::ArcSegment(D2D1::Point2F(0.1503f,0.2832f), D2D1::SizeF(0.45f,0.45f), 102, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
            pGeometrySink->EndFigure(D2D1_FIGURE_END_CLOSED);
            hr = pGeometrySink->Close();

            // Create gauge background brush
            if (SUCCEEDED(hr))
            {
                ID2D1GradientStopCollection *pGradientStops = NULL;
                D2D1_GRADIENT_STOP gradientStops[4];
                gradientStops[0].color = D2D1::ColorF(D2D1::ColorF::LightGray, 1);
                gradientStops[0].position = 0.0f;
                gradientStops[1].color = D2D1::ColorF(D2D1::ColorF::LightGray, 1);
                gradientStops[1].position = 0.34f;
                gradientStops[2].color = D2D1::ColorF(D2D1::ColorF::WhiteSmoke, 1);
                gradientStops[2].position = 0.37f;
                gradientStops[3].color = D2D1::ColorF(D2D1::ColorF::WhiteSmoke, 1);
                gradientStops[3].position = 1.0f;
                hr = m_pRenderTarget->CreateGradientStopCollection(
                    gradientStops,
                    4,
                    &pGradientStops
                    );

                if (SUCCEEDED(hr))
                {
                    hr = m_pRenderTarget->CreateRadialGradientBrush(D2D1::RadialGradientBrushProperties(D2D1::Point2F(0.5f,0.0f), D2D1::Point2F(0.0f,0.0f), 1.0f, 1.0f), pGradientStops, &m_pBeamGaugeFill);

                    if (SUCCEEDED(hr))
                    {
                        // Create gauge needle shape and fill brush
                        hr = CreateBeamGaugeNeedle();
                    }
                }

                SafeRelease(pGradientStops);
            }
        }

        SafeRelease(pGeometrySink);
    }

    return hr;
}

/// <summary>
/// Create gauge needle used to display beam angle.
/// </summary>
/// <returns>S_OK on success, otherwise failure code.</returns>
HRESULT AudioPanel::CreateBeamGaugeNeedle()
{
    HRESULT hr = m_pD2DFactory->CreatePathGeometry(&m_pBeamNeedle);

    // Create gauge needle shape
    if (SUCCEEDED(hr))
    {
        ID2D1GeometrySink *pGeometrySink = NULL;
        hr = m_pBeamNeedle->Open(&pGeometrySink);

        if (SUCCEEDED(hr))
        { 
            pGeometrySink->BeginFigure(D2D1::Point2F(0.495f,0.35f), D2D1_FIGURE_BEGIN_FILLED);
            pGeometrySink->AddLine(D2D1::Point2F(0.505f,0.35f));
            pGeometrySink->AddLine(D2D1::Point2F(0.5f,0.44f));
            pGeometrySink->EndFigure(D2D1_FIGURE_END_CLOSED);
            hr = pGeometrySink->Close();

            // Create gauge needle brush
            if (SUCCEEDED(hr))
            {
                ID2D1GradientStopCollection *pGradientStops = NULL;
                D2D1_GRADIENT_STOP gradientStops[4];
                gradientStops[0].color = D2D1::ColorF(D2D1::ColorF::LightGray, 1);
                gradientStops[0].position = 0.0f;
                gradientStops[1].color = D2D1::ColorF(D2D1::ColorF::LightGray, 1);
                gradientStops[1].position = 0.35f;
                gradientStops[2].color = D2D1::ColorF(D2D1::ColorF::BlueViolet, 1);
                gradientStops[2].position = 0.395f;
                gradientStops[3].color = D2D1::ColorF(D2D1::ColorF::BlueViolet, 1);
                gradientStops[3].position = 1.0f;
                hr = m_pRenderTarget->CreateGradientStopCollection(
                    gradientStops,
                    4,
                    &pGradientStops
                    );

                if (SUCCEEDED(hr))
                {
                    hr = m_pRenderTarget->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(D2D1::Point2F(0.5f,0.0f), D2D1::Point2F(0.5f,1.0f)), pGradientStops, &m_pBeamNeedleFill);
                }

                SafeRelease(pGradientStops);
            }
        }

        SafeRelease(pGeometrySink);
    }

    return hr;
}

/// <summary>
/// Create gauge (with position cloud) used to display sound source angle.
/// </summary>
/// <returns>S_OK on success, otherwise failure code.</returns>
HRESULT AudioPanel::CreateSourceGauge()
{
    HRESULT hr = m_pD2DFactory->CreatePathGeometry(&m_pSourceGauge);

    // Create gauge background shape
    if (SUCCEEDED(hr))
    {
        ID2D1GeometrySink *pGeometrySink = NULL;
        hr = m_pSourceGauge->Open(&pGeometrySink);

        if (SUCCEEDED(hr))
        {
            pGeometrySink->BeginFigure(D2D1::Point2F(0.1270f,0.3021f), D2D1_FIGURE_BEGIN_FILLED);
            pGeometrySink->AddLine(D2D1::Point2F(0.1503f,0.2832f));
            pGeometrySink->AddArc(D2D1::ArcSegment(D2D1::Point2F(0.8497f,0.2832f), D2D1::SizeF(0.45f,0.45f), 102, D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
            pGeometrySink->AddLine(D2D1::Point2F(0.8730f,0.3021f));
            pGeometrySink->AddArc(D2D1::ArcSegment(D2D1::Point2F(0.1270f,0.3021f), D2D1::SizeF(0.48f,0.48f), 102, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
            pGeometrySink->EndFigure(D2D1_FIGURE_END_CLOSED);
            hr = pGeometrySink->Close();

            // Create gauge background brush
            if (SUCCEEDED(hr))
            {
                hr = CreateSourceGaugeFill(0.1f);
            }
        }

        SafeRelease(pGeometrySink);
    }

    return hr;
}

/// <summary>
/// Create gradient used to represent sound source confidence, with the
/// specified width.
/// </summary>
/// <param name="width">
/// Width of gradient, specified in [0.0,1.0] interval.
/// </param>
/// <returns>S_OK on success, otherwise failure code.</returns>
HRESULT AudioPanel::CreateSourceGaugeFill(const float & width)
{
    HRESULT hr = S_OK;

    float halfWidth = width / 2;
    ID2D1GradientStopCollection *pGradientStops = NULL;
    D2D1_GRADIENT_STOP gradientStops[5];
    gradientStops[0].color = D2D1::ColorF(D2D1::ColorF::White, 1);
    gradientStops[0].position = 0.0f;
    gradientStops[1].color = D2D1::ColorF(D2D1::ColorF::White, 1);
    gradientStops[1].position = max(0.5f - halfWidth, 0.0f);
    gradientStops[2].color = D2D1::ColorF(D2D1::ColorF::BlueViolet, 1);
    gradientStops[2].position = 0.5f;
    gradientStops[3].color = D2D1::ColorF(D2D1::ColorF::White, 1);
    gradientStops[3].position = min(0.5f + halfWidth, 1.0f);
    gradientStops[4].color = D2D1::ColorF(D2D1::ColorF::White, 1);
    gradientStops[4].position = 1.0f;
    hr = m_pRenderTarget->CreateGradientStopCollection(
        gradientStops,
        5,
        &pGradientStops
        );

    if (SUCCEEDED(hr))
    {
        SafeRelease(m_pSourceGaugeFill);
        hr = m_pRenderTarget->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(D2D1::Point2F(0.0f,0.5f), D2D1::Point2F(1.0f,0.5f)), pGradientStops, &m_pSourceGaugeFill);
    }

    SafeRelease(pGradientStops);
    return hr;
}

/// <summary>
/// Create outline that frames both gauges and energy display into a cohesive panel.
/// </summary>
/// <returns>S_OK on success, otherwise failure code.</returns>
HRESULT AudioPanel::CreatePanelOutline()
{
    HRESULT hr = m_pD2DFactory->CreatePathGeometry(&m_pPanelOutline);

    // Create panel outline path
    if (SUCCEEDED(hr))
    {
        ID2D1GeometrySink *pGeometrySink = NULL;
        hr = m_pPanelOutline->Open(&pGeometrySink);

        if (SUCCEEDED(hr))
        {
            /// Draw left wave display frame
            pGeometrySink->BeginFigure(D2D1::Point2F(0.15f,0.0353f), D2D1_FIGURE_BEGIN_FILLED);
            pGeometrySink->AddLine(D2D1::Point2F(0.13f,0.0353f));
            pGeometrySink->AddLine(D2D1::Point2F(0.13f,0.2203f));
            pGeometrySink->AddLine(D2D1::Point2F(0.2280f,0.2203f));

            // Draw gauge outline
            pGeometrySink->AddLine(D2D1::Point2F(0.1270f,0.3021f));
            pGeometrySink->AddArc(D2D1::ArcSegment(D2D1::Point2F(0.8730f,0.3021f), D2D1::SizeF(0.48f,0.48f), 102, D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
            pGeometrySink->AddLine(D2D1::Point2F(0.7720f,0.2203f));
            pGeometrySink->AddArc(D2D1::ArcSegment(D2D1::Point2F(0.2280f,0.2203f), D2D1::SizeF(0.35f,0.35f), 102, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));

            // Reposition geometry without drawing
            pGeometrySink->SetSegmentFlags(D2D1_PATH_SEGMENT_FORCE_UNSTROKED);
            pGeometrySink->AddLine(D2D1::Point2F(0.7720f,0.2203f));
            pGeometrySink->SetSegmentFlags(D2D1_PATH_SEGMENT_NONE);

            // Draw right wave display frame
            pGeometrySink->AddLine(D2D1::Point2F(0.87f,0.2203f));
            pGeometrySink->AddLine(D2D1::Point2F(0.87f,0.0353f));
            pGeometrySink->AddLine(D2D1::Point2F(0.85f,0.0353f));
            pGeometrySink->EndFigure(D2D1_FIGURE_END_OPEN);
            hr = pGeometrySink->Close();

            // Create panel outline brush
            if (SUCCEEDED(hr))
            {
                hr = m_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightGray), &m_pPanelOutlineStroke);
            }
        }

        SafeRelease(pGeometrySink);
    }

    return hr;
}