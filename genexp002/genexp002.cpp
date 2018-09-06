static void
Update(TGenExp002& E002, TDirectX12& Dx, double Time, float DeltaTime)
{
	ImGui::ShowDemoWindow();
}

static void
Initialize(TGenExp002& E002, TDirectX12& Dx)
{
}

static void
Shutdown(TGenExp002& E002, TDirectX12& Dx)
{
	WaitForGpu(Dx);
}
// vim: set ts=4 sw=4 expandtab:
