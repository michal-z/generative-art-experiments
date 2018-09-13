static eastl::vector<uint8_t>
FLoadFile(const char* FileName)
{
    FILE* File = fopen(FileName, "rb");
    assert(File);
    fseek(File, 0, SEEK_END);
    long Size = ftell(File);
    assert(Size != -1);
    eastl::vector<uint8_t> Content(Size);
    fseek(File, 0, SEEK_SET);
    fread(&Content[0], 1, Content.size(), File);
    fclose(File);
    return Content;
}

static double
FGetAbsoluteTime()
{
    static LARGE_INTEGER StartCounter;
    static LARGE_INTEGER Frequency;
    if (StartCounter.QuadPart == 0)
    {
        QueryPerformanceFrequency(&Frequency);
        QueryPerformanceCounter(&StartCounter);
    }
    LARGE_INTEGER Counter;
    QueryPerformanceCounter(&Counter);
    return (Counter.QuadPart - StartCounter.QuadPart) / (double)Frequency.QuadPart;
}

// returns [0.0f, 1.0f)
static inline float
FRandomf()
{
	unsigned exponent = 127;
	unsigned significand = (unsigned)(rand() & 0x7fff); // get 15 random bits
	unsigned result = (exponent << 23) | (significand << 8);
	return *(float*)&result - 1.0f;
}

// returns [begin, end)
static inline float
FRandomf(float begin, float end)
{
	assert(begin < end);
	return begin + (end - begin) * FRandomf();
}

/*
.align 16
.globl rng_hash_128
.type  rng_hash_128,@function
rng_hash_128:
        mov    8(%rdi), %rax

        movabs $0x6595a395a1ec531b, %rcx

#   Option 1)
#   Non-threaded, fastest.  No xor instruction used.

#   Option 2)
#   Threaded, use the address of the seed as a nonce.
#      xor    %rdi, %rax

#   Option 3)
#   Threaded, pass a nonce as a second parameter.
#        xor    %rsi, %rax
#  or have it stored in a larger seed:
#      xor    16(%rdi), %rax

        mov    (%rdi), %rsi
        mul    %rcx

        add    %rcx, (%rdi)
        adc    %rsi, 8(%rdi)

        xor    %rsi, %rax
        xor    %rdx, %rax
        mul    %rcx
        add    %rsi, %rax
        add    %rdx, %rax

        retq
.size        rng_hash_128, .-rng_hash_128 
*/

static unsigned
FRngHash128(uint64_t Seed[2])
{
	uint64_t Rax = Seed[1];
	uint64_t Rcx = 7319936632422683419ull;
	Rax = Rax ^ (uint64_t)Seed;
	uint64_t Rsi = Seed[0];
	uint64_t Rdx;
	Rax = _umul128(Rax, Rcx, &Rdx);
	uint8_t Carry = _addcarry_u64(0, Seed[0], Rcx, &Seed[0]);
	_addcarry_u64(Carry, Seed[1], Rsi, &Seed[1]);
	Rax = Rax ^ Rsi;
	Rax = Rax ^ Rdx;
	Rax = _umul128(Rax, Rcx, &Rdx);
	Rax = Rax + Rsi;
	Rax = Rax + Rdx;
	return (unsigned)Rax;
}
// vim: set ts=4 sw=4 expandtab:
