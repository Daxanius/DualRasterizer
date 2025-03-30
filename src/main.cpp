#include "pch.h"

#if defined(_DEBUG)
#include "vld.h"
#endif

#undef main
#include "Renderer.h"
#include "DirectXRenderBackend.h"
#include "SoftwareRenderBackend.h"
#include "Utils.h"
#include "MeshEffect.h"
#include "Texture.h"

using namespace dae;

static void ShutDown(SDL_Window* pWindow)
{
	SDL_DestroyWindow(pWindow);
	SDL_Quit();
}

static void PrintControls() {
	std::cout << "[Key bindings - Shared]" << '\n';
	std::cout << "    [F1] Toggle Rasterizer Mode (Directx/Software)" << '\n';
	std::cout << "    [F2] Toggle Vehicle Rotation (ON/OFF)" << '\n';
	std::cout << "    [F9] Cycle CullMode (BACK/FRONT/NONE)" << '\n';
	std::cout << "    [F10] Toggle Uniform ClearColor (ON/OFF)" << '\n';
	std::cout << "    [F11] Toggle Print FPS (ON/OFF)" << '\n';
	std::cout << '\n';

	std::cout << "[Key bindings - Directx]" << '\n';
	std::cout << "    [F3] Toggle FireFX (ON/OFF)" << '\n';
	std::cout << "    [F4] Cycle Sampler State (POINT/LINEAR/ANISOTROPIC)" << '\n';
	std::cout << '\n';

	std::cout << "[Key bindings - Software]" << '\n';
	std::cout << "    [F5] Cycle Shading Mode (COMBINED/OBSERVED_AREA/DIFFUSE/SPECULAR)" << '\n';
	std::cout << "    [F6] Toggle NormalMap (ON/OFF)" << '\n';
	std::cout << "    [F7] Toggle DepthBuffer Visualization (ON/OFF)" << '\n';
	std::cout << "    [F8] Toggle BoundingBox Visualization (ON/OFF)" << '\n';
	std::cout << '\n';
}

int main(int argc, char* args[])
{
	//Unreferenced parameters
	(void)argc;
	(void)args;

	//Create window + surfaces
	SDL_Init(SDL_INIT_VIDEO);

	const uint32_t width = 640;
	const uint32_t height = 480;

	SDL_Window* pWindow = SDL_CreateWindow(
		"Dual Rasterizer",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width, height, 0);

	if (!pWindow)
		return 1;

	PrintControls();

	// The colors
	const ColorRGB uniformColor{ .1f, .1f, .1f };
	const ColorRGB directXColor{ .39f, .59f, .93f };
	const ColorRGB softwareColor{ .39f, .39f, .39f };

	// Create the renderers
	DirectXRenderBackend* directXBackend = new DirectXRenderBackend(pWindow);
	SoftwareRenderBackend* softwareBackend = new SoftwareRenderBackend(pWindow);

	// Set the colors
	directXBackend->SetBackgroundColor(directXColor);
	softwareBackend->SetBackgroundColor(softwareColor);

	// Load the textures
	std::shared_ptr<Texture> vehicleDiffuse{ Texture::LoadFromFile("resources/vehicle_diffuse.png", directXBackend->GetDevice()) };
	std::shared_ptr<Texture> vehicleNormal{ Texture::LoadFromFile("resources/vehicle_normal.png", directXBackend->GetDevice()) };
	std::shared_ptr<Texture> vehicleGloss{ Texture::LoadFromFile("resources/vehicle_gloss.png", directXBackend->GetDevice()) };
	std::shared_ptr<Texture> vehicleSpecular{ Texture::LoadFromFile("resources/vehicle_specular.png", directXBackend->GetDevice()) };

	std::shared_ptr<Texture> fireDiffuse{ Texture::LoadFromFile("resources/fireFX_diffuse.png", directXBackend->GetDevice()) };

	// Create the scene / renderer
	const auto pTimer = new Timer();
	Renderer* pRenderer = new Renderer(directXBackend);

	// Get the mesh pointer list
	std::vector<Mesh*>& meshes = pRenderer->GetMeshes();

	// Load the meshes
	std::vector<Vertex> vehicleVertices{};
	std::vector<uint32_t> vehicleIndices{};

	std::vector<Vertex> fireVertices{};
	std::vector<uint32_t> fireIndices{};

	Utils::ParseOBJ("resources/vehicle.obj", vehicleVertices, vehicleIndices);
	Utils::ParseOBJ("resources/fireFX.obj", fireVertices, fireIndices);

	std::shared_ptr<MeshEffect> vehicleEffect = std::make_shared<MeshEffect>(directXBackend->GetDevice(), L"resources/PostCol3D.fx");
	std::shared_ptr<FireMeshEffect> fireEffect = std::make_shared<FireMeshEffect>(directXBackend->GetDevice(), L"resources/Fire3D.fx");

	Mesh vehicleMesh{ Mesh::PrimitiveTopology::TriangleList, std::move(vehicleVertices), std::move(vehicleIndices), vehicleEffect };
	vehicleMesh.SetDiffuse(vehicleDiffuse);
	vehicleMesh.SetNormal(vehicleNormal);
	vehicleMesh.SetGlossiness(vehicleGloss);
	vehicleMesh.SetSpecular(vehicleSpecular);
	meshes.push_back(&vehicleMesh);

	Mesh fireMesh{ Mesh::PrimitiveTopology::TriangleList, std::move(fireVertices), std::move(fireIndices), fireEffect };
	fireMesh.SetDiffuse(fireDiffuse);
	fireMesh.DisableSoftwareRendering();
	meshes.push_back(&fireMesh);

	// Settings
	bool isDirectX{ true };
	bool printFps{ true };
	bool isUniform{ false };

	//Start loop
	pTimer->Start();
	float printTimer = 0.f;
	bool isLooping = true;
	while (isLooping)
	{
		//--------- Get input events ---------
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_QUIT:
				isLooping = false;
				break;
			case SDL_KEYUP:
				//Test for a key
				if (e.key.keysym.scancode == SDL_SCANCODE_F1) {
					if (isDirectX) {
						pRenderer->SetRenderBackend(softwareBackend);
						std::cout << "Switched to software backend" << std::endl;
					} else {
						pRenderer->SetRenderBackend(directXBackend);
						std::cout << "Switched to DirectX backend" << std::endl;
					}

					isDirectX = !isDirectX;
				}

				if (e.key.keysym.scancode == SDL_SCANCODE_F2) {
					pRenderer->ToggleRotation();
				}

				if (e.key.keysym.scancode == SDL_SCANCODE_F3) {
					fireMesh.SetVisible(!fireMesh.Visible());
				}

				if (e.key.keysym.scancode == SDL_SCANCODE_F4) {
					directXBackend->SwitchTechnique();
				}

				if (e.key.keysym.scancode == SDL_SCANCODE_F5) {
					softwareBackend->CycleShadingMode();
				}

				if (e.key.keysym.scancode == SDL_SCANCODE_F6) {
					softwareBackend->ToggleNormalMap();
				}

				if (e.key.keysym.scancode == SDL_SCANCODE_F7) {
					softwareBackend->CycleViewMode();
				}

				if (e.key.keysym.scancode == SDL_SCANCODE_F8) {
					softwareBackend->ToggleBoundingBox();
				}

				if (e.key.keysym.scancode == SDL_SCANCODE_F9) {
					// Switch cullmode for all meshes
					for (Mesh* mesh : meshes) {
						mesh->ToggleCullMode();
					}
				}

				if (e.key.keysym.scancode == SDL_SCANCODE_F10) {
					if (isUniform) {
						softwareBackend->SetBackgroundColor(softwareColor);
						directXBackend->SetBackgroundColor(directXColor);

						std::cout << "Disabled uniform color" << std::endl;
					} else {
						softwareBackend->SetBackgroundColor(uniformColor);
						directXBackend->SetBackgroundColor(uniformColor);

						std::cout << "Enabled uniform color" << std::endl;
					}

					isUniform = !isUniform;
				}

				if (e.key.keysym.scancode == SDL_SCANCODE_F11) {
					printFps = !printFps;

					if (printFps) {
						std::cout << "Enabled FPS printing" << std::endl;
					} else {
						std::cout << "Disabled FPS printing" << std::endl;
					}
				}
				break;
			default: ;
			}
		}

		//--------- Update ---------
		pRenderer->Update(pTimer);

		//--------- Render ---------
		pRenderer->Render();

		//--------- Timer ---------
		pTimer->Update();
		printTimer += pTimer->GetElapsed();
		if (printTimer >= 1.f && printFps)
		{
			printTimer = 0.f;
			std::cout << "dFPS: " << pTimer->GetdFPS() << std::endl;
		}
	}
	pTimer->Stop();

	//Shutdown "framework"
	delete pRenderer;
	delete directXBackend;
	delete softwareBackend;
	delete pTimer;

	ShutDown(pWindow);
	return 0;
}