struct TSphere
{
    D2D1_ELLIPSE Shape;
    D2D1_COLOR_F Color;
};

static const unsigned k_ResolutionX = 800;
static const unsigned k_ResolutionY = 800;
static const unsigned k_SphereCount = 500;

static ID2D1SolidColorBrush* s_FillBrush;
static ID2D1SolidColorBrush* s_StrokeBrush;
static TSphere* s_Spheres;

// returns [0.0f, 1.0f)
static inline float
Randomf()
{
    unsigned Exponent = 127;
    unsigned Significand = (unsigned)(rand() & 0x7fff); // get 15 random bits
    unsigned Result = (Exponent << 23) | (Significand << 8);
    return *(float*)&Result - 1.0f;
}

// returns [Begin, End)
static inline float
Randomf(float Begin, float End)
{
    assert(Begin < End);
    return Begin + (End - Begin) * Randomf();
}

static void
Update()
{
    s_Graphics->BeginDraw();
    s_Graphics->Clear(ColorF(ColorF::White));

    for (unsigned SphereIdx = 0; SphereIdx < k_SphereCount; ++SphereIdx)
    {
        s_FillBrush->SetColor(s_Spheres[SphereIdx].Color);
        s_Graphics->FillEllipse(s_Spheres[SphereIdx].Shape, s_FillBrush);
        s_Graphics->DrawEllipse(s_Spheres[SphereIdx].Shape, s_StrokeBrush, 3.0f);
    }

    VHR(s_Graphics->EndDraw());
}

static void
Initialize()
{
    VHR(s_Graphics->CreateSolidColorBrush(ColorF(0), &s_FillBrush));
    VHR(s_Graphics->CreateSolidColorBrush(ColorF(0, 0.125f), &s_StrokeBrush));

    s_Spheres = new TSphere[k_SphereCount];
    for (unsigned SphereIdx = 0; SphereIdx < k_SphereCount; ++SphereIdx)
    {
        float X = Randomf(0.0f, (float)k_ResolutionX);
        float Y = Randomf(0.0f, (float)k_ResolutionY);
        float R = Randomf(20.0f, 120.0f);

        s_Spheres[SphereIdx].Shape = Ellipse(Point2F(X, Y), R, R);
        s_Spheres[SphereIdx].Color = ColorF(Randomf(), Randomf(), Randomf(), 0.2f);
    }
}

static void
Shutdown()
{
    delete[] s_Spheres;
    CRELEASE(s_FillBrush);
    CRELEASE(s_StrokeBrush);
}
// vim: set ts=4 sw=4 expandtab:
