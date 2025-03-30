#include "Renderer.h"


dae::Renderer::Renderer(AbstractRenderBackend* pRenderBackend) : m_pRenderBackend(pRenderBackend) {
	m_Camera.Initialize(pRenderBackend->GetWidth(), pRenderBackend->GetHeight(), 45.f, {.0f,.0f, 0.f});
}

void dae::Renderer::Update(const Timer* pTimer) {
	// Update the camera of course
	m_Camera.Update(pTimer);

	if (m_RotationEnabled) {
		m_Rotation += PI / 1 * pTimer->GetElapsed();
	}

	// Update all the world meshes with the rotation
	for (Mesh* worldMesh : m_WorldMeshes) {
		worldMesh->SetWorldMatrix(Matrix::CreateRotationY(m_Rotation) * Matrix::CreateTranslation({ 0.f, 0.f, 50.f }));
	}
}

void dae::Renderer::Render() {
	m_pRenderBackend->Render(m_Camera, m_WorldMeshes);
}

void dae::Renderer::SetRenderBackend(AbstractRenderBackend* pRenderBackend) {
	m_pRenderBackend = pRenderBackend;
}

std::vector<Mesh*>& dae::Renderer::GetMeshes() {
	return m_WorldMeshes;
}

void dae::Renderer::ToggleRotation() {
	m_RotationEnabled = !m_RotationEnabled;

	if (m_RotationEnabled) {
		std::cout << "Enabled rotation" << std::endl;
	} else {
		std::cout << "Disabled rotation" << std::endl;
	}
}
