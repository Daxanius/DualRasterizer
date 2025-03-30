#include "Camera.h"

void dae::Camera::Initialize(int screenWidth, int screenHeight, float _fovAngle, Vector3 _origin) {
	aspectRatio = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
	fovAngle = _fovAngle;
	fov = tanf((fovAngle * TO_RADIANS) / 2.f);

	origin = _origin;

	CalculateProjectionMatrix();
	CalculateViewMatrix();
}

void dae::Camera::CalculateViewMatrix() {
	forward = Matrix::CreateRotation(totalPitch, totalYaw, 0).TransformVector(Vector3::UnitZ).Normalized();

	right = Vector3::Cross(Vector3::UnitY, forward).Normalized();
	up = Vector3::Cross(forward, right).Normalized();

	viewMatrix = { { right, 0.f }, { up, 0.f }, { forward, 0.f }, { origin, 1.f } };
	invViewMatrix = viewMatrix;
	invViewMatrix.Inverse();
}

void dae::Camera::CalculateProjectionMatrix() {
	projectionMatrix = Matrix::CreatePerspectiveFovLH(fov, aspectRatio, zNear, zFar);
}

dae::Matrix dae::Camera::GetWorldViewProjectionMatrix() const {
	return invViewMatrix * projectionMatrix;
}

void dae::Camera::Update(const Timer* pTimer) {
	const float deltaTime = pTimer->GetElapsed();
	const float rotationSpeed = 0.004f;
	const float panSpeed = 0.02f;
	float movementSpeed = 50.f;

	//Keyboard Input
	const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

	//Mouse Input
	int mouseX{}, mouseY{};
	const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

	bool dirty{};

	// Handle Camera Panning (Up/Down/Left/Right) when Right Mouse Button is held
	if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT) & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
		origin += right * static_cast<float>(mouseX) * panSpeed;
		origin += up * static_cast<float>(-mouseY) * panSpeed;

		dirty = true;
	} else if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
		// Update rotation based on mouse movement (yaw and pitch)
		totalYaw += mouseX * rotationSpeed;
		totalPitch -= mouseY * rotationSpeed;

		// Limit pitch to avoid gimbal lock
		totalPitch = std::clamp(totalPitch, -89.0f, 89.0f);

		dirty = true;
	} else if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) {
		origin += right * static_cast<float>(mouseX) * panSpeed;
		origin += forward * static_cast<float>(-mouseY) * panSpeed;

		dirty = true;
	}

	if (pKeyboardState[SDL_SCANCODE_LSHIFT]) {
		movementSpeed *= 2.f;
	}

	if (pKeyboardState[SDL_SCANCODE_W]) {
		origin += forward * movementSpeed * deltaTime;
		dirty = true;
	}

	if (pKeyboardState[SDL_SCANCODE_S]) {
		origin -= forward * movementSpeed * deltaTime;
		dirty = true;
	}

	// Right/left strafing (A/D keys)
	if (pKeyboardState[SDL_SCANCODE_D]) {
		origin += right * movementSpeed * deltaTime;
		dirty = true;
	}

	if (pKeyboardState[SDL_SCANCODE_A]) {
		origin -= right * movementSpeed * deltaTime;
		dirty = true;
	}

	//Update Matrices

	if (dirty) {
		CalculateViewMatrix();
	}
}
